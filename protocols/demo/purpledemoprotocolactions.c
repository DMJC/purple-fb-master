/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpledemoprotocol.h"
#include "purpledemoprotocolactions.h"
#include "purpledemoresource.h"

/******************************************************************************
 * Connection failure action implementations
 *****************************************************************************/
#define REAPER_BUDDY_NAME ("Gary")
#define DEFAULT_REAP_TIME (5)  /* seconds */
#define FATAL_TICK_STR N_("Reaping connection in %d second...")
#define FATAL_TICK_PLURAL_STR N_("Reaping connection in %d seconds...")
#define FATAL_DISCONNECT_STR N_("%s reaped the connection")
#define TEMPORARY_TICK_STR N_("Pruning connection in %d second...")
#define TEMPORARY_TICK_PLURAL_STR N_("Pruning connection in %d seconds...")
#define TEMPORARY_DISCONNECT_STR N_("%s pruned the connection")

static gboolean
purple_demo_protocol_failure_tick(gpointer data,
                                  PurpleConnectionError error_code,
                                  const gchar *tick_str,
                                  const gchar *tick_plural_str,
                                  const gchar *disconnect_str)
{
	PurpleConnection *connection = PURPLE_CONNECTION(data);
	PurpleAccount *account = purple_connection_get_account(connection);
	gchar *message = NULL;
	gint timeout = 0;

	timeout = GPOINTER_TO_INT(g_object_steal_data(G_OBJECT(connection),
	                                              "reaping-time"));
	timeout--;
	if(timeout > 0) {
		PurpleContact *contact = NULL;
		PurpleContactManager *manager = NULL;

		g_object_set_data(G_OBJECT(connection), "reaping-time",
		                  GINT_TO_POINTER(timeout));

		manager = purple_contact_manager_get_default();
		contact = purple_contact_manager_find_with_username(manager, account,
		                                                    REAPER_BUDDY_NAME);

		if(PURPLE_IS_CONTACT(contact)) {
			PurpleContactInfo *info = PURPLE_CONTACT_INFO(contact);
			PurplePresence *presence = purple_contact_info_get_presence(info);
			const char *format = ngettext(tick_str, tick_plural_str, timeout);

			message = g_strdup_printf(format, timeout);
			purple_presence_set_message(presence, message);
			g_free(message);
		}

		return G_SOURCE_CONTINUE;
	}

	message = g_strdup_printf(_(disconnect_str), REAPER_BUDDY_NAME);
	purple_connection_error(connection, error_code, message);
	g_free(message);

	return G_SOURCE_REMOVE;
}

static gboolean
purple_demo_protocol_fatal_failure_cb(gpointer data) {
	return purple_demo_protocol_failure_tick(data,
	                                         PURPLE_CONNECTION_ERROR_CUSTOM_FATAL,
	                                         FATAL_TICK_STR,
	                                         FATAL_TICK_PLURAL_STR,
	                                         FATAL_DISCONNECT_STR);
}

static gboolean
purple_demo_protocol_temporary_failure_cb(gpointer data) {
	return purple_demo_protocol_failure_tick(data,
	                                         PURPLE_CONNECTION_ERROR_CUSTOM_TEMPORARY,
	                                         TEMPORARY_TICK_STR,
	                                         TEMPORARY_TICK_PLURAL_STR,
	                                         TEMPORARY_DISCONNECT_STR);
}

static void
purple_demo_protocol_failure_action_activate(G_GNUC_UNUSED GSimpleAction *action,
                                             GVariant *parameter,
                                             const gchar *tick_str,
                                             const gchar *tick_plural_str,
                                             GSourceFunc cb)
{
	PurpleAccountManager *account_manager = NULL;
	PurpleAccount *account = NULL;
	PurpleConnection *connection = NULL;
	PurpleContact *contact = NULL;
	PurpleContactManager *contact_manager = NULL;
	const char *account_id = NULL;

	if(!g_variant_is_of_type(parameter, G_VARIANT_TYPE_STRING)) {
		g_critical("Demo failure action parameter is of incorrect type %s",
		           g_variant_get_type_string(parameter));
		return;
	}

	account_id = g_variant_get_string(parameter, NULL);
	account_manager = purple_account_manager_get_default();
	account = purple_account_manager_find_by_id(account_manager, account_id);
	connection = purple_account_get_connection(account);

	/* Do nothing if disconnected, or already in process of reaping. */
	if(!PURPLE_IS_CONNECTION(connection)) {
		g_clear_object(&account);

		return;
	}

	if(g_object_get_data(G_OBJECT(connection), "reaping-time")) {
		g_clear_object(&account);

		return;
	}

	/* Find the reaper. */
	contact_manager = purple_contact_manager_get_default();
	contact = purple_contact_manager_find_with_username(contact_manager,
	                                                    account,
	                                                    REAPER_BUDDY_NAME);

	if(PURPLE_IS_CONTACT(contact)) {
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(contact);
		PurplePresence *presence = purple_contact_info_get_presence(info);
		const char *format = NULL;
		char *message = NULL;

		format = ngettext(tick_str, tick_plural_str, DEFAULT_REAP_TIME);
		message = g_strdup_printf(format, DEFAULT_REAP_TIME);

		purple_presence_set_idle(presence, FALSE);
		purple_presence_set_message(presence, message);
		g_free(message);
	}

	g_object_set_data(G_OBJECT(connection), "reaping-time",
	                  GINT_TO_POINTER(DEFAULT_REAP_TIME));
	g_timeout_add_seconds(1, cb, connection);

	g_clear_object(&account);
}

static void
purple_demo_protocol_temporary_failure_action_activate(GSimpleAction *action,
                                                       GVariant *parameter,
                                                       G_GNUC_UNUSED gpointer data)
{
	purple_demo_protocol_failure_action_activate(action, parameter,
	                                             TEMPORARY_TICK_STR,
	                                             TEMPORARY_TICK_PLURAL_STR,
	                                             purple_demo_protocol_temporary_failure_cb);
}

static void
purple_demo_protocol_fatal_failure_action_activate(GSimpleAction *action,
                                                   GVariant *parameter,
                                                   G_GNUC_UNUSED gpointer data)
{
	purple_demo_protocol_failure_action_activate(action, parameter,
	                                             FATAL_TICK_STR,
	                                             FATAL_TICK_PLURAL_STR,
	                                             purple_demo_protocol_fatal_failure_cb);
}

/******************************************************************************
 * Request API action implementations
 *****************************************************************************/

static void
purple_demo_protocol_request_fields_ok_cb(G_GNUC_UNUSED gpointer data,
                                          PurpleRequestPage *page)
{
	PurpleAccount *account = NULL;
	PurpleRequestFieldList *field = NULL;
	GList *list = NULL;
	const char *tmp = NULL;
	GString *info = NULL;

	info = g_string_new(_("Basic group:\n"));

	g_string_append_printf(info, _("\tString: %s\n"),
	                       purple_request_page_get_string(page, "string"));
	g_string_append_printf(info, _("\tMultiline string: %s\n"),
	                       purple_request_page_get_string(page,
	                                                      "multiline-string"));
	g_string_append_printf(info, _("\tMasked string: %s\n"),
	                       purple_request_page_get_string(page,
	                                                      "masked-string"));
	g_string_append_printf(info, _("\tAlphanumeric string: %s\n"),
	                       purple_request_page_get_string(page,
	                                                      "alphanumeric"));
	g_string_append_printf(info, _("\tEmail string: %s\n"),
	                       purple_request_page_get_string(page, "email"));
	g_string_append_printf(info, _("\tInteger: %d\n"),
	                       purple_request_page_get_integer(page, "int"));
	g_string_append_printf(info, _("\tBoolean: %s\n"),
	                       purple_request_page_get_bool(page, "bool") ?
	                       _("TRUE") : _("FALSE"));

	g_string_append(info, _("Multiple-choice group:\n"));

	tmp = (const char *)purple_request_page_get_choice(page, "choice");
	g_string_append_printf(info, _("\tChoice: %s\n"), tmp);

	field = PURPLE_REQUEST_FIELD_LIST(purple_request_page_get_field(page,
	                                                                "list"));
	list = purple_request_field_list_get_selected(field);
	if(list != NULL) {
		tmp = (const char *)list->data;
	} else {
		tmp = _("(unset)");
	}
	g_string_append_printf(info, _("\tList: %s\n"), tmp);

	field = PURPLE_REQUEST_FIELD_LIST(purple_request_page_get_field(page,
	                                                                "multilist"));
	list = purple_request_field_list_get_selected(field);
	g_string_append(info, _("\tMulti-list: ["));
	while(list != NULL) {
		tmp = (const char *)list->data;
		g_string_append_printf(info, "%s%s", tmp,
		                       list->next != NULL ? ", " : "");
		list = list->next;
	}
	g_string_append(info, _("]\n"));

	g_string_append(info, _("Special group:\n"));

	account = purple_request_page_get_account(page, "account");
	if(PURPLE_IS_ACCOUNT(account)) {
		PurpleContactInfo *info = NULL;

		info = purple_account_get_contact_info(account);
		tmp = purple_contact_info_get_name_for_display(info);
	} else {
		tmp = _("(unset)");
	}
	g_string_append_printf(info, _("\tAccount: %s\n"), tmp);

	g_message(_("Successfully requested fields:\n%s"), info->str);

	g_string_free(info, TRUE);
}

static void
purple_demo_protocol_request_fields_cancel_cb(G_GNUC_UNUSED gpointer data,
                                              G_GNUC_UNUSED PurpleRequestPage *page)
{
	g_message(_("UI cancelled field request"));
}

static void
purple_demo_protocol_request_fields_activate(G_GNUC_UNUSED GSimpleAction *action,
                                             GVariant *parameter,
                                             G_GNUC_UNUSED gpointer data)
{
	PurpleConnection *connection = NULL;
	const gchar *account_id = NULL;
	PurpleAccountManager *manager = NULL;
	PurpleAccount *account = NULL;
	PurpleRequestPage *page = NULL;
	PurpleRequestGroup *group = NULL;
	PurpleRequestField *boolfield = NULL;
	PurpleRequestField *field = NULL;
	PurpleRequestFieldChoice *choice_field = NULL;
	PurpleRequestFieldList *list_field = NULL;
	GBytes *icon = NULL;
	gconstpointer icon_data = NULL;
	gsize icon_len = 0;

	if(!g_variant_is_of_type(parameter, G_VARIANT_TYPE_STRING)) {
		g_critical("Demo failure action parameter is of incorrect type %s",
		           g_variant_get_type_string(parameter));
		return;
	}

	account_id = g_variant_get_string(parameter, NULL);
	manager = purple_account_manager_get_default();
	account = purple_account_manager_find_by_id(manager, account_id);
	connection = purple_account_get_connection(account);

	page = purple_request_page_new();

	/* This group will contain basic fields. */
	group = purple_request_group_new(_("Basic"));
	purple_request_page_add_group(page, group);

	boolfield = purple_request_field_bool_new("bool", _("Sensitive?"), TRUE);
	purple_request_field_set_tooltip(boolfield,
	                                 _("Allow modifying all fields."));
	purple_request_group_add_field(group, boolfield);

	field = purple_request_field_label_new("basic-label",
	                                       _("This group contains basic fields"));
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	field = purple_request_field_string_new("string", _("A string"),
	                                        _("default"), FALSE);
	purple_request_field_set_required(field, TRUE);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);
	field = purple_request_field_string_new("multiline-string",
	                                        _("A multiline string"),
	                                        _("default"), TRUE);
	purple_request_group_add_field(group, field);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	field = purple_request_field_string_new("masked-string",
	                                        _("A masked string"), _("default"),
	                                        FALSE);
	purple_request_field_string_set_masked(PURPLE_REQUEST_FIELD_STRING(field),
	                                       TRUE);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);
	field = purple_request_field_string_new("alphanumeric",
	                                        _("An alphanumeric string"),
	                                        _("default"), FALSE);
	purple_request_field_set_validator(field,
	                                   purple_request_field_alphanumeric_validator,
	                                   NULL, NULL);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);
	field = purple_request_field_string_new("email", _("An email"),
	                                        _("me@example.com"), FALSE);
	purple_request_field_set_validator(field,
	                                   purple_request_field_email_validator,
	                                   NULL, NULL);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);
	field = purple_request_field_int_new("int", _("An integer"), 123, -42, 1337);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	/* This group will contain fields with multiple options. */
	group = purple_request_group_new(_("Multiple"));
	purple_request_page_add_group(page, group);

	field = purple_request_field_label_new("multiple-label",
	                                       _("This group contains fields with multiple options"));
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	field = purple_request_field_choice_new("choice", _("A choice"), "foo");
	choice_field = PURPLE_REQUEST_FIELD_CHOICE(field);
	purple_request_field_choice_add(choice_field, _("foo"), "foo");
	purple_request_field_choice_add(choice_field, _("bar"), "bar");
	purple_request_field_choice_add(choice_field, _("baz"), "baz");
	purple_request_field_choice_add(choice_field, _("quux"), "quux");
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	field = purple_request_field_list_new("list", _("A list"));
	list_field = PURPLE_REQUEST_FIELD_LIST(field);
	purple_request_field_list_add_icon(list_field, _("foo"), NULL, "foo");
	purple_request_field_list_add_icon(list_field, _("bar"), NULL, "bar");
	purple_request_field_list_add_icon(list_field, _("baz"), NULL, "baz");
	purple_request_field_list_add_icon(list_field, _("quux"), NULL, "quux");
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	field = purple_request_field_list_new("multilist", _("A multi-select list"));
	list_field = PURPLE_REQUEST_FIELD_LIST(field);
	purple_request_field_list_set_multi_select(list_field, TRUE);
	purple_request_field_list_add_icon(list_field, _("foo"), NULL, "foo");
	purple_request_field_list_add_icon(list_field, _("bar"), NULL, "bar");
	purple_request_field_list_add_icon(list_field, _("baz"), NULL, "baz");
	purple_request_field_list_add_icon(list_field, _("quux"), NULL, "quux");
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	/* This group will contain specialized fields. */
	group = purple_request_group_new(_("Special"));
	purple_request_page_add_group(page, group);

	field = purple_request_field_label_new("special-label",
	                                       _("This group contains specialized fields"));
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	icon = g_resource_lookup_data(purple_demo_get_resource(),
	                              "/im/pidgin/purple/demo/icons/scalable/apps/im-purple-demo.svg",
	                              G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	icon_data = g_bytes_get_data(icon, &icon_len);
	field = purple_request_field_image_new("image", _("An image"),
	                                       icon_data, icon_len);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);
	g_bytes_unref(icon);

	field = purple_request_field_account_new("account", _("An account"),
	                                         account);
	g_object_bind_property(boolfield, "value", field, "sensitive", 0);
	purple_request_group_add_field(group, field);

	purple_request_fields(connection, _("Request Fields Demo"),
	                      _("Please fill out these fields…"), NULL, page,
	                      _("OK"),
	                      G_CALLBACK(purple_demo_protocol_request_fields_ok_cb),
	                      _("Cancel"),
	                      G_CALLBACK(purple_demo_protocol_request_fields_cancel_cb),
	                      purple_request_cpar_from_connection(connection),
	                      NULL);

	g_clear_object(&account);
}

/******************************************************************************
 * Contact action implementations
 *****************************************************************************/
static const gchar *contacts[] = {"Alice", "Bob", "Carlos", "Erin" };

static void
purple_demo_protocol_remote_add(G_GNUC_UNUSED GSimpleAction *action,
                                GVariant *parameter,
                                G_GNUC_UNUSED gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleAccountManager *account_manager = NULL;
	PurpleAddContactRequest *request = NULL;
	PurpleNotification *notification = NULL;
	PurpleNotificationManager *notification_manager = NULL;
	const gchar *account_id = NULL;
	static guint counter = 0;

	account_id = g_variant_get_string(parameter, NULL);
	account_manager = purple_account_manager_get_default();
	account = purple_account_manager_find_by_id(account_manager, account_id);

	request = purple_add_contact_request_new(account, contacts[counter]);
	notification = purple_notification_new_from_add_contact_request(request);

	notification_manager = purple_notification_manager_get_default();
	purple_notification_manager_add(notification_manager, notification);

	counter++;
	if(counter >= G_N_ELEMENTS(contacts)) {
		counter = 0;
	}

	g_clear_object(&notification);
	g_clear_object(&account);
}

static const char *puns[] = {
	"Toucan play at that game!",
	"As long as it's not too much of a birden...",
	"Sounds like a bit of ostrich...",
	"People can't stop raven!",
	"Have you heard about the bird?",
};

static void
purple_demo_protocol_generic_notification(G_GNUC_UNUSED GSimpleAction *action,
                                          GVariant *parameter,
                                          G_GNUC_UNUSED gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleAccountManager *account_manager = NULL;
	PurpleNotification *notification = NULL;
	PurpleNotificationManager *notification_manager = NULL;
	const char *account_id = NULL;
	static guint counter = 0;

	account_id = g_variant_get_string(parameter, NULL);
	account_manager = purple_account_manager_get_default();
	account = purple_account_manager_find_by_id(account_manager, account_id);

	notification = purple_notification_new(PURPLE_NOTIFICATION_TYPE_GENERIC,
	                                       account, NULL, NULL);
	purple_notification_set_title(notification, puns[counter]);
	purple_notification_set_icon_name(notification, "im-purple-demo");

	notification_manager = purple_notification_manager_get_default();
	purple_notification_manager_add(notification_manager, notification);

	counter++;
	if(counter >= G_N_ELEMENTS(puns)) {
		counter = 0;
	}

	g_clear_object(&notification);
	g_clear_object(&account);
}

/******************************************************************************
 * PurpleProtocolActions Implementation
 *****************************************************************************/
static const gchar *
purple_demo_protocol_get_prefix(G_GNUC_UNUSED PurpleProtocolActions *actions) {
	return "prpl-demo";
}

static GActionGroup *
purple_demo_protocol_get_action_group(G_GNUC_UNUSED PurpleProtocolActions *actions,
                                      G_GNUC_UNUSED PurpleConnection *connection)
{
	GSimpleActionGroup *group = NULL;
	GActionEntry entries[] = {
		{
			.name = "temporary-failure",
			.activate = purple_demo_protocol_temporary_failure_action_activate,
			.parameter_type = "s",
		}, {
			.name = "fatal-failure",
			.activate = purple_demo_protocol_fatal_failure_action_activate,
			.parameter_type = "s",
		}, {
			.name = "generic-notification",
			.activate = purple_demo_protocol_generic_notification,
			.parameter_type = "s",
		}, {
			.name = "remote-add",
			.activate = purple_demo_protocol_remote_add,
			.parameter_type = "s",
		}, {
			.name = "request-fields",
			.activate = purple_demo_protocol_request_fields_activate,
			.parameter_type = "s",
		}
	};
	gsize nentries = G_N_ELEMENTS(entries);

	group = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(group), entries, nentries,
	                                NULL);

	return G_ACTION_GROUP(group);
}

static GMenu *
purple_demo_protocol_get_menu(G_GNUC_UNUSED PurpleProtocolActions *actions,
                              G_GNUC_UNUSED PurpleConnection *connection)
{
	GMenu *menu = NULL;
	GMenuItem *item = NULL;

	menu = g_menu_new();

	item = g_menu_item_new(_("Trigger temporary connection failure..."),
	                       "prpl-demo.temporary-failure");
	g_menu_item_set_attribute(item, PURPLE_MENU_ATTRIBUTE_DYNAMIC_TARGET, "s",
	                          "account");
	g_menu_append_item(menu, item);
	g_object_unref(item);

	item = g_menu_item_new(_("Trigger fatal connection failure..."),
	                       "prpl-demo.fatal-failure");
	g_menu_item_set_attribute(item, PURPLE_MENU_ATTRIBUTE_DYNAMIC_TARGET, "s",
	                          "account");
	g_menu_append_item(menu, item);
	g_object_unref(item);

	item = g_menu_item_new(_("Trigger a generic notification"),
	                       "prpl-demo.generic-notification");
	g_menu_item_set_attribute(item, PURPLE_MENU_ATTRIBUTE_DYNAMIC_TARGET, "s",
	                          "account");
	g_menu_append_item(menu, item);
	g_object_unref(item);

	item = g_menu_item_new(_("Trigger a contact adding you"),
	                       "prpl-demo.remote-add");
	g_menu_item_set_attribute(item, PURPLE_MENU_ATTRIBUTE_DYNAMIC_TARGET, "s",
	                          "account");
	g_menu_append_item(menu, item);
	g_object_unref(item);

	item = g_menu_item_new(_("Request Fields"), "prpl-demo.request-fields");
	g_menu_item_set_attribute(item, PURPLE_MENU_ATTRIBUTE_DYNAMIC_TARGET, "s",
	                          "account");
	g_menu_append_item(menu, item);
	g_object_unref(item);

	return menu;
}

void
purple_demo_protocol_actions_init(PurpleProtocolActionsInterface *iface) {
	iface->get_prefix = purple_demo_protocol_get_prefix;
	iface->get_action_group = purple_demo_protocol_get_action_group;
	iface->get_menu = purple_demo_protocol_get_menu;
}

BirbActionMenu *
purple_demo_protocol_get_action_menu(G_GNUC_UNUSED PurpleProtocol *protocol,
                                     G_GNUC_UNUSED PurpleAccount *account)
{
	BirbActionMenu *action_menu = NULL;
	GActionGroup *group = NULL;
	GMenu *menu = NULL;
	GMenu *section = NULL;

	action_menu = birb_action_menu_new();

	group = purple_demo_protocol_get_action_group(NULL, NULL);
	birb_action_menu_add_action_group(action_menu, "prpl-demo", group);
	g_clear_object(&group);

	menu = birb_action_menu_get_menu(action_menu);

	section = purple_demo_protocol_get_menu(NULL, NULL);
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_clear_object(&section);

	return action_menu;
}
