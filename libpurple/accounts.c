/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "accounts.h"
#include "core.h"
#include "debug.h"
#include "network.h"
#include "prefs.h"
#include "purpleaccountmanager.h"
#include "purpleconversationmanager.h"
#include "purplecredentialmanager.h"
#include "purpleenums.h"
#include "purpleprivate.h"
#include "signals.h"
#include "util.h"

static guint    save_timer = 0;
static gboolean accounts_loaded = FALSE;

static void
purple_accounts_network_changed_cb(G_GNUC_UNUSED GNetworkMonitor *m,
                                   gboolean available,
                                   G_GNUC_UNUSED gpointer data)
{
	if(available) {
		PurpleAccountManager *manager = NULL;

		manager = purple_account_manager_get_default();
		purple_account_manager_set_online(manager, TRUE);
	}
}

/*********************************************************************
 * Writing to disk                                                   *
 *********************************************************************/
static void
accounts_to_xmlnode_helper(PurpleAccount *account, gpointer data) {
	PurpleXmlNode *node = data, *child = NULL;

	child = _purple_account_to_xmlnode(account);
	purple_xmlnode_insert_child(node, child);
}

static PurpleXmlNode *
accounts_to_xmlnode(void)
{
	PurpleAccountManager *manager = purple_account_manager_get_default();
	PurpleXmlNode *node = NULL;

	node = purple_xmlnode_new("account");
	purple_xmlnode_set_attrib(node, "version", "1.0");

	purple_account_manager_foreach(manager, accounts_to_xmlnode_helper, node);

	return node;
}

static void
sync_accounts(void)
{
	PurpleXmlNode *node;
	char *data;

	if (!accounts_loaded)
	{
		purple_debug_error("accounts", "Attempted to save accounts before "
						 "they were read!\n");
		return;
	}

	node = accounts_to_xmlnode();
	data = purple_xmlnode_to_formatted_str(node, NULL);
	purple_util_write_data_to_config_file("accounts.xml", data, -1);
	g_free(data);
	purple_xmlnode_free(node);
}

static gboolean
save_cb(G_GNUC_UNUSED gpointer data)
{
	sync_accounts();
	save_timer = 0;
	return FALSE;
}

void
purple_accounts_schedule_save(void)
{
	if (save_timer == 0)
		save_timer = g_timeout_add_seconds(5, save_cb, NULL);
}

static void
migrate_xmpp_encryption(PurpleAccount *account)
{
	/* When this is removed, nuke the "old_ssl" and "require_tls" settings */
	if (g_str_equal(purple_account_get_protocol_id(account), "prpl-jabber")) {
		const char *sec = purple_account_get_string(account, "connection_security", "");

		if (g_str_equal("", sec)) {
			const char *val = "require_tls";
			if (purple_account_get_bool(account, "old_ssl", FALSE))
				val = "old_ssl";
			else if (!purple_account_get_bool(account, "require_tls", TRUE))
				val = "opportunistic_tls";

			purple_account_set_string(account, "connection_security", val);
		}
	}
}

static void
parse_settings(PurpleXmlNode *node, PurpleAccount *account)
{
	PurpleXmlNode *child;

	/* Read settings, one by one */
	for (child = purple_xmlnode_get_child(node, "setting"); child != NULL;
			child = purple_xmlnode_get_next_twin(child))
	{
		const char *name, *str_type;
		PurplePrefType type;
		char *data;

		name = purple_xmlnode_get_attrib(child, "name");
		if (name == NULL)
			/* Ignore this setting */
			continue;

		str_type = purple_xmlnode_get_attrib(child, "type");
		if (str_type == NULL)
			/* Ignore this setting */
			continue;

		if (purple_strequal(str_type, "string"))
			type = PURPLE_PREF_STRING;
		else if (purple_strequal(str_type, "int"))
			type = PURPLE_PREF_INT;
		else if (purple_strequal(str_type, "bool"))
			type = PURPLE_PREF_BOOLEAN;
		else
			/* Ignore this setting */
			continue;

		data = purple_xmlnode_get_data(child);
		if (data == NULL)
			/* Ignore this setting */
			continue;

		if (type == PURPLE_PREF_STRING)
			purple_account_set_string(account, name, data);
		else if (type == PURPLE_PREF_INT)
			purple_account_set_int(account, name, atoi(data));
		else if (type == PURPLE_PREF_BOOLEAN)
			purple_account_set_bool(account, name,
								  (*data == '0' ? FALSE : TRUE));

		g_free(data);
	}

	/* we do this here because we need to do it before the user views the
	 * Edit Account dialog. */
	migrate_xmpp_encryption(account);
}

static void
parse_proxy_info(PurpleXmlNode *node, PurpleAccount *account)
{
	PurpleProxyInfo *proxy_info;
	PurpleXmlNode *child;
	char *data;

	proxy_info = purple_proxy_info_new();

	/* Use the global proxy settings, by default */
	purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_USE_GLOBAL);

	/* Read proxy type */
	child = purple_xmlnode_get_child(node, "type");
	if ((child != NULL) && ((data = purple_xmlnode_get_data(child)) != NULL))
	{
		if (purple_strequal(data, "global"))
			purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_USE_GLOBAL);
		else if (purple_strequal(data, "none"))
			purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_NONE);
		else if (purple_strequal(data, "http"))
			purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_HTTP);
		else if (purple_strequal(data, "socks4"))
			purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_SOCKS4);
		else if (purple_strequal(data, "socks5"))
			purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_SOCKS5);
		else if (purple_strequal(data, "tor"))
			purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_TOR);
		else if (purple_strequal(data, "envvar"))
			purple_proxy_info_set_proxy_type(proxy_info, PURPLE_PROXY_TYPE_USE_ENVVAR);
		else
		{
			purple_debug_error("accounts", "Invalid proxy type found when "
			                   "loading account information for %s\n",
			                   purple_account_get_username(account));
		}
		g_free(data);
	}

	/* Read proxy host */
	child = purple_xmlnode_get_child(node, "host");
	if ((child != NULL) && ((data = purple_xmlnode_get_data(child)) != NULL))
	{
		purple_proxy_info_set_hostname(proxy_info, data);
		g_free(data);
	}

	/* Read proxy port */
	child = purple_xmlnode_get_child(node, "port");
	if ((child != NULL) && ((data = purple_xmlnode_get_data(child)) != NULL))
	{
		purple_proxy_info_set_port(proxy_info, atoi(data));
		g_free(data);
	}

	/* Read proxy username */
	child = purple_xmlnode_get_child(node, "username");
	if ((child != NULL) && ((data = purple_xmlnode_get_data(child)) != NULL))
	{
		purple_proxy_info_set_username(proxy_info, data);
		g_free(data);
	}

	/* Read proxy password */
	child = purple_xmlnode_get_child(node, "password");
	if ((child != NULL) && ((data = purple_xmlnode_get_data(child)) != NULL))
	{
		purple_proxy_info_set_password(proxy_info, data);
		g_free(data);
	}

	/* If there are no values set then proxy_info NULL */
	if ((purple_proxy_info_get_proxy_type(proxy_info) == PURPLE_PROXY_TYPE_USE_GLOBAL) &&
		(purple_proxy_info_get_hostname(proxy_info) == NULL) &&
		(purple_proxy_info_get_port(proxy_info) == 0) &&
		(purple_proxy_info_get_username(proxy_info) == NULL) &&
		(purple_proxy_info_get_password(proxy_info) == NULL))
	{
		g_clear_object(&proxy_info);
		return;
	}

	purple_account_set_proxy_info(account, proxy_info);

	g_clear_object(&proxy_info);
}

static void
parse_current_error(PurpleXmlNode *node, PurpleAccount *account)
{
	guint type;
	char *type_str = NULL, *description = NULL;
	PurpleXmlNode *child;
	PurpleConnectionErrorInfo *current_error = NULL;

	child = purple_xmlnode_get_child(node, "type");
	if (child == NULL || (type_str = purple_xmlnode_get_data(child)) == NULL)
		return;
	type = atoi(type_str);
	g_free(type_str);

	if(type > PURPLE_CONNECTION_ERROR_OTHER_ERROR) {
		purple_debug_error("accounts",
		                   "Invalid PurpleConnectionError value %d found when "
		                   "loading account information for %s\n",
		                   type,
		                   purple_account_get_username(account));
		type = PURPLE_CONNECTION_ERROR_OTHER_ERROR;
	}

	child = purple_xmlnode_get_child(node, "description");
	if(child) {
		description = purple_xmlnode_get_data(child);
	}
	current_error = purple_connection_error_info_new(type,
	                                                 (description != NULL) ? description : "");
	g_free(description);

	purple_account_set_error(account, current_error);
}

static PurpleAccount *
parse_account(PurpleXmlNode *node)
{
	PurpleAccount *ret;
	PurpleXmlNode *child;
	gchar *id = NULL;
	char *protocol_id = NULL;
	char *name = NULL;
	char *data;
	gboolean enabled = FALSE;
	gboolean require_password = FALSE;

	child = purple_xmlnode_get_child(node, "id");
	if(child != NULL) {
		id = purple_xmlnode_get_data(child);
	}

	child = purple_xmlnode_get_child(node, "protocol");
	if (child != NULL)
		protocol_id = purple_xmlnode_get_data(child);

	child = purple_xmlnode_get_child(node, "name");
	if (child != NULL)
		name = purple_xmlnode_get_data(child);
	if (name == NULL)
	{
		/* Do we really need to do this? */
		child = purple_xmlnode_get_child(node, "username");
		if (child != NULL)
			name = purple_xmlnode_get_data(child);
	}

	child = purple_xmlnode_get_child(node, "require_password");
	if(child != NULL) {
		data = purple_xmlnode_get_data(child);
		require_password = atoi(data);
		g_free(data);
	}

	child = purple_xmlnode_get_child(node, "enabled");
	if(child != NULL) {
		data = purple_xmlnode_get_data(child);
		enabled = atoi(data);
		g_free(data);
	}

	if ((protocol_id == NULL) || (name == NULL))
	{
		g_free(id);
		g_free(protocol_id);
		g_free(name);
		return NULL;
	}

	/* Manually create the account as the id parameter is construct only and we
	 * don't want people messing with it.
	 */
	ret = g_object_new(PURPLE_TYPE_ACCOUNT,
		"id", id,
		"username", name,
		"protocol-id", protocol_id,
		"require-password", require_password,
		NULL);

	purple_account_set_enabled_plain(ret, enabled);

	g_free(id);
	g_free(name);
	g_free(protocol_id);

	/* Read the alias */
	child = purple_xmlnode_get_child(node, "alias");
	if ((child != NULL) && ((data = purple_xmlnode_get_data(child)) != NULL))
	{
		if (*data != '\0') {
			PurpleContactInfo *info = NULL;

			info = purple_account_get_contact_info(ret);
			purple_contact_info_set_alias(info, data);
		}
		g_free(data);
	}

	/* Read the userinfo */
	child = purple_xmlnode_get_child(node, "userinfo");
	if ((child != NULL) && ((data = purple_xmlnode_get_data(child)) != NULL))
	{
		purple_account_set_user_info(ret, data);
		g_free(data);
	}

	/* Read settings (both core and UI) */
	for (child = purple_xmlnode_get_child(node, "settings"); child != NULL;
			child = purple_xmlnode_get_next_twin(child))
	{
		parse_settings(child, ret);
	}

	/* Read proxy */
	child = purple_xmlnode_get_child(node, "proxy");
	if (child != NULL)
	{
		parse_proxy_info(child, ret);
	}

	/* Read current error */
	child = purple_xmlnode_get_child(node, "current_error");
	if (child != NULL)
	{
		parse_current_error(child, ret);
	}

	return ret;
}

static void
load_accounts(void) {
	PurpleAccountManager *manager = NULL;
	PurpleXmlNode *node, *child;

	accounts_loaded = TRUE;

	node = purple_util_read_xml_from_config_file("accounts.xml", _("accounts"));

	if(node == NULL) {
		return;
	}

	manager = purple_account_manager_get_default();

	for(child = purple_xmlnode_get_child(node, "account"); child != NULL;
	    child = purple_xmlnode_get_next_twin(child))
	{
		PurpleAccount *new_acct;
		new_acct = parse_account(child);

		purple_account_manager_add(manager, new_acct);
		g_clear_object(&new_acct);
	}

	purple_xmlnode_free(node);
}

static void
purple_accounts_delete_set(GObject *obj, GAsyncResult *res, gpointer d) {
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	PurpleAccount *account = PURPLE_ACCOUNT(d);
	GError *error = NULL;
	gboolean r = FALSE;

	r = purple_credential_manager_clear_password_finish(manager, res, &error);
	if(r != TRUE) {
		purple_debug_warning("accounts",
		                     "Failed to remove password for account %s: %s",
		                     purple_account_get_username(account),
		                     (error != NULL) ? error->message : "Unknown error");

		g_clear_error(&error);
	}

	g_object_unref(account);
}

void
purple_accounts_delete(PurpleAccount *account)
{
	PurpleAccountManager *manager = NULL;
	PurpleConversationManager *conv_manager = NULL;
	PurpleCredentialManager *cred_manager = NULL;
	GList *iter = NULL;

	g_return_if_fail(account != NULL);

	/*
	 * Disable the account before blowing it out of the water.
	 * Conceptually it probably makes more sense to disable the
	 * account for all UIs rather than the just the current UI,
	 * but it doesn't really matter.
	 */
	purple_account_set_enabled(account, FALSE);

	purple_request_close_with_handle(account);

	manager = purple_account_manager_get_default();
	purple_account_manager_remove(manager, account);

	/* Remove any open conversation for this account */
	conv_manager = purple_conversation_manager_get_default();
	iter = purple_conversation_manager_get_all(conv_manager);
	while(iter != NULL) {
		PurpleConversation *conv = iter->data;

		if(purple_conversation_get_account(conv) == account) {
			g_object_unref(conv);
		}

		iter = g_list_delete_link(iter, iter);
	}

	/* This is async because we do not want the
	 * account being overwritten before we are done.
	 */
	cred_manager = purple_credential_manager_get_default();
	purple_credential_manager_clear_password_async(cred_manager, account, NULL,
	                                               purple_accounts_delete_set,
	                                               g_object_ref(account));
}

void *
purple_accounts_get_handle(void)
{
	static int handle;

	return &handle;
}

static void
connection_error_cb(PurpleConnection *gc,
                    PurpleConnectionError type,
                    const gchar *description,
                    G_GNUC_UNUSED gpointer unused)
{
	PurpleAccount *account;
	PurpleConnectionErrorInfo *err;

	account = purple_connection_get_account(gc);

	g_return_if_fail(account != NULL);

	err = purple_connection_error_info_new(type, description);
	purple_account_set_error(account, err);
}

void
purple_accounts_init(void)
{
	void *handle = purple_accounts_get_handle();
	void *conn_handle = purple_connections_get_handle();

	purple_signal_connect(conn_handle, "connection-error", handle,
	                      G_CALLBACK(connection_error_cb), NULL);

	load_accounts();

	g_signal_connect(G_OBJECT(g_network_monitor_get_default()),
	                 "network-changed",
	                 G_CALLBACK(purple_accounts_network_changed_cb), NULL);
}

void
purple_accounts_uninit(void)
{
	gpointer handle = purple_accounts_get_handle();
	if(save_timer != 0) {
		g_clear_handle_id(&save_timer, g_source_remove);
		sync_accounts();
	}

	purple_signals_disconnect_by_handle(handle);
}
