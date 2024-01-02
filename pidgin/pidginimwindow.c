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

#include <purple.h>

#include "pidginimwindow.h"

#include "pidginaccountrow.h"

struct _PidginIMWindow {
	AdwMessageDialog parent;

	GtkCustomFilter *filter;

	GtkWidget *account;
	GtkWidget *username;
};

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
pidgin_im_window_filter_accounts(gpointer item, G_GNUC_UNUSED gpointer data) {
	if(PURPLE_IS_ACCOUNT(item)) {
		PurpleAccount *account = PURPLE_ACCOUNT(item);
		PurpleProtocol *protocol = purple_account_get_protocol(account);

		if(PURPLE_IS_PROTOCOL(protocol)) {
			return PURPLE_PROTOCOL_IMPLEMENTS(protocol, CONVERSATION,
			                                  send_message_async);
		}
	}

	return FALSE;

}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_im_window_username_changed_cb(GtkEditable *self, gpointer data) {
	const char *text = NULL;
	gboolean enabled = FALSE;

	text = gtk_editable_get_text(self);
	enabled = !purple_strempty(text);

	adw_message_dialog_set_response_enabled(data, "okay", enabled);
}

static void
pidgin_im_window_response_cb(AdwMessageDialog *self,
                             const char *response,
                             G_GNUC_UNUSED gpointer data)
{
	PidginIMWindow *window = PIDGIN_IM_WINDOW(self);
	PurpleAccount *account = NULL;
	PurpleConversation *im = NULL;
	PurpleConversationManager *manager = NULL;
	const char *username = NULL;

	if(!purple_strequal(response, "okay")) {
		return;
	}

	account = pidgin_account_row_get_account(PIDGIN_ACCOUNT_ROW(window->account)) ;
	username = gtk_editable_get_text(GTK_EDITABLE(window->username));

	manager = purple_conversation_manager_get_default();
	im = purple_conversation_manager_find_im(manager, account, username);

	if(!PURPLE_IS_IM_CONVERSATION(im)) {
		/* This constructor automagically registers the conversation with the
		 * manager.
		 */
		purple_im_conversation_new(account, username);
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_TYPE(PidginIMWindow, pidgin_im_window, ADW_TYPE_MESSAGE_DIALOG)

static void
pidgin_im_window_init(PidginIMWindow *window) {
	gtk_widget_init_template(GTK_WIDGET(window));

	gtk_custom_filter_set_filter_func(window->filter,
	                                  pidgin_im_window_filter_accounts,
	                                  NULL, NULL);
}

static void
pidgin_im_window_class_init(PidginIMWindowClass *klass) {
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(
		widget_class,
		"/im/pidgin/Pidgin3/imwindow.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginIMWindow,
	                                     filter);

	gtk_widget_class_bind_template_child(widget_class, PidginIMWindow,
	                                     account);
	gtk_widget_class_bind_template_child(widget_class, PidginIMWindow,
	                                     username);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_im_window_username_changed_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_im_window_response_cb);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
GtkWidget *
pidgin_im_window_new(void) {
	return g_object_new(PIDGIN_TYPE_IM_WINDOW, NULL);
}
