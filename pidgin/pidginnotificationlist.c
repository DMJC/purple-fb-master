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

#include "pidgin/pidginnotificationlist.h"

#include "pidgin/pidginnotificationaddcontact.h"
#include "pidgin/pidginnotificationauthorizationrequest.h"
#include "pidgin/pidginnotificationconnectionerror.h"

struct _PidginNotificationList {
	GtkBox parent;

	GtkStack *stack;
	GtkSingleSelection *selection_model;
};

G_DEFINE_FINAL_TYPE(PidginNotificationList, pidgin_notification_list,
                    GTK_TYPE_BOX)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
pidgin_notification_gpointer_to_char(G_GNUC_UNUSED GBinding *binding,
                                     const GValue *from_value,
                                     GValue *to_value,
                                     G_GNUC_UNUSED gpointer user_data)
{
	g_value_set_string(to_value, (char *)g_value_get_pointer(from_value));

	return TRUE;
}

static GtkWidget *
pidgin_notification_list_unknown_notification(PurpleNotification *notification) {
	GtkWidget *widget = NULL;
	gchar *label = NULL;
	const gchar *title = NULL;

	title = purple_notification_get_title(notification);
	if(title != NULL) {
		label = g_strdup_printf(_("Unknown notification type %d: %s"),
		                        purple_notification_get_notification_type(notification),
		                        title);
	} else {
		label = g_strdup_printf(_("Unknown notification type %d"),
		                        purple_notification_get_notification_type(notification));
	}

	widget = gtk_label_new(label);

	g_free(label);

	return widget;
}

static GtkWidget *
pidgin_notification_generic_new(PurpleNotification *notification) {
	GtkWidget *row = NULL;
	GtkWidget *icon = NULL;

	icon = gtk_image_new();
	gtk_image_set_icon_size(GTK_IMAGE(icon), GTK_ICON_SIZE_LARGE);
	g_object_bind_property(notification, "icon-name", icon, "icon-name",
	                       G_BINDING_SYNC_CREATE);

	row = adw_action_row_new();
	adw_action_row_add_prefix(ADW_ACTION_ROW(row), icon);

	g_object_bind_property(notification, "title", row, "title",
	                       G_BINDING_SYNC_CREATE);
	g_object_bind_property_full(notification, "data", row, "subtitle",
	                            G_BINDING_SYNC_CREATE,
	                            pidgin_notification_gpointer_to_char,
	                            NULL,
	                            NULL,
	                            NULL);

	return row;
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_notification_list_items_changed_cb(GListModel *model,
                                          G_GNUC_UNUSED guint position,
                                          G_GNUC_UNUSED guint added,
                                          G_GNUC_UNUSED guint removed,
                                          gpointer data)
{
	PidginNotificationList *list = data;

	if(g_list_model_get_n_items(model) != 0) {
		gtk_stack_set_visible_child_name(list->stack, "view");
	} else {
		gtk_stack_set_visible_child_name(list->stack, "placeholder");
	}
}

static void
pidgin_notification_list_bind_cb(G_GNUC_UNUSED GtkSignalListItemFactory *self,
                                 GObject *object,
                                 G_GNUC_UNUSED gpointer data)
{
	PurpleNotification *notification = NULL;
	GtkListItem *item = GTK_LIST_ITEM(object);
	GtkWidget *widget = NULL;

	notification = gtk_list_item_get_item(item);

	switch(purple_notification_get_notification_type(notification)) {
		case PURPLE_NOTIFICATION_TYPE_GENERIC:
			widget = pidgin_notification_generic_new(notification);
			break;
		case PURPLE_NOTIFICATION_TYPE_CONNECTION_ERROR:
			widget = pidgin_notification_connection_error_new(notification);
			break;
		case PURPLE_NOTIFICATION_TYPE_AUTHORIZATION_REQUEST:
			widget = pidgin_notification_authorization_request_new(notification);
			break;
		case PURPLE_NOTIFICATION_TYPE_ADD_CONTACT:
			widget = pidgin_notification_add_contact_new(notification);
			break;
		default:
			widget = pidgin_notification_list_unknown_notification(notification);
			break;
	}

	if(!GTK_IS_WIDGET(widget)) {
		widget = pidgin_notification_list_unknown_notification(notification);
	}

	gtk_list_item_set_child(item, widget);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_notification_list_init(PidginNotificationList *list) {
	GListModel *model = NULL;

	gtk_widget_init_template(GTK_WIDGET(list));

	model = purple_notification_manager_get_default_as_model();
	gtk_single_selection_set_model(list->selection_model, model);
	g_signal_connect(model, "items-changed",
	                 G_CALLBACK(pidgin_notification_list_items_changed_cb),
	                 list);
	pidgin_notification_list_items_changed_cb(model, 0, 0, 0, list);
}

static void
pidgin_notification_list_class_init(PidginNotificationListClass *klass) {
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/notificationlist.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginNotificationList,
	                                     stack);
	gtk_widget_class_bind_template_child(widget_class, PidginNotificationList,
	                                     selection_model);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_notification_list_bind_cb);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_notification_list_new(void) {
	return g_object_new(PIDGIN_TYPE_NOTIFICATION_LIST, NULL);
}
