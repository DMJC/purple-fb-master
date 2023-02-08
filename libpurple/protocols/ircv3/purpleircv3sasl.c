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

#include <gsasl.h>

#include "purpleircv3sasl.h"

#include "purpleircv3capabilities.h"
#include "purpleircv3connection.h"
#include "purpleircv3core.h"

#define PURPLE_IRCV3_SASL_DATA_KEY ("sasl-data")

typedef struct {
	PurpleConnection *connection;

	Gsasl *ctx;
	Gsasl_session *session;

	char *current_mechanism;
	GString *mechanisms;

	GString *server_in_buffer;
} PurpleIRCv3SASLData;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static const char *
purple_ircv3_sasl_get_username(PurpleConnection *connection) {
	PurpleAccount *account = NULL;
	const char *username = NULL;

	account = purple_connection_get_account(connection);

	username = purple_account_get_string(account, "sasl-login-name", "");
	if(username != NULL && username[0] != '\0') {
		return username;
	}

	return purple_connection_get_display_name(connection);
}

/******************************************************************************
 * SASL Callbacks
 *****************************************************************************/
static int
purple_ircv3_sasl_callback(G_GNUC_UNUSED Gsasl *ctx, Gsasl_session *session,
                           Gsasl_property property)
{
	PurpleIRCv3SASLData *data = NULL;
	int res = GSASL_NO_CALLBACK;

	data = gsasl_session_hook_get(session);

	switch(property) {
		case GSASL_AUTHID:
			gsasl_property_set(session, GSASL_AUTHID,
			                   purple_ircv3_sasl_get_username(data->connection));
			res = GSASL_OK;
			break;
		case GSASL_AUTHZID:
			/* AUTHZID is only used implemented for the PLAIN mechanism. If we
			 * return a value for SCRAM, it needs to match AUTHID, which we
			 * always set which would be redundant.
			 *
			 * So instead, we just check if the mechanism is PLAIN and if so
			 * set it to empty string because it's the user logging in on their
			 * own behalf and IRCv3 doesn't really let an admin login as a
			 * normal user.
			 *
			 * See https://www.gnu.org/software/gsasl/manual/gsasl.html#PLAIN
			 * for further explanation.
			 */
			if(purple_strequal(data->current_mechanism, "PLAIN")) {
				gsasl_property_set(session, GSASL_AUTHZID, "");
				res = GSASL_OK;
			}
			break;
		case GSASL_PASSWORD:
			gsasl_property_set(session, GSASL_PASSWORD,
			                   purple_connection_get_password(data->connection));
			res = GSASL_OK;
			break;
		case GSASL_SCRAM_SALTED_PASSWORD:
			/* Ignored, we let gsasl figure it out from the password above. */
			break;
		case GSASL_CB_TLS_UNIQUE:
		case GSASL_CB_TLS_EXPORTER:
			/* TODO: implement these in the near future when we're implementing
			 * EXTERNAL support for PIDGIN-17741.
			 */
			break;
		default:
			g_warning("Unknown property %d", property);
			break;
	}

	return res;
}

/******************************************************************************
 * SASL Helpers
 *****************************************************************************/
static void
purple_ircv3_sasl_connection_error(PurpleConnection *connection, int res,
                                   int err, const char *msg)
{
	GError *error = NULL;

	error = g_error_new(PURPLE_CONNECTION_ERROR, err, "%s: %s", msg,
	                    gsasl_strerror(res));

	purple_connection_take_error(connection, error);
}

static void
purple_ircv3_sasl_data_free(PurpleIRCv3SASLData *data) {
	g_clear_object(&data->connection);
	g_clear_pointer(&data->session, gsasl_finish);
	g_clear_pointer(&data->ctx, gsasl_done);
	g_clear_pointer(&data->current_mechanism, g_free);

	g_string_free(data->mechanisms, TRUE);
	data->mechanisms = NULL;

	g_string_free(data->server_in_buffer, TRUE);

	g_free(data);
}

static void
purple_ircv3_sasl_data_add(PurpleConnection *connection, Gsasl *ctx,
                           const char *mechanisms)
{
	PurpleIRCv3SASLData *data = NULL;
	GStrv parts = NULL;

	data = g_new0(PurpleIRCv3SASLData, 1);
	g_object_set_data_full(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY,
	                       data, (GDestroyNotify)purple_ircv3_sasl_data_free);

	/* We don't reference this because the life cycle of this data is tied
	 * directly to the connection and adding a reference to the connection
	 * would keep both alive forever.
	 */
	data->connection = connection;
	data->ctx = ctx;
	gsasl_callback_set(data->ctx, purple_ircv3_sasl_callback);

	/* We truncate the server_in_buffer when we need to so that we can minimize
	 * allocations and simplify the logic involved with it.
	 */
	data->server_in_buffer = g_string_new("");

	/* Create a GString for the mechanisms with a leading and trailing ` `.
	 * This is so we can easily remove attempted mechanism by removing
	 * ` <attempted_mechanism> ` from the string which will make sure we're
	 * always removing the proper mechanism. This is necessary because some
	 * mechanisms have the same prefix, and others have the same suffix, which
	 * could lead to incorrect removals.
	 *
	 * For example, if the list contains `EAP-AES128-PLUS EAP-AES128` and we
	 * try `EAP-AES128` first, that means we would remove `EAP-AES128` from the
	 * list which would first find `EAP-AES128-PLUS` and replace it with an
	 * empty string, leaving the list as `-PLUS EAP-AES128` which is obviously
	 * wrong. Instead the additional spaces mean our replace can be
	 * ` EAP-AES128 `, which will get the proper value and update the
	 * list to just contain ` EAP-AES128-PLUS `.
	 *
	 * For a list of mechanisms see
	 * https://www.iana.org/assignments/sasl-mechanisms/sasl-mechanisms.xhtml
	 *
	 */
	data->mechanisms = g_string_new("");

	parts = g_strsplit(mechanisms, ",", -1);
	for(int i = 0; parts[i] != NULL; i++) {
		g_string_append_printf(data->mechanisms, " %s ", parts[i]);
	}
	g_strfreev(parts);
}

static void
purple_ircv3_sasl_attempt_mechanism(PurpleIRCv3Connection *connection,
                                    PurpleIRCv3SASLData *data,
                                    const char *next_mechanism)
{
	int res = GSASL_OK;

	g_free(data->current_mechanism);
	data->current_mechanism = g_strdup(next_mechanism);

	res = gsasl_client_start(data->ctx, next_mechanism, &data->session);
	if(res != GSASL_OK) {
		purple_ircv3_sasl_connection_error(PURPLE_CONNECTION(connection),
		                                   res,
		                                   PURPLE_CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE,
		                                   _("Failed to setup SASL client"));
		return;
	}
	gsasl_session_hook_set(data->session, data);

	purple_ircv3_connection_writef(connection, "AUTHENTICATE %s",
	                               next_mechanism);
}

static void
purple_ircv3_sasl_attempt(PurpleIRCv3Connection *connection) {
	PurpleIRCv3SASLData *data = NULL;
	PurpleAccount *account = NULL;
	const char *next_mechanism = NULL;
	gboolean allow_plain = TRUE;
	gboolean good_mechanism = FALSE;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);

	/* If this is not our first attempt, remove the previous mechanism from the
	 * list of mechanisms to try.
	 */
	if(data->current_mechanism != NULL) {
		char *to_remove = g_strdup_printf(" %s ", data->current_mechanism);

		g_message("SASL '%s' mechanism failed", data->current_mechanism);

		g_string_replace(data->mechanisms, to_remove, "", 0);
		g_free(to_remove);
	}

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	if(!purple_account_get_bool(account, "use-tls", TRUE)) {
		if(!purple_account_get_bool(account, "plain-sasl-in-clear", FALSE)) {
			allow_plain = FALSE;
		}
	}

	while(!good_mechanism) {
		good_mechanism = TRUE;

		next_mechanism = gsasl_client_suggest_mechanism(data->ctx,
		                                                data->mechanisms->str);

		if(next_mechanism == NULL) {
			GError *error = g_error_new(PURPLE_CONNECTION_ERROR,
			                            PURPLE_CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE,
			                            _("No valid SASL mechanisms found"));

			purple_connection_take_error(PURPLE_CONNECTION(connection), error);

			return;
		}

		if(purple_strequal(next_mechanism, "PLAIN") && !allow_plain) {
			g_message("skipping SASL 'PLAIN' as it's not allowed without tls");

			good_mechanism = FALSE;

			g_string_replace(data->mechanisms, " PLAIN ", "", 0);
		}
	}

	g_message("trying SASL '%s' mechanism", next_mechanism);

	purple_ircv3_sasl_attempt_mechanism(connection, data, next_mechanism);
}

static void
purple_ircv3_sasl_start(PurpleIRCv3Capabilities *caps) {
	PurpleIRCv3Connection *connection = NULL;
	PurpleAccount *account = NULL;
	PurpleConnection *purple_connection = NULL;
	Gsasl *ctx = NULL;
	const char *mechanisms = NULL;
	gint res;

	connection = purple_ircv3_capabilities_get_connection(caps);
	purple_connection = PURPLE_CONNECTION(connection);
	account = purple_connection_get_account(purple_connection);

	res = gsasl_init(&ctx);
	if(res != GSASL_OK) {
		purple_ircv3_sasl_connection_error(purple_connection, res,
		                                   PURPLE_CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE,
		                                   _("Failed to initialize SASL"));

		return;
	}

	/* At this point we are ready to start our sasl negotiation, so add a wait
	 * counter to the capabilities and start the negotiations!
	 */
	purple_ircv3_capabilities_add_wait(caps);

	mechanisms = purple_account_get_string(account, "sasl-mechanisms", "");
	if(purple_strempty(mechanisms)) {
		/* If the user didn't specify any mechanisms, grab the mechanisms that
		 * the server advertised.
		 */
		mechanisms = purple_ircv3_capabilities_lookup(caps, "sasl", NULL);
	}

	/* Create our SASLData object, add it to the connection. */
	purple_ircv3_sasl_data_add(purple_connection, ctx, mechanisms);

	/* Make it go! */
	purple_ircv3_sasl_attempt(connection);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_ircv3_sasl_ack_cb(PurpleIRCv3Capabilities *caps,
                         G_GNUC_UNUSED const char *capability,
                         G_GNUC_UNUSED gpointer data)
{
	purple_ircv3_sasl_start(caps);
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_ircv3_sasl_request(PurpleIRCv3Capabilities *capabilities) {
	purple_ircv3_capabilities_request(capabilities, "sasl");

	g_signal_connect(capabilities, "ack::sasl",
	                 G_CALLBACK(purple_ircv3_sasl_ack_cb), NULL);
}

gboolean
purple_ircv3_sasl_logged_in(G_GNUC_UNUSED GHashTable *tags,
                            G_GNUC_UNUSED const char *source,
                            G_GNUC_UNUSED const char *command,
                            G_GNUC_UNUSED guint n_params,
                            G_GNUC_UNUSED GStrv params,
                            G_GNUC_UNUSED GError **error,
                            gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "RPL_LOGGEDIN received with no SASL data "
		                    "present");

		return FALSE;
	}

	/* At this point, we have the users authenticated username, we _may_ want
	 * to update the account's ID to this, but we'll need more testing to
	 * verify that.
	 * -- GK 2023-01-12
	 */

	return TRUE;
}

gboolean
purple_ircv3_sasl_logged_out(G_GNUC_UNUSED GHashTable *tags,
                             G_GNUC_UNUSED const char *source,
                             G_GNUC_UNUSED const char *command,
                             G_GNUC_UNUSED guint n_params,
                             G_GNUC_UNUSED GStrv params,
                             G_GNUC_UNUSED GError **error,
                             gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "RPL_LOGGEDOUT received with no SASL data "
		                    "present");

		return FALSE;
	}

	/* Not sure how to trigger this or what we should do in this case to be
	 * honest, so just note it for now.
	 * -- GK 2023-01-12
	 */
	g_warning("Server sent SASL logged out");

	return TRUE;
}

gboolean
purple_ircv3_sasl_nick_locked(G_GNUC_UNUSED GHashTable *tags,
                              G_GNUC_UNUSED const char *source,
                              G_GNUC_UNUSED const char *command,
                              G_GNUC_UNUSED guint n_params,
                              GStrv params,
                              GError **error,
                              gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;
	char *message = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "ERR_NICKLOCKED received with no SASL data "
		                    "present");

		return FALSE;
	}

	message = g_strjoinv(" ", params);

	g_set_error(error, PURPLE_CONNECTION_ERROR,
	            PURPLE_CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE,
	            _("Nick name is locked: %s"), message);

	g_free(message);

	return FALSE;
}


gboolean
purple_ircv3_sasl_success(G_GNUC_UNUSED GHashTable *tags,
                          G_GNUC_UNUSED const char *source,
                          G_GNUC_UNUSED const char *command,
                          G_GNUC_UNUSED guint n_params,
                          G_GNUC_UNUSED GStrv params,
                          GError **error,
                          gpointer user_data)
{
	PurpleIRCv3Capabilities *capabilities = NULL;
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	capabilities = purple_ircv3_connection_get_capabilities(connection);

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "RPL_SASLSUCCESS received with no SASL data "
		                    "present");

		return FALSE;
	}

	/* This needs to be after we've checked the SASL data otherwise we might
	 * end up removing a wait that we don't own.
	 */
	purple_ircv3_capabilities_remove_wait(capabilities);

	g_message("successfully authenticated with SASL '%s' mechanism.",
	          data->current_mechanism);

	return TRUE;
}

gboolean
purple_ircv3_sasl_failed(G_GNUC_UNUSED GHashTable *tags,
                         G_GNUC_UNUSED const char *source,
                         G_GNUC_UNUSED const char *command,
                         G_GNUC_UNUSED guint n_params,
                         G_GNUC_UNUSED GStrv params,
                         G_GNUC_UNUSED GError **error,
                         gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "ERR_SASLFAIL received with no SASL data present");

		return FALSE;
	}

	purple_ircv3_sasl_attempt(connection);

	return TRUE;
}

gboolean
purple_ircv3_sasl_message_too_long(G_GNUC_UNUSED GHashTable *tags,
                                   G_GNUC_UNUSED const char *source,
                                   G_GNUC_UNUSED const char *command,
                                   G_GNUC_UNUSED guint n_params,
                                   G_GNUC_UNUSED GStrv params,
                                   G_GNUC_UNUSED GError **error,
                                   gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "ERR_SASLTOOLONG received with no SASL data "
		                    "present");

		return FALSE;
	}

	return TRUE;
}

gboolean
purple_ircv3_sasl_aborted(G_GNUC_UNUSED GHashTable *tags,
                          G_GNUC_UNUSED const char *source,
                          G_GNUC_UNUSED const char *command,
                          G_GNUC_UNUSED guint n_params,
                          G_GNUC_UNUSED GStrv params,
                          G_GNUC_UNUSED GError **error,
                          gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "ERR_SASLABORTED received with no SASL data "
		                    "present");

		return FALSE;
	}

	/* This is supposed to get sent, when the client sends `AUTHENTICATE *`,
	 * but we don't do this, so I guess we'll note it for now...?
	 * --GK 2023-01-12
	 */
	g_warning("The server claims we aborted SASL authentication.");

	return TRUE;
}

gboolean
purple_ircv3_sasl_already_authed(G_GNUC_UNUSED GHashTable *tags,
                                 G_GNUC_UNUSED const char *source,
                                 G_GNUC_UNUSED const char *command,
                                 G_GNUC_UNUSED guint n_params,
                                 G_GNUC_UNUSED GStrv params,
                                 G_GNUC_UNUSED GError **error,
                                 gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "ERR_SASLALREADY received with no SASL data "
		                    "present");

		return FALSE;
	}

	/* Similar to aborted above, we don't allow this, so just note that it
	 * happened.
	 * -- GK 2023-01-12
	 */
	g_warning("Server claims we tried to SASL authenticate again.");

	return TRUE;
}

gboolean
purple_ircv3_sasl_mechanisms(G_GNUC_UNUSED GHashTable *tags,
                             G_GNUC_UNUSED const char *source,
                             G_GNUC_UNUSED const char *command,
                             guint n_params,
                             GStrv params,
                             G_GNUC_UNUSED GError **error,
                             gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;

	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "RPL_SASLMECHS received with no SASL data "
		                    "present");

		return FALSE;
	}

	/* We need to find a server that sends this message. The specification says
	 * it _may_ be sent when the client sends AUTHENTICATE with an unknown
	 * mechanism, but ergo doesn't.
	 *
	 * We can't just blindly accept the new list either, as depending on how
	 * the server implements it, we'll need to remove mechanisms we've already
	 * tried in the event the server just dumps the entire list. As we're not
	 * currently tracking which mechanisms we've tried, this will have to be
	 * addressed as well.
	 */
	if(n_params > 0) {
		char *message = g_strjoinv(" ", params);

		g_message("Server sent the following SASL mechanisms: %s", message);

		g_free(message);
	} else {
		g_message("Server sent an empty list of SASL mechanisms");
	}

	return TRUE;
}

gboolean
purple_ircv3_sasl_authenticate(G_GNUC_UNUSED GHashTable *tags,
                               G_GNUC_UNUSED const char *source,
                               G_GNUC_UNUSED const char *command,
                               guint n_params,
                               GStrv params,
                               GError **error,
                               gpointer user_data)
{
	PurpleIRCv3Connection *connection = user_data;
	PurpleIRCv3SASLData *data = NULL;
	char *payload = NULL;
	gboolean done = FALSE;

	if(n_params != 1) {
		g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
		            "ignoring AUTHENTICATE with %d parameters", n_params);

		return FALSE;
	}

	payload = params[0];
	data = g_object_get_data(G_OBJECT(connection), PURPLE_IRCV3_SASL_DATA_KEY);
	if(data == NULL) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "AUTHENTICATE received with no SASL data present");

		return FALSE;
	}

	/* If the server sent us a payload, combine the chunks. */
	if(payload[0] != '+') {
		g_string_append(data->server_in_buffer, payload);

		if(strlen(payload) < 400) {
			done = TRUE;
		}
	} else {
		/* The server sent a + which is an empty message or the final message
		 * ended on a 400 byte barrier. */
		done = TRUE;
	}

	if(done) {
		char *server_in = NULL;
		char *client_out = NULL;
		gsize server_in_length = 0;
		size_t client_out_length = 0;
		int res = 0;

		/* If we have a buffer, base64 decode it, and then truncate it. */
		if(data->server_in_buffer->len > 0) {
			server_in = (char *)g_base64_decode(data->server_in_buffer->str,
			                                    &server_in_length);
			g_string_truncate(data->server_in_buffer, 0);
		}

		/* Try to move to the next step of the sasl client. */
		res = gsasl_step(data->session, server_in, server_in_length,
		                 &client_out, &client_out_length);

		/* We should be done with server_in, so free it.*/
		g_clear_pointer(&server_in, g_free);

		/* If we didn't get ok or needs more, it's an error.
		 *
		 * We allow needs more as SCRAM will is client first and returns
		 * GSASL_NEEDS_MORE on the first step. We could tie this now a bit
		 * more, but this seems to be fine for now. -- GK 2023-01-28
		 */
		if(res != GSASL_OK && res != GSASL_NEEDS_MORE) {
			g_set_error(error, PURPLE_CONNECTION_ERROR,
			            PURPLE_CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE,
			            _("SASL authentication failed: %s"),
			            gsasl_strerror(res));

			return FALSE;
		}

		/* If we got an output for the client, write it out. */
		if(client_out_length > 0) {
			char *encoded = g_base64_encode((guchar *)client_out,
			                                client_out_length);

			purple_ircv3_connection_writef(connection, "AUTHENTICATE %s",
			                               encoded);

			g_free(encoded);
		} else {
			purple_ircv3_connection_writef(connection, "AUTHENTICATE +");
		}
	}

	return TRUE;
}
