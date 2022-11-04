/*
 * purple - Jabber Protocol Plugin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301 USA
 *
 */
#include <glib/gi18n-lib.h>

#include <purple.h>

#include <libsoup/soup.h>
#include "libpurple/soupcompat.h"

#include "bosh.h"

/*
TODO: test, what happens, if the http server (BOSH server) doesn't support
keep-alive (sends connection: close).
*/

#define JABBER_BOSH_SEND_DELAY 250

#define JABBER_BOSH_TIMEOUT 10

static gchar *jabber_bosh_useragent = NULL;

struct _PurpleJabberBOSHConnection {
	JabberStream *js;
	SoupSession *payload_reqs;

	gchar *url;
	gboolean is_ssl;
	gboolean is_terminating;

	gchar *sid;
	guint64 rid; /* Must be big enough to hold 2^53 - 1 */

	GString *send_buff;
	guint send_timer;
};

static SoupMessage *jabber_bosh_connection_http_request_new(
        PurpleJabberBOSHConnection *conn, const GString *data);
static void
jabber_bosh_connection_session_create(PurpleJabberBOSHConnection *conn);
static void
jabber_bosh_connection_send_now(PurpleJabberBOSHConnection *conn);

void
jabber_bosh_init(void)
{
	PurpleUi *ui = purple_core_get_ui();
	const gchar *ui_name = NULL;
	const gchar *ui_version = NULL;

	if(PURPLE_IS_UI(ui)) {
		ui_name = purple_ui_get_name(ui);
		ui_version = purple_ui_get_version(ui);
	}

	if(ui_name) {
		jabber_bosh_useragent = g_strdup_printf(
			"%s%s%s (libpurple " VERSION ")", ui_name,
			ui_version ? " " : "", ui_version ? ui_version : "");
	} else {
		jabber_bosh_useragent = g_strdup("libpurple " VERSION);
	}
}

void jabber_bosh_uninit(void)
{
	g_free(jabber_bosh_useragent);
	jabber_bosh_useragent = NULL;
}

PurpleJabberBOSHConnection*
jabber_bosh_connection_new(JabberStream *js, const gchar *url)
{
	PurpleJabberBOSHConnection *conn;
	PurpleAccount *account;
	GProxyResolver *resolver;
	GError *error = NULL;
	const gchar *scheme;

	account = purple_connection_get_account(js->gc);
	resolver = purple_proxy_get_proxy_resolver(account, &error);
	if (resolver == NULL) {
		purple_debug_error("jabber-bosh",
		                   "Unable to get account proxy resolver: %s",
		                   error->message);
		g_error_free(error);
		return NULL;
	}

	scheme = g_uri_peek_scheme(url);
	if (scheme == NULL) {
		purple_debug_error("jabber-bosh", "Unable to parse given BOSH URL: %s",
		                   url);
		g_object_unref(resolver);
		return NULL;
	}

	conn = g_new0(PurpleJabberBOSHConnection, 1);
	conn->payload_reqs = soup_session_new_with_options(
	        "proxy-resolver", resolver,
	        "timeout", JABBER_BOSH_TIMEOUT + 2,
	        "user-agent", jabber_bosh_useragent,
	        NULL);
	conn->url = g_strdup(url);
	conn->js = js;
	conn->is_ssl = g_str_equal(scheme, "https");
	conn->send_buff = g_string_new(NULL);

	/*
	 * Random 64-bit integer masked off by 2^52 - 1.
	 *
	 * This should produce a random integer in the range [0, 2^52). It's
	 * unlikely we'll send enough packets in one session to overflow the
	 * rid.
	 */
	conn->rid = (((guint64)g_random_int() << 32) | g_random_int());
	conn->rid &= 0xFFFFFFFFFFFFFLL;

	g_object_unref(resolver);

	jabber_bosh_connection_session_create(conn);

	return conn;
}

void
jabber_bosh_connection_destroy(PurpleJabberBOSHConnection *conn)
{
	if (conn == NULL || conn->is_terminating)
		return;
	conn->is_terminating = TRUE;

	if (conn->sid != NULL) {
		purple_debug_info("jabber-bosh",
			"Terminating a session for %p\n", conn);
		jabber_bosh_connection_send_now(conn);
	}

	if (conn->send_timer)
		g_source_remove(conn->send_timer);

	soup_session_abort(conn->payload_reqs);

	g_clear_object(&conn->payload_reqs);
	g_string_free(conn->send_buff, TRUE);
	conn->send_buff = NULL;

	g_free(conn->sid);
	conn->sid = NULL;
	g_free(conn->url);
	conn->url = NULL;

	g_free(conn);
}

gboolean
jabber_bosh_connection_is_ssl(const PurpleJabberBOSHConnection *conn)
{
	return conn->is_ssl;
}

static PurpleXmlNode *
jabber_bosh_connection_parse(PurpleJabberBOSHConnection *conn,
                             SoupMessage *msg, GBytes *response_body,
                             GError *error)
{
	gconstpointer body = NULL;
	gsize length = 0;
	PurpleXmlNode *root;
	const gchar *type;

	g_return_val_if_fail(conn != NULL, NULL);
	g_return_val_if_fail(msg != NULL, NULL);

	if (conn->is_terminating || purple_account_is_disconnecting(
		purple_connection_get_account(conn->js->gc)))
	{
		return NULL;
	}

	if(error != NULL ||
	   !SOUP_STATUS_IS_SUCCESSFUL(soup_message_get_status(msg))) {
		gchar *tmp = g_strdup_printf(
		    _("Unable to connect: %s"),
		    error ? error->message : soup_message_get_reason_phrase(msg));
		purple_connection_error(conn->js->gc,
		                        PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
		g_free(tmp);
		return NULL;
	}

	body = g_bytes_get_data(response_body, &length);
	root = purple_xmlnode_from_str(body, length);

	type = purple_xmlnode_get_attrib(root, "type");
	if (purple_strequal(type, "terminate")) {
		purple_connection_error(conn->js->gc,
			PURPLE_CONNECTION_ERROR_OTHER_ERROR, _("The BOSH "
			"connection manager terminated your session."));
		purple_xmlnode_free(root);
		return NULL;
	}

	return root;
}

static void
jabber_bosh_connection_recv(GObject *source, GAsyncResult *result,
                            gpointer data)
{
	SoupMessage *msg = data;
	PurpleJabberBOSHConnection *bosh_conn = NULL;
	GBytes *response_body = NULL;
	GError *error = NULL;
	PurpleXmlNode *node, *child;

	response_body = soup_session_send_and_read_finish(SOUP_SESSION(source),
	                                                  result, &error);
	bosh_conn = g_object_get_data(G_OBJECT(msg), "bosh-connection");

	if (response_body != NULL && purple_debug_is_verbose() &&
	    purple_debug_is_unsafe())
	{
		const gchar *body = NULL;
		gsize length = 0;

		body = g_bytes_get_data(response_body, &length);
		purple_debug_misc("jabber-bosh", "received: %*s", (int)length, body);
	}

	node = jabber_bosh_connection_parse(bosh_conn, msg, response_body, error);
	g_clear_pointer(&response_body, g_bytes_unref);
	g_clear_error(&error);
	g_clear_object(&msg);

	if (node == NULL) {
		return;
	}

	child = node->child;
	while (child != NULL) {
		/* jabber_process_packet might free child */
		PurpleXmlNode *next = child->next;
		const gchar *xmlns;

		if (child->type != PURPLE_XMLNODE_TYPE_TAG) {
			child = next;
			continue;
		}

		/* Workaround for non-compliant servers that don't stamp
		 * the right xmlns on these packets. See #11315.
		 */
		xmlns = purple_xmlnode_get_namespace(child);
		if ((xmlns == NULL || purple_strequal(xmlns, NS_BOSH)) &&
			(purple_strequal(child->name, "iq") ||
			purple_strequal(child->name, "message") ||
			purple_strequal(child->name, "presence")))
		{
			purple_xmlnode_set_namespace(child, NS_XMPP_CLIENT);
		}

		jabber_process_packet(bosh_conn->js, &child);

		child = next;
	}

	jabber_bosh_connection_send(bosh_conn, NULL);
}

static void
jabber_bosh_connection_send_now(PurpleJabberBOSHConnection *conn)
{
	SoupMessage *req;
	GString *data;

	g_return_if_fail(conn != NULL);

	if (conn->send_timer != 0) {
		g_source_remove(conn->send_timer);
		conn->send_timer = 0;
	}

	if (conn->sid == NULL)
		return;

	data = g_string_new(NULL);

	/* missing parameters: route, from, ack */
	g_string_printf(data, "<body "
		"rid='%" G_GUINT64_FORMAT "' "
		"sid='%s' "
		"xmlns='" NS_BOSH "' "
		"xmlns:xmpp='" NS_XMPP_BOSH "' ",
		++conn->rid, conn->sid);

	if (conn->js->reinit && !conn->is_terminating) {
		g_string_append(data, "xmpp:restart='true'/>");
		conn->js->reinit = FALSE;
	} else {
		if (conn->is_terminating)
			g_string_append(data, "type='terminate' ");
		g_string_append_c(data, '>');
		g_string_append_len(data, conn->send_buff->str,
			conn->send_buff->len);
		g_string_append(data, "</body>");
		g_string_set_size(conn->send_buff, 0);
	}

	if (purple_debug_is_verbose() && purple_debug_is_unsafe())
		purple_debug_misc("jabber-bosh", "sending: %s\n", data->str);

	req = jabber_bosh_connection_http_request_new(conn, data);
	g_string_free(data, FALSE);

	if (conn->is_terminating) {
#if SOUP_MAJOR_VERSION >= 3
		soup_session_send_async(conn->payload_reqs, req, G_PRIORITY_DEFAULT,
		                        NULL, NULL, NULL);
#else
		soup_session_send_async(conn->payload_reqs, req, NULL, NULL, NULL);
#endif
		g_free(conn->sid);
		conn->sid = NULL;
	} else {
		g_object_set_data(G_OBJECT(req), "bosh-connection", conn);
		soup_session_send_and_read_async(conn->payload_reqs, req,
		                                 G_PRIORITY_DEFAULT, NULL,
		                                 jabber_bosh_connection_recv, req);
	}
}

static gboolean
jabber_bosh_connection_send_delayed(gpointer _conn)
{
	PurpleJabberBOSHConnection *conn = _conn;

	conn->send_timer = 0;
	jabber_bosh_connection_send_now(conn);

	return FALSE;
}

void
jabber_bosh_connection_send(PurpleJabberBOSHConnection *conn,
	const gchar *data)
{
	g_return_if_fail(conn != NULL);

	if (data)
		g_string_append(conn->send_buff, data);

	if (conn->send_timer == 0) {
		conn->send_timer = g_timeout_add(
			JABBER_BOSH_SEND_DELAY,
			jabber_bosh_connection_send_delayed, conn);
	}
}

void
jabber_bosh_connection_send_keepalive(PurpleJabberBOSHConnection *conn)
{
	g_return_if_fail(conn != NULL);

	jabber_bosh_connection_send_now(conn);
}

static gboolean
jabber_bosh_version_check(const gchar *version, int major_req, int minor_min)
{
	const gchar *dot;
	int major, minor = 0;

	if (version == NULL)
		return FALSE;

	major = atoi(version);
	dot = strchr(version, '.');
	if (dot)
		minor = atoi(dot + 1);

	if (major != major_req)
		return FALSE;
	if (minor < minor_min)
		return FALSE;
	return TRUE;
}

static void
jabber_bosh_connection_session_created(GObject *source, GAsyncResult *result,
                                       gpointer data)
{
	SoupMessage *msg = data;
	PurpleJabberBOSHConnection *bosh_conn = NULL;
	GBytes *response_body = NULL;
	GError *error = NULL;
	PurpleXmlNode *node, *features;
	const gchar *sid, *ver, *inactivity_str;
	int inactivity = 0;

	response_body = soup_session_send_and_read_finish(SOUP_SESSION(source),
	                                                  result, &error);
	bosh_conn = g_object_get_data(G_OBJECT(msg), "bosh-connection");

	if (response_body != NULL && purple_debug_is_verbose() &&
	    purple_debug_is_unsafe())
	{
		const gchar *body = NULL;
		gsize length = 0;

		body = g_bytes_get_data(response_body, &length);
		purple_debug_misc("jabber-bosh", "received (session creation): %*s",
		                  (int)length, body);
	}

	node = jabber_bosh_connection_parse(bosh_conn, msg, response_body, error);
	g_clear_pointer(&response_body, g_bytes_unref);
	g_clear_error(&error);
	g_clear_object(&msg);

	if (node == NULL) {
		return;
	}

	sid = purple_xmlnode_get_attrib(node, "sid");
	ver = purple_xmlnode_get_attrib(node, "ver");
	inactivity_str = purple_xmlnode_get_attrib(node, "inactivity");
	/* requests = purple_xmlnode_get_attrib(node, "requests"); */

	if (!sid) {
		purple_connection_error(bosh_conn->js->gc,
			PURPLE_CONNECTION_ERROR_OTHER_ERROR,
			_("No BOSH session ID given"));
		purple_xmlnode_free(node);
		return;
	}

	if (ver == NULL) {
		purple_debug_info("jabber-bosh", "Missing version in BOSH initiation\n");
	} else if (!jabber_bosh_version_check(ver, 1, 6)) {
		purple_debug_error("jabber-bosh",
			"Unsupported BOSH version: %s\n", ver);
		purple_connection_error(bosh_conn->js->gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			_("Unsupported version of BOSH protocol"));
		purple_xmlnode_free(node);
		return;
	}

	purple_debug_misc("jabber-bosh", "Session created for %p\n", bosh_conn);

	bosh_conn->sid = g_strdup(sid);

	if (inactivity_str)
		inactivity = atoi(inactivity_str);
	if (inactivity < 0 || inactivity > 3600) {
		purple_debug_warning("jabber-bosh", "Ignoring invalid "
			"inactivity value: %s\n", inactivity_str);
		inactivity = 0;
	}
	if (inactivity > 0) {
		inactivity -= 5; /* rounding */
		if (inactivity <= 0)
			inactivity = 1;
		bosh_conn->js->max_inactivity = inactivity;
		if (bosh_conn->js->inactivity_timer == 0) {
			purple_debug_misc("jabber-bosh", "Starting inactivity "
				"timer for %d secs (compensating for "
				"rounding)\n", inactivity);
			jabber_stream_restart_inactivity_timer(bosh_conn->js);
		}
	}

	jabber_stream_set_state(bosh_conn->js, JABBER_STREAM_AUTHENTICATING);

	/* FIXME: Depending on receiving features might break with some hosts */
	features = purple_xmlnode_get_child(node, "features");
	jabber_stream_features_parse(bosh_conn->js, features);

	purple_xmlnode_free(node);

	jabber_bosh_connection_send(bosh_conn, NULL);
}

static void
jabber_bosh_connection_session_create(PurpleJabberBOSHConnection *conn)
{
	SoupMessage *req;
	GString *data;

	purple_debug_misc("jabber-bosh", "Requesting Session Create for %p\n",
		conn);

	data = g_string_new(NULL);

	/* missing optional parameters: route, from, ack */
	g_string_printf(data, "<body content='text/xml; charset=utf-8' "
		"rid='%" G_GUINT64_FORMAT "' "
		"to='%s' "
		"xml:lang='en' "
		"ver='1.10' "
		"wait='%d' "
		"hold='1' "
		"xmlns='" NS_BOSH "' "
		"xmpp:version='1.0' "
		"xmlns:xmpp='urn:xmpp:xbosh' "
		"/>",
		++conn->rid, conn->js->user->domain, JABBER_BOSH_TIMEOUT);

	req = jabber_bosh_connection_http_request_new(conn, data);
	g_string_free(data, FALSE);

	g_object_set_data(G_OBJECT(req), "bosh-connection", conn);
	soup_session_send_and_read_async(conn->payload_reqs, req,
	                                 G_PRIORITY_DEFAULT, NULL,
	                                 jabber_bosh_connection_session_created,
	                                 req);
}

static SoupMessage *
jabber_bosh_connection_http_request_new(PurpleJabberBOSHConnection *conn,
                                        const GString *data)
{
	SoupMessage *req;
	GBytes *body = NULL;

	jabber_stream_restart_inactivity_timer(conn->js);

	req = soup_message_new("POST", conn->url);
	body = g_bytes_new_take(data->str, data->len);
	soup_message_set_request_body_from_bytes(req, "text/xml; charset=utf-8",
	                                         body);
	g_bytes_unref(body);

	return req;
}
