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

#include "pidgin/pidgincontactlist.h"

struct _PidginContactList {
	AdwBin parent;

	GtkWidget *view;
};

G_DEFINE_TYPE(PidginContactList, pidgin_contact_list, ADW_TYPE_BIN)

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static GdkTexture *
pidgin_contact_list_avatar_cb(G_GNUC_UNUSED GObject *self,
                              PurplePerson *person,
                              G_GNUC_UNUSED gpointer data)
{
	PurpleContactInfo *info = NULL;
	PurpleContact *contact = NULL;
	PurpleBuddyIcon *icon = NULL;
	GdkTexture *texture = NULL;

	info = purple_person_get_priority_contact_info(person);

	/* All of the contact info in the manager are PurpleContact's so this cast
	 * is fine.
	 */
	contact = PURPLE_CONTACT(info);

	icon = purple_buddy_icons_find(purple_contact_get_account(contact),
	                               purple_contact_info_get_username(info));

	if(icon != NULL) {
		GBytes *bytes = NULL;
		GError *error = NULL;
		gsize size;
		gconstpointer data = NULL;

		data = purple_buddy_icon_get_data(icon, &size);
		bytes = g_bytes_new(data, size);

		texture = gdk_texture_new_from_bytes(bytes, &error);
		g_bytes_unref(bytes);

		if(error != NULL) {
			g_warning("Failed to create texture: %s", error->message);

			g_clear_error(&error);
		}
	}

	return texture;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_contact_list_init(PidginContactList *list) {
	PurpleContactManager *manager = NULL;
	GtkSingleSelection *selection = NULL;

	gtk_widget_init_template(GTK_WIDGET(list));

	manager = purple_contact_manager_get_default();
	selection = gtk_single_selection_new(G_LIST_MODEL(manager));

	gtk_list_view_set_model(GTK_LIST_VIEW(list->view),
	                        GTK_SELECTION_MODEL(selection));
}

static void
pidgin_contact_list_class_init(PidginContactListClass *klass) {
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/ContactList/widget.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginContactList,
	                                     view);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_contact_list_avatar_cb);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_contact_list_new(void) {
	return g_object_new(PIDGIN_TYPE_CONTACT_LIST, NULL);
}
