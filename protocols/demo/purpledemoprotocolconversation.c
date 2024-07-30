/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpledemoprotocolconversation.h"

#include "purpledemoplugin.h"
#include "purpledemoprotocol.h"

typedef struct {
	PurpleConversation *conversation;
	PurpleMessage *message;
} PurpleDemoProtocolIMInfo;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_demo_protocol_im_info_free(PurpleDemoProtocolIMInfo *info) {
	g_clear_object(&info->conversation);
	g_clear_object(&info->message);
	g_free(info);
}

static gint
purple_demo_protocol_contact_sort(gconstpointer a, gconstpointer b,
                                  G_GNUC_UNUSED gpointer data)
{
	return purple_contact_info_compare(PURPLE_CONTACT_INFO((gpointer)a),
	                                   PURPLE_CONTACT_INFO((gpointer)b));
}

static char *
purple_demo_protocol_generate_conversation_id(PurpleAccount *account,
                                              PurpleCreateConversationDetails *details)
{
	GChecksum *checksum = NULL;
	GListModel *participants = NULL;
	GListStore *sorted = NULL;
	char *ret = NULL;
	const char *id = NULL;

	/* Sort the participants. */
	sorted = g_list_store_new(PURPLE_TYPE_CONTACT);
	participants = purple_create_conversation_details_get_participants(details);
	for(guint i = 0; i < g_list_model_get_n_items(participants); i++) {
		PurpleContactInfo *info = NULL;

		info = g_list_model_get_item(participants, i);
		g_list_store_insert_sorted(sorted, info,
		                           purple_demo_protocol_contact_sort,
		                           NULL);
		g_clear_object(&info);
	}

	/* Build a checksum of the account and the sorted participants. */
	checksum = g_checksum_new(G_CHECKSUM_SHA256);

	id = purple_account_get_id(account);
	g_checksum_update(checksum, (guchar *)id, -1);

	for(guint i = 0; i < g_list_model_get_n_items(G_LIST_MODEL(sorted)); i++) {
		PurpleContactInfo *info = NULL;

		info = g_list_model_get_item(G_LIST_MODEL(sorted), i);
		id = purple_contact_info_get_id(info);
		g_checksum_update(checksum, (guchar *)id, -1);
		g_clear_object(&info);
	}

	ret = g_strdup(g_checksum_get_string(checksum));

	g_clear_pointer(&checksum, g_checksum_free);

	return ret;
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
purple_demo_protocol_echo_im_cb(gpointer data) {
	PurpleDemoProtocolIMInfo *info = data;

	purple_conversation_write_message(info->conversation, info->message);

	return G_SOURCE_REMOVE;
}

/******************************************************************************
 * PurpleProtocolConversation Implementation
 *****************************************************************************/
static PurpleCreateConversationDetails *
purple_demo_protocol_get_create_conversation_details(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                                     G_GNUC_UNUSED PurpleAccount *account)
{
	return purple_create_conversation_details_new(9);
}

static void
purple_demo_protocol_create_conversation_async(PurpleProtocolConversation *protocol,
                                               PurpleAccount *account,
                                               PurpleCreateConversationDetails *details,
                                               GCancellable *cancellable,
                                               GAsyncReadyCallback callback,
                                               gpointer data)
{
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *manager = NULL;
	PurpleConversationType type = PURPLE_CONVERSATION_TYPE_UNSET;
	GListModel *participants = NULL;
	GTask *task = NULL;
	char *id = NULL;

	task = g_task_new(protocol, cancellable, callback, data);
	g_task_set_source_tag(task,
	                      purple_demo_protocol_create_conversation_async);

	participants = purple_create_conversation_details_get_participants(details);
	if(g_list_model_get_n_items(participants) == 1) {
		type = PURPLE_CONVERSATION_TYPE_DM;
	} else {
		type = PURPLE_CONVERSATION_TYPE_GROUP_DM;
	}
	id = purple_demo_protocol_generate_conversation_id(account, details);

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"id", id,
		"type", type,
		NULL);
	g_clear_pointer(&id, g_free);

	for(guint i = 0; i < g_list_model_get_n_items(participants); i++) {
		PurpleContactInfo *info = NULL;

		info = g_list_model_get_item(participants, i);
		purple_conversation_add_member(conversation, info, FALSE, NULL);
		g_clear_object(&info);
	}

	manager = purple_conversation_manager_get_default();
	if(!purple_conversation_manager_register(manager, conversation)) {
		g_task_return_new_error(task, PURPLE_DEMO_DOMAIN, 0,
		                        _("This conversation already exists."));
		g_clear_object(&task);

		return;
	}

	g_task_return_pointer(task, conversation, g_object_unref);

	g_clear_object(&task);
}

static PurpleConversation *
purple_demo_protocol_create_conversation_finish(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                                GAsyncResult *result,
                                                GError **error)
{
	GTask *task = G_TASK(result);

	g_return_val_if_fail(g_task_get_source_tag(task) ==
	                     purple_demo_protocol_create_conversation_async,
	                     NULL);

	return g_task_propagate_pointer(task, error);
}

static void
purple_demo_protocol_send_message_async(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                        PurpleConversation *conversation,
                                        PurpleMessage *message,
                                        GCancellable *cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer data)
{
	GTask *task = NULL;

	if(purple_conversation_is_dm(conversation)) {
		PurpleAccount *account = NULL;
		PurpleContact *contact = NULL;
		PurpleContactManager *manager = NULL;

		account = purple_conversation_get_account(conversation);

		manager = purple_contact_manager_get_default();

		/* Check if this dm is with echo. */
		contact = purple_contact_manager_find_with_username(manager, account,
		                                                    "Echo");
		if(purple_conversation_has_member(conversation,
		                                  PURPLE_CONTACT_INFO(contact), NULL))
		{
			PurpleDemoProtocolIMInfo *info = NULL;
			PurpleMessageFlags flags;

			flags = purple_message_get_flags(message);
			flags &= ~PURPLE_MESSAGE_SEND;
			flags |= PURPLE_MESSAGE_RECV;

			info = g_new(PurpleDemoProtocolIMInfo, 1);
			info->conversation = g_object_ref(conversation);
			info->message = g_object_new(
				PURPLE_TYPE_MESSAGE,
				"author", "Echo",
				"flags", flags,
				"contents", purple_message_get_contents(message),
				NULL);

			g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
			                purple_demo_protocol_echo_im_cb, info,
			                (GDestroyNotify)purple_demo_protocol_im_info_free);
		}

		/* Check if this dm is with aegina. */
		contact = purple_contact_manager_find_with_username(manager, account,
		                                                    "Aegina");
		if(purple_conversation_has_member(conversation,
		                                  PURPLE_CONTACT_INFO(contact), NULL))
		{
			PurpleDemoProtocolIMInfo *info = g_new(PurpleDemoProtocolIMInfo, 1);
			const char *author = purple_message_get_author_name(message);
			const char *contents = NULL;

			if(purple_strequal(author, "Hades")) {
				contents = "🫥️";
			} else {
				/* TRANSLATORS: This is a reference to the Cap of Invisibility owned by
				 * various Greek gods, such as Hades, as mentioned. */
				contents = _("Don't tell Hades I have his Cap");
			}

			info->conversation = g_object_ref(conversation);
			info->message = g_object_new(
				PURPLE_TYPE_MESSAGE,
				"author", author,
				"contents", contents,
				"flags", PURPLE_MESSAGE_SEND,
				NULL);

			g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, purple_demo_protocol_echo_im_cb,
			                info, (GDestroyNotify)purple_demo_protocol_im_info_free);
		}
	}

	purple_conversation_write_message(conversation, message);

	task = g_task_new(protocol, cancellable, callback, data);
	g_task_return_boolean(task, TRUE);

	g_clear_object(&task);
}

static gboolean
purple_demo_protocol_send_message_finish(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                         GAsyncResult *result,
                                         GError **error)
{
	g_return_val_if_fail(G_IS_TASK(result), FALSE);

	return g_task_propagate_boolean(G_TASK(result), error);
}

void
purple_demo_protocol_conversation_init(PurpleProtocolConversationInterface *iface) {
	iface->get_create_conversation_details =
		purple_demo_protocol_get_create_conversation_details;
	iface->create_conversation_async =
		purple_demo_protocol_create_conversation_async;
	iface->create_conversation_finish =
		purple_demo_protocol_create_conversation_finish;

	iface->send_message_async = purple_demo_protocol_send_message_async;
	iface->send_message_finish = purple_demo_protocol_send_message_finish;
}
