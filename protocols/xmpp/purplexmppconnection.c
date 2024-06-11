/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include <xeme.h>

#include "purplexmppconnection.h"

#include "purplexmppconstants.h"
#include "purplexmppcore.h"

struct _PurpleXmppConnection {
	PurpleConnection parent;

	XemeConnection *xeme_connection;
	XemeOutputStream *output;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(PurpleXmppConnection, purple_xmpp_connection,
                               PURPLE_TYPE_CONNECTION, G_TYPE_FLAG_FINAL, {})

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_xmpp_connection_connect_cb(GObject *source, GAsyncResult *result,
                                  gpointer data)
{
	PurpleXmppConnection *connection = data;
	XemeOutputStream *output = NULL;
	GError *error = NULL;

	output = xeme_connection_connect_finish(XEME_CONNECTION(source), result,
	                                        &error);
	if(error != NULL) {
		purple_connection_take_error(PURPLE_CONNECTION(connection), error);

		return;
	}

	connection->output = output;

	purple_connection_set_state(PURPLE_CONNECTION(connection),
	                            PURPLE_CONNECTION_STATE_CONNECTED);
}

/******************************************************************************
 * PurpleConnection Implementation
 *****************************************************************************/
static gboolean
purple_xmpp_connection_connect(PurpleConnection *purple_connection,
                               GError **error)
{
	PurpleXmppConnection *connection = NULL;
	PurpleAccount *account = NULL;
	XemeInputStream *input = NULL;
	XemeTlsMode xeme_tls_mode = XEME_TLS_MODE_DIRECT_TLS;
	GCancellable *cancellable = NULL;
	GError *local_error = NULL;
	GProxyResolver *resolver = NULL;
	const char *resource = NULL;
	const char *server = NULL;
	const char *tls_mode = NULL;
	const char *username = NULL;
	guint16 port = PURPLE_XMPP_DEFAULT_PORT_TLS;

	g_return_val_if_fail(PURPLE_XMPP_IS_CONNECTION(purple_connection), FALSE);

	purple_connection_set_state(purple_connection,
	                            PURPLE_CONNECTION_STATE_CONNECTING);

	connection = PURPLE_XMPP_CONNECTION(purple_connection);
	cancellable = purple_connection_get_cancellable(purple_connection);
	account = purple_connection_get_account(purple_connection);

	/* Figure out what proxy resolver to use. */
	resolver = purple_proxy_get_proxy_resolver(account, &local_error);
	if(!G_IS_PROXY_RESOLVER(resolver) || local_error != NULL) {
		if(local_error == NULL) {
			local_error = g_error_new(PURPLE_XMPP_DOMAIN, 0, "unknown error");
		}

		g_propagate_error(error, local_error);

		g_clear_object(&resolver);

		return FALSE;
	}

	/* Grab some more information that we need. */
	username = purple_contact_info_get_username(PURPLE_CONTACT_INFO(account));
	connection->xeme_connection = xeme_connection_new(username);

	/* If we have a resource, tell the xeme connection about it. */
	resource = purple_account_get_string(account, PURPLE_XMPP_OPTION_RESOURCE,
	                                     NULL);
	if(!purple_strempty(resource)) {
		xeme_connection_set_resource(connection->xeme_connection, resource);
	}

	/* If we have a server, tell the xeme connection about it. */
	server = purple_account_get_string(account, PURPLE_XMPP_OPTION_SERVER,
	                                   NULL);
	if(!purple_strempty(server)) {
		xeme_connection_set_server(connection->xeme_connection, server);
	}

	/* Set whatever port we have on our xeme connection. */
	port = purple_account_get_int(account, PURPLE_XMPP_OPTION_PORT,
	                              PURPLE_XMPP_DEFAULT_PORT_TLS);
	xeme_connection_set_port(connection->xeme_connection, port);

	/* Figure out what if any TLS we're doing. This is initialized to direct
	 * tls above, so if we don't know which to use, that's what we'll end up
	 * using. Doing so should avoid possible accidental leaks.
	 */
	tls_mode = purple_account_get_string(account, PURPLE_XMPP_OPTION_TLS_MODE,
	                                     PURPLE_XMPP_TLS_DIRECT);
	if(purple_strequal(tls_mode, PURPLE_XMPP_TLS_DIRECT)) {
		xeme_tls_mode = XEME_TLS_MODE_DIRECT_TLS;
	} else if(purple_strequal(tls_mode, PURPLE_XMPP_TLS_STARTTLS)) {
		xeme_tls_mode = XEME_TLS_MODE_START_TLS;
	} else if(purple_strequal(tls_mode, PURPLE_XMPP_TLS_NONE)) {
		xeme_tls_mode = XEME_TLS_MODE_NONE;
	}

	xeme_connection_set_tls_mode(connection->xeme_connection, xeme_tls_mode);

	input = xeme_input_stream_new();

	/* Finally start connecting!! */
	xeme_connection_connect_async(connection->xeme_connection, input, resolver,
	                              cancellable, purple_xmpp_connection_connect_cb,
	                              connection);

	g_clear_object(&input);

	return TRUE;
}

static gboolean
purple_xmpp_connection_disconnect(PurpleConnection *purple_connection,
                                  GError **error)
{
	PurpleXmppConnection *connection = NULL;

	g_return_val_if_fail(PURPLE_XMPP_IS_CONNECTION(purple_connection), FALSE);

	connection = PURPLE_XMPP_CONNECTION(purple_connection);

	return xeme_connection_close(connection->xeme_connection, error);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_xmpp_connection_dispose(GObject *obj) {
	PurpleXmppConnection *connection = PURPLE_XMPP_CONNECTION(obj);

	g_clear_object(&connection->xeme_connection);
	g_clear_object(&connection->output);

	G_OBJECT_CLASS(purple_xmpp_connection_parent_class)->dispose(obj);
}

static void
purple_xmpp_connection_init(G_GNUC_UNUSED PurpleXmppConnection *connection) {
}

static void
purple_xmpp_connection_class_init(PurpleXmppConnectionClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	PurpleConnectionClass *connection_class = PURPLE_CONNECTION_CLASS(klass);

	obj_class->dispose = purple_xmpp_connection_dispose;

	connection_class->connect = purple_xmpp_connection_connect;
	connection_class->disconnect = purple_xmpp_connection_disconnect;
}

static void
purple_xmpp_connection_class_finalize(G_GNUC_UNUSED PurpleXmppConnectionClass *klass) {
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_xmpp_connection_register(GPluginNativePlugin *plugin) {
	purple_xmpp_connection_register_type(G_TYPE_MODULE(plugin));
}
