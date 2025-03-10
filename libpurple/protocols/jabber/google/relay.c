/**
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

#include "internal.h"
#include <purple.h>

#include "relay.h"

typedef struct {
	GoogleSession *session;
	JabberGoogleRelayCallback *cb;
} JabberGoogleRelayCallbackData;

static void
jabber_google_relay_parse_response(const gchar *response, gchar **ip,
	guint *udp, guint *tcp, guint *ssltcp, gchar **username, gchar **password)
{
	gchar **lines = g_strsplit(response, "\n", -1);
	int i = 0;

	for (; lines[i] ; i++) {
		gchar *line = lines[i];
		gchar **parts = g_strsplit(line, "=", 2);

		if (parts[0] && parts[1]) {
			if (purple_strequal(parts[0], "relay.ip")) {
				*ip = g_strdup(parts[1]);
			} else if (purple_strequal(parts[0], "relay.udp_port")) {
				*udp = atoi(parts[1]);
			} else if (purple_strequal(parts[0], "relay.tcp_port")) {
				*tcp = atoi(parts[1]);
			} else if (purple_strequal(parts[0], "relay.ssltcp_port")) {
				*ssltcp = atoi(parts[1]);
			} else if (purple_strequal(parts[0], "username")) {
				*username = g_strdup(parts[1]);
			} else if (purple_strequal(parts[0], "password")) {
				*password = g_strdup(parts[1]);
			}
		}
		g_strfreev(parts);
	}

	g_strfreev(lines);
}

static void
jabber_google_relay_fetch_cb(G_GNUC_UNUSED SoupSession *soup, SoupMessage *msg,
                             gpointer user_data)
{
	JabberGoogleRelayCallbackData *data =
		(JabberGoogleRelayCallbackData *) user_data;
	GoogleSession *session = data->session;
	JabberGoogleRelayCallback *cb = data->cb;
	gchar *relay_ip = NULL;
	guint relay_udp = 0;
	guint relay_tcp = 0;
	guint relay_ssltcp = 0;
	gchar *relay_username = NULL;
	gchar *relay_password = NULL;

	g_free(data);

	purple_debug_info("jabber", "got response on HTTP request to relay server\n");

	if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		const gchar *got_data = msg->response_body->data;
		purple_debug_info("jabber", "got Google relay request response:\n%s\n",
			got_data);
		jabber_google_relay_parse_response(got_data, &relay_ip, &relay_udp,
			&relay_tcp, &relay_ssltcp, &relay_username, &relay_password);
	}

	if (cb)
		cb(session, relay_ip, relay_udp, relay_tcp, relay_ssltcp,
		   relay_username, relay_password);

	g_free(relay_ip);
	g_free(relay_username);
	g_free(relay_password);
}

void
jabber_google_do_relay_request(JabberStream *js, GoogleSession *session,
	JabberGoogleRelayCallback cb)
{
	SoupURI *uri;
	SoupMessage *msg;
	JabberGoogleRelayCallbackData *data = g_new0(JabberGoogleRelayCallbackData, 1);

	data->session = session;
	data->cb = cb;
	purple_debug_info("jabber", "sending Google relay request\n");

	uri = soup_uri_new(NULL);
	soup_uri_set_scheme(uri, SOUP_URI_SCHEME_HTTP);
	soup_uri_set_host(uri, js->google_relay_host);
	soup_uri_set_path(uri, "/create_session");
	msg = soup_message_new_from_uri("GET", uri);
	g_object_unref(uri);

	/* yes, the relay token is included twice as different request headers,
	   this is apparently needed to make Google's relay servers work... */
	soup_message_headers_replace(msg->request_headers,
	                             "X-Talk-Google-Relay-Auth",
	                             js->google_relay_token);
	soup_message_headers_replace(msg->request_headers, "X-Google-Relay-Auth",
	                             js->google_relay_token);
	soup_session_queue_message(js->http_conns, msg,
	                           jabber_google_relay_fetch_cb, data);
}
