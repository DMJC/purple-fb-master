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

#include "purpleircv3messagehandlers.h"

#include "purpleircv3connection.h"
#include "purpleircv3constants.h"
#include "purpleircv3core.h"
#include "purpleircv3formatting.h"
#include "purpleircv3source.h"

/******************************************************************************
 * Fallback
 *****************************************************************************/
gboolean
purple_ircv3_message_handler_fallback(PurpleIRCv3Message *message,
                                      G_GNUC_UNUSED GError **error,
                                      gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	char *new_command = NULL;
	const char *command = NULL;

	command = purple_ircv3_message_get_command(message);

	new_command = g_strdup_printf(_("unknown command '%s'"), command);
	purple_ircv3_message_set_command(message, new_command);
	purple_ircv3_connection_add_status_message(connection, message);

	g_clear_pointer(&new_command, g_free);

	return TRUE;
}

/******************************************************************************
 * Status Messages
 *****************************************************************************/
gboolean
purple_ircv3_message_handler_status(PurpleIRCv3Message *message,
                                    G_GNUC_UNUSED GError **error,
                                    gpointer data)
{
	purple_ircv3_connection_add_status_message(data, message);

	return TRUE;
}

gboolean
purple_ircv3_message_handler_status_ignore_param0(PurpleIRCv3Message *message,
                                                  GError **error,
                                                  gpointer data)
{
	GStrv params = NULL;
	GStrv new_params = NULL;
	guint n_params = 0;

	params = purple_ircv3_message_get_params(message);
	if(params != NULL) {
		n_params = g_strv_length(params);
	}

	if(n_params <= 1) {
		g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
		            "expected n_params > 1, got %u", n_params);

		return FALSE;
	}

	/* We need to make a copy because otherwise we'd get a use after free in
	 * set_params.
	 */
	new_params = g_strdupv(params + 1);
	purple_ircv3_message_set_params(message, new_params);
	g_clear_pointer(&new_params, g_strfreev);

	purple_ircv3_connection_add_status_message(data, message);

	return TRUE;
}

/******************************************************************************
 * General Commands
 *****************************************************************************/
gboolean
purple_ircv3_message_handler_ping(PurpleIRCv3Message *message,
                                  G_GNUC_UNUSED GError **error,
                                  gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	GStrv params = NULL;

	params = purple_ircv3_message_get_params(message);

	if(params != NULL && g_strv_length(params) == 1) {
		purple_ircv3_connection_writef(connection, "PONG %s", params[0]);
	} else {
		purple_ircv3_connection_writef(connection, "PONG");
	}

	return TRUE;
}

gboolean
purple_ircv3_message_handler_privmsg(PurpleIRCv3Message *v3_message,
                                     G_GNUC_UNUSED GError **error,
                                     gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleContactManager *contact_manager = NULL;
	PurpleConversation *conversation = NULL;
	PurpleMessage *message = NULL;
	PurpleMessageFlags flags = PURPLE_MESSAGE_RECV;
	GDateTime *dt = NULL;
	GHashTable *tags = NULL;
	GStrv params = NULL;
	gpointer raw_id = NULL;
	gpointer raw_timestamp = NULL;
	char *nick = NULL;
	char *stripped = NULL;
	const char *command = NULL;
	const char *id = NULL;
	const char *source = NULL;
	const char *target = NULL;

	command = purple_ircv3_message_get_command(v3_message);
	params = purple_ircv3_message_get_params(v3_message);
	source = purple_ircv3_message_get_source(v3_message);
	tags = purple_ircv3_message_get_tags(v3_message);

	if(params == NULL) {
		g_warning("privmsg received with no parameters");

		return FALSE;
	}

	if(g_strv_length(params) != 2) {
		char *body = g_strjoinv(" ", params);
		g_warning("unknown privmsg message format: '%s'", body);
		g_free(body);

		return FALSE;
	}

	purple_ircv3_source_parse(source, &nick, NULL, NULL);

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));

	contact_manager = purple_contact_manager_get_default();
	contact = purple_contact_manager_find_with_username(contact_manager,
	                                                    account,
	                                                    nick);
	if(!PURPLE_IS_CONTACT(contact)) {
		contact = purple_contact_new(account, NULL);
		purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact), nick);
		purple_contact_info_set_sid(PURPLE_CONTACT_INFO(contact), source);
		purple_contact_manager_add(contact_manager, contact);
	}
	g_clear_object(&contact);

	target = params[0];
	if(!purple_ircv3_connection_is_channel(connection, target)) {
		target = nick;
	}
	conversation = purple_ircv3_connection_find_or_create_conversation(connection,
	                                                                   target);

	/* Grab the msgid if one was provided. */
	if(g_hash_table_lookup_extended(tags, "msgid", NULL, &raw_id)) {
		if(!purple_strempty(raw_id)) {
			id = raw_id;
		}
	}

	if(purple_strequal(command, "NOTICE")) {
		flags |= PURPLE_MESSAGE_NOTIFY;
	}

	/* Determine the timestamp of the message. */
	if(g_hash_table_lookup_extended(tags, "time", NULL, &raw_timestamp)) {
		const char *timestamp = raw_timestamp;

		if(!purple_strempty(timestamp)) {
			GTimeZone *tz = g_time_zone_new_utc();

			dt = g_date_time_new_from_iso8601(timestamp, tz);

			g_time_zone_unref(tz);
		}
	}

	/* If the server didn't provide a time, use the current local time. */
	if(dt == NULL) {
		dt = g_date_time_new_now_local();
	}

	stripped = purple_ircv3_formatting_strip(params[1]);
	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"author", source,
		"contents", stripped,
		"flags", flags,
		"id", id,
		"timestamp", dt,
		NULL);
	g_free(stripped);

	g_date_time_unref(dt);

	purple_conversation_write_message(conversation, message);

	g_clear_pointer(&nick, g_free);
	g_clear_object(&message);

	return TRUE;
}

gboolean
purple_ircv3_message_handler_topic(PurpleIRCv3Message *message,
                                   GError **error,
                                   gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleConversation *conversation = NULL;
	GStrv params = NULL;
	const char *channel = NULL;
	const char *command = NULL;
	const char *topic = NULL;
	guint n_params = 0;

	command = purple_ircv3_message_get_command(message);
	params = purple_ircv3_message_get_params(message);
	n_params = g_strv_length(params);

	if(purple_strequal(command, PURPLE_IRCV3_MSG_TOPIC)) {
		if(n_params != 2) {
			g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
			            "received TOPIC with %u parameters, expected 2",
			            n_params);

			return FALSE;
		}

		channel = params[0];
		topic = params[1];
	} else if(purple_strequal(command, PURPLE_IRCV3_RPL_NOTOPIC)) {
		if(n_params != 3) {
			g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
			            "received RPL_NOTOPIC with %u parameters, expected 3",
			            n_params);

			return FALSE;
		}

		channel = params[1];
		topic = "";
	} else if(purple_strequal(command, PURPLE_IRCV3_RPL_TOPIC)) {
		if(n_params != 3) {
			g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
			            "received RPL_TOPIC with %u parameters, expected 3",
			            n_params);

			return FALSE;
		}

		channel = params[1];
		topic = params[2];
	} else {
		g_set_error(error, PURPLE_IRCV3_DOMAIN, 0, "unexpected command %s",
		            command);

		return FALSE;
	}

	conversation = purple_ircv3_connection_find_or_create_conversation(connection,
	                                                                   channel);
	if(!PURPLE_IS_CONVERSATION(conversation)) {
		g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
		            "failed to find or create channel '%s'", channel);

		return FALSE;
	}

	purple_conversation_set_topic(conversation, topic);

	return TRUE;
}
