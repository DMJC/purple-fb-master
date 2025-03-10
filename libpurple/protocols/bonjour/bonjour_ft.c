/*
 * purple - Bonjour Protocol Plugin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA
 */
#include "internal.h"
#include <purple.h>

#include "bonjour.h"
#include "bonjour_ft.h"

static void
bonjour_bytestreams_init(PurpleXfer *xfer);
static void
bonjour_bytestreams_connect(PurpleXfer *xfer);
static void
bonjour_xfer_init(PurpleXfer *xfer);
static void
bonjour_xfer_receive(PurpleConnection *pc, const char *id, const char *sid, const char *from,
		     goffset filesize, const char *filename, int option);

/* Look for specific xfer handle */
static unsigned int next_id = 0;

struct _XepXfer
{
	PurpleXfer parent;

	void *data;
	char *filename;
	int filesize;
	char *iq_id;
	char *sid;
	char *recv_id;
	char *buddy_ip;
	int mode;
	GSocketService *service;
	GSocketConnection *conn;
	int sock5_req_state;
	int rxlen;
	char rx_buf[0x500];
	char tx_buf[0x500];
	PurpleProxyInfo *proxy_info;
	PurpleProxyConnectData *proxy_connection;
	char *jid;
	char *proxy_host;
	int proxy_port;
	PurpleXmlNode *streamhost;
	PurpleBuddy *pb;
};

G_DEFINE_DYNAMIC_TYPE(XepXfer, xep_xfer, PURPLE_TYPE_XFER);

static void
xep_ft_si_reject(BonjourData *bd, const char *id, const char *to, const char *error_code, const char *error_type)
{
	PurpleXmlNode *error_node;
	XepIq *iq;

	g_return_if_fail(error_code != NULL);
	g_return_if_fail(error_type != NULL);

	if(!to || !id) {
		purple_debug_info("bonjour", "xep file transfer stream initialization error.\n");
		return;
	}

	iq = xep_iq_new(bd, XEP_IQ_ERROR, to, bonjour_get_jid(bd->xmpp_data->account), id);
	if(iq == NULL)
		return;

	error_node = purple_xmlnode_new_child(iq->node, "error");
	purple_xmlnode_set_attrib(error_node, "code", error_code);
	purple_xmlnode_set_attrib(error_node, "type", error_type);

	/* TODO: Make this better */
	if (purple_strequal(error_code, "403")) {
		PurpleXmlNode *tmp_node = purple_xmlnode_new_child(error_node, "forbidden");
		purple_xmlnode_set_namespace(tmp_node, "urn:ietf:params:xml:ns:xmpp-stanzas");

		tmp_node = purple_xmlnode_new_child(error_node, "text");
		purple_xmlnode_set_namespace(tmp_node, "urn:ietf:params:xml:ns:xmpp-stanzas");
		purple_xmlnode_insert_data(tmp_node, "Offer Declined", -1);
	} else if (purple_strequal(error_code, "404")) {
		PurpleXmlNode *tmp_node = purple_xmlnode_new_child(error_node, "item-not-found");
		purple_xmlnode_set_namespace(tmp_node, "urn:ietf:params:xml:ns:xmpp-stanzas");
	}

	xep_iq_send_and_free(iq);
}

static void bonjour_xfer_cancel_send(PurpleXfer *xfer)
{
	purple_debug_info("bonjour", "Bonjour-xfer-cancel-send.\n");
}

static void bonjour_xfer_request_denied(PurpleXfer *xfer)
{
	XepXfer *xf = XEP_XFER(xfer);

	purple_debug_info("bonjour", "Bonjour-xfer-request-denied.\n");

	if(xf) {
		xep_ft_si_reject(xf->data, xf->sid, purple_xfer_get_remote_user(xfer), "403", "cancel");
	}
}

static void bonjour_xfer_cancel_recv(PurpleXfer *xfer)
{
	purple_debug_info("bonjour", "Bonjour-xfer-cancel-recv.\n");
}

struct socket_cleanup {
	int fd;
	guint handle;
};

static void
_wait_for_socket_close(gpointer data, gint source, PurpleInputCondition cond)
{
	struct socket_cleanup *sc = data;
	char buf[1];
	int ret;

	ret = recv(source, buf, 1, 0);

	if (ret == 0 || (ret == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK))) {
		purple_debug_info("bonjour", "Client completed recieving; closing server socket.\n");
		purple_input_remove(sc->handle);
		close(sc->fd);
		g_free(sc);
	}
}

static void bonjour_xfer_end(PurpleXfer *xfer)
{
	purple_debug_info("bonjour", "Bonjour-xfer-end.\n");

	/* We can't allow the server side to close the connection until the client is complete,
	 * otherwise there is a RST resulting in an error on the client side */
	if (purple_xfer_get_xfer_type(xfer) == PURPLE_XFER_TYPE_SEND && purple_xfer_is_completed(xfer)) {
		struct socket_cleanup *sc = g_new0(struct socket_cleanup, 1);
		sc->fd = purple_xfer_get_fd(xfer);
		purple_xfer_set_fd(xfer, -1);
		sc->handle = purple_input_add(sc->fd, PURPLE_INPUT_READ,
						 _wait_for_socket_close, sc);
	}
}

static PurpleXfer*
bonjour_si_xfer_find(BonjourData *bd, const char *sid, const char *from)
{
	GSList *xfers;
	PurpleXfer *xfer;
	XepXfer *xf;

	if(!sid || !from || !bd)
		return NULL;

	purple_debug_info("bonjour", "Look for sid=%s from=%s xferlists.\n",
			  sid, from);

	for(xfers = bd->xfer_lists; xfers; xfers = xfers->next) {
		xfer = xfers->data;
		if(xfer == NULL)
			break;
		xf = XEP_XFER(xfer);
		if(xf == NULL)
			break;
		if(xf->sid && purple_xfer_get_remote_user(xfer) && purple_strequal(xf->sid, sid) &&
				purple_strequal(purple_xfer_get_remote_user(xfer), from))
			return xfer;
	}

	purple_debug_info("bonjour", "Look for xfer list fail\n");

	return NULL;
}

static void
xep_ft_si_offer(PurpleXfer *xfer, const gchar *to)
{
	PurpleXmlNode *si_node, *feature, *field, *file, *x;
	XepIq *iq;
	XepXfer *xf = XEP_XFER(xfer);
	BonjourData *bd = NULL;
	char buf[32];

	if(!xf)
		return;

	bd = xf->data;
	if(!bd)
		return;

	purple_debug_info("bonjour", "xep file transfer stream initialization offer-id=%d.\n", next_id);

	/* Assign stream id. */
	g_free(xf->iq_id);
	xf->iq_id = g_strdup_printf("%u", next_id++);
	iq = xep_iq_new(xf->data, XEP_IQ_SET, to, bonjour_get_jid(bd->xmpp_data->account), xf->iq_id);
	if(iq == NULL)
		return;

	/*Construct Stream initialization offer message.*/
	si_node = purple_xmlnode_new_child(iq->node, "si");
	purple_xmlnode_set_namespace(si_node, "http://jabber.org/protocol/si");
	purple_xmlnode_set_attrib(si_node, "profile", "http://jabber.org/protocol/si/profile/file-transfer");
	g_free(xf->sid);
	xf->sid = g_strdup(xf->iq_id);
	purple_xmlnode_set_attrib(si_node, "id", xf->sid);

	file = purple_xmlnode_new_child(si_node, "file");
	purple_xmlnode_set_namespace(file, "http://jabber.org/protocol/si/profile/file-transfer");
	purple_xmlnode_set_attrib(file, "name", purple_xfer_get_filename(xfer));
	g_snprintf(buf, sizeof(buf), "%" G_GOFFSET_FORMAT, purple_xfer_get_size(xfer));
	purple_xmlnode_set_attrib(file, "size", buf);

	feature = purple_xmlnode_new_child(si_node, "feature");
	purple_xmlnode_set_namespace(feature, "http://jabber.org/protocol/feature-neg");

	x = purple_xmlnode_new_child(feature, "x");
	purple_xmlnode_set_namespace(x, "jabber:x:data");
	purple_xmlnode_set_attrib(x, "type", "form");

	field = purple_xmlnode_new_child(x, "field");
	purple_xmlnode_set_attrib(field, "var", "stream-method");
	purple_xmlnode_set_attrib(field, "type", "list-single");

	if (xf->mode & XEP_BYTESTREAMS) {
		PurpleXmlNode *option = purple_xmlnode_new_child(field, "option");
		PurpleXmlNode *value = purple_xmlnode_new_child(option, "value");
		purple_xmlnode_insert_data(value, "http://jabber.org/protocol/bytestreams", -1);
	}
	if (xf->mode & XEP_IBB) {
		PurpleXmlNode *option = purple_xmlnode_new_child(field, "option");
		PurpleXmlNode *value = purple_xmlnode_new_child(option, "value");
		purple_xmlnode_insert_data(value, "http://jabber.org/protocol/ibb", -1);
	}

	xep_iq_send_and_free(iq);
}

static void
xep_ft_si_result(PurpleXfer *xfer, const char *to)
{
	PurpleXmlNode *si_node, *feature, *field, *value, *x;
	XepIq *iq;
	XepXfer *xf;
	BonjourData *bd;

	if(!to || !xfer)
		return;
	xf = XEP_XFER(xfer);

	bd = xf->data;

	purple_debug_info("bonjour", "xep file transfer stream initialization result.\n");
	iq = xep_iq_new(bd, XEP_IQ_RESULT, to, bonjour_get_jid(bd->xmpp_data->account), xf->iq_id);
	if(iq == NULL)
		return;

	si_node = purple_xmlnode_new_child(iq->node, "si");
	purple_xmlnode_set_namespace(si_node, "http://jabber.org/protocol/si");
	/*purple_xmlnode_set_attrib(si_node, "profile", "http://jabber.org/protocol/si/profile/file-transfer");*/

	feature = purple_xmlnode_new_child(si_node, "feature");
	purple_xmlnode_set_namespace(feature, "http://jabber.org/protocol/feature-neg");

	x = purple_xmlnode_new_child(feature, "x");
	purple_xmlnode_set_namespace(x, "jabber:x:data");
	purple_xmlnode_set_attrib(x, "type", "submit");

	field = purple_xmlnode_new_child(x, "field");
	purple_xmlnode_set_attrib(field, "var", "stream-method");

	value = purple_xmlnode_new_child(field, "value");
	purple_xmlnode_insert_data(value, "http://jabber.org/protocol/bytestreams", -1);

	xep_iq_send_and_free(iq);
}

/**
 * Frees the whole tree of an xml node
 *
 * First determines the root of the xml tree and then frees the whole tree
 * from there.
 *
 * @param node	The node to free the tree from
 */
static void
purple_xmlnode_free_tree(PurpleXmlNode *node)
{
	g_return_if_fail(node != NULL);

	while(purple_xmlnode_get_parent(node))
		node = purple_xmlnode_get_parent(node);

	purple_xmlnode_free(node);
}

PurpleXfer *
bonjour_new_xfer(PurpleProtocolXfer *prplxfer, PurpleConnection *gc, const char *who)
{
	PurpleXfer *xfer;
	XepXfer *xep_xfer;
	BonjourData *bd;

	if(who == NULL || gc == NULL)
		return NULL;

	purple_debug_info("bonjour", "Bonjour-new-xfer to %s.\n", who);
	bd = purple_connection_get_protocol_data(gc);
	if(bd == NULL)
		return NULL;

	/* Build the file transfer handle */
	xep_xfer = g_object_new(
		XEP_TYPE_XFER,
		"account", purple_connection_get_account(gc),
		"type", PURPLE_XFER_TYPE_SEND,
		"remote-user", who,
		NULL
	);
	xfer = PURPLE_XFER(xep_xfer);
	xep_xfer->data = bd;

	purple_debug_info("bonjour", "Bonjour-new-xfer bd=%p data=%p.\n", bd, xep_xfer->data);

	/* We don't support IBB yet */
	/*xep_xfer->mode = XEP_BYTESTREAMS | XEP_IBB;*/
	xep_xfer->mode = XEP_BYTESTREAMS;
	xep_xfer->sid = NULL;

	bd->xfer_lists = g_slist_append(bd->xfer_lists, xfer);

	return xfer;
}

void
bonjour_send_file(PurpleProtocolXfer *prplxfer, PurpleConnection *gc, const char *who, const char *file)
{
	PurpleXfer *xfer;

	g_return_if_fail(gc != NULL);
	g_return_if_fail(who != NULL);

	purple_debug_info("bonjour", "Bonjour-send-file to=%s.\n", who);

	xfer = bonjour_new_xfer(prplxfer, gc, who);

	if (file)
		purple_xfer_request_accepted(xfer, file);
	else
		purple_xfer_request(xfer);

}

static void
bonjour_xfer_init(PurpleXfer *xfer)
{
	PurpleBuddy *buddy;
	BonjourBuddy *bb;
	XepXfer *xf;

	xf = XEP_XFER(xfer);

	purple_debug_info("bonjour", "Bonjour-xfer-init.\n");

	buddy = purple_blist_find_buddy(purple_xfer_get_account(xfer), purple_xfer_get_remote_user(xfer));
	/* this buddy is offline. */
	if (buddy == NULL || (bb = purple_buddy_get_protocol_data(buddy)) == NULL)
		return;

	/* Assume it is the first IP. We could do something like keep track of which one is in use or something. */
	if (bb->ips)
		xf->buddy_ip = g_strdup(bb->ips->data);
	if (purple_xfer_get_xfer_type(xfer) == PURPLE_XFER_TYPE_SEND) {
		/* initiate file transfer, send SI offer. */
		purple_debug_info("bonjour", "Bonjour xfer type is PURPLE_XFER_TYPE_SEND.\n");
		xep_ft_si_offer(xfer, purple_xfer_get_remote_user(xfer));
	} else {
		/* accept file transfer request, send SI result. */
		xep_ft_si_result(xfer, purple_xfer_get_remote_user(xfer));
		purple_debug_info("bonjour", "Bonjour xfer type is PURPLE_XFER_TYPE_RECEIVE.\n");
	}
}

void
xep_si_parse(PurpleConnection *pc, PurpleXmlNode *packet, PurpleBuddy *pb)
{
	const char *type, *id;
	BonjourData *bd;
	PurpleXfer *xfer;
	const gchar *name = NULL;

	g_return_if_fail(pc != NULL);
	g_return_if_fail(packet != NULL);
	g_return_if_fail(pb != NULL);

	bd = purple_connection_get_protocol_data(pc);
	if(bd == NULL)
		return;

	purple_debug_info("bonjour", "xep-si-parse.\n");

	name = purple_buddy_get_name(pb);

	type = purple_xmlnode_get_attrib(packet, "type");
	id = purple_xmlnode_get_attrib(packet, "id");
	if(!type)
		return;

	if(purple_strequal(type, "set")) {
		PurpleXmlNode *si;
		gboolean parsed_receive = FALSE;

		si = purple_xmlnode_get_child(packet, "si");

		purple_debug_info("bonjour", "si offer Message type - SET.\n");
		if (si) {
			const char *profile;

			profile = purple_xmlnode_get_attrib(si, "profile");

			if (purple_strequal(profile, "http://jabber.org/protocol/si/profile/file-transfer")) {
				const char *filename = NULL, *filesize_str = NULL;
				goffset filesize = 0;
				PurpleXmlNode *file;

				const char *sid = purple_xmlnode_get_attrib(si, "id");

				if ((file = purple_xmlnode_get_child(si, "file"))) {
					filename = purple_xmlnode_get_attrib(file, "name");
					if((filesize_str = purple_xmlnode_get_attrib(file, "size")))
						filesize = g_ascii_strtoll(filesize_str, NULL, 10);
				}

				/* TODO: Make sure that it is advertising a bytestreams transfer */

				if (filename) {
					bonjour_xfer_receive(pc, id, sid, name, filesize, filename, XEP_BYTESTREAMS);

					parsed_receive = TRUE;
				}
			}
		}

		if (!parsed_receive) {
			BonjourData *bd = purple_connection_get_protocol_data(pc);

			purple_debug_info("bonjour", "rejecting unrecognized si SET offer.\n");
			xep_ft_si_reject(bd, id, name, "403", "cancel");
			/*TODO: Send Cancel (501) */
		}
	} else if(purple_strequal(type, "result")) {
		purple_debug_info("bonjour", "si offer Message type - RESULT.\n");

		xfer = bonjour_si_xfer_find(bd, id, name);

		if(xfer == NULL) {
			BonjourData *bd = purple_connection_get_protocol_data(pc);
			purple_debug_info("bonjour", "xfer find fail.\n");
			xep_ft_si_reject(bd, id, name, "403", "cancel");
		} else
			bonjour_bytestreams_init(xfer);

	} else if(purple_strequal(type, "error")) {
		purple_debug_info("bonjour", "si offer Message type - ERROR.\n");

		xfer = bonjour_si_xfer_find(bd, id, name);

		if(xfer == NULL)
			purple_debug_info("bonjour", "xfer find fail.\n");
		else
			purple_xfer_cancel_remote(xfer);
	} else
		purple_debug_info("bonjour", "si offer Message type - Unknown-%s.\n", type);
}

/**
 * Will compare a host with a buddy_ip.
 *
 * Additionally to a common 'purple_strequal(host, buddy_ip)', it will also return TRUE
 * if 'host' is a link local IPv6 address without an appended interface
 * identifier and 'buddy_ip' string is "host" + "%iface".
 *
 * Note: This may theoretically result in the attempt to connect to the wrong
 * host, because we do not know for sure which interface the according link
 * local IPv6 address might relate to and RFC4862 for instance only ensures the
 * uniqueness of this address on a given link. So we could possibly have two
 * distinct buddies with the same ipv6 link local address on two distinct
 * interfaces. Unfortunately XEP-0065 does not seem to specify how to deal with
 * link local ip addresses properly...
 * However, in practice the possiblity for such a conflict is relatively low
 * (2011 - might be different in the future though?).
 *
 * @param host		ipv4 or ipv6 address string
 * @param buddy_ip	ipv4 or ipv6 address string
 * @return		TRUE if they match, FALSE otherwise
 */
static gboolean
xep_cmp_addr(const char *host, const char *buddy_ip)
{
#if defined(AF_INET6) && defined(HAVE_GETADDRINFO)
	struct addrinfo hint, *res = NULL;
	common_sockaddr_t addr;
	int ret;

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_flags = AI_NUMERICHOST;

	ret = getaddrinfo(host, NULL, &hint, &res);
	if(ret)
		goto out;
	memcpy(&addr, res->ai_addr, sizeof(addr));

	if (res->ai_family != AF_INET6 ||
		!IN6_IS_ADDR_LINKLOCAL(&addr.in6.sin6_addr))
	{
		freeaddrinfo(res);
		goto out;
	}
	freeaddrinfo(res);

	if(strlen(buddy_ip) <= strlen(host) ||
	   buddy_ip[strlen(host)] != '%')
		return FALSE;

	return !strncmp(host, buddy_ip, strlen(host));

out:
#endif
	return purple_strequal(host, buddy_ip);
}

static inline gint
xep_addr_differ(const char *buddy_ip, const char *host)
{
	return !xep_cmp_addr(host, buddy_ip);
}

/**
 * Create and insert an identical twin
 *
 * Creates a copy of the specified node and inserts it right after
 * this original node.
 *
 * @param node	The node to clone
 * @return	A pointer to the new, cloned twin if successful
 *		or NULL otherwise.
 */
static PurpleXmlNode *
purple_xmlnode_insert_twin_copy(PurpleXmlNode *node) {
	PurpleXmlNode *copy;

	g_return_val_if_fail(node != NULL, NULL);

	copy = purple_xmlnode_copy(node);
	g_return_val_if_fail(copy != NULL, NULL);

	copy->next = node->next;
	node->next = copy;

	return copy;
}

/**
 * Tries to append an interface scope to an IPv6 link local address.
 *
 * If the given address is a link local IPv6 address (with no
 * interface scope) then we try to determine all fitting interfaces
 * from our Bonjour IP address list.
 *
 * For any such found matches we insert a copy of our current xml
 * streamhost entry right after this streamhost entry and append
 * the determined interface to the host address of this copy.
 *
 * @param cur_streamhost	The XML streamhost node we examine
 * @param host	The host address to examine in text form
 * @param pb	Buddy to get the list of link local IPv6 addresses
 *		and their interface from
 * @return	Returns TRUE if the specified 'host' address is a
 *		link local IPv6 address with no interface scope.
 *		Otherwise returns FALSE.
 */
static gboolean
add_ipv6_link_local_ifaces(PurpleXmlNode *cur_streamhost, const char *host,
			   PurpleBuddy *pb)
{
	PurpleXmlNode *new_streamhost = NULL;
	struct in6_addr in6_addr;
	BonjourBuddy *bb;
	GSList *ip_elem;

	if (inet_pton(AF_INET6, host, &in6_addr) != 1 ||
	    !IN6_IS_ADDR_LINKLOCAL(&in6_addr) ||
	    strchr(host, '%'))
		return FALSE;

	bb = purple_buddy_get_protocol_data(pb);

	for (ip_elem = bb->ips;
	     (ip_elem = g_slist_find_custom(ip_elem, host, (GCompareFunc)&xep_addr_differ));
	     ip_elem = ip_elem->next) {
		purple_debug_info("bonjour", "Inserting an PurpleXmlNode twin copy for %s with new host address %s\n",
				  host, (char*)ip_elem->data);
		new_streamhost = purple_xmlnode_insert_twin_copy(cur_streamhost);
		purple_xmlnode_set_attrib(new_streamhost, "host", ip_elem->data);
	}

	if (!new_streamhost)
		purple_debug_info("bonjour", "No interface for this IPv6 link local address found: %s\n",
				  host);

	return TRUE;
}

static gboolean
__xep_bytestreams_parse(PurpleBuddy *pb, PurpleXfer *xfer, PurpleXmlNode *streamhost,
			const char *iq_id)
{
	char *tmp_iq_id;
	const char *jid, *host, *port;
	int portnum;
	XepXfer *xf = XEP_XFER(xfer);

	for(; streamhost; streamhost = purple_xmlnode_get_next_twin(streamhost)) {
		if(!(jid = purple_xmlnode_get_attrib(streamhost, "jid")) ||
		   !(host = purple_xmlnode_get_attrib(streamhost, "host")) ||
		   !(port = purple_xmlnode_get_attrib(streamhost, "port")) ||
		   !(portnum = atoi(port))) {
			purple_debug_info("bonjour", "bytestream offer Message parse error.\n");
			continue;
		}

		/* skip IPv6 link local addresses with no interface scope
		 * (but try to add a new one with an interface scope then) */
		if(add_ipv6_link_local_ifaces(streamhost, host, pb))
			continue;

		tmp_iq_id = g_strdup(iq_id);
		g_free(xf->iq_id);
		g_free(xf->jid);
		g_free(xf->proxy_host);

		xf->iq_id = tmp_iq_id;
		xf->jid = g_strdup(jid);
		xf->proxy_host = g_strdup(host);
		xf->proxy_port = portnum;
		xf->streamhost = streamhost;
		xf->pb = pb;
		purple_debug_info("bonjour", "bytestream offer parse"
				  "jid=%s host=%s port=%d.\n", jid, host, portnum);
		bonjour_bytestreams_connect(xfer);
		return TRUE;
	}

	return FALSE;
}

void
xep_bytestreams_parse(PurpleConnection *pc, PurpleXmlNode *packet, PurpleBuddy *pb)
{
	const char *type, *from, *iq_id, *sid;
	PurpleXmlNode *query, *streamhost;
	BonjourData *bd;
	PurpleXfer *xfer;

	g_return_if_fail(pc != NULL);
	g_return_if_fail(packet != NULL);
	g_return_if_fail(pb != NULL);

	bd = purple_connection_get_protocol_data(pc);
	if(bd == NULL)
		return;

	purple_debug_info("bonjour", "xep-bytestreams-parse.\n");

	type = purple_xmlnode_get_attrib(packet, "type");
	from = purple_buddy_get_name(pb);
	query = purple_xmlnode_get_child(packet,"query");
	if(!type)
		return;

	query = purple_xmlnode_copy(query);
	if (!query)
		return;

	if(!purple_strequal(type, "set")) {
		purple_debug_info("bonjour", "bytestream offer Message type - Unknown-%s.\n", type);
		return;
	}

	purple_debug_info("bonjour", "bytestream offer Message type - SET.\n");

	iq_id = purple_xmlnode_get_attrib(packet, "id");

	sid = purple_xmlnode_get_attrib(query, "sid");
	xfer = bonjour_si_xfer_find(bd, sid, from);
	streamhost = purple_xmlnode_get_child(query, "streamhost");

	if(xfer && streamhost && __xep_bytestreams_parse(pb, xfer, streamhost, iq_id))
		return; /* success */

	purple_debug_error("bonjour", "Didn't find an acceptable streamhost.\n");

	if (iq_id && xfer != NULL)
		xep_ft_si_reject(bd, iq_id, purple_xfer_get_remote_user(xfer), "404", "cancel");
}

static void
bonjour_xfer_receive(PurpleConnection *pc, const char *id, const char *sid, const char *from,
		     goffset filesize, const char *filename, int option)
{
	PurpleXfer *xfer;
	XepXfer *xf;
	BonjourData *bd;

	if(pc == NULL || id == NULL || from == NULL)
		return;

	bd = purple_connection_get_protocol_data(pc);
	if(bd == NULL)
		return;

	purple_debug_info("bonjour", "bonjour-xfer-receive.\n");

	/* Build the file transfer handle */
	xf = g_object_new(
		XEP_TYPE_XFER,
		"account", purple_connection_get_account(pc),
		"type", PURPLE_XFER_TYPE_RECEIVE,
		"remote-user", from,
		NULL
	);

	xfer = PURPLE_XFER(xf);

	xf->data = bd;
	purple_xfer_set_filename(xfer, filename);
	xf->iq_id = g_strdup(id);
	xf->sid = g_strdup(sid);

	if(filesize > 0)
		purple_xfer_set_size(xfer, filesize);

	bd->xfer_lists = g_slist_append(bd->xfer_lists, xfer);

	purple_xfer_request(xfer);
}

static void
bonjour_sock5_request_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleXfer *xfer = data;
	XepXfer *xf = XEP_XFER(xfer);
	int len = 0;

	if(xf == NULL)
		return;

	purple_debug_info("bonjour", "bonjour_sock5_request_cb - req_state = 0x%x\n", xf->sock5_req_state);

	switch(xf->sock5_req_state){
	case 0x01:
		purple_xfer_set_fd(xfer, source);
		len = read(source, xf->rx_buf + xf->rxlen, 3);
		if(len < 0 && errno == EAGAIN)
			return;
		else if(len <= 0){
			purple_xfer_cancel_remote(xfer);
			return;
		} else {
			purple_input_remove(purple_xfer_get_watcher(xfer));
			purple_xfer_set_watcher(xfer, purple_input_add(source, PURPLE_INPUT_WRITE,
							 bonjour_sock5_request_cb, xfer));
			xf->sock5_req_state++;
			xf->rxlen = 0;
			bonjour_sock5_request_cb(xfer, source, PURPLE_INPUT_WRITE);
		}
		break;
	case 0x02:
		xf->tx_buf[0] = 0x05;
		xf->tx_buf[1] = 0x00;
		len = write(source, xf->tx_buf, 2);
		if (len < 0 && errno == EAGAIN)
			return;
		else if (len < 0) {
			close(source);
			purple_xfer_cancel_remote(xfer);
			return;
		} else {
			purple_input_remove(purple_xfer_get_watcher(xfer));
			purple_xfer_set_watcher(xfer, purple_input_add(source, PURPLE_INPUT_READ,
							 bonjour_sock5_request_cb, xfer));
			xf->sock5_req_state++;
			xf->rxlen = 0;
		}
		break;
	case 0x03:
		len = read(source, xf->rx_buf + xf->rxlen, 20);
		if(len<=0){
		} else {
			purple_input_remove(purple_xfer_get_watcher(xfer));
			purple_xfer_set_watcher(xfer, purple_input_add(source, PURPLE_INPUT_WRITE,
							 bonjour_sock5_request_cb, xfer));
			xf->sock5_req_state++;
			xf->rxlen = 0;
			bonjour_sock5_request_cb(xfer, source, PURPLE_INPUT_WRITE);
		}
		break;
	case 0x04:
		xf->tx_buf[0] = 0x05;
		xf->tx_buf[1] = 0x00;
		xf->tx_buf[2] = 0x00;
		xf->tx_buf[3] = 0x03;
		xf->tx_buf[4] = strlen(xf->buddy_ip);
		memcpy(xf->tx_buf + 5, xf->buddy_ip, strlen(xf->buddy_ip));
		xf->tx_buf[5+strlen(xf->buddy_ip)] = 0x00;
		xf->tx_buf[6+strlen(xf->buddy_ip)] = 0x00;
		len = write(source, xf->tx_buf, 7 + strlen(xf->buddy_ip));
		if (len < 0 && errno == EAGAIN) {
			return;
		} else if (len < 0) {
			close(source);
			purple_xfer_cancel_remote(xfer);
			return;
		} else {
			purple_input_remove(purple_xfer_get_watcher(xfer));
			purple_xfer_set_watcher(xfer, 0);
			xf->rxlen = 0;
			/*close(source);*/
			purple_xfer_start(xfer, source, NULL, -1);
		}
		break;
	default:
		break;
	}
}

static void
bonjour_sock5_incoming_cb(GSocketService *service,
                          GSocketConnection *connection, GObject *source_object,
                          G_GNUC_UNUSED gpointer data)
{
	PurpleXfer *xfer = PURPLE_XFER(source_object);
	XepXfer *xf = XEP_XFER(xfer);
	GSocket *sock;
	int fd;

	if (xf == NULL) {
		return;
	}

	purple_debug_info("bonjour", "bonjour_sock5_incoming_cb - req_state = 0x%x",
	                  xf->sock5_req_state);

	if (G_UNLIKELY(xf->sock5_req_state != 0x0)) {
		purple_debug_error("bonjour", "bonjour_sock5_incoming_cb - socks5 "
		                              "proxy is in invalid state.");
		purple_xfer_cancel_local(xfer);
		return;
	}

	xf->conn = g_object_ref(connection);
	g_socket_service_stop(xf->service);
	g_clear_object(&xf->service);

	sock = g_socket_connection_get_socket(connection);
	fd = g_socket_get_fd(sock);
	purple_debug_info("bonjour", "Accepted SOCKS5 ft connection - fd=%d", fd);

	_purple_network_set_common_socket_flags(fd);
	purple_xfer_set_watcher(xfer,
	                        purple_input_add(fd, PURPLE_INPUT_READ,
	                                         bonjour_sock5_request_cb, xfer));
	xf->sock5_req_state++;
	xf->rxlen = 0;
}

static void
bonjour_bytestreams_init(PurpleXfer *xfer)
{
	XepXfer *xf;
	XepIq *iq;
	PurpleXmlNode *query, *streamhost;
	guint16 port;
	gchar *port_str;
	GList *local_ips;
	BonjourData *bd;
	GError *error = NULL;

	if (xfer == NULL) {
		return;
	}

	purple_debug_info("bonjour", "Bonjour-bytestreams-init.\n");
	xf = XEP_XFER(xfer);

	xf->service = g_socket_service_new();
	port = purple_socket_listener_add_any_inet_port(
	        G_SOCKET_LISTENER(xf->service), G_OBJECT(xfer), &error);
	if (port == 0) {
		purple_debug_error("bonjour",
		                   "Unable to open port for file transfer: %s",
		                   error->message);
		purple_xfer_cancel_local(xfer);
		g_error_free(error);
		return;
	}

	g_signal_connect(xf->service, "incoming",
	                 G_CALLBACK(bonjour_sock5_incoming_cb), NULL);

	bd = xf->data;

	iq = xep_iq_new(bd, XEP_IQ_SET, purple_xfer_get_remote_user(xfer), bonjour_get_jid(bd->xmpp_data->account), xf->sid);

	query = purple_xmlnode_new_child(iq->node, "query");
	purple_xmlnode_set_namespace(query, "http://jabber.org/protocol/bytestreams");
	purple_xmlnode_set_attrib(query, "sid", xf->sid);
	purple_xmlnode_set_attrib(query, "mode", "tcp");

	purple_xfer_set_local_port(xfer, port);

	local_ips = purple_network_get_all_local_system_ips();

	port_str = g_strdup_printf("%hu", port);
	while(local_ips) {
		streamhost = purple_xmlnode_new_child(query, "streamhost");
		purple_xmlnode_set_attrib(streamhost, "jid", xf->sid);
		purple_xmlnode_set_attrib(streamhost, "host", local_ips->data);
		purple_xmlnode_set_attrib(streamhost, "port", port_str);
		g_free(local_ips->data);
		local_ips = g_list_delete_link(local_ips, local_ips);
	}
	g_free(port_str);

	xep_iq_send_and_free(iq);
}

static void
bonjour_bytestreams_connect_cb(gpointer data, gint source, const gchar *error_message)
{
	PurpleXfer *xfer = data;
	XepXfer *xf = XEP_XFER(xfer);
	XepIq *iq;
	PurpleXmlNode *q_node, *tmp_node;
	BonjourData *bd;
	gboolean ret = FALSE;

	xf->proxy_connection = NULL;

	if(source < 0) {
		purple_debug_error("bonjour", "Error connecting via SOCKS5 to %s - %s\n",
			xf->proxy_host, error_message ? error_message : "(null)");

		tmp_node = purple_xmlnode_get_next_twin(xf->streamhost);
		ret = __xep_bytestreams_parse(xf->pb, xfer, tmp_node, xf->iq_id);

		if (!ret) {
			xep_ft_si_reject(xf->data, xf->iq_id, purple_xfer_get_remote_user(xfer), "404", "cancel");
			/* Cancel the connection */
			purple_xfer_cancel_local(xfer);
		}
		return;
	}

	purple_debug_info("bonjour", "Connected successfully via SOCKS5, starting transfer.\n");

	bd = xf->data;

	/* Here, start the file transfer.*/

	/* Notify Initiator of Connection */
	iq = xep_iq_new(bd, XEP_IQ_RESULT, purple_xfer_get_remote_user(xfer), bonjour_get_jid(bd->xmpp_data->account), xf->iq_id);
	q_node = purple_xmlnode_new_child(iq->node, "query");
	purple_xmlnode_set_namespace(q_node, "http://jabber.org/protocol/bytestreams");
	tmp_node = purple_xmlnode_new_child(q_node, "streamhost-used");
	purple_xmlnode_set_attrib(tmp_node, "jid", xf->jid);
	xep_iq_send_and_free(iq);

	purple_xfer_start(xfer, source, NULL, -1);
}

static void
bonjour_bytestreams_connect(PurpleXfer *xfer)
{
	PurpleBuddy *pb;
	PurpleAccount *account = NULL;
	GChecksum *hash;
	XepXfer *xf;
	char dstaddr[41];
	const gchar *name = NULL;
	unsigned char hashval[20];
	gsize digest_len = 20;
	char *p;
	int i;

	if(xfer == NULL)
		return;

	purple_debug_info("bonjour", "bonjour-bytestreams-connect.\n");

	xf = XEP_XFER(xfer);

	pb = xf->pb;
	name = purple_buddy_get_name(pb);
	account = purple_buddy_get_account(pb);

	p = g_strdup_printf("%s%s%s", xf->sid, name, bonjour_get_jid(account));

	hash = g_checksum_new(G_CHECKSUM_SHA1);
	g_checksum_update(hash, (guchar *)p, -1);
	g_checksum_get_digest(hash, hashval, &digest_len);
	g_checksum_free(hash);

	g_free(p);

	memset(dstaddr, 0, 41);
	p = dstaddr;
	for (i = 0; i < 20; i++, p += 2) {
		g_snprintf(p, 3, "%02x", hashval[i]);
	}

	xf->proxy_info = purple_proxy_info_new();
	purple_proxy_info_set_proxy_type(xf->proxy_info, PURPLE_PROXY_SOCKS5);
	purple_proxy_info_set_host(xf->proxy_info, xf->proxy_host);
	purple_proxy_info_set_port(xf->proxy_info, xf->proxy_port);
	xf->proxy_connection = purple_proxy_connect_socks5_account(
							   purple_account_get_connection(account),
							   account,
							   xf->proxy_info,
							   dstaddr, 0,
							   bonjour_bytestreams_connect_cb, xfer);

	if(xf->proxy_connection == NULL) {
		xep_ft_si_reject(xf->data, xf->iq_id, purple_xfer_get_remote_user(xfer), "404", "cancel");
		/* Cancel the connection */
		purple_xfer_cancel_local(xfer);
	}
}

static void
xep_xfer_init(XepXfer *xfer) {

}

static void
xep_xfer_finalize(GObject *obj) {
	XepXfer *xf = XEP_XFER(obj);
	BonjourData *bd = (BonjourData*)xf->data;

	if(bd != NULL) {
		bd->xfer_lists = g_slist_remove(bd->xfer_lists, PURPLE_XFER(xf));
		purple_debug_misc("bonjour", "B free xfer from lists(%p).\n", bd->xfer_lists);
	}
	if (xf->proxy_connection != NULL) {
		purple_proxy_connect_cancel(xf->proxy_connection);
	}
	if (xf->proxy_info != NULL) {
		purple_proxy_info_destroy(xf->proxy_info);
	}
	if (xf->service) {
		g_socket_service_stop(xf->service);
	}
	g_clear_object(&xf->service);
	g_clear_object(&xf->conn);

	g_free(xf->iq_id);
	g_free(xf->jid);
	g_free(xf->proxy_host);
	g_free(xf->buddy_ip);
	g_free(xf->sid);

	g_clear_pointer(&xf->streamhost, purple_xmlnode_free_tree);

	G_OBJECT_CLASS(xep_xfer_parent_class)->finalize(obj);
}

static void
xep_xfer_class_finalize(XepXferClass *klass) {

}

static void
xep_xfer_class_init(XepXferClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	PurpleXferClass *xfer_class = PURPLE_XFER_CLASS(klass);

	obj_class->finalize = xep_xfer_finalize;

	xfer_class->init = bonjour_xfer_init;
	xfer_class->request_denied = bonjour_xfer_request_denied;
	xfer_class->cancel_recv = bonjour_xfer_cancel_recv;
	xfer_class->cancel_send = bonjour_xfer_cancel_send;
	xfer_class->end = bonjour_xfer_end;
}

void
xep_xfer_register(GTypeModule *module) {
	xep_xfer_register_type(module);
}
