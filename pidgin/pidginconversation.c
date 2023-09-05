/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include <purple.h>

#include "pidgin/pidginconversation.h"
#include "pidgin/pidgininfopane.h"
#include "pidgin/pidginmessage.h"

#define PIDGIN_CONVERSATION_DATA ("pidgin-conversation")

enum {
	PROP_0,
	PROP_CONVERSATION,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL,};

struct _PidginConversation {
	GtkBox parent;

	PurpleConversation *conversation;

	GtkWidget *info_pane;
	GtkWidget *editor;
	GtkWidget *history;
};

G_DEFINE_TYPE(PidginConversation, pidgin_conversation, GTK_TYPE_BOX)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
pidgin_conversation_set_conversation(PidginConversation *conversation,
                                     PurpleConversation *purple_conversation)
{
	if(g_set_object(&conversation->conversation, purple_conversation)) {
		if(PURPLE_IS_CONVERSATION(purple_conversation)) {
			g_object_set_data(G_OBJECT(purple_conversation),
			                  PIDGIN_CONVERSATION_DATA, conversation);
		}

		pidgin_info_pane_set_conversation(PIDGIN_INFO_PANE(conversation->info_pane),
		                                  purple_conversation);

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_CONVERSATION]);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_conversation_send_message_cb(TalkatuInput *input, gpointer data) {
	PidginConversation *conversation = data;
	const char *contents = NULL;

	contents = talkatu_message_get_contents(TALKATU_MESSAGE(input));

	purple_conversation_send(conversation->conversation, contents);

	talkatu_message_set_contents(TALKATU_MESSAGE(input), "");
}

static void
pidgin_conversation_detach(PidginConversation *conversation) {
	if(PURPLE_IS_CONVERSATION(conversation->conversation)) {
		gpointer us = NULL;

		us = g_object_get_data(G_OBJECT(conversation->conversation),
		                       PIDGIN_CONVERSATION_DATA);

		if(conversation == us) {
			g_object_set_data(G_OBJECT(conversation->conversation),
			                  PIDGIN_CONVERSATION_DATA, NULL);
		}
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_conversation_dispose(GObject *obj) {
	PidginConversation *conversation = PIDGIN_CONVERSATION(obj);

	pidgin_conversation_detach(conversation);

	g_clear_object(&conversation->conversation);

	G_OBJECT_CLASS(pidgin_conversation_parent_class)->dispose(obj);
}

static void
pidgin_conversation_get_property(GObject *obj, guint param_id, GValue *value,
                                 GParamSpec *pspec)
{
	PidginConversation *conversation = PIDGIN_CONVERSATION(obj);

	switch(param_id) {
	case PROP_CONVERSATION:
		g_value_set_object(value,
		                   pidgin_conversation_get_conversation(conversation));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
pidgin_conversation_set_property(GObject *obj, guint param_id,
                                 const GValue *value, GParamSpec *pspec)
{
	PidginConversation *conversation = PIDGIN_CONVERSATION(obj);

	switch(param_id) {
	case PROP_CONVERSATION:
		pidgin_conversation_set_conversation(conversation,
		                                     g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
pidgin_conversation_init(PidginConversation *conversation) {
	gtk_widget_init_template(GTK_WIDGET(conversation));
}

static void
pidgin_conversation_class_init(PidginConversationClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->dispose = pidgin_conversation_dispose;
	obj_class->get_property = pidgin_conversation_get_property;
	obj_class->set_property = pidgin_conversation_set_property;

	/**
	 * PidginConversation:conversation:
	 *
	 * The [class@Purple.Conversation] that this conversation is displaying.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_CONVERSATION] = g_param_spec_object(
		"conversation", "conversation",
		"The purple conversation this widget is for.",
		PURPLE_TYPE_CONVERSATION,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/* Template stuff. */
	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/Conversations/conversation.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginConversation,
	                                     info_pane);
	gtk_widget_class_bind_template_child(widget_class, PidginConversation,
	                                     editor);
	gtk_widget_class_bind_template_child(widget_class, PidginConversation,
	                                     history);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_conversation_send_message_cb);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_conversation_new(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return g_object_new(
		PIDGIN_TYPE_CONVERSATION,
		"conversation", conversation,
		NULL);
}

GtkWidget *
pidgin_conversation_from_purple_conversation(PurpleConversation *conversation)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return g_object_get_data(G_OBJECT(conversation), PIDGIN_CONVERSATION_DATA);
}

PurpleConversation *
pidgin_conversation_get_conversation(PidginConversation *conversation) {
	g_return_val_if_fail(PIDGIN_IS_CONVERSATION(conversation), NULL);

	return conversation->conversation;
}

void
pidgin_conversation_write_message(PidginConversation *conversation,
                                  PurpleMessage *purple_message)
{
	PidginMessage *message = NULL;

	g_return_if_fail(PIDGIN_IS_CONVERSATION(conversation));
	g_return_if_fail(PURPLE_IS_MESSAGE(purple_message));

	message = pidgin_message_new(purple_message);

	talkatu_history_write_message(TALKATU_HISTORY(conversation->history),
	                              TALKATU_MESSAGE(message));

	g_clear_object(&message);
}

void
pidgin_conversation_close(PidginConversation *conversation) {
	g_return_if_fail(PIDGIN_IS_CONVERSATION(conversation));

	pidgin_conversation_detach(conversation);
}
