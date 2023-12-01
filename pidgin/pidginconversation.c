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

#include "pidginautoadjustment.h"
#include "pidgincolor.h"
#include "pidgincontactinfomenu.h"
#include "pidginconversation.h"
#include "pidgininfopane.h"

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
	GtkWidget *history;
	GtkAdjustment *history_adjustment;

	GtkWidget *input;
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

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_CONVERSATION]);
	}
}

/**
 * pidgin_conversation_set_tooltip_for_timestamp: (skip)
 * @tooltip: The tooltip to update.
 * @timestamp: The timestamp to set.
 *
 * Updates @tooltip to display @timestamp. This is meant to be called from
 * a GtkWidget::query-tooltip signal and its return value should be returned
 * from that handler.
 *
 * Returns: %TRUE if a tooltip was set, otherwise %FALSE.
 *
 * Since: 3.0.0
 */
static gboolean
pidgin_conversation_set_tooltip_for_timestamp(GtkTooltip *tooltip,
                                              GDateTime *timestamp)
{
	GDateTime *local = NULL;
	char *text = NULL;

	if(timestamp == NULL) {
		return FALSE;
	}

	local = g_date_time_to_local(timestamp);
	text = g_date_time_format(local, "%c");
	g_clear_pointer(&local, g_date_time_unref);

	gtk_tooltip_set_text(tooltip, text);
	g_clear_pointer(&text, g_free);

	return TRUE;
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
pidgin_conversation_input_key_pressed_cb(G_GNUC_UNUSED GtkEventControllerKey *self,
                                         guint keyval,
                                         G_GNUC_UNUSED guint keycode,
                                         GdkModifierType state,
                                         gpointer data)
{
	PidginConversation *conversation = data;
	gboolean handled = TRUE;

	if(keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
		GtkTextBuffer *buffer = NULL;
		GtkTextIter start;
		GtkTextIter end;
		char *contents = NULL;

		if(state == GDK_SHIFT_MASK || state == GDK_CONTROL_MASK) {
			return FALSE;
		}

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(conversation->input));
		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);

		contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

		purple_conversation_send(conversation->conversation, contents);

		g_clear_pointer(&contents, g_free);

		gtk_text_buffer_set_text(buffer, "", -1);
	} else if(keyval == GDK_KEY_Page_Up) {
		pidgin_auto_adjustment_decrement(PIDGIN_AUTO_ADJUSTMENT(conversation->history_adjustment));
	} else if(keyval == GDK_KEY_Page_Down) {
		pidgin_auto_adjustment_increment(PIDGIN_AUTO_ADJUSTMENT(conversation->history_adjustment));
	} else {
		handled = FALSE;
	}

	return handled;
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

static PangoAttrList *
pidgin_conversation_get_author_attributes(G_GNUC_UNUSED GObject *self,
                                          PurpleMessage *message,
                                          G_GNUC_UNUSED gpointer data)
{
	const char *author = NULL;
	const char *custom_color = NULL;
	GdkRGBA rgba;
	PangoAttrList *attrs = NULL;
	gboolean color_valid = FALSE;

	if(!PURPLE_IS_MESSAGE(message)) {
		return NULL;
	}

	author = purple_message_get_author_alias(message);
	if(purple_strempty(author)) {
		author = purple_message_get_author(message);
	}

	custom_color = purple_message_get_author_name_color(message);
	if(!purple_strempty(custom_color)) {
		color_valid = gdk_rgba_parse(&rgba, custom_color);
	}

	if(!color_valid) {
		pidgin_color_calculate_for_text(author, &rgba);
		color_valid = TRUE;
	}

	attrs = pango_attr_list_new();

	if(color_valid) {
		PangoAttribute *attr = NULL;

		attr = pango_attr_foreground_new(0xFFFF * rgba.red,
		                                 0xFFFF * rgba.green,
		                                 0xFFFF * rgba.blue);
		pango_attr_list_insert(attrs, attr);
	}

	return attrs;
}

static char *
pidgin_converation_get_timestamp_string(G_GNUC_UNUSED GObject *self,
                                        PurpleMessage *message,
                                        G_GNUC_UNUSED gpointer data)
{
	GDateTime *timestamp = NULL;

	if(!PURPLE_IS_MESSAGE(message)) {
		return NULL;
	}

	timestamp = purple_message_get_timestamp(message);
	if(timestamp != NULL) {
		GDateTime *local = NULL;
		char *ret = NULL;

		local = g_date_time_to_local(timestamp);
		ret = g_date_time_format(local, "%I:%M %p");
		g_date_time_unref(local);

		return ret;
	}

	return NULL;
}

static gboolean
pidgin_conversation_query_tooltip_timestamp_cb(G_GNUC_UNUSED GtkWidget *self,
                                               G_GNUC_UNUSED gint x,
                                               G_GNUC_UNUSED gint y,
                                               G_GNUC_UNUSED gboolean keyboard_mode,
                                               GtkTooltip *tooltip,
                                               gpointer data)
{

	PurpleMessage *message = gtk_list_item_get_item(data);
	GDateTime *timestamp = NULL;

	if(!PURPLE_IS_MESSAGE(message)) {
		return FALSE;
	}

	timestamp = purple_message_get_timestamp(message);

	return pidgin_conversation_set_tooltip_for_timestamp(tooltip, timestamp);
}

static gboolean
pidgin_conversation_query_tooltip_edited_cb(G_GNUC_UNUSED GtkWidget *self,
                                            G_GNUC_UNUSED gint x,
                                            G_GNUC_UNUSED gint y,
                                            G_GNUC_UNUSED gboolean keyboard_mode,
                                            GtkTooltip *tooltip,
                                            gpointer data)
{
	PurpleMessage *message = gtk_list_item_get_item(data);
	GDateTime *timestamp = NULL;

	if(!PURPLE_IS_MESSAGE(message)) {
		return FALSE;
	}

	timestamp = purple_message_get_edited_at(message);

	return pidgin_conversation_set_tooltip_for_timestamp(tooltip, timestamp);
}

static char *
pidgin_conversation_process_message_contents_cb(G_GNUC_UNUSED GObject *self,
                                                const char *contents,
                                                G_GNUC_UNUSED gpointer data)
{
	return g_markup_escape_text(contents, -1);
}

static void
pidgin_conversation_member_list_context_cb(GtkGestureSingle *self,
                                           G_GNUC_UNUSED gint n_press,
                                           gdouble x,
                                           gdouble y,
                                           gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleContactInfo *info = NULL;
	PurpleConversationMember *member = NULL;
	GtkWidget *parent = NULL;
	GtkListItem *item = data;

	parent = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));

	member = gtk_list_item_get_item(item);
	info = purple_conversation_member_get_contact_info(member);

	/* ConversationMembers are a PurpleAccount for the libpurple user, or in
	 * most cases are PurpleContact for all the other users. Because of this,
	 * we have to do a runtime check to determine which one they are.
	 */
	if(PURPLE_IS_ACCOUNT(info)) {
		account = PURPLE_ACCOUNT(info);
	} else if(PURPLE_IS_CONTACT(info)) {
		account = purple_contact_get_account(PURPLE_CONTACT(info));
	}

	pidgin_contact_info_menu_popup(info, account, parent, x, y);
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
	                                     history);
	gtk_widget_class_bind_template_child(widget_class, PidginConversation,
	                                     history_adjustment);
	gtk_widget_class_bind_template_child(widget_class, PidginConversation,
	                                     input);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_conversation_input_key_pressed_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_conversation_get_author_attributes);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_converation_get_timestamp_string);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_conversation_query_tooltip_timestamp_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_conversation_query_tooltip_edited_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_conversation_process_message_contents_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_conversation_member_list_context_cb);
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
pidgin_conversation_close(PidginConversation *conversation) {
	g_return_if_fail(PIDGIN_IS_CONVERSATION(conversation));

	pidgin_conversation_detach(conversation);
}
