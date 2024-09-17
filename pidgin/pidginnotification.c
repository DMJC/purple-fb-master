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

#include "pidgin/pidginnotification.h"

typedef struct {
	PurpleNotification *notification;

	GtkWidget *details;
	GtkWidget *child;
} PidginNotificationPrivate;

enum {
	PROP_0,
	PROP_NOTIFICATION,
	PROP_CHILD,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(PidginNotification, pidgin_notification,
                           GTK_TYPE_BOX)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
pidgin_notification_set_notification(PidginNotification *pidgin_notification,
                                     PurpleNotification *purple_notification)
{
	PidginNotificationPrivate *priv = NULL;

	priv = pidgin_notification_get_instance_private(pidgin_notification);

	if(g_set_object(&priv->notification, purple_notification)) {
		g_object_notify_by_pspec(G_OBJECT(pidgin_notification),
		                         properties[PROP_NOTIFICATION]);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
pidgin_notification_show_subtitle(G_GNUC_UNUSED PidginNotification *notification,
                                  const char *subtitle)
{
	return !purple_strempty(subtitle);
}

/******************************************************************************
 * Widget Actions
 *****************************************************************************/
static void
pidgin_notification_close(GtkWidget *self,
                          G_GNUC_UNUSED const char *action_name,
                          G_GNUC_UNUSED GVariant *parameter)
{
	PidginNotification *notification = PIDGIN_NOTIFICATION(self);
	PidginNotificationPrivate *priv = NULL;
	PurpleNotificationManager *manager = NULL;

	priv = pidgin_notification_get_instance_private(notification);
	purple_notification_delete(priv->notification);

	manager = purple_notification_manager_get_default();
	purple_notification_manager_remove(manager, priv->notification);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_notification_get_property(GObject *obj, guint param_id, GValue *value,
                                 GParamSpec *pspec)
{
	PidginNotification *notification = PIDGIN_NOTIFICATION(obj);

	switch(param_id) {
	case PROP_NOTIFICATION:
		g_value_set_object(value,
		                   pidgin_notification_get_notification(notification));
		break;
	case PROP_CHILD:
		g_value_set_object(value,
		                   pidgin_notification_get_child(notification));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
pidgin_notification_set_property(GObject *obj, guint param_id,
                                 const GValue *value, GParamSpec *pspec)
{
	PidginNotification *notification = PIDGIN_NOTIFICATION(obj);

	switch(param_id) {
	case PROP_NOTIFICATION:
		pidgin_notification_set_notification(notification,
		                                     g_value_get_object(value));
		break;
	case PROP_CHILD:
		pidgin_notification_set_child(notification, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
pidgin_notification_dispose(GObject *obj) {
	PidginNotification *notification = PIDGIN_NOTIFICATION(obj);
	PidginNotificationPrivate *priv = NULL;

	priv = pidgin_notification_get_instance_private(notification);

	g_clear_object(&priv->notification);
	g_clear_pointer(&priv->child, gtk_widget_unparent);

	G_OBJECT_CLASS(pidgin_notification_parent_class)->dispose(obj);
}

static void
pidgin_notification_init(PidginNotification *notification) {
	gtk_widget_init_template(GTK_WIDGET(notification));
}

static void
pidgin_notification_class_init(PidginNotificationClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->get_property = pidgin_notification_get_property;
	obj_class->set_property = pidgin_notification_set_property;
	obj_class->dispose = pidgin_notification_dispose;

	/**
	 * PidginNotification:notification:
	 *
	 * The [type@Purple.Notification] that is being displayed.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NOTIFICATION] = g_param_spec_object(
		"notification", NULL, NULL,
		PURPLE_TYPE_NOTIFICATION,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PidginNotification:child:
	 *
	 * Child widgets to show between the details and the close button.
	 *
	 * This is typically actions like message, add contact, etc.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CHILD] = g_param_spec_object(
		"child", NULL, NULL,
		GTK_TYPE_WIDGET,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/* Actions */
	gtk_widget_class_install_action(widget_class, "notification.close",
	                                NULL, pidgin_notification_close);

	/* Bindings */
	gtk_widget_class_set_template_from_resource(widget_class,
	                                            "/im/pidgin/Pidgin3/notification.ui");

	gtk_widget_class_bind_template_child_private(widget_class,
	                                             PidginNotification, details);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_notification_show_subtitle);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_notification_new(PurpleNotification *notification) {
	return g_object_new(
		PIDGIN_TYPE_NOTIFICATION,
		"notification", notification,
		NULL);
}

PurpleNotification *
pidgin_notification_get_notification(PidginNotification *notification)
{
	PidginNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PIDGIN_IS_NOTIFICATION(notification), NULL);

	priv = pidgin_notification_get_instance_private(notification);

	return priv->notification;
}

GtkWidget *
pidgin_notification_get_child(PidginNotification *notification) {
	PidginNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PIDGIN_IS_NOTIFICATION(notification), NULL);

	priv = pidgin_notification_get_instance_private(notification);

	return priv->child;
}

void
pidgin_notification_set_child(PidginNotification *notification,
                              GtkWidget *child)
{
	PidginNotificationPrivate *priv = NULL;

	g_return_if_fail(PIDGIN_IS_NOTIFICATION(notification));

	priv = pidgin_notification_get_instance_private(notification);

	if(child == priv->child) {
		return;
	}

	g_clear_pointer(&priv->child, gtk_widget_unparent);
	if(GTK_IS_WIDGET(child)) {
		priv->child = child;
		gtk_box_insert_child_after(GTK_BOX(notification), child,
		                           priv->details);
	}

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_CHILD]);
}
