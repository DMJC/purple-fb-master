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

#include "gtkdialogs.h"
#include "gtkutils.h"
#include "pidgincore.h"

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
		/* This constructor automagically registers the conversation with the
		 * manager.
		 */
		purple_im_conversation_new(account, username);
	}
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
