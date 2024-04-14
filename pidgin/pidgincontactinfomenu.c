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

#include "pidgincontactinfomenu.h"

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
pidgin_contact_info_menu_parse_parameter(GVariant *parameter,
                                         PurpleAccount **account,
                                         PurpleContact **contact)
{
	PurpleAccount *local_account = NULL;
	PurpleAccountManager *account_manager = NULL;
	PurpleContact *local_contact = NULL;
	PurpleContactManager *contact_manager = NULL;
	char *account_id = NULL;
	char *contact_id = NULL;

	g_variant_get(parameter, "(ss)", &account_id, &contact_id);

	/* Find the account. */
	account_manager = purple_account_manager_get_default();
	local_account = purple_account_manager_find_by_id(account_manager,
	                                                  account_id);
	if(!PURPLE_IS_ACCOUNT(local_account)) {
		g_clear_pointer(&account_id, g_free);
		g_clear_pointer(&contact_id, g_free);

		return FALSE;
	}

	if(account != NULL) {
		*account = local_account;
	}

	/* Find the contact. */
	contact_manager = purple_contact_manager_get_default();
	local_contact = purple_contact_manager_find_with_id(contact_manager,
	                                                    local_account,
	                                                    contact_id);
	if(!PURPLE_IS_CONTACT(local_contact)) {
		g_clear_pointer(&account_id, g_free);
		g_clear_pointer(&contact_id, g_free);

		return FALSE;
	}

	if(contact != NULL) {
		*contact = local_contact;
	}

	g_clear_pointer(&account_id, g_free);
	g_clear_pointer(&contact_id, g_free);

	return TRUE;
}

static GMenuModel *
pidgin_contact_info_menu_get_starting_items(PurpleContactInfo *info,
                                            PurpleAccount *account)
{
	GMenu *menu = NULL;
	GMenuItem *item = NULL;
	const char *account_id = NULL;
	const char *contact_id = NULL;

	account_id = purple_contact_info_get_id(PURPLE_CONTACT_INFO(account));
	contact_id = purple_contact_info_get_id(info);

	menu = g_menu_new();

	item = g_menu_item_new(_("View Profile"), NULL);
	g_menu_item_set_action_and_target(item, "contact.profile", "(ss)",
	                                  account_id, contact_id);
	g_menu_append_item(menu, item);
	g_clear_object(&item);

	item = g_menu_item_new(_("Message"), NULL);
	g_menu_item_set_action_and_target(item, "contact.message", "(ss)",
	                                  account_id, contact_id);
	g_menu_append_item(menu, item);
	g_clear_object(&item);

	g_menu_append(menu, _("Send File..."), "contact.send-file");

	return G_MENU_MODEL(menu);
}

static void
pidgin_contact_info_menu_disable_items(GActionMap *map,
                                       PurpleAccount *account)
{
	PurpleProtocol *protocol = NULL;
	GAction *action = NULL;
	gboolean enabled = TRUE;

	protocol = purple_account_get_protocol(account);
	if(!PURPLE_IS_PROTOCOL_CONTACTS(protocol)) {
		enabled = FALSE;
	} else {
		enabled = PURPLE_PROTOCOL_IMPLEMENTS(protocol, CONTACTS,
		                                     get_profile_async);
	}

	action = g_action_map_lookup_action(map, "profile");
	g_simple_action_set_enabled(G_SIMPLE_ACTION(action), enabled);
}

/******************************************************************************
 * Contact Actions
 *****************************************************************************/
static void
pidgin_contact_info_menu_get_info_activate_cb(GObject *contacts,
                                              GAsyncResult *result,
                                              gpointer data)
{
	PurpleContact *contact = data;
	GError *error = NULL;
	char *profile = NULL;

	profile = purple_protocol_contacts_get_profile_finish(PURPLE_PROTOCOL_CONTACTS(contacts),
	                                                      result, &error);

	if(error != NULL) {
		g_warning("failed to get profile for '%s': %s",
		          purple_contact_info_get_id(PURPLE_CONTACT_INFO(contact)),
		          error->message);

		g_clear_error(&error);

		return;
	}

	g_message("profile data for '%s': %s",
	          purple_contact_info_get_id(PURPLE_CONTACT_INFO(contact)),
	          profile);

	g_clear_object(&contact);
}

static void
pidgin_contact_info_menu_get_info_activate(G_GNUC_UNUSED GSimpleAction *action,
                                           GVariant *parameter,
                                           G_GNUC_UNUSED gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleProtocol *protocol = NULL;

	g_return_if_fail(g_variant_is_of_type(parameter, (GVariantType*)"(ss)"));

	if(!pidgin_contact_info_menu_parse_parameter(parameter, &account, &contact)) {
		g_clear_object(&account);
		g_clear_object(&contact);

		g_warning("failed to parse parameter");

		return;
	}

	protocol = purple_account_get_protocol(account);
	if(PURPLE_IS_PROTOCOL_CONTACTS(protocol)) {
		if(PURPLE_PROTOCOL_IMPLEMENTS(protocol, CONTACTS, get_profile_async)) {
			purple_protocol_contacts_get_profile_async(PURPLE_PROTOCOL_CONTACTS(protocol),
			                                           PURPLE_CONTACT_INFO(contact),
			                                           NULL,
			                                           pidgin_contact_info_menu_get_info_activate_cb,
			                                           g_object_ref(contact));
		}
	}

	g_clear_object(&account);
	g_clear_object(&contact);
}

static void
pidgin_contact_info_menu_message_activate(G_GNUC_UNUSED GSimpleAction *action,
                                          GVariant *parameter,
                                          G_GNUC_UNUSED gpointer data)
{
	PurpleContact *contact = NULL;
	PurpleConversation *conversation = NULL;

	g_return_if_fail(g_variant_is_of_type(parameter, (GVariantType*)"(ss)"));

	if(!pidgin_contact_info_menu_parse_parameter(parameter, NULL, &contact)) {
		g_clear_object(&contact);

		g_warning("failed to parse parameter");

		return;
	}

	if(!PURPLE_IS_CONTACT(contact)) {
		g_warning("failed to find contact");

		return;
	}

	conversation = purple_contact_find_dm(contact);
	if(PURPLE_IS_CONVERSATION(conversation)) {
		purple_conversation_present(conversation);
	} else {
		purple_contact_create_dm_async(contact, NULL, NULL, NULL);
	}

	g_clear_object(&contact);
}

static GActionGroup *
pidgin_contact_info_menu_get_contact_actions(void) {
	GSimpleActionGroup *action_group = NULL;
	GActionEntry entries[] = {
		{
			.name = "profile",
			.activate = pidgin_contact_info_menu_get_info_activate,
			.parameter_type = "(ss)",

		}, {
			.name = "message",
			.activate = pidgin_contact_info_menu_message_activate,
			.parameter_type = "(ss)",
		},
	};

	action_group = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(action_group), entries,
	                                G_N_ELEMENTS(entries), NULL);

	return G_ACTION_GROUP(action_group);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
void
pidgin_contact_info_menu_popup(PurpleContactInfo *info, PurpleAccount *account,
                               GtkWidget *parent, gdouble x, gdouble y)
{
	GtkWidget *popover = NULL;
	GActionGroup *action_group = NULL;
	GMenu *menu = NULL;
	GMenuModel *section = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT_INFO(info));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(GTK_IS_WIDGET(parent));

	/* Add actions to the parent. */
	action_group = pidgin_contact_info_menu_get_contact_actions();
	gtk_widget_insert_action_group(parent, "contact", action_group);

	/* Disable any actions that aren't currently supported. */
	pidgin_contact_info_menu_disable_items(G_ACTION_MAP(action_group),
	                                       account);

	/* Create the menu. */
	menu = g_menu_new();

	section = pidgin_contact_info_menu_get_starting_items(info, account);
	g_menu_append_section(menu, NULL, section);
	g_clear_object(&section);

	/* Create the popover and pop it up! */
	popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
	gtk_widget_set_parent(popover, parent);
	gtk_popover_set_pointing_to(GTK_POPOVER(popover),
	                            &(const GdkRectangle){x, y, 1, 1});
	gtk_popover_popup(GTK_POPOVER(popover));

	g_clear_object(&menu);
}
