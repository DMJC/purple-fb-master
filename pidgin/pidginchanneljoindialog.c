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

#include <glib/gi18n.h>

#include "pidginchanneljoindialog.h"

#include "pidginaccountrow.h"
#include "pidgincore.h"

struct _PidginChannelJoinDialog {
	GtkWindow parent;

	GtkWidget *account;
	GtkCustomFilter *filter;
	GtkWidget *entries;
	GtkWidget *name;
	GtkWidget *nickname;
	GtkWidget *password;
	GtkWidget *error;
	GtkWidget *join;
	GtkWidget *cancel;

	GCancellable *cancellable;

	PurpleChannelJoinDetails *details;
};

G_DEFINE_TYPE(PidginChannelJoinDialog, pidgin_channel_join_dialog,
              GTK_TYPE_WINDOW)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
pidgin_channel_join_dialog_filter_accounts(gpointer item,
                                           G_GNUC_UNUSED gpointer data)
{
	if(PURPLE_IS_ACCOUNT(item)) {
		PurpleAccount *account = PURPLE_ACCOUNT(item);
		PurpleProtocol *protocol = purple_account_get_protocol(account);

		if(PURPLE_IS_PROTOCOL(protocol)) {
			return PURPLE_PROTOCOL_IMPLEMENTS(protocol, CONVERSATION,
			                                  get_channel_join_details);
		}
	}

	return FALSE;
}

static void
pidgin_channel_join_dialog_update(PidginChannelJoinDialog *dialog) {
	PurpleAccount *account = NULL;
	PurpleChannelJoinDetails *details = NULL;
	PurpleProtocol *protocol = NULL;
	PurpleProtocolConversation *protocol_conversation = NULL;

	account = pidgin_account_row_get_account(PIDGIN_ACCOUNT_ROW(dialog->account));
	if(!PURPLE_IS_ACCOUNT(account)) {
		return;
	}

	protocol = purple_account_get_protocol(account);
	if(!PURPLE_IS_PROTOCOL_CONVERSATION(protocol)) {
		return;
	}

	protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(protocol);

	details = purple_protocol_conversation_get_channel_join_details(protocol_conversation,
	                                                                account);
	if(PURPLE_IS_CHANNEL_JOIN_DETAILS(details)) {
		purple_channel_join_details_merge(details, dialog->details);
	}
}

static gboolean
pidgin_channel_join_dialog_details_to_row(G_GNUC_UNUSED GBinding *binding,
                                          const GValue *from_value,
                                          GValue *to_value,
                                          G_GNUC_UNUSED gpointer data)
{
	const char *str = NULL;

	str = g_value_get_string(from_value);
	if(purple_strempty(str)) {
		g_value_set_string(to_value, "");
	} else {
		g_value_set_string(to_value, str);
	}

	return TRUE;
}

static gboolean
pidgin_channel_join_dialog_row_to_details(G_GNUC_UNUSED GBinding *binding,
                                          const GValue *from_value,
                                          GValue *to_value,
                                          G_GNUC_UNUSED gpointer data)
{
	const char *str = NULL;

	str = g_value_get_string(from_value);
	if(purple_strempty(str)) {
		g_value_set_string(to_value, NULL);
	} else {
		g_value_set_string(to_value, str);
	}

	return TRUE;
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_channel_join_dialog_account_changed_cb(G_GNUC_UNUSED GObject *obj,
                                              G_GNUC_UNUSED GParamSpec *pspec,
                                              gpointer data)
{
	pidgin_channel_join_dialog_update(data);
}

static void
pidgin_channel_join_dialog_join_named_changed_cb(GtkEditable *self,
                                                 gpointer data)
{
	PidginChannelJoinDialog *dialog = data;
	const char *text = NULL;
	gboolean sensitive = FALSE;

	text = gtk_editable_get_text(self);
	sensitive = !purple_strempty(text);

	gtk_widget_set_sensitive(dialog->join, sensitive);
}

static void
pidgin_channel_join_dialog_cancelled_cb(G_GNUC_UNUSED GCancellable *cancellable,
                                        gpointer data)
{
	PidginChannelJoinDialog *dialog = data;

	gtk_widget_set_sensitive(dialog->entries, TRUE);
	gtk_widget_set_sensitive(dialog->join, TRUE);
	gtk_widget_set_sensitive(dialog->cancel, TRUE);

	g_clear_object(&dialog->cancellable);
}

static void
pidgin_channel_join_dialog_cancel_clicked(G_GNUC_UNUSED GtkButton *self,
                                          gpointer data)
{
	PidginChannelJoinDialog *dialog = data;

	gtk_widget_set_sensitive(dialog->cancel, FALSE);

	/* Check if the user is cancelling a join that was started but hadn't
	 * completed yet.
	 */
	if(G_IS_CANCELLABLE(dialog->cancellable)) {
		if(!g_cancellable_is_cancelled(dialog->cancellable)) {
			g_cancellable_cancel(dialog->cancellable);
		}

		g_clear_object(&dialog->cancellable);

		return;
	}

	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void
pidgin_channel_join_dialog_join_cb(GObject *source, GAsyncResult *result,
                                   gpointer data)
{
	PidginChannelJoinDialog *dialog = data;
	PurpleProtocolConversation *protocol_conversation = NULL;
	GError *error = NULL;
	gboolean joined = FALSE;

	protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(source);

	joined = purple_protocol_conversation_join_channel_finish(protocol_conversation,
	                                                          result, &error);

	if(!joined) {
		const char *error_message = _("Unknown error");

		if(error != NULL) {
			error_message = error->message;
		}

		gtk_label_set_text(GTK_LABEL(dialog->error), error_message);

		gtk_widget_set_sensitive(dialog->entries, TRUE);
		gtk_widget_set_sensitive(dialog->join, TRUE);
	} else {
		g_clear_object(&dialog->cancellable);
		gtk_window_destroy(GTK_WINDOW(dialog));
	}

	g_clear_error(&error);
}

static void
pidgin_channel_join_dialog_join_clicked(G_GNUC_UNUSED GtkButton *self,
                                        gpointer data)
{
	PidginChannelJoinDialog *dialog = data;
	PurpleAccount *account = NULL;
	PurpleProtocol *protocol = NULL;
	PurpleProtocolConversation *protocol_conversation = NULL;

	gtk_label_set_text(GTK_LABEL(dialog->error), "");

	gtk_widget_set_sensitive(dialog->entries, FALSE);
	gtk_widget_set_sensitive(dialog->join, FALSE);

	account = pidgin_account_row_get_account(PIDGIN_ACCOUNT_ROW(dialog->account));
	if(!PURPLE_IS_ACCOUNT(account)) {
		return;
	}

	protocol = purple_account_get_protocol(account);
	if(!PURPLE_IS_PROTOCOL_CONVERSATION(protocol)) {
		return;
	}

	g_return_if_fail(!G_IS_CANCELLABLE(dialog->cancellable));

	dialog->cancellable = g_cancellable_new();
	g_cancellable_connect(dialog->cancellable,
	                      G_CALLBACK(pidgin_channel_join_dialog_cancelled_cb),
	                      dialog, NULL);

	protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(protocol);

	purple_protocol_conversation_join_channel_async(protocol_conversation,
	                                                account,
	                                                dialog->details,
	                                                dialog->cancellable,
	                                                pidgin_channel_join_dialog_join_cb,
	                                                dialog);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_channel_join_dialog_finalize(GObject *obj) {
	PidginChannelJoinDialog *dialog = PIDGIN_CHANNEL_JOIN_DIALOG(obj);

	g_clear_object(&dialog->cancellable);
	g_clear_object(&dialog->details);

	G_OBJECT_CLASS(pidgin_channel_join_dialog_parent_class)->finalize(obj);
}

static void
pidgin_channel_join_dialog_init(PidginChannelJoinDialog *dialog) {
	gtk_widget_init_template(GTK_WIDGET(dialog));

	gtk_custom_filter_set_filter_func(dialog->filter,
	                                  pidgin_channel_join_dialog_filter_accounts,
	                                  NULL, NULL);

	dialog->details = purple_channel_join_details_new(FALSE, FALSE);

	/* Bind the visible properties. */
	g_object_bind_property(dialog->details, "nickname-supported",
	                       dialog->nickname, "visible",
	                       G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
	g_object_bind_property(dialog->details, "password-supported",
	                       dialog->password, "visible",
	                       G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

	/* Bind the data properties. */
	g_object_bind_property_full(dialog->details, "name",
	                            dialog->name, "text",
	                            G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL,
	                            pidgin_channel_join_dialog_details_to_row,
	                            pidgin_channel_join_dialog_row_to_details,
	                            NULL, NULL);
	g_object_bind_property_full(dialog->details, "nickname",
	                            dialog->nickname, "text",
	                            G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL,
	                            pidgin_channel_join_dialog_details_to_row,
	                            pidgin_channel_join_dialog_row_to_details,
	                            NULL, NULL);
	g_object_bind_property_full(dialog->details, "password",
	                            dialog->password, "text",
	                            G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL,
	                            pidgin_channel_join_dialog_details_to_row,
	                            pidgin_channel_join_dialog_row_to_details,
	                            NULL, NULL);

	/* Make sure we are synchronized with the account that is selected. */
	pidgin_channel_join_dialog_update(dialog);
}

static void
pidgin_channel_join_dialog_class_init(PidginChannelJoinDialogClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->finalize = pidgin_channel_join_dialog_finalize;

	gtk_widget_class_set_template_from_resource(
		widget_class,
		"/im/pidgin/Pidgin3/channeljoindialog.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     account);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     filter);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     entries);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     name);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     nickname);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     password);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     error);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     join);
	gtk_widget_class_bind_template_child(widget_class, PidginChannelJoinDialog,
	                                     cancel);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_channel_join_dialog_account_changed_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_channel_join_dialog_join_named_changed_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_channel_join_dialog_cancel_clicked);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_channel_join_dialog_join_clicked);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
GtkWidget *
pidgin_channel_join_dialog_new(void) {
	return g_object_new(PIDGIN_TYPE_CHANNEL_JOIN_DIALOG, NULL);
}
