/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpleircv3messagehandlers.h"

#include "purpleircv3connection.h"
#include "purpleircv3core.h"

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_ircv3_add_contact_to_conversation(PurpleContact *contact,
                                         PurpleConversation *conversation)
{
	PurpleConversationMember *member = NULL;

	member = purple_conversation_find_member(conversation,
	                                         PURPLE_CONTACT_INFO(contact));

	if(!PURPLE_IS_CONVERSATION_MEMBER(member)) {
		purple_conversation_add_member(conversation,
		                               PURPLE_CONTACT_INFO(contact),
		                               TRUE, NULL);
	}
}

/******************************************************************************
 * General Commands
 *****************************************************************************/
gboolean
purple_ircv3_message_handler_join(G_GNUC_UNUSED IbisClient *client,
                                  G_GNUC_UNUSED const char *command,
                                  IbisMessage *message,
                                  gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation = NULL;
	GStrv params = NULL;
	guint n_params = 0;
	const char *conversation_name = NULL;

	contact = purple_ircv3_connection_find_or_create_contact(connection,
	                                                         message);

	params = ibis_message_get_params(message);
	n_params = g_strv_length(params);

	/* A normal join command has the channel as the only parameter. */
	if(n_params == 1) {
		conversation_name = params[0];
	} else {
		/* TODO: write this to join to the status window saying we didn't know
		 * how to parse it.
		 */

		return TRUE;
	}

	conversation = purple_ircv3_connection_find_or_create_conversation(connection,
	                                                                   conversation_name);
	purple_ircv3_add_contact_to_conversation(contact, conversation);

	return TRUE;
}

gboolean
purple_ircv3_message_handler_part(G_GNUC_UNUSED IbisClient *client,
                                  G_GNUC_UNUSED const char *command,
                                  IbisMessage *message,
                                  gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *manager = NULL;
	GStrv params = NULL;
	guint n_params = 0;
	char *reason = NULL;
	const char *conversation_name = NULL;

	params = ibis_message_get_params(message);
	if(g_strv_length(params) == 0) {
		/* TODO: mention unparsable message in the status window. */
		return TRUE;
	}

	/* TODO: The spec says servers _SHOULD NOT_ send a comma separated list of
	 * channels, but we should support that at some point just in case.
	 */
	conversation_name = params[0];

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	manager = purple_conversation_manager_get_default();
	conversation = purple_conversation_manager_find_with_id(manager, account,
	                                                        conversation_name);

	if(!PURPLE_IS_CONVERSATION(conversation)) {
		/* TODO: write status message unknown channel. */

		return TRUE;
	}

	/* We do want to find or create the contact, even on a part, because we
	 * could have connected to a BNC and we weren't told about the contact yet.
	 */
	contact = purple_ircv3_connection_find_or_create_contact(connection,
	                                                         message);

	/* If a part message was given, join the remaining parameters with a space.
	 */
	if(n_params > 1) {
		reason = g_strjoinv(" ", params + 1);
	}

	purple_conversation_remove_member(conversation,
	                                  PURPLE_CONTACT_INFO(contact), TRUE,
	                                  reason);

	g_clear_pointer(&reason, g_free);

	return TRUE;
}

gboolean
purple_ircv3_message_handler_privmsg(G_GNUC_UNUSED IbisClient *client,
                                     const char *command,
                                     IbisMessage *ibis_message, gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation = NULL;
	PurpleMessage *message = NULL;
	GDateTime *dt = NULL;
	IbisTags *tags = NULL;
	GStrv params = NULL;
	char *stripped = NULL;
	const char *target = NULL;

	params = ibis_message_get_params(ibis_message);
	tags = ibis_message_get_tags(ibis_message);

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

	/* Find or create the contact. */
	contact = purple_ircv3_connection_find_or_create_contact(connection,
	                                                         ibis_message);

	/* Find or create the conversation. */
	target = params[0];
	if(!purple_ircv3_connection_is_channel(connection, target)) {
		target = purple_contact_info_get_id(PURPLE_CONTACT_INFO(contact));
	}

	conversation = purple_ircv3_connection_find_or_create_conversation(connection,
	                                                                   target);

	purple_ircv3_add_contact_to_conversation(contact, conversation);

	stripped = ibis_formatting_strip(params[1]);
	message = purple_message_new(PURPLE_CONTACT_INFO(contact), stripped);
	g_clear_pointer(&stripped, g_free);

	if(purple_strequal(command, IBIS_MSG_NOTICE)) {
		purple_message_set_notice(message, TRUE);
	}

	if(IBIS_IS_TAGS(tags)) {
		const char *raw_tag = NULL;

		/* Grab the msgid if one was provided. */
		raw_tag = ibis_tags_lookup(tags, "msgid");
		if(!purple_strempty(raw_tag)) {
			purple_message_set_id(message, raw_tag);
		}

		/* Determine the timestamp of the message. */
		raw_tag = ibis_tags_lookup(tags, "time");
		if(!purple_strempty(raw_tag)) {
			GTimeZone *tz = g_time_zone_new_utc();

			dt = g_date_time_new_from_iso8601(raw_tag, tz);
			g_time_zone_unref(tz);

			purple_message_set_timestamp(message, dt);
			g_date_time_unref(dt);
		}
	}

	/* If the server didn't provide a time, use the current local time. */
	if(dt == NULL) {
		purple_message_set_timestamp_now(message);
	}

	/* Check if this is a CTCP message. */
	if(ibis_message_get_ctcp(ibis_message)) {
		/* TODO: later... */
	}

	purple_conversation_write_message(conversation, message);

	g_clear_object(&message);

	return TRUE;
}

gboolean
purple_ircv3_message_handler_topic(G_GNUC_UNUSED IbisClient *client,
                                   const char *command, IbisMessage *message,
                                   gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleConversation *conversation = NULL;
	GStrv params = NULL;
	const char *channel = NULL;
	const char *topic = NULL;
	guint n_params = 0;

	params = ibis_message_get_params(message);
	n_params = g_strv_length(params);

	if(purple_strequal(command, IBIS_MSG_TOPIC)) {
		if(n_params != 2) {
			g_message("received TOPIC with %u parameters, expected 2",
			          n_params);

			return FALSE;
		}

		channel = params[0];
		topic = params[1];
	} else if(purple_strequal(command, IBIS_RPL_NOTOPIC)) {
		if(n_params != 3) {
			g_message("received RPL_NOTOPIC with %u parameters, expected 3",
			          n_params);

			return FALSE;
		}

		channel = params[1];
		topic = "";
	} else if(purple_strequal(command, IBIS_RPL_TOPIC)) {
		if(n_params != 3) {
			g_message("received RPL_TOPIC with %u parameters, expected 3",
			          n_params);

			return FALSE;
		}

		channel = params[1];
		topic = params[2];
	} else {
		g_message("unexpected command %s", command);

		return FALSE;
	}

	conversation = purple_ircv3_connection_find_or_create_conversation(connection,
	                                                                   channel);
	if(!PURPLE_IS_CONVERSATION(conversation)) {
		g_message("failed to find or create channel '%s'", channel);

		return FALSE;
	}

	purple_conversation_set_topic(conversation, topic);

	return TRUE;
}
