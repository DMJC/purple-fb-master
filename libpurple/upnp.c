/* purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */
#include <gio/gio.h>
#include <libsoup/soup.h>

#include "internal.h"

#include "upnp.h"

#include "debug.h"
#include "eventloop.h"
#include "network.h"
#include "proxy.h"
#include "signals.h"
#include "util.h"
#include "xmlnode.h"

/***************************************************************
** General Defines                                             *
****************************************************************/
#define HTTP_OK "200 OK"
#define DEFAULT_HTTP_PORT 80
#define DISCOVERY_TIMEOUT 1
/* limit UPnP-triggered http downloads to 128k */
#define MAX_UPNP_DOWNLOAD (128 * 1024)

/***************************************************************
** Discovery/Description Defines                               *
****************************************************************/
#define NUM_UDP_ATTEMPTS 2

/* Address and port of an SSDP request used for discovery */
#define HTTPMU_HOST_ADDRESS "239.255.255.250"
#define HTTPMU_HOST_PORT 1900

#define SEARCH_REQUEST_DEVICE "urn:schemas-upnp-org:service:%s"

#define SEARCH_REQUEST_STRING \
	"M-SEARCH * HTTP/1.1\r\n" \
	"MX: 2\r\n" \
	"HOST: 239.255.255.250:1900\r\n" \
	"MAN: \"ssdp:discover\"\r\n" \
	"ST: urn:schemas-upnp-org:service:%s\r\n" \
	"\r\n"

#define WAN_IP_CONN_SERVICE "WANIPConnection:1"
#define WAN_PPP_CONN_SERVICE "WANPPPConnection:1"

/******************************************************************
** Action Defines                                                 *
*******************************************************************/
#define SOAP_ACTION \
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n" \
	"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" " \
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n" \
	  "<s:Body>\r\n" \
	    "<u:%s xmlns:u=\"urn:schemas-upnp-org:service:%s\">\r\n" \
	      "%s" \
	    "</u:%s>\r\n" \
	  "</s:Body>\r\n" \
	"</s:Envelope>"

#define PORT_MAPPING_LEASE_TIME "0"
#define PORT_MAPPING_DESCRIPTION "PURPLE_UPNP_PORT_FORWARD"

#define ADD_PORT_MAPPING_PARAMS \
	"<NewRemoteHost></NewRemoteHost>\r\n" \
	"<NewExternalPort>%i</NewExternalPort>\r\n" \
	"<NewProtocol>%s</NewProtocol>\r\n" \
	"<NewInternalPort>%i</NewInternalPort>\r\n" \
	"<NewInternalClient>%s</NewInternalClient>\r\n" \
	"<NewEnabled>1</NewEnabled>\r\n" \
	"<NewPortMappingDescription>" \
	PORT_MAPPING_DESCRIPTION \
	"</NewPortMappingDescription>\r\n" \
	"<NewLeaseDuration>" \
	PORT_MAPPING_LEASE_TIME \
	"</NewLeaseDuration>\r\n"

#define DELETE_PORT_MAPPING_PARAMS \
	"<NewRemoteHost></NewRemoteHost>\r\n" \
	"<NewExternalPort>%i</NewExternalPort>\r\n" \
	"<NewProtocol>%s</NewProtocol>\r\n"

typedef enum {
	PURPLE_UPNP_STATUS_UNDISCOVERED = -1,
	PURPLE_UPNP_STATUS_UNABLE_TO_DISCOVER,
	PURPLE_UPNP_STATUS_DISCOVERING,
	PURPLE_UPNP_STATUS_DISCOVERED
} PurpleUPnPStatus;

typedef struct {
	PurpleUPnPStatus status;
	gchar* control_url;
	gchar service_type[20];
	char publicip[16];
	char internalip[16];
	time_t lookup_time;
} PurpleUPnPControlInfo;

typedef struct {
	guint inpa;	/* purple_input_add handle */
	guint tima;	/* g_timeout_add handle */
	int fd;
	struct sockaddr_in server;
	gchar service_type[20];
	int retry_count;
	gchar *full_url;
} UPnPDiscoveryData;

struct _PurpleUPnPMappingAddRemove
{
	unsigned short portmap;
	gchar protocol[4];
	gboolean add;
	PurpleUPnPCallback cb;
	gpointer cb_data;
	gboolean success;
	guint tima; /* g_timeout_add handle */
	SoupMessage *msg;
};

static PurpleUPnPControlInfo control_info = {
	PURPLE_UPNP_STATUS_UNDISCOVERED,
	NULL, "\0", "\0", "\0", 0};

static SoupSession *session = NULL;
static GSList *discovery_callbacks = NULL;

static void purple_upnp_discover_send_broadcast(UPnPDiscoveryData *dd);
static void lookup_public_ip(void);
static void lookup_internal_ip(void);

static gboolean
fire_ar_cb_async_and_free(gpointer data)
{
	PurpleUPnPMappingAddRemove *ar = data;
	if (ar) {
		if (ar->cb)
			ar->cb(ar->success, ar->cb_data);
		g_free(ar);
	}

	return FALSE;
}

static void
fire_discovery_callbacks(gboolean success)
{
	while(discovery_callbacks) {
		gpointer data;
		PurpleUPnPCallback cb = discovery_callbacks->data;
		discovery_callbacks = g_slist_delete_link(discovery_callbacks, discovery_callbacks);
		data = discovery_callbacks->data;
		discovery_callbacks = g_slist_delete_link(discovery_callbacks, discovery_callbacks);
		cb(success, data);
	}
}

static gboolean
purple_upnp_compare_device(const PurpleXmlNode* device, const gchar* deviceType)
{
	PurpleXmlNode* deviceTypeNode = purple_xmlnode_get_child(device, "deviceType");
	char *tmp;
	gboolean ret;

	if(deviceTypeNode == NULL) {
		return FALSE;
	}

	tmp = purple_xmlnode_get_data(deviceTypeNode);
	ret = !g_ascii_strcasecmp(tmp, deviceType);
	g_free(tmp);

	return ret;
}

static gboolean
purple_upnp_compare_service(const PurpleXmlNode* service, const gchar* serviceType)
{
	PurpleXmlNode* serviceTypeNode;
	char *tmp;
	gboolean ret;

	if(service == NULL) {
		return FALSE;
	}

	serviceTypeNode = purple_xmlnode_get_child(service, "serviceType");

	if(serviceTypeNode == NULL) {
		return FALSE;
	}

	tmp = purple_xmlnode_get_data(serviceTypeNode);
	ret = !g_ascii_strcasecmp(tmp, serviceType);
	g_free(tmp);

	return ret;
}

static gchar*
purple_upnp_parse_description_response(const gchar* httpResponse, gsize len,
	const gchar* httpURL, const gchar* serviceType)
{
	gchar *baseURL, *controlURL, *service;
	PurpleXmlNode *xmlRootNode, *serviceTypeNode, *controlURLNode, *baseURLNode;
	char *tmp;

	/* create the xml root node */
	if ((xmlRootNode = purple_xmlnode_from_str(httpResponse, len)) == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): Could not parse xml root node\n");
		return NULL;
	}

	/* get the baseURL of the device */
	baseURL = NULL;
	if((baseURLNode = purple_xmlnode_get_child(xmlRootNode, "URLBase")) != NULL) {
		baseURL = purple_xmlnode_get_data(baseURLNode);
	}
	/* fixes upnp-descriptions with empty urlbase-element */
	if(baseURL == NULL){
		baseURL = g_strdup(httpURL);
	}

	/* get the serviceType child that has the service type as its data */

	/* get urn:schemas-upnp-org:device:InternetGatewayDevice:1 and its devicelist */
	serviceTypeNode = purple_xmlnode_get_child(xmlRootNode, "device");
	while(!purple_upnp_compare_device(serviceTypeNode,
			"urn:schemas-upnp-org:device:InternetGatewayDevice:1") &&
			serviceTypeNode != NULL) {
		serviceTypeNode = purple_xmlnode_get_next_twin(serviceTypeNode);
	}
	if(serviceTypeNode == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): could not get serviceTypeNode 1\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}
	serviceTypeNode = purple_xmlnode_get_child(serviceTypeNode, "deviceList");
	if(serviceTypeNode == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): could not get serviceTypeNode 2\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}

	/* get urn:schemas-upnp-org:device:WANDevice:1 and its devicelist */
	serviceTypeNode = purple_xmlnode_get_child(serviceTypeNode, "device");
	while(!purple_upnp_compare_device(serviceTypeNode,
			"urn:schemas-upnp-org:device:WANDevice:1") &&
			serviceTypeNode != NULL) {
		serviceTypeNode = purple_xmlnode_get_next_twin(serviceTypeNode);
	}
	if(serviceTypeNode == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): could not get serviceTypeNode 3\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}
	serviceTypeNode = purple_xmlnode_get_child(serviceTypeNode, "deviceList");
	if(serviceTypeNode == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): could not get serviceTypeNode 4\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}

	/* get urn:schemas-upnp-org:device:WANConnectionDevice:1 and its servicelist */
	serviceTypeNode = purple_xmlnode_get_child(serviceTypeNode, "device");
	while(serviceTypeNode && !purple_upnp_compare_device(serviceTypeNode,
			"urn:schemas-upnp-org:device:WANConnectionDevice:1")) {
		serviceTypeNode = purple_xmlnode_get_next_twin(serviceTypeNode);
	}
	if(serviceTypeNode == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): could not get serviceTypeNode 5\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}
	serviceTypeNode = purple_xmlnode_get_child(serviceTypeNode, "serviceList");
	if(serviceTypeNode == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): could not get serviceTypeNode 6\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}

	/* get the serviceType variable passed to this function */
	service = g_strdup_printf(SEARCH_REQUEST_DEVICE, serviceType);
	serviceTypeNode = purple_xmlnode_get_child(serviceTypeNode, "service");
	while(!purple_upnp_compare_service(serviceTypeNode, service) &&
			serviceTypeNode != NULL) {
		serviceTypeNode = purple_xmlnode_get_next_twin(serviceTypeNode);
	}

	g_free(service);
	if(serviceTypeNode == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): could not get serviceTypeNode 7\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}

	/* get the controlURL of the service */
	if((controlURLNode = purple_xmlnode_get_child(serviceTypeNode,
			"controlURL")) == NULL) {
		purple_debug_error("upnp",
			"parse_description_response(): Could not find controlURL\n");
		g_free(baseURL);
		purple_xmlnode_free(xmlRootNode);
		return NULL;
	}

	tmp = purple_xmlnode_get_data(controlURLNode);
	if (baseURL && !g_str_has_prefix(tmp, "http://") &&
	    !g_str_has_prefix(tmp, "HTTP://")) {
		/* Handle absolute paths in a relative URL.  This probably
		 * belongs in util.c. */
		if (tmp[0] == '/') {
			size_t length;
			const char *path, *start = strstr(baseURL, "://");
			start = start ? start + 3 : baseURL;
			path = strchr(start, '/');
			length = path ? (gsize)(path - baseURL) : strlen(baseURL);
			controlURL = g_strdup_printf("%.*s%s", (int)length, baseURL, tmp);
		} else {
			controlURL = g_strdup_printf("%s%s", baseURL, tmp);
		}
		g_free(tmp);
	} else {
		controlURL = tmp;
	}
	g_free(baseURL);
	purple_xmlnode_free(xmlRootNode);

	return controlURL;
}

static void
upnp_parse_description_cb(G_GNUC_UNUSED SoupSession *session, SoupMessage *msg,
                          gpointer _dd)
{
	UPnPDiscoveryData *dd = _dd;
	gchar *control_url = NULL;

	if (msg && SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		control_url = purple_upnp_parse_description_response(
		        msg->response_body->data, msg->response_body->length,
		        dd->full_url, dd->service_type);
	}

	g_free(dd->full_url);

	if(control_url == NULL) {
		purple_debug_error("upnp",
			"purple_upnp_parse_description(): control URL is NULL\n");
	}

	control_info.status = control_url ? PURPLE_UPNP_STATUS_DISCOVERED
		: PURPLE_UPNP_STATUS_UNABLE_TO_DISCOVER;
	control_info.lookup_time = time(NULL);
	control_info.control_url = control_url;
	g_strlcpy(control_info.service_type, dd->service_type,
		sizeof(control_info.service_type));

	fire_discovery_callbacks(control_url != NULL);

	/* Look up the public and internal IPs */
	if(control_url != NULL) {
		lookup_public_ip();
		lookup_internal_ip();
	}

	if (dd->inpa > 0)
		purple_input_remove(dd->inpa);
	if (dd->tima > 0)
		g_source_remove(dd->tima);

	g_free(dd);
}

static void
purple_upnp_parse_description(const gchar* descriptionURL, UPnPDiscoveryData *dd)
{
	SoupMessage *msg;
	SoupURI *uri;

	/* Remove the timeout because everything it is waiting for has
	 * successfully completed */
	g_source_remove(dd->tima);
	dd->tima = 0;

	/* Extract base url out of the descriptionURL.
	 * Example description URL: http://192.168.1.1:5678/rootDesc.xml
	 */
	uri = soup_uri_new(descriptionURL);
	if (!uri) {
		upnp_parse_description_cb(NULL, NULL, dd);
		return;
	}
	dd->full_url = g_strdup_printf("http://%s:%d", uri->host, uri->port);
	soup_uri_free(uri);

	msg = soup_message_new("GET", descriptionURL);
	// purple_http_request_set_max_len(msg, MAX_UPNP_DOWNLOAD);
	soup_session_queue_message(session, msg, upnp_parse_description_cb, dd);
}

static void
purple_upnp_parse_discover_response(const gchar* buf, unsigned int buf_len,
	UPnPDiscoveryData *dd)
{
	gchar* startDescURL;
	gchar* endDescURL;
	gchar* descURL;

	if(g_strstr_len(buf, buf_len, HTTP_OK) == NULL) {
		purple_debug_error("upnp",
			"parse_discover_response(): Failed In HTTP_OK\n");
		return;
	}

	if((startDescURL = g_strstr_len(buf, buf_len, "http://")) == NULL) {
		purple_debug_error("upnp",
			"parse_discover_response(): Failed In finding http://\n");
		return;
	}

	endDescURL = g_strstr_len(startDescURL, buf_len - (startDescURL - buf),
			"\r");
	if(endDescURL == NULL) {
		endDescURL = g_strstr_len(startDescURL,
				buf_len - (startDescURL - buf), "\n");
		if(endDescURL == NULL) {
			purple_debug_error("upnp",
				"parse_discover_response(): Failed In endDescURL\n");
			return;
		}
	}

	/* XXX: I'm not sure how this could ever happen */
	if(endDescURL == startDescURL) {
		purple_debug_error("upnp",
			"parse_discover_response(): endDescURL == startDescURL\n");
		return;
	}

	descURL = g_strndup(startDescURL, endDescURL - startDescURL);

	purple_upnp_parse_description(descURL, dd);

	g_free(descURL);

}

static gboolean
purple_upnp_discover_timeout(gpointer data)
{
	UPnPDiscoveryData* dd = data;

	if (dd->inpa)
		purple_input_remove(dd->inpa);
	if (dd->tima > 0)
		g_source_remove(dd->tima);
	dd->inpa = 0;
	dd->tima = 0;

	if (dd->retry_count < NUM_UDP_ATTEMPTS) {
		/* TODO: We probably shouldn't be incrementing retry_count in two places */
		dd->retry_count++;
		purple_upnp_discover_send_broadcast(dd);
	} else {
		if (dd->fd != -1)
			close(dd->fd);

		control_info.status = PURPLE_UPNP_STATUS_UNABLE_TO_DISCOVER;
		control_info.lookup_time = time(NULL);
		control_info.service_type[0] = '\0';
		g_free(control_info.control_url);
		control_info.control_url = NULL;

		fire_discovery_callbacks(FALSE);

		g_free(dd);
	}

	return FALSE;
}

static void
purple_upnp_discover_udp_read(gpointer data, gint sock, PurpleInputCondition cond)
{
	int len;
	UPnPDiscoveryData *dd = data;
	gchar buf[65536];

	do {
		len = recv(dd->fd, buf,
			sizeof(buf) - 1, 0);

		if(len >= 0) {
			buf[len] = '\0';
			break;
		} else if(errno != EINTR) {
			/* We'll either get called again, or time out */
			return;
		}
	} while (errno == EINTR);

	purple_input_remove(dd->inpa);
	dd->inpa = 0;

	close(dd->fd);
	dd->fd = -1;

	/* parse the response, and see if it was a success */
	purple_upnp_parse_discover_response(buf, len, dd);

	/* We'll either time out or continue successfully */
}

static void
purple_upnp_discover_send_broadcast(UPnPDiscoveryData *dd)
{
	gchar *sendMessage = NULL;
	size_t totalSize;
	gboolean sentSuccess;

	/* because we are sending over UDP, if there is a failure
	   we should retry the send NUM_UDP_ATTEMPTS times. Also,
	   try different requests for WANIPConnection and WANPPPConnection*/
	for(; dd->retry_count < NUM_UDP_ATTEMPTS; dd->retry_count++) {
		sentSuccess = FALSE;

		if((dd->retry_count % 2) == 0) {
			g_strlcpy(dd->service_type, WAN_IP_CONN_SERVICE, sizeof(dd->service_type));
		} else {
			g_strlcpy(dd->service_type, WAN_PPP_CONN_SERVICE, sizeof(dd->service_type));
		}

		sendMessage = g_strdup_printf(SEARCH_REQUEST_STRING, dd->service_type);

		totalSize = strlen(sendMessage);

		do {
			gssize sent = sendto(dd->fd, sendMessage, totalSize, 0,
				(struct sockaddr*) &(dd->server),
				sizeof(struct sockaddr_in));
			if (sent >= 0 && (gsize)sent == totalSize) {
				sentSuccess = TRUE;
				break;
			}
		} while (errno == EINTR || errno == EAGAIN);

		g_free(sendMessage);

		if(sentSuccess) {
			dd->tima = g_timeout_add_seconds(DISCOVERY_TIMEOUT,
			                                 purple_upnp_discover_timeout, dd);
			dd->inpa = purple_input_add(dd->fd, PURPLE_INPUT_READ,
				purple_upnp_discover_udp_read, dd);

			return;
		}
	}

	/* We have already done all our retries. Make sure that the callback
	 * doesn't get called before the original function returns */
	dd->tima = g_timeout_add(10, purple_upnp_discover_timeout, dd);
}

void
purple_upnp_discover(PurpleUPnPCallback cb, gpointer cb_data)
{
	/* Socket Setup Variables */
	int sock;
	struct hostent* hp;

	/* UDP RECEIVE VARIABLES */
	UPnPDiscoveryData *dd;

	if (control_info.status == PURPLE_UPNP_STATUS_DISCOVERING) {
		if (cb) {
			discovery_callbacks = g_slist_append(
					discovery_callbacks, cb);
			discovery_callbacks = g_slist_append(
					discovery_callbacks, cb_data);
		}
		return;
	}

	dd = g_new0(UPnPDiscoveryData, 1);
	if (cb) {
		discovery_callbacks = g_slist_append(discovery_callbacks, cb);
		discovery_callbacks = g_slist_append(discovery_callbacks,
				cb_data);
	}

	/* Set up the sockets */
	dd->fd = sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) {
		purple_debug_error("upnp",
			"purple_upnp_discover(): Failed In sock creation\n");
		/* Short circuit the retry attempts */
		dd->retry_count = NUM_UDP_ATTEMPTS;
		dd->tima = g_timeout_add(10, purple_upnp_discover_timeout, dd);
		return;
	}

	/* TODO: Non-blocking! */
	if((hp = gethostbyname(HTTPMU_HOST_ADDRESS)) == NULL) {
		purple_debug_error("upnp",
			"purple_upnp_discover(): Failed In gethostbyname\n");
		/* Short circuit the retry attempts */
		dd->retry_count = NUM_UDP_ATTEMPTS;
		dd->tima = g_timeout_add(10, purple_upnp_discover_timeout, dd);
		return;
	}

	memset(&(dd->server), 0, sizeof(struct sockaddr));
	dd->server.sin_family = AF_INET;
	memcpy(&(dd->server.sin_addr), hp->h_addr_list[0], hp->h_length);
	dd->server.sin_port = htons(HTTPMU_HOST_PORT);

	control_info.status = PURPLE_UPNP_STATUS_DISCOVERING;

	purple_upnp_discover_send_broadcast(dd);
}

static SoupMessage *
purple_upnp_generate_action_message_and_send(const gchar *actionName,
                                             const gchar *actionParams,
                                             SoupSessionCallback cb,
                                             gpointer cb_data)
{
	SoupMessage *msg;
	gchar *action;
	gchar* soapMessage;

	/* set the soap message */
	soapMessage = g_strdup_printf(SOAP_ACTION, actionName,
		control_info.service_type, actionParams, actionName);

	msg = soup_message_new("POST", control_info.control_url);
	// purple_http_request_set_max_len(msg, MAX_UPNP_DOWNLOAD);
	action = g_strdup_printf("\"urn:schemas-upnp-org:service:%s#%s\"",
	                         control_info.service_type, actionName);
	soup_message_headers_replace(msg->request_headers, "SOAPAction", action);
	g_free(action);
	soup_message_set_request(msg, "text/xml; charset=utf-8", SOUP_MEMORY_TAKE,
	                         soapMessage, strlen(soapMessage));
	soup_session_queue_message(session, msg, cb, cb_data);

	return msg;
}

const gchar *
purple_upnp_get_public_ip()
{
	if (control_info.status == PURPLE_UPNP_STATUS_DISCOVERED
			&& *control_info.publicip)
		return control_info.publicip;

	/* Trigger another UPnP discovery if 5 minutes have elapsed since the
	 * last one, and it wasn't successful */
	if (control_info.status < PURPLE_UPNP_STATUS_DISCOVERING
			&& (time(NULL) - control_info.lookup_time) > 300)
		purple_upnp_discover(NULL, NULL);

	return NULL;
}

static void
looked_up_public_ip_cb(G_GNUC_UNUSED SoupSession *session, SoupMessage *msg,
                       gpointer user_data)
{
	gchar* temp, *temp2;
	const gchar *got_data;
	size_t got_len;

	if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		return;
	}

	/* extract the ip, or see if there is an error */
	got_data = msg->response_body->data;
	got_len = msg->response_body->length;
	if((temp = g_strstr_len(got_data, got_len,
			"<NewExternalIPAddress")) == NULL) {
		purple_debug_error("upnp",
			"looked_up_public_ip_cb(): Failed Finding <NewExternalIPAddress\n");
		return;
	}
	if(!(temp = g_strstr_len(temp, got_len - (temp - got_data), ">"))) {
		purple_debug_error("upnp",
			"looked_up_public_ip_cb(): Failed In Finding >\n");
		return;
	}
	if(!(temp2 = g_strstr_len(temp, got_len - (temp - got_data), "<"))) {
		purple_debug_error("upnp",
			"looked_up_public_ip_cb(): Failed In Finding <\n");
		return;
	}
	*temp2 = '\0';

	g_strlcpy(control_info.publicip, temp + 1,
			sizeof(control_info.publicip));

	purple_debug_info("upnp", "NAT Returned IP: %s\n", control_info.publicip);
}

static void
lookup_public_ip()
{
	purple_upnp_generate_action_message_and_send("GetExternalIPAddress", "",
			looked_up_public_ip_cb, NULL);
}

/* TODO: This could be exported */
static const gchar *
purple_upnp_get_internal_ip(void)
{
	if (control_info.status == PURPLE_UPNP_STATUS_DISCOVERED
			&& *control_info.internalip)
		return control_info.internalip;

	/* Trigger another UPnP discovery if 5 minutes have elapsed since the
	 * last one, and it wasn't successful */
	if (control_info.status < PURPLE_UPNP_STATUS_DISCOVERING
			&& (time(NULL) - control_info.lookup_time) > 300)
		purple_upnp_discover(NULL, NULL);

	return NULL;
}

static void
looked_up_internal_ip_cb(gpointer data, gint source, const gchar *error_message)
{
	if (source != -1) {
		g_strlcpy(control_info.internalip,
			purple_network_get_local_system_ip(source),
			sizeof(control_info.internalip));
		purple_debug_info("upnp", "Local IP: %s\n",
				control_info.internalip);
		close(source);
	} else
		purple_debug_error("upnp", "Unable to look up local IP\n");

}

static void
lookup_internal_ip()
{
	SoupURI *uri;

	uri = soup_uri_new(control_info.control_url);
	if (!uri) {
		purple_debug_error("upnp",
			"lookup_internal_ip(): Failed In Parse URL\n");
		return;
	}

	if (purple_proxy_connect(NULL, NULL, uri->host, uri->port,
	                         looked_up_internal_ip_cb, NULL) == NULL) {
		purple_debug_error(
		        "upnp", "Get Local IP Connect Failed: Address: %s @@@ Port %d",
		        uri->host, uri->port);
	}

	soup_uri_free(uri);
}

static void
done_port_mapping_cb(G_GNUC_UNUSED SoupSession *session, SoupMessage *msg,
                     gpointer user_data)
{
	PurpleUPnPMappingAddRemove *ar = user_data;

	gboolean success = TRUE;

	/* determine if port mapping was a success */
	if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		purple_debug_error("upnp",
		                   "purple_upnp_set_port_mapping(): Failed HTTP_OK: %s",
		                   msg->reason_phrase);
		success =  FALSE;
	} else {
		purple_debug_info("upnp",
		                  "Successfully completed port mapping operation");
	}

	ar->success = success;
	ar->tima = g_timeout_add(0, fire_ar_cb_async_and_free, ar);
}

static void
do_port_mapping_cb(gboolean has_control_mapping, gpointer data)
{
	PurpleUPnPMappingAddRemove *ar = data;

	if (has_control_mapping) {
		gchar action_name[25];
		gchar *action_params;
		if(ar->add) {
			const gchar *internal_ip;
			/* get the internal IP */
			if(!(internal_ip = purple_upnp_get_internal_ip())) {
				purple_debug_error("upnp",
					"purple_upnp_set_port_mapping(): couldn't get local ip\n");
				ar->success = FALSE;
				ar->tima = g_timeout_add(0, fire_ar_cb_async_and_free, ar);
				return;
			}
			strncpy(action_name, "AddPortMapping",
					sizeof(action_name));
			action_params = g_strdup_printf(
					ADD_PORT_MAPPING_PARAMS,
					ar->portmap, ar->protocol, ar->portmap,
					internal_ip);
		} else {
			strncpy(action_name, "DeletePortMapping", sizeof(action_name));
			action_params = g_strdup_printf(
				DELETE_PORT_MAPPING_PARAMS,
				ar->portmap, ar->protocol);
		}

		ar->msg = purple_upnp_generate_action_message_and_send(
		        action_name, action_params, done_port_mapping_cb, ar);

		g_free(action_params);
		return;
	}

	ar->success = FALSE;
	ar->tima = g_timeout_add(0, fire_ar_cb_async_and_free, ar);
}

static gboolean
fire_port_mapping_failure_cb(gpointer data)
{
	PurpleUPnPMappingAddRemove *ar = data;

	ar->tima = 0;
	do_port_mapping_cb(FALSE, data);
	return FALSE;
}

void purple_upnp_cancel_port_mapping(PurpleUPnPMappingAddRemove *ar)
{
	GSList *l;

	/* Remove ar from discovery_callbacks if present; it was inserted after a cb.
	 * The same cb may be in the list multiple times, so be careful to remove
	 * the one associated with ar. */
	l = discovery_callbacks;
	while (l)
	{
		GSList *next = l->next;

		if (next && (next->data == ar)) {
			discovery_callbacks = g_slist_delete_link(discovery_callbacks, next);
			next = l->next;
			discovery_callbacks = g_slist_delete_link(discovery_callbacks, l);
		}

		l = next;
	}

	if (ar->tima > 0)
		g_source_remove(ar->tima);

	soup_session_cancel_message(session, ar->msg, SOUP_STATUS_CANCELLED);

	g_free(ar);
}

PurpleUPnPMappingAddRemove *
purple_upnp_set_port_mapping(unsigned short portmap, const gchar* protocol,
		PurpleUPnPCallback cb, gpointer cb_data)
{
	PurpleUPnPMappingAddRemove *ar;

	ar = g_new0(PurpleUPnPMappingAddRemove, 1);
	ar->cb = cb;
	ar->cb_data = cb_data;
	ar->add = TRUE;
	ar->portmap = portmap;
	g_strlcpy(ar->protocol, protocol, sizeof(ar->protocol));

	/* If we're waiting for a discovery, add to the callbacks list */
	if(control_info.status == PURPLE_UPNP_STATUS_DISCOVERING) {
		/* TODO: This will fail because when this cb is triggered,
		 * the internal IP lookup won't be complete */
		discovery_callbacks = g_slist_append(
				discovery_callbacks, do_port_mapping_cb);
		discovery_callbacks = g_slist_append(
				discovery_callbacks, ar);
		return ar;
	}

	/* If we haven't had a successful UPnP discovery, check if 5 minutes has
	 * elapsed since the last try, try again */
	if(control_info.status == PURPLE_UPNP_STATUS_UNDISCOVERED ||
			(control_info.status == PURPLE_UPNP_STATUS_UNABLE_TO_DISCOVER
			 && (time(NULL) - control_info.lookup_time) > 300)) {
		purple_upnp_discover(do_port_mapping_cb, ar);
		return ar;
	} else if(control_info.status == PURPLE_UPNP_STATUS_UNABLE_TO_DISCOVER) {
		if (cb) {
			/* Asynchronously trigger a failed response */
			ar->tima = g_timeout_add(10, fire_port_mapping_failure_cb, ar);
		} else {
			/* No need to do anything if nobody expects a response*/
			g_free(ar);
			ar = NULL;
		}
		return ar;
	}

	do_port_mapping_cb(TRUE, ar);
	return ar;
}

PurpleUPnPMappingAddRemove *
purple_upnp_remove_port_mapping(unsigned short portmap, const char* protocol,
		PurpleUPnPCallback cb, gpointer cb_data)
{
	PurpleUPnPMappingAddRemove *ar;

	ar = g_new0(PurpleUPnPMappingAddRemove, 1);
	ar->cb = cb;
	ar->cb_data = cb_data;
	ar->add = FALSE;
	ar->portmap = portmap;
	g_strlcpy(ar->protocol, protocol, sizeof(ar->protocol));

	/* If we're waiting for a discovery, add to the callbacks list */
	if(control_info.status == PURPLE_UPNP_STATUS_DISCOVERING) {
		discovery_callbacks = g_slist_append(
				discovery_callbacks, do_port_mapping_cb);
		discovery_callbacks = g_slist_append(
				discovery_callbacks, ar);
		return ar;
	}

	/* If we haven't had a successful UPnP discovery, check if 5 minutes has
	 * elapsed since the last try, try again */
	if(control_info.status == PURPLE_UPNP_STATUS_UNDISCOVERED ||
			(control_info.status == PURPLE_UPNP_STATUS_UNABLE_TO_DISCOVER
			 && (time(NULL) - control_info.lookup_time) > 300)) {
		purple_upnp_discover(do_port_mapping_cb, ar);
		return ar;
	} else if(control_info.status == PURPLE_UPNP_STATUS_UNABLE_TO_DISCOVER) {
		if (cb) {
			/* Asynchronously trigger a failed response */
			ar->tima = g_timeout_add(10, fire_port_mapping_failure_cb, ar);
		} else {
			/* No need to do anything if nobody expects a response*/
			g_free(ar);
			ar = NULL;
		}
		return ar;
	}

	do_port_mapping_cb(TRUE, ar);
	return ar;
}

static void
purple_upnp_network_config_changed_cb(GNetworkMonitor *monitor, gboolean available, gpointer data)
{
	/* Reset the control_info to default values */
	control_info.status = PURPLE_UPNP_STATUS_UNDISCOVERED;
	g_free(control_info.control_url);
	control_info.control_url = NULL;
	control_info.service_type[0] = '\0';
	control_info.publicip[0] = '\0';
	control_info.internalip[0] = '\0';
	control_info.lookup_time = 0;
}

void
purple_upnp_init()
{
	session = soup_session_new();

	g_signal_connect(g_network_monitor_get_default(),
	                 "network-changed",
	                 G_CALLBACK(purple_upnp_network_config_changed_cb),
	                 NULL);
}

void
purple_upnp_uninit(void)
{
	soup_session_abort(session);
	g_clear_object(&session);
}
