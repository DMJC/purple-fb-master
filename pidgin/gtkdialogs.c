/* pidgin
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include <purpleconfig.h>

#include <glib/gi18n-lib.h>

#include <gst/gst.h>

#include <purple.h>

#include "gtkblist.h"
#include "gtkconv.h"
#include "gtkdialogs.h"
#include "gtkutils.h"
#include "pidgincore.h"

static void
pidgin_dialogs_im_cb(G_GNUC_UNUSED gpointer data, PurpleRequestPage *page) {
	PurpleAccount *account;
	const char *username;

	account = purple_request_page_get_account(page, "account");
	username = purple_request_page_get_string(page,  "screenname");

	pidgin_dialogs_im_with_user(account, username);
}

static gboolean
pidgin_dialogs_im_name_validator(PurpleRequestField *field, char **errmsg,
                                 gpointer data)
{
	PurpleRequestFieldAccount *account_field = data;
	PurpleAccount *account;
	PurpleProtocol *protocol;
	const char *username;
	gboolean valid = FALSE;

	account = purple_request_field_account_get_value(account_field);
	protocol = purple_account_get_protocol(account);
	username = purple_request_field_string_get_value(PURPLE_REQUEST_FIELD_STRING(field));

	if (username) {
		valid = purple_validate(protocol, username);
	}

	if (errmsg && !valid)
		*errmsg = g_strdup(_("Invalid username"));

	return valid;
}

void
pidgin_dialogs_im(void)
{
	PurpleRequestPage *page;
	PurpleRequestGroup *group;
	PurpleRequestField *username_field = NULL;
	PurpleRequestField *account_field = NULL;

	page = purple_request_page_new();

	group = purple_request_group_new(NULL);
	purple_request_page_add_group(page, group);

	account_field = purple_request_field_account_new("account", _("_Account"),
	                                                 NULL);
	purple_request_field_set_type_hint(account_field, "account");
	purple_request_field_set_visible(account_field,
		(purple_connections_get_all() != NULL &&
		 purple_connections_get_all()->next != NULL));
	purple_request_field_set_required(account_field, TRUE);
	purple_request_group_add_field(group, account_field);

	purple_request_field_set_validator(username_field,
	                                   pidgin_dialogs_im_name_validator,
	                                   account_field, NULL);

	username_field = purple_request_field_string_new("screenname", _("_Name"),
	                                                 NULL, FALSE);
	purple_request_field_set_type_hint(username_field, "screenname");
	purple_request_field_set_required(username_field, TRUE);
	purple_request_group_add_field(group, username_field);

	purple_request_fields(
	        purple_blist_get_default(), _("New Instant Message"), NULL,
	        _("Please enter the username or alias of the person "
	          "you would like to IM."),
	        page, _("OK"), G_CALLBACK(pidgin_dialogs_im_cb), _("Cancel"),
	        NULL, NULL, NULL);
}

void
pidgin_dialogs_im_with_user(PurpleAccount *account, const char *username)
{
	PurpleConversation *im;
	PurpleConversationManager *manager;

	g_return_if_fail(account != NULL);
	g_return_if_fail(username != NULL);

	manager = purple_conversation_manager_get_default();
	im = purple_conversation_manager_find_im(manager, account, username);

	if(!PURPLE_IS_IM_CONVERSATION(im)) {
		im = purple_im_conversation_new(account, username);
	}

	pidgin_conv_attach_to_conversation(im);
	purple_conversation_present(im);
}

static void
pidgin_dialogs_info_cb(G_GNUC_UNUSED gpointer data, PurpleRequestPage *page) {
	char *username;
	PurpleAccount *account;
	const gchar *screenname = NULL;

	account = purple_request_page_get_account(page, "account");

	screenname = purple_request_page_get_string(page, "screenname");
	username = g_strdup(purple_normalize(account, screenname));

	if(username != NULL && *username != '\0' && account != NULL) {
		pidgin_retrieve_user_info(purple_account_get_connection(account),
		                          username);
	}

	g_free(username);
}

void
pidgin_dialogs_info(void)
{
	PurpleRequestPage *page;
	PurpleRequestGroup *group;
	PurpleRequestField *field;

	page = purple_request_page_new();

	group = purple_request_group_new(NULL);
	purple_request_page_add_group(page, group);

	field = purple_request_field_account_new("account", _("_Account"), NULL);
	purple_request_field_set_type_hint(field, "account");
	purple_request_field_set_visible(field,
		(purple_connections_get_all() != NULL &&
		 purple_connections_get_all()->next != NULL));
	purple_request_field_set_required(field, TRUE);
	purple_request_group_add_field(group, field);

	field = purple_request_field_string_new("screenname", _("_Name"), NULL, FALSE);
	purple_request_field_set_type_hint(field, "screenname");
	purple_request_field_set_required(field, TRUE);
	purple_request_group_add_field(group, field);

	purple_request_fields(
	        purple_blist_get_default(), _("Get User Info"), NULL,
	        _("Please enter the username or alias of the person "
	          "whose info you would like to view."),
	        page, _("OK"), G_CALLBACK(pidgin_dialogs_info_cb),
	        _("Cancel"), NULL, NULL, NULL);
}

static void
pidgin_dialogs_alias_buddy_cb(PurpleBuddy *buddy, const char *new_alias)
{
	purple_buddy_set_local_alias(buddy, new_alias);
	purple_serv_alias_buddy(buddy);
}

void
pidgin_dialogs_alias_buddy(PurpleBuddy *buddy)
{
	gchar *secondary;

	g_return_if_fail(buddy != NULL);

	secondary = g_strdup_printf(_("Enter an alias for %s."), purple_buddy_get_name(buddy));

	purple_request_input(NULL, _("Alias Buddy"), NULL,
					   secondary, purple_buddy_get_local_alias(buddy), FALSE, FALSE, NULL,
					   _("Alias"), G_CALLBACK(pidgin_dialogs_alias_buddy_cb),
					   _("Cancel"), NULL,
					   purple_request_cpar_from_account(purple_buddy_get_account(buddy)),
					   buddy);

	g_free(secondary);
}

static void
pidgin_dialogs_alias_chat_cb(PurpleChat *chat, const char *new_alias)
{
	purple_chat_set_alias(chat, new_alias);
}

void
pidgin_dialogs_alias_chat(PurpleChat *chat)
{
	gchar *alias;

	g_return_if_fail(chat != NULL);

	g_object_get(chat, "alias", &alias, NULL);

	purple_request_input(NULL, _("Alias Chat"), NULL,
					   _("Enter an alias for this chat."),
					   alias, FALSE, FALSE, NULL,
					   _("Alias"), G_CALLBACK(pidgin_dialogs_alias_chat_cb),
					   _("Cancel"), NULL,
					   purple_request_cpar_from_account(purple_chat_get_account(chat)),
					   chat);

	g_free(alias);
}

