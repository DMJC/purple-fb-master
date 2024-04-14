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

#include "pidgin/pidginavatar.h"

struct _PidginAvatar {
	GtkBox parent;

	GtkWidget *icon;
	GMenuModel *menu;

	GdkPixbufAnimation *animation;
	gboolean animate;

	PurpleContactInfo *info;
	PurpleConversation *conversation;
};

enum {
	PROP_0,
	PROP_ANIMATE,
	PROP_CONTACT_INFO,
	PROP_CONVERSATION,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_FINAL_TYPE(PidginAvatar, pidgin_avatar, GTK_TYPE_BOX)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
pidgin_avatar_update(PidginAvatar *avatar) {
	PurpleAvatar *purple_avatar = NULL;
	GdkPixbufAnimation *animation = NULL;
	GdkPixbuf *pixbuf = NULL;

	if(PURPLE_IS_CONTACT_INFO(avatar->info)) {
		purple_avatar = purple_contact_info_get_avatar(avatar->info);
	} else if(PURPLE_IS_CONVERSATION(avatar->conversation)) {
		purple_avatar = purple_conversation_get_avatar(avatar->conversation);
	}

	if(PURPLE_IS_AVATAR(purple_avatar)) {
		animation = purple_avatar_get_animation(purple_avatar);
	}

	g_set_object(&avatar->animation, animation);

	if(GDK_IS_PIXBUF_ANIMATION(avatar->animation)) {
		if(avatar->animate &&
		   !gdk_pixbuf_animation_is_static_image(avatar->animation)) {
			pixbuf = GDK_PIXBUF(avatar->animation);
		} else {
			pixbuf = gdk_pixbuf_animation_get_static_image(avatar->animation);
		}
	}

	gtk_picture_set_pixbuf(GTK_PICTURE(avatar->icon), pixbuf);

	g_clear_object(&animation);
}

/******************************************************************************
 * Actions
 *****************************************************************************/
static GActionEntry actions[] = {};

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
pidgin_avatar_button_press_handler(G_GNUC_UNUSED GtkGestureClick *event,
                                   G_GNUC_UNUSED gint n_press,
                                   gdouble x,
                                   gdouble y,
                                   gpointer data)
{
	PidginAvatar *avatar = PIDGIN_AVATAR(data);
	GtkWidget *menu = NULL;

	menu = gtk_popover_menu_new_from_model(avatar->menu);
	gtk_widget_set_parent(menu, GTK_WIDGET(avatar));
	gtk_popover_set_pointing_to(GTK_POPOVER(menu),
	                            &(const GdkRectangle){(int)x, (int)y, 0, 0});

	gtk_popover_popup(GTK_POPOVER(menu));

	return TRUE;
}

/*
 * This function is a callback for when properties change on the contact info
 * we're tracking. It should not be reused for the conversation we're tracking
 * because we have to disconnect old handlers and reuse of this function will
 * cause issues if a contact info is changed but a conversation is not and vice
 * versa.
 */
static void
pidgin_avatar_contact_info_updated(G_GNUC_UNUSED GObject *obj,
                                   G_GNUC_UNUSED GParamSpec *pspec,
                                   gpointer d)
{
	PidginAvatar *avatar = PIDGIN_AVATAR(d);

	pidgin_avatar_update(avatar);
}

/*
 * This function is a callback for when properties change on the conversation
 * we're tracking.  It should not be reused for the buddy we're tracking
 * because we have to disconnect old handlers and reuse of this function will
 * cause issues if a buddy is changed but a conversation is not and vice versa.
 */
static void
pidgin_avatar_conversation_updated(G_GNUC_UNUSED GObject *obj,
                                   G_GNUC_UNUSED GParamSpec *pspec, gpointer d)
{
	PidginAvatar *avatar = PIDGIN_AVATAR(d);

	pidgin_avatar_update(avatar);
}

static gboolean
pidgin_avatar_enter_notify_handler(G_GNUC_UNUSED GtkEventControllerMotion *event,
                                   G_GNUC_UNUSED gdouble x,
                                   G_GNUC_UNUSED gdouble y,
                                   gpointer data)
{
	PidginAvatar *avatar = PIDGIN_AVATAR(data);

	pidgin_avatar_set_animate(avatar, TRUE);

	return FALSE;
}

static gboolean
pidgin_avatar_leave_notify_handler(G_GNUC_UNUSED GtkEventControllerMotion *event,
                                   gpointer data)
{
	PidginAvatar *avatar = PIDGIN_AVATAR(data);

	pidgin_avatar_set_animate(avatar, FALSE);

	return FALSE;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_avatar_get_property(GObject *obj, guint param_id, GValue *value,
                           GParamSpec *pspec)
{
	PidginAvatar *avatar = PIDGIN_AVATAR(obj);

	switch(param_id) {
		case PROP_ANIMATE:
			g_value_set_boolean(value, pidgin_avatar_get_animate(avatar));
			break;
		case PROP_CONTACT_INFO:
			g_value_set_object(value, pidgin_avatar_get_contact_info(avatar));
			break;
		case PROP_CONVERSATION:
			g_value_set_object(value, pidgin_avatar_get_conversation(avatar));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
pidgin_avatar_set_property(GObject *obj, guint param_id, const GValue *value,
                           GParamSpec *pspec)
{
	PidginAvatar *avatar = PIDGIN_AVATAR(obj);

	switch(param_id) {
		case PROP_ANIMATE:
			pidgin_avatar_set_animate(avatar, g_value_get_boolean(value));
			break;
		case PROP_CONTACT_INFO:
			pidgin_avatar_set_contact_info(avatar, g_value_get_object(value));
			break;
		case PROP_CONVERSATION:
			pidgin_avatar_set_conversation(avatar, g_value_get_object(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
pidgin_avatar_dispose(GObject *obj) {
	PidginAvatar *avatar = PIDGIN_AVATAR(obj);

	g_clear_object(&avatar->info);
	g_clear_object(&avatar->conversation);

	g_clear_object(&avatar->animation);

	G_OBJECT_CLASS(pidgin_avatar_parent_class)->dispose(obj);
}

static void
pidgin_avatar_init(PidginAvatar *avatar) {
	GSimpleActionGroup *group = NULL;

	gtk_widget_init_template(GTK_WIDGET(avatar));

	gtk_picture_set_content_fit(GTK_PICTURE(avatar->icon),
	                            GTK_CONTENT_FIT_SCALE_DOWN);

	/* Now setup our actions. */
	group = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(group), actions,
	                                G_N_ELEMENTS(actions), avatar);

	gtk_widget_insert_action_group(GTK_WIDGET(avatar), "avatar",
	                               G_ACTION_GROUP(group));
}

static void
pidgin_avatar_class_init(PidginAvatarClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->get_property = pidgin_avatar_get_property;
	obj_class->set_property = pidgin_avatar_set_property;
	obj_class->dispose = pidgin_avatar_dispose;

	/**
	 * PidginAvatar:animate:
	 *
	 * Whether or not an animated avatar should be animated.
	 */
	properties[PROP_ANIMATE] = g_param_spec_boolean(
		"animate", "animate",
		"Whether or not to animate an animated avatar",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PidginAvatar:contact-info:
	 *
	 * The contact info whose avatar will be displayed.
	 */
	properties[PROP_CONTACT_INFO] = g_param_spec_object(
		"contact-info", "contact-info",
		"The contact info whose avatar to display",
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PidginAvatar:conversation:
	 *
	 * The conversation which will be used to find the correct avatar.
	 */
	properties[PROP_CONVERSATION] = g_param_spec_object(
		"conversation", "conversation",
		"The conversation used to find the correct avatar.",
		PURPLE_TYPE_CONVERSATION,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/avatar.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginAvatar, icon);
	gtk_widget_class_bind_template_child(widget_class, PidginAvatar, menu);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_avatar_button_press_handler);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_avatar_enter_notify_handler);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_avatar_leave_notify_handler);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_avatar_new(void) {
	return g_object_new(PIDGIN_TYPE_AVATAR, NULL);
}

void
pidgin_avatar_set_animate(PidginAvatar *avatar, gboolean animate) {
	g_return_if_fail(PIDGIN_IS_AVATAR(avatar));

	avatar->animate = animate;

	if(GDK_IS_PIXBUF_ANIMATION(avatar->animation)) {
		if(avatar->animate &&
		   !gdk_pixbuf_animation_is_static_image(avatar->animation)) {
			gtk_picture_set_pixbuf(GTK_PICTURE(avatar->icon),
			                       GDK_PIXBUF(avatar->animation));
		} else {
			GdkPixbuf *frame = NULL;

			frame = gdk_pixbuf_animation_get_static_image(avatar->animation);

			gtk_picture_set_pixbuf(GTK_PICTURE(avatar->icon), frame);
		}
	}
}

gboolean
pidgin_avatar_get_animate(PidginAvatar *avatar) {
	g_return_val_if_fail(PIDGIN_IS_AVATAR(avatar), FALSE);

	return avatar->animate;
}

void
pidgin_avatar_set_contact_info(PidginAvatar *avatar, PurpleContactInfo *info) {
	g_return_if_fail(PIDGIN_IS_AVATAR(avatar));

	/* Remove our old signal handler. */
	if(PURPLE_IS_CONTACT_INFO(avatar->info)) {
		g_signal_handlers_disconnect_by_func(avatar->info,
		                                     pidgin_avatar_contact_info_updated,
		                                     avatar);
	}

	if(g_set_object(&avatar->info, info)) {
		pidgin_avatar_update(avatar);

		g_object_notify_by_pspec(G_OBJECT(avatar),
		                         properties[PROP_CONTACT_INFO]);
	}

	/* Add the notify signal so we can update when the icon changes. */
	if(PURPLE_IS_CONTACT_INFO(avatar->info)) {
		g_signal_connect_object(G_OBJECT(avatar->info), "notify::avatar",
		                        G_CALLBACK(pidgin_avatar_contact_info_updated),
		                        avatar, 0);
	}
}

PurpleContactInfo *
pidgin_avatar_get_contact_info(PidginAvatar *avatar) {
	g_return_val_if_fail(PIDGIN_IS_AVATAR(avatar), NULL);

	return avatar->info;
}

void
pidgin_avatar_set_conversation(PidginAvatar *avatar,
                               PurpleConversation *conversation)
{
	g_return_if_fail(PIDGIN_IS_AVATAR(avatar));

	/* Remove our old signal handler. */
	if(PURPLE_IS_CONVERSATION(avatar->conversation)) {
		g_signal_handlers_disconnect_by_func(avatar->conversation,
		                                     pidgin_avatar_conversation_updated,
		                                     avatar);
	}

	if(g_set_object(&avatar->conversation, conversation)) {
		g_object_notify_by_pspec(G_OBJECT(avatar),
		                         properties[PROP_CONVERSATION]);
	}

	/* Add the notify signal so we can update when the icon changes. */
	if(PURPLE_IS_CONVERSATION(avatar->conversation)) {
		g_signal_connect_object(G_OBJECT(avatar->conversation), "notify",
		                        G_CALLBACK(pidgin_avatar_conversation_updated),
		                        avatar, 0);
	}
}

PurpleConversation *
pidgin_avatar_get_conversation(PidginAvatar *avatar) {
	g_return_val_if_fail(PIDGIN_IS_AVATAR(avatar), NULL);

	return avatar->conversation;
}
