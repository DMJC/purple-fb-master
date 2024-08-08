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

#include "purplemessages.h"

enum {
	PROP_0,
	PROP_CONVERSATION,
	PROP_ITEM_TYPE,
	PROP_N_ITEMS,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

struct _PurpleMessages {
	GObject parent;

	PurpleConversation *conversation;

	GListStore *model;
};

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_messages_set_conversation(PurpleMessages *messages,
                                 PurpleConversation *conversation)
{
	g_return_if_fail(PURPLE_IS_MESSAGES(messages));

	if(g_set_object(&messages->conversation, conversation)) {
		g_object_notify_by_pspec(G_OBJECT(messages),
		                         properties[PROP_CONVERSATION]);
	}
}

static int
purple_messages_compare(gconstpointer a, gconstpointer b,
                        G_GNUC_UNUSED gpointer data)
{
	PurpleMessage *message1 = PURPLE_MESSAGE((gpointer)a);
	PurpleMessage *message2 = PURPLE_MESSAGE((gpointer)b);

	return purple_message_compare_timestamp(message1, message2);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_messages_items_changed_cb(G_GNUC_UNUSED GListModel *model,
                                 guint position, guint removed, guint added,
                                 gpointer data)
{
	g_list_model_items_changed(data, position, removed, added);
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_messages_get_item_type(GListModel *model) {
	PurpleMessages *messages = PURPLE_MESSAGES(model);

	return g_list_model_get_item_type(G_LIST_MODEL(messages->model));
}

static guint
purple_messages_get_n_items(GListModel *model) {
	PurpleMessages *messages = PURPLE_MESSAGES(model);

	return g_list_model_get_n_items(G_LIST_MODEL(messages->model));
}

static gpointer
purple_messages_get_item(GListModel *model, guint position) {
	PurpleMessages *messages = PURPLE_MESSAGES(model);

	return g_list_model_get_item(G_LIST_MODEL(messages->model), position);
}

static void
purple_messages_list_model_iface_init(GListModelInterface *iface) {
	iface->get_item_type = purple_messages_get_item_type;
	iface->get_n_items = purple_messages_get_n_items;
	iface->get_item = purple_messages_get_item;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE_WITH_CODE(
	PurpleMessages,
	purple_messages,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
	                      purple_messages_list_model_iface_init))

static void
purple_messages_dispose(GObject *obj) {
	PurpleMessages *message = PURPLE_MESSAGES(obj);

	g_clear_object(&message->conversation);

	g_list_store_remove_all(message->model);

	G_OBJECT_CLASS(purple_messages_parent_class)->dispose(obj);
}

static void
purple_messages_finalize(GObject *obj) {
	PurpleMessages *message = PURPLE_MESSAGES(obj);

	g_clear_object(&message->model);

	G_OBJECT_CLASS(purple_messages_parent_class)->finalize(obj);
}

static void
purple_messages_get_property(GObject *obj, guint param_id, GValue *value,
                             GParamSpec *pspec)
{
	PurpleMessages *messages = PURPLE_MESSAGES(obj);

	switch(param_id) {
	case PROP_CONVERSATION:
		g_value_set_object(value, purple_messages_get_conversation(messages));
		break;
	case PROP_ITEM_TYPE:
		g_value_set_gtype(value,
		                  g_list_model_get_item_type(G_LIST_MODEL(messages->model)));
		break;
	case PROP_N_ITEMS:
		g_value_set_uint(value,
		                 g_list_model_get_n_items(G_LIST_MODEL(messages->model)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_messages_set_property(GObject *obj, guint param_id, const GValue *value,
                             GParamSpec *pspec)
{
	PurpleMessages *messages = PURPLE_MESSAGES(obj);

	switch(param_id) {
	case PROP_CONVERSATION:
		purple_messages_set_conversation(messages, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_messages_init(PurpleMessages *messages) {
	messages->model = g_list_store_new(PURPLE_TYPE_MESSAGE);
	g_signal_connect_object(messages->model, "items-changed",
	                        G_CALLBACK(purple_messages_items_changed_cb),
	                        messages, G_CONNECT_DEFAULT);
}

static void
purple_messages_class_init(PurpleMessagesClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->dispose = purple_messages_dispose;
	obj_class->finalize = purple_messages_finalize;
	obj_class->get_property = purple_messages_get_property;
	obj_class->set_property = purple_messages_set_property;

	/**
	 * PurpleMessages:conversation:
	 *
	 * The [class@Conversation] that these messages belong to.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONVERSATION] = g_param_spec_object(
		"conversation", NULL, NULL,
		PURPLE_TYPE_CONVERSATION,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessages:item-type:
	 *
	 * The type of items. See [iface@Gio.ListModel.get_item_type].
	 *
	 * Since: 3.0
	 */
	properties[PROP_ITEM_TYPE] = g_param_spec_gtype(
		"item-type", NULL, NULL,
		PURPLE_TYPE_MESSAGE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessages:n-items:
	 *
	 * The number of items. See [iface@Gio.ListModel.get_n_items].
	 *
	 * Since: 3.0
	 */
	properties[PROP_N_ITEMS] = g_param_spec_uint(
		"n-items", NULL, NULL,
		0, G_MAXUINT, 0,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleMessages *
purple_messages_new(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return g_object_new(
		PURPLE_TYPE_MESSAGES,
		"conversation", conversation,
		NULL);
}

PurpleConversation *
purple_messages_get_conversation(PurpleMessages *messages) {
	g_return_val_if_fail(PURPLE_IS_MESSAGES(messages), NULL);

	return messages->conversation;
}

void
purple_messages_add(PurpleMessages *messages, PurpleMessage *message) {
	g_return_if_fail(PURPLE_IS_MESSAGES(messages));
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_list_store_insert_sorted(messages->model, message,
	                           (GCompareDataFunc)purple_messages_compare, NULL);
}
