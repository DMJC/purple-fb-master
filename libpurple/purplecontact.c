/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include "purplecontact.h"

#include "purpleconversationmanager.h"
#include "purpleprotocolconversation.h"

struct _PurpleContact {
	PurpleContactInfo parent;

	PurpleAccount *account;
};

enum {
	PROP_0,
	PROP_ACCOUNT,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_FINAL_TYPE(PurpleContact, purple_contact, PURPLE_TYPE_CONTACT_INFO)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_contact_set_account(PurpleContact *contact, PurpleAccount *account) {
	g_return_if_fail(PURPLE_IS_CONTACT(contact));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_set_object(&contact->account, account)) {
		g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_ACCOUNT]);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_contact_create_dm_cb(GObject *obj, GAsyncResult *result, gpointer data)
{
	PurpleProtocolConversation *protocol = PURPLE_PROTOCOL_CONVERSATION(obj);
	PurpleConversation *conversation = NULL;
	GError *error = NULL;
	GTask *task = data;

	/* task and result share a cancellable, so we just need to clear task to
	 * make sure it's callback gets called.
	 */
	if(g_task_return_error_if_cancelled(G_TASK(task))) {
		g_clear_object(&task);

		return;
	}

	conversation = purple_protocol_conversation_create_conversation_finish(protocol,
	                                                                       result,
	                                                                       &error);

	if(error != NULL) {
		g_task_return_error(task, error);
	} else {
		g_task_return_pointer(task, conversation, g_object_unref);
	}

	g_clear_object(&task);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_contact_get_property(GObject *obj, guint param_id, GValue *value,
                            GParamSpec *pspec)
{
	PurpleContact *contact = PURPLE_CONTACT(obj);

	switch(param_id) {
		case PROP_ACCOUNT:
			g_value_set_object(value, purple_contact_get_account(contact));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_contact_set_property(GObject *obj, guint param_id, const GValue *value,
                            GParamSpec *pspec)
{
	PurpleContact *contact = PURPLE_CONTACT(obj);

	switch(param_id) {
		case PROP_ACCOUNT:
			purple_contact_set_account(contact, g_value_get_object(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_contact_dispose(GObject *obj) {
	PurpleContact *contact = PURPLE_CONTACT(obj);

	g_clear_object(&contact->account);

	G_OBJECT_CLASS(purple_contact_parent_class)->dispose(obj);
}

static void
purple_contact_init(G_GNUC_UNUSED PurpleContact *contact) {
}

static void
purple_contact_class_init(PurpleContactClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->dispose = purple_contact_dispose;
	obj_class->get_property = purple_contact_get_property;
	obj_class->set_property = purple_contact_set_property;

	/**
	 * PurpleContact:account:
	 *
	 * The account that this contact belongs to.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", "account",
		"The account this contact belongs to",
		PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleContact *
purple_contact_new(PurpleAccount *account, const gchar *id) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return g_object_new(
		PURPLE_TYPE_CONTACT,
		"account", account,
		"id", id,
		NULL);
}

PurpleAccount *
purple_contact_get_account(PurpleContact *contact) {
	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	return contact->account;
}

PurpleConversation *
purple_contact_find_dm(PurpleContact *contact) {
	PurpleConversationManager *manager = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	manager = purple_conversation_manager_get_default();

	return purple_conversation_manager_find_dm(manager, contact);
}

void
purple_contact_create_dm_async(PurpleContact *contact,
                               GCancellable *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer data)
{
	PurpleProtocol *protocol = NULL;
	PurpleProtocolConversation *protocol_conversation = NULL;
	PurpleCreateConversationDetails *details = NULL;
	GListStore *participants = NULL;
	GTask *task = NULL;
	gboolean implements = FALSE;

	task = g_task_new(contact, cancellable, callback, data);
	g_task_set_source_tag(task, purple_contact_create_dm_async);

	protocol = purple_account_get_protocol(contact->account);
	if(!PURPLE_IS_PROTOCOL_CONVERSATION(protocol)) {
		g_task_return_new_error(task, PURPLE_CONTACT_DOMAIN, 0,
		                        "protocol %s does not implement the "
		                        "ProtocolConversation interface.",
		                        purple_account_get_protocol_id(contact->account));
		g_clear_object(&task);

		return;
	}

	protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(protocol);
	implements = purple_protocol_conversation_implements_create_conversation(protocol_conversation);
	if(!implements) {
		g_task_return_new_error(task, PURPLE_CONTACT_DOMAIN, 0,
		                        "protocol %s does not implement creating "
		                        "conversations.",
		                        purple_account_get_protocol_id(contact->account));
		g_clear_object(&task);

		return;
	}

	details = purple_protocol_conversation_get_create_conversation_details(protocol_conversation,
	                                                                       contact->account);

	participants = g_list_store_new(PURPLE_TYPE_CONTACT);
	g_list_store_append(participants, contact);
	purple_create_conversation_details_set_participants(details,
	                                                    G_LIST_MODEL(participants));
	g_clear_object(&participants);

	purple_protocol_conversation_create_conversation_async(protocol_conversation,
	                                                       contact->account,
	                                                       details,
	                                                       cancellable,
	                                                       purple_contact_create_dm_cb,
	                                                       task);
}

PurpleConversation *
purple_contact_create_dm_finish(PurpleContact *contact, GAsyncResult *result,
                                GError **error)
{
	GTask *task = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);
	g_return_val_if_fail(G_IS_TASK(result), NULL);

	task = G_TASK(result);

	g_return_val_if_fail(g_task_get_source_tag(task) == purple_contact_create_dm_async,
	                     NULL);

	return g_task_propagate_pointer(task, error);
}
