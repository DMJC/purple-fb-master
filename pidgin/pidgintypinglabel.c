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

#include "pidgintypinglabel.h"

struct _PidginTypingLabel {
	GtkBox parent;

	PurpleConversation *conversation;

	GtkLabel *label;

	GtkFilterListModel *typers;
};

enum {
	PROP_0,
	PROP_CONVERSATION,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

static void
pidgin_typing_label_items_changed_cb(GListModel *model, guint position,
                                     guint removed, guint added,
                                     gpointer data);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
pidgin_typing_label_filter_active_typers(GObject *item, gpointer data) {
	PurpleAccount *account = NULL;
	PurpleContactInfo *account_info = NULL;
	PurpleContactInfo *member_info = NULL;
	PurpleConversation *conversation = data;
	PurpleConversationMember *member = PURPLE_CONVERSATION_MEMBER(item);

	account = purple_conversation_get_account(conversation);
	account_info = purple_account_get_contact_info(account);

	member_info = purple_conversation_member_get_contact_info(member);

	/* We don't want to show our own account, so if member_info and
	 * account_info match, we return FALSE.
	 */
	if(member_info == account_info) {
		return FALSE;
	}

	return TRUE;
}

static void
pidgin_typing_label_set_conversation(PidginTypingLabel *label,
                                     PurpleConversation *conversation)
{
	g_return_if_fail(PIDGIN_IS_TYPING_LABEL(label));

	if(g_set_object(&label->conversation, conversation)) {
		if(PURPLE_IS_CONVERSATION(conversation)) {
			PurpleConversationMembers *members = NULL;
			GtkCustomFilter *filter = NULL;
			GListModel *active_typers = NULL;

			members = purple_conversation_get_members(conversation);

			active_typers = purple_conversation_members_get_active_typers(members);

			gtk_filter_list_model_set_model(GTK_FILTER_LIST_MODEL(label->typers),
			                                active_typers);

			filter = gtk_custom_filter_new((GtkCustomFilterFunc)pidgin_typing_label_filter_active_typers,
			                               conversation, NULL);
			gtk_filter_list_model_set_filter(GTK_FILTER_LIST_MODEL(label->typers),
			                                 GTK_FILTER(filter));
			g_clear_object(&filter);

			g_signal_connect_object(label->typers, "items-changed",
			                        G_CALLBACK(pidgin_typing_label_items_changed_cb),
			                        label, G_CONNECT_DEFAULT);
		}

		g_object_notify_by_pspec(G_OBJECT(label),
		                         properties[PROP_CONVERSATION]);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_typing_label_items_changed_cb(GListModel *model,
                                     G_GNUC_UNUSED guint position,
                                     G_GNUC_UNUSED guint removed,
                                     G_GNUC_UNUSED guint added,
                                     gpointer data)
{
	PidginTypingLabel *label = data;
	guint n_items = 0;

	n_items = g_list_model_get_n_items(model);
	if(n_items == 0) {
		gtk_label_set_label(label->label, "");
	} else if(n_items == 1) {
		PurpleConversationMember *member = NULL;
		char *text = NULL;
		const char *name = NULL;

		member = g_list_model_get_item(model, 0);
		name = purple_conversation_member_get_name_for_display(member);
		text = g_strdup_printf(_("<b>%s</b> is typing..."), name);

		gtk_label_set_label(label->label, text);

		g_clear_pointer(&text, g_free);
		g_clear_object(&member);
	} else if(n_items == 2) {
		PurpleConversationMember *member1 = NULL;
		PurpleConversationMember *member2 = NULL;
		char *text = NULL;
		const char *name1 = NULL;
		const char *name2 = NULL;

		member1 = g_list_model_get_item(model, 0);
		name1 = purple_conversation_member_get_name_for_display(member1);

		member2 = g_list_model_get_item(model, 1);
		name2 = purple_conversation_member_get_name_for_display(member2);

		text = g_strdup_printf(_("<b>%s</b> and <b>%s</b> are typing..."),
		                       name1, name2);

		gtk_label_set_label(label->label, text);

		g_clear_pointer(&text, g_free);

		g_clear_object(&member1);
		g_clear_object(&member2);
	} else if(n_items == 3) {
		PurpleConversationMember *member1 = NULL;
		PurpleConversationMember *member2 = NULL;
		PurpleConversationMember *member3 = NULL;
		char *text = NULL;
		const char *name1 = NULL;
		const char *name2 = NULL;
		const char *name3 = NULL;

		member1 = g_list_model_get_item(model, 0);
		name1 = purple_conversation_member_get_name_for_display(member1);

		member2 = g_list_model_get_item(model, 1);
		name2 = purple_conversation_member_get_name_for_display(member2);

		member3 = g_list_model_get_item(model, 2);
		name3 = purple_conversation_member_get_name_for_display(member3);

		text = g_strdup_printf(_("<b>%s</b>, <b>%s</b>, and <b>%s</b> are typing..."),
		                       name1, name2, name3);

		gtk_label_set_label(label->label, text);

		g_clear_pointer(&text, g_free);

		g_clear_object(&member1);
		g_clear_object(&member2);
		g_clear_object(&member3);
	} else if(n_items > 3) {
		gtk_label_set_label(label->label, _("Several people are typing..."));
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PidginTypingLabel, pidgin_typing_label, GTK_TYPE_BOX)

static void
pidgin_typing_label_finalize(GObject *obj) {
	PidginTypingLabel *label = PIDGIN_TYPING_LABEL(obj);

	g_clear_object(&label->conversation);
	g_clear_object(&label->typers);

	G_OBJECT_CLASS(pidgin_typing_label_parent_class)->finalize(obj);
}

static void
pidgin_typing_label_get_property(GObject *obj, guint param_id, GValue *value,
                                 GParamSpec *pspec)
{
	PidginTypingLabel *label = PIDGIN_TYPING_LABEL(obj);

	switch(param_id) {
	case PROP_CONVERSATION:
		g_value_set_object(value, pidgin_typing_label_get_conversation(label));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
pidgin_typing_label_set_property(GObject *obj, guint param_id,
                                 const GValue *value, GParamSpec *pspec)
{
	PidginTypingLabel *label = PIDGIN_TYPING_LABEL(obj);

	switch(param_id) {
	case PROP_CONVERSATION:
		pidgin_typing_label_set_conversation(label, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
pidgin_typing_label_init(PidginTypingLabel *label) {
	gtk_widget_init_template(GTK_WIDGET(label));

	label->typers = gtk_filter_list_model_new(NULL, NULL);
}

static void
pidgin_typing_label_class_init(PidginTypingLabelClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->finalize = pidgin_typing_label_finalize;
	obj_class->get_property = pidgin_typing_label_get_property;
	obj_class->set_property = pidgin_typing_label_set_property;

	/**
	 * PidginTypingLabel:conversation:
	 *
	 * The [class@Purple.Conversation] that this label is for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONVERSATION] = g_param_spec_object(
		"conversation", NULL, NULL,
		PURPLE_TYPE_CONVERSATION,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/* Template stuff. */
	gtk_widget_class_set_template_from_resource(widget_class,
	                                            "/im/pidgin/Pidgin3/typinglabel.ui");

	gtk_widget_class_bind_template_child(widget_class, PidginTypingLabel,
	                                     label);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
GtkWidget *
pidgin_typing_label_new(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return g_object_new(
		PIDGIN_TYPE_TYPING_LABEL,
		"conversation", conversation,
		NULL);
}

PurpleConversation *
pidgin_typing_label_get_conversation(PidginTypingLabel *label) {
	g_return_val_if_fail(PIDGIN_IS_TYPING_LABEL(label), NULL);

	return label->conversation;
}
