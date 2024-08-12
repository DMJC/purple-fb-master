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

#include "purpleaccount.h"

#include "accounts.h"
#include "core.h"
#include "debug.h"
#include "network.h"
#include "prefs.h"
#include "purpleaddcontactrequest.h"
#include "purpleconversationmanager.h"
#include "purplecredentialmanager.h"
#include "purplenotification.h"
#include "purplenotificationmanager.h"
#include "purpleprivate.h"
#include "purpleprotocolmanager.h"
#include "purpleprotocolserver.h"
#include "request.h"
#include "request/purplerequestfieldbool.h"
#include "request/purplerequestfieldstring.h"
#include "signals.h"
#include "util.h"

typedef struct {
	GSList *names;
	guint ref_count;
} PurpleAccountSettingFreezeQueue;

G_LOCK_DEFINE_STATIC(setting_notify_lock);

struct _PurpleAccount {
	GObject parent;

	char *id;
	char *username;
	PurpleContactInfo *contact_info;

	gboolean require_password;
	char *user_info;

	char *buddy_icon_path;

	gboolean enabled;
	gboolean remember_pass;

	char *protocol_id;

	PurpleConnection *gc;
	gboolean disconnecting;

	GHashTable *settings;
	PurpleAccountSettingFreezeQueue *freeze_queue;

	PurpleProxyInfo *proxy_info;

	PurplePresence *presence;

	PurpleConnectionErrorInfo *error;
	PurpleNotification *error_notification;
} PurpleAccountPrivate;

typedef struct {
	char *ui;
	GValue value;
} PurpleAccountSetting;

enum {
	PROP_0,
	PROP_ID,
	PROP_USERNAME,
	PROP_CONTACT_INFO,
	PROP_REQUIRE_PASSWORD,
	PROP_ENABLED,
	PROP_CONNECTION,
	PROP_PROTOCOL_ID,
	PROP_USER_INFO,
	PROP_BUDDY_ICON_PATH,
	PROP_REMEMBER_PASSWORD,
	PROP_PROXY_INFO,
	PROP_ERROR,
	PROP_CONNECTED,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_SETTING_CHANGED,
	SIG_CONNECTED,
	SIG_DISCONNECTED,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_account_set_id(PurpleAccount *account, const char *id) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_set_str(&account->id, id)) {
		g_object_notify_by_pspec(G_OBJECT(account), properties[PROP_ID]);
	}
}

static void
purple_account_free_notify_settings(PurpleAccount *account) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(account->freeze_queue != NULL) {
		g_slist_free_full(account->freeze_queue->names, g_free);

		g_slice_free(PurpleAccountSettingFreezeQueue, account->freeze_queue);
		account->freeze_queue = NULL;
	}
}

static void
purple_account_setting_changed_emit(PurpleAccount *account, const char *name) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(name != NULL);

	G_LOCK(setting_notify_lock);

	if(account->freeze_queue != NULL) {
		GSList *names = account->freeze_queue->names;

		if(g_slist_find_custom(names, name, (GCompareFunc)g_strcmp0) == NULL) {
			names = g_slist_prepend(names, g_strdup(name));
			account->freeze_queue->names = names;
		}
	} else {
		g_signal_emit(account, signals[SIG_SETTING_CHANGED],
		              g_quark_from_string(name), name);
	}

	G_UNLOCK(setting_notify_lock);
}

static void
purple_account_real_connect(PurpleAccount *account, const char *password) {
	PurpleConnection *connection = NULL;
	PurpleProtocol *protocol = NULL;
	GError *error = NULL;

	protocol = purple_account_get_protocol(account);

	connection = purple_protocol_create_connection(protocol, account, password,
	                                               &error);
	if(error != NULL) {
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(account);

		purple_debug_warning("failed to create connection for %s: %s",
		                     purple_contact_info_get_username(info),
		                     error->message);
		g_clear_error(&error);

		return;
	}

	g_return_if_fail(PURPLE_IS_CONNECTION(connection));

	purple_account_set_connection(account, connection);
	if(!purple_connection_connect(connection, &error)) {
		const char *message = "unknown error";

		if(error != NULL && error->message != NULL) {
			message = error->message;
		}

		purple_connection_error(connection,
		                        PURPLE_CONNECTION_ERROR_OTHER_ERROR,
		                        message);

		g_clear_error(&error);
	}

	/* Finally remove our reference to the connection. */
	g_object_unref(connection);
}

static void
request_password_write_cb(GObject *obj, GAsyncResult *res, gpointer data) {
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	PurpleAccount *account = PURPLE_ACCOUNT(data);
	GError *error = NULL;
	char *password = NULL;

	/* We stash the password on the account to get it to this call back... It's
	 * kind of gross but shouldn't be a big deal because any plugin has access
	 * to the credential store, so it's not really a security leak.
	 */
	password = (char *)g_object_get_data(G_OBJECT(account), "_tmp_password");
	g_object_set_data(G_OBJECT(account), "_tmp_password", NULL);

	if(!purple_credential_manager_write_password_finish(manager, res, &error))
	{
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(account);
		const char *name = purple_contact_info_get_name_for_display(info);

		/* we can't error an account without a connection, so we just drop a
		 * debug message for now and continue to connect the account.
		 */
		purple_debug_info("account",
		                  "failed to save password for account \"%s\": %s",
		                  name,
		                  error != NULL ? error->message : "unknown error");
		g_clear_error(&error);
	}

	purple_account_real_connect(account, password);

	g_free(password);
}

static void
request_password_ok_cb(PurpleAccount *account, PurpleRequestPage *page) {
	const char *entry;
	gboolean remember;

	entry = purple_request_page_get_string(page, "password");
	remember = purple_request_page_get_bool(page, "remember");

	if(purple_strempty(entry)) {
		g_warning(_("Password is required to sign on for '%s'."),
		          account->username);
		return;
	}

	purple_account_set_remember_password(account, remember);

	if(remember) {
		PurpleCredentialManager *manager = NULL;

		manager = purple_credential_manager_get_default();

		/* The requests field can be invalidated by the time we write the
		 * password and we want to use it in the write callback, so we need to
		 * duplicate it for that callback.
		 */
		g_object_set_data(G_OBJECT(account), "_tmp_password", g_strdup(entry));
		purple_credential_manager_write_password_async(manager, account, entry,
		                                               NULL,
		                                               request_password_write_cb,
		                                               account);
	} else {
		purple_account_real_connect(account, entry);
	}
}

static void
request_password_cancel_cb(PurpleAccount *account,
                           G_GNUC_UNUSED PurpleRequestPage *page)
{
	/* Disable the account as the user has cancelled connecting */
	purple_account_set_enabled(account, FALSE);
}


static void
purple_account_connect_got_password_cb(GObject *obj, GAsyncResult *res,
                                       gpointer data)
{
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	PurpleAccount *account = PURPLE_ACCOUNT(data);
	PurpleProtocol *protocol = NULL;
	PurpleProtocolOptions options;
	GError *error = NULL;
	char *password = NULL;
	gboolean require_password = TRUE;

	password = purple_credential_manager_read_password_finish(manager, res,
	                                                          &error);

	if(error != NULL) {
		purple_debug_warning("account", "failed to read password %s",
		                     error->message);

		g_error_free(error);
	}

	protocol = purple_account_get_protocol(account);
	options = purple_protocol_get_options(protocol);
	if(options & OPT_PROTO_PASSWORD_OPTIONAL) {
		require_password = purple_account_get_require_password(account);
	} else if(options & OPT_PROTO_NO_PASSWORD) {
		require_password = FALSE;
	}

	if((password == NULL || *password == '\0') && require_password) {
		purple_account_request_password(account,
		                                G_CALLBACK(request_password_ok_cb),
		                                G_CALLBACK(request_password_cancel_cb),
		                                account);
	} else {
		purple_account_real_connect(account, password);
	}

	g_free(password);
}

static gboolean
no_password_cb(gpointer data) {
	PurpleAccount *account = data;

	purple_account_real_connect(account, NULL);

	return G_SOURCE_REMOVE;
}

static void
delete_setting(void *data) {
	PurpleAccountSetting *setting = (PurpleAccountSetting *)data;

	g_free(setting->ui);
	g_value_unset(&setting->value);

	g_free(setting);
}

static PurpleConnectionState
purple_account_get_state(PurpleAccount *account) {
	PurpleConnection *gc;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account),
	                     PURPLE_CONNECTION_STATE_DISCONNECTED);

	gc = purple_account_get_connection(account);
	if(!gc) {
		return PURPLE_CONNECTION_STATE_DISCONNECTED;
	}

	return purple_connection_get_state(gc);
}

static void
purple_account_connection_state_cb(GObject *obj,
                                   G_GNUC_UNUSED GParamSpec *pspec,
                                   gpointer data)
{
	PurpleAccount *account = data;
	PurpleConnection *connection = PURPLE_CONNECTION(obj);
	PurpleConnectionState state = PURPLE_CONNECTION_STATE_DISCONNECTED;

	state = purple_connection_get_state(connection);
	if(state == PURPLE_CONNECTION_STATE_CONNECTED) {
		g_signal_emit(account, signals[SIG_CONNECTED], 0);
	} else if(state == PURPLE_CONNECTION_STATE_DISCONNECTED) {
		g_signal_emit(account, signals[SIG_DISCONNECTED], 0);
	}
}

static void
purple_account_can_connect_cb(GObject *source, GAsyncResult *result,
                              gpointer data)
{
	PurpleAccount *account = data;
	PurpleProtocol *protocol = PURPLE_PROTOCOL(source);
	PurpleProtocolOptions options;
	GError *error = NULL;
	gboolean can_connect = FALSE;
	gboolean require_password = TRUE;

	can_connect = purple_protocol_can_connect_finish(protocol, result, &error);
	if(!can_connect || error != NULL) {
		PurpleConnectionErrorInfo *info = NULL;
		const char *error_message = _("unknown error");

		if(error != NULL) {
			error_message = error->message;
		}

		info = purple_connection_error_info_new(PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
		                                        error_message);
		purple_account_set_error(account, info);

		g_clear_error(&error);

		return;
	}

	options = purple_protocol_get_options(protocol);
	if(options & OPT_PROTO_PASSWORD_OPTIONAL) {
		require_password = purple_account_get_require_password(account);
	} else if(options & OPT_PROTO_NO_PASSWORD) {
		require_password = FALSE;
	}

	if(require_password) {
		PurpleCredentialManager *manager = NULL;

		manager = purple_credential_manager_get_default();
		purple_credential_manager_read_password_async(manager, account, NULL,
		                                              purple_account_connect_got_password_cb,
		                                              account);
	} else {
		g_timeout_add_seconds(0, no_password_cb, account);
	}
}

/******************************************************************************
 * XmlNode Helpers
 *****************************************************************************/
static PurpleXmlNode *
proxy_settings_to_xmlnode(PurpleProxyInfo *proxy_info) {
	PurpleXmlNode *node, *child;
	PurpleProxyType proxy_type;
	const char *value;
	int int_value;
	char buf[21];

	proxy_type = purple_proxy_info_get_proxy_type(proxy_info);

	node = purple_xmlnode_new("proxy");

	child = purple_xmlnode_new_child(node, "type");
	purple_xmlnode_insert_data(child,
			(proxy_type == PURPLE_PROXY_TYPE_USE_GLOBAL ? "global" :
			 proxy_type == PURPLE_PROXY_TYPE_NONE       ? "none"   :
			 proxy_type == PURPLE_PROXY_TYPE_HTTP       ? "http"   :
			 proxy_type == PURPLE_PROXY_TYPE_SOCKS4     ? "socks4" :
			 proxy_type == PURPLE_PROXY_TYPE_SOCKS5     ? "socks5" :
			 proxy_type == PURPLE_PROXY_TYPE_TOR        ? "tor" :
			 proxy_type == PURPLE_PROXY_TYPE_USE_ENVVAR ? "envvar" : "unknown"), -1);

	if((value = purple_proxy_info_get_hostname(proxy_info)) != NULL) {
		child = purple_xmlnode_new_child(node, "host");
		purple_xmlnode_insert_data(child, value, -1);
	}

	if((int_value = purple_proxy_info_get_port(proxy_info)) != 0) {
		g_snprintf(buf, sizeof(buf), "%d", int_value);
		child = purple_xmlnode_new_child(node, "port");
		purple_xmlnode_insert_data(child, buf, -1);
	}

	if((value = purple_proxy_info_get_username(proxy_info)) != NULL) {
		child = purple_xmlnode_new_child(node, "username");
		purple_xmlnode_insert_data(child, value, -1);
	}

	if((value = purple_proxy_info_get_password(proxy_info)) != NULL) {
		child = purple_xmlnode_new_child(node, "password");
		purple_xmlnode_insert_data(child, value, -1);
	}

	return node;
}

static PurpleXmlNode *
current_error_to_xmlnode(PurpleConnectionErrorInfo *err) {
	PurpleXmlNode *node, *child;
	char type_str[3];

	node = purple_xmlnode_new("current_error");

	if(err == NULL) {
		return node;
	}

	/* It doesn't make sense to have transient errors persist across a
	 * restart.
	 */
	if(!purple_connection_error_is_fatal (err->type)) {
		return node;
	}

	child = purple_xmlnode_new_child(node, "type");
	g_snprintf(type_str, sizeof(type_str), "%u", err->type);
	purple_xmlnode_insert_data(child, type_str, -1);

	child = purple_xmlnode_new_child(node, "description");
	if(err->description) {
		char *utf8ized = purple_utf8_try_convert(err->description);
		if(utf8ized == NULL) {
			utf8ized = g_utf8_make_valid(err->description, -1);
		}
		purple_xmlnode_insert_data(child, utf8ized, -1);
		g_free(utf8ized);
	}

	return node;
}

static void
setting_to_xmlnode(gpointer key, gpointer value, gpointer user_data)
{
	const char *name;
	PurpleAccountSetting *setting;
	PurpleXmlNode *node, *child;
	char buf[21];

	name = (const char *)key;
	setting = (PurpleAccountSetting *)value;
	node = (PurpleXmlNode *)user_data;

	child = purple_xmlnode_new_child(node, "setting");
	purple_xmlnode_set_attrib(child, "name", name);

	if(G_VALUE_HOLDS_INT(&setting->value)) {
		purple_xmlnode_set_attrib(child, "type", "int");
		g_snprintf(buf, sizeof(buf), "%d", g_value_get_int(&setting->value));
		purple_xmlnode_insert_data(child, buf, -1);
	} else if(G_VALUE_HOLDS_STRING(&setting->value) &&
	          g_value_get_string(&setting->value) != NULL)
	{
		purple_xmlnode_set_attrib(child, "type", "string");
		purple_xmlnode_insert_data(child, g_value_get_string(&setting->value),
		                           -1);
	} else if (G_VALUE_HOLDS_BOOLEAN(&setting->value)) {
		purple_xmlnode_set_attrib(child, "type", "bool");
		g_snprintf(buf, sizeof(buf), "%d",
		           g_value_get_boolean(&setting->value));
		purple_xmlnode_insert_data(child, buf, -1);
	}
}

PurpleXmlNode *
_purple_account_to_xmlnode(PurpleAccount *account) {
	PurpleXmlNode *node, *child;
	char *data = NULL;
	const char *tmp;
	PurpleProxyInfo *proxy_info;

	node = purple_xmlnode_new("account");

	if(account->id != NULL) {
		child = purple_xmlnode_new_child(node, "id");
		purple_xmlnode_insert_data(child, account->id, -1);
	}

	child = purple_xmlnode_new_child(node, "protocol");
	purple_xmlnode_insert_data(child, purple_account_get_protocol_id(account),
	                           -1);

	child = purple_xmlnode_new_child(node, "name");
	purple_xmlnode_insert_data(child, account->username, -1);

	child = purple_xmlnode_new_child(node, "require_password");
	data = g_strdup_printf("%d", account->require_password);
	purple_xmlnode_insert_data(child, data, -1);
	g_clear_pointer(&data, g_free);

	child = purple_xmlnode_new_child(node, "enabled");
	data = g_strdup_printf("%d", account->enabled);
	purple_xmlnode_insert_data(child, data, -1);
	g_clear_pointer(&data, g_free);

	tmp = purple_contact_info_get_alias(account->contact_info);
	if(tmp != NULL) {
		child = purple_xmlnode_new_child(node, "alias");
		purple_xmlnode_insert_data(child, tmp, -1);
	}

	if((tmp = purple_account_get_user_info(account)) != NULL) {
		/* TODO: Do we need to call purple_str_strip_char(tmp, '\r') here? */
		child = purple_xmlnode_new_child(node, "user-info");
		purple_xmlnode_insert_data(child, tmp, -1);
	}

	if(g_hash_table_size(account->settings) > 0) {
		child = purple_xmlnode_new_child(node, "settings");
		g_hash_table_foreach(account->settings, setting_to_xmlnode, child);
	}

	if((proxy_info = purple_account_get_proxy_info(account)) != NULL) {
		child = proxy_settings_to_xmlnode(proxy_info);
		purple_xmlnode_insert_child(node, child);
	}

	child = current_error_to_xmlnode(account->error);
	purple_xmlnode_insert_child(node, child);

	return node;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleAccount, purple_account, G_TYPE_OBJECT);

static void
purple_account_set_property(GObject *obj, guint param_id, const GValue *value,
                            GParamSpec *pspec)
{
	PurpleAccount *account = PURPLE_ACCOUNT(obj);

	switch(param_id) {
	case PROP_ID:
		purple_account_set_id(account, g_value_get_string(value));
		break;
	case PROP_USERNAME:
		purple_account_set_username(account, g_value_get_string(value));
		break;
	case PROP_REQUIRE_PASSWORD:
		purple_account_set_require_password(account,
		                                    g_value_get_boolean(value));
		break;
	case PROP_ENABLED:
		purple_account_set_enabled(account, g_value_get_boolean(value));
		break;
	case PROP_CONNECTION:
		purple_account_set_connection(account, g_value_get_object(value));
		break;
	case PROP_PROTOCOL_ID:
		purple_account_set_protocol_id(account, g_value_get_string(value));
		break;
	case PROP_USER_INFO:
		purple_account_set_user_info(account, g_value_get_string(value));
		break;
	case PROP_BUDDY_ICON_PATH:
		purple_account_set_buddy_icon_path(account, g_value_get_string(value));
		break;
	case PROP_REMEMBER_PASSWORD:
		purple_account_set_remember_password(account,
		                                     g_value_get_boolean(value));
		break;
	case PROP_PROXY_INFO:
		purple_account_set_proxy_info(account, g_value_get_object(value));
		break;
	case PROP_ERROR:
		purple_account_set_error(account, g_value_get_boxed(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_account_get_property(GObject *obj, guint param_id, GValue *value,
                            GParamSpec *pspec)
{
	PurpleAccount *account = PURPLE_ACCOUNT(obj);

	switch(param_id) {
	case PROP_ID:
		g_value_set_string(value, purple_account_get_id(account));
		break;
	case PROP_USERNAME:
		g_value_set_string(value, purple_account_get_username(account));
		break;
	case PROP_CONTACT_INFO:
		g_value_set_object(value, purple_account_get_contact_info(account));
		break;
	case PROP_REQUIRE_PASSWORD:
		g_value_set_boolean(value,
		                    purple_account_get_require_password(account));
		break;
	case PROP_ENABLED:
		g_value_set_boolean(value, purple_account_get_enabled(account));
		break;
	case PROP_CONNECTION:
		g_value_set_object(value, purple_account_get_connection(account));
		break;
	case PROP_PROTOCOL_ID:
		g_value_set_string(value, purple_account_get_protocol_id(account));
		break;
	case PROP_USER_INFO:
		g_value_set_string(value, purple_account_get_user_info(account));
		break;
	case PROP_BUDDY_ICON_PATH:
		g_value_set_string(value, purple_account_get_buddy_icon_path(account));
		break;
	case PROP_REMEMBER_PASSWORD:
		g_value_set_boolean(value,
		                    purple_account_get_remember_password(account));
		break;
	case PROP_PROXY_INFO:
		g_value_set_object(value, purple_account_get_proxy_info(account));
		break;
	case PROP_ERROR:
		g_value_set_boxed(value, purple_account_get_error(account));
		break;
	case PROP_CONNECTED:
		g_value_set_boolean(value, purple_account_is_connected(account));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_account_init(PurpleAccount *account) {
	account->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
	                                          delete_setting);
}

static void
purple_account_constructed(GObject *object) {
	PurpleAccount *account = PURPLE_ACCOUNT(object);

	G_OBJECT_CLASS(purple_account_parent_class)->constructed(object);

	/* If we didn't get an id, checksum the protocol id and the username. */
	if(purple_strempty(account->id)) {
		GChecksum *checksum = NULL;

		checksum = g_checksum_new(G_CHECKSUM_SHA256);

		g_checksum_update(checksum, (const guchar *)account->protocol_id, -1);
		g_checksum_update(checksum, (const guchar *)account->username, -1);

		purple_account_set_id(account, g_checksum_get_string(checksum));

		g_checksum_free(checksum);
	}

	account->contact_info = purple_contact_info_new(NULL);
	account->presence = purple_presence_new();
}

static void
purple_account_dispose(GObject *object) {
	PurpleAccount *account = PURPLE_ACCOUNT(object);

	if(!purple_account_is_disconnected(account)) {
		purple_account_disconnect(account);
	}

	g_clear_object(&account->gc);
	g_clear_object(&account->presence);

	G_OBJECT_CLASS(purple_account_parent_class)->dispose(object);
}

static void
purple_account_finalize(GObject *object) {
	PurpleAccount *account = PURPLE_ACCOUNT(object);

	purple_debug_info("account", "Destroying account %p", account);

	purple_account_free_notify_settings(account);

	g_clear_pointer(&account->id, g_free);
	g_clear_pointer(&account->username, g_free);
	g_clear_object(&account->contact_info);

	g_clear_object(&account->proxy_info);

	g_clear_pointer(&account->error, purple_connection_error_info_free);
	g_clear_object(&account->error_notification);

	g_free(account->user_info);
	g_free(account->buddy_icon_path);
	g_free(account->protocol_id);

	g_hash_table_destroy(account->settings);

	G_OBJECT_CLASS(purple_account_parent_class)->finalize(object);
}

static void
purple_account_class_init(PurpleAccountClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->constructed = purple_account_constructed;
	obj_class->dispose = purple_account_dispose;
	obj_class->finalize = purple_account_finalize;
	obj_class->get_property = purple_account_get_property;
	obj_class->set_property = purple_account_set_property;

	/**
	 * PurpleAccount:id:
	 *
	 * The unique identifier for the account.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:username:
	 *
	 * The username for the account.
	 *
	 * Since: 3.0
	 */
	properties[PROP_USERNAME] = g_param_spec_string(
		"username", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:contact-info:
	 *
	 * The [class@ContactInfo] for the account.
	 *
	 * This should be completely managed by the protocol that this account was
	 * created for. Writing any properties to this from anything but the
	 * protocol will lead to de-synchronization.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONTACT_INFO] = g_param_spec_object(
		"contact-info", NULL, NULL,
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:require-password:
	 *
	 * Whether or not this account should require a password. This is only used
	 * if the [class@Purple.Protocol] that this account is for allows optional
	 * passwords.
	 *
	 * Since: 3.0
	 */
	properties[PROP_REQUIRE_PASSWORD] = g_param_spec_boolean(
		"require-password", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:user-info:
	 *
	 * The user information or profile for the account.
	 *
	 * Since: 3.0
	 */
	properties[PROP_USER_INFO] = g_param_spec_string(
		"user-info", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:buddy-icon-path:
	 *
	 * The path to the file to use as the avatar for this account.
	 *
	 * Since: 3.0
	 */
	properties[PROP_BUDDY_ICON_PATH] = g_param_spec_string(
		"buddy-icon-path", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:enabled:
	 *
	 * Whether or not this account should track the user's global status.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ENABLED] = g_param_spec_boolean(
		"enabled", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:remember-password:
	 *
	 * Whether or not the password for this account should be stored in the
	 * configured [class@CredentialProvider].
	 *
	 * Since: 3.0
	 */
	properties[PROP_REMEMBER_PASSWORD] = g_param_spec_boolean(
		"remember-password", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:connection:
	 *
	 * The [class@Connection] object for this account. This will be %NULL when
	 * the account is offline.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONNECTION] = g_param_spec_object(
		"connection", NULL, NULL,
		PURPLE_TYPE_CONNECTION,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:protocol-id:
	 *
	 * The identifier of the protocol that this account is using.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PROTOCOL_ID] = g_param_spec_string(
		"protocol-id", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:proxy-info:
	 *
	 * The [class@ProxyInfo] for this account.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PROXY_INFO] = g_param_spec_object(
		"proxy-info", NULL, NULL,
		PURPLE_TYPE_PROXY_INFO,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:error:
	 *
	 * The [type@GLib.Error] of the account. This is set when an account enters
	 * an error state and is automatically cleared when a connection attempt is
	 * made.
	 *
	 * Setting this will not disconnect an account, but this will be set when
	 * there is a connection failure.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ERROR] = g_param_spec_boxed(
		"error", NULL, NULL,
		PURPLE_TYPE_CONNECTION_ERROR_INFO,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccount:connected:
	 *
	 * Whether or not the account is connected.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONNECTED] = g_param_spec_boolean(
		"connected", NULL, NULL,
		FALSE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleAccount::setting-changed:
	 * @account: The account whose setting changed.
	 * @name: The name of the setting that changed.
	 *
	 * The ::setting-changed signal is emitted whenever an account setting is
	 * changed.
	 *
	 * This signal supports details, so you can be notified when a single
	 * setting changes. For example, say there's a setting named `foo`,
	 * connecting to `setting-changed::foo` will only be called when the `foo`
	 * setting is changed.
	 *
	 * Since: 3.0
	 */
	signals[SIG_SETTING_CHANGED] = g_signal_new_class_handler(
		"setting-changed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_STRING);

	/**
	 * PurpleAccount::connected:
	 * @account: The account instance.
	 *
	 * This is emitted when the [property@Account:connection]'s
	 * [property@Connection:state] has changed to
	 * %PURPLE_CONNECTION_STATE_CONNECTED.
	 *
	 * Since: 3.0
	 */
	signals[SIG_CONNECTED] = g_signal_new_class_handler(
		"connected",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0);

	/**
	 * PurpleAccount::disconnected:
	 * @account: The account instance.
	 *
	 * This is emitted when the [property@Account:connection]'s
	 * [property@Connection:state] has changed to
	 * %PURPLE_CONNECTION_STATE_DISCONNECTED.
	 *
	 * Since: 3.0
	 */
	signals[SIG_DISCONNECTED] = g_signal_new_class_handler(
		"disconnected",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0);
}

/******************************************************************************
 * Private API
 *****************************************************************************/

/* This is a temporary method that the deserializer can call to set the
 * enabled property without bringing the account online.
 */
void
purple_account_set_enabled_plain(PurpleAccount *account, gboolean enabled) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	account->enabled = enabled;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleAccount *
purple_account_new(const char *username, const char *protocol_id) {
	g_return_val_if_fail(username != NULL, NULL);
	g_return_val_if_fail(protocol_id != NULL, NULL);

	return g_object_new(
		PURPLE_TYPE_ACCOUNT,
		"username", username,
		"protocol-id", protocol_id,
		"enabled", FALSE,
		NULL);
}

const char *
purple_account_get_id(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->id;
}

const char *
purple_account_get_username(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->username;
}

void
purple_account_set_username(PurpleAccount *account, const char *username) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_set_str(&account->username, username)) {
		g_object_notify_by_pspec(G_OBJECT(account), properties[PROP_USERNAME]);
	}
}

PurpleContactInfo *
purple_account_get_contact_info(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->contact_info;
}

void
purple_account_connect(PurpleAccount *account)
{
	PurpleProtocol *protocol = NULL;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	purple_account_set_error(account, NULL);

	if(!purple_account_get_enabled(account)) {
		purple_debug_info("account",
		                  "Account %s not enabled, not connecting.\n",
		                  account->username);
		return;
	}

	protocol = purple_account_get_protocol(account);
	if(protocol == NULL) {
		PurpleNotification *notification = NULL;
		PurpleNotificationManager *manager = NULL;
		char *value = NULL;

		notification = purple_notification_new(PURPLE_NOTIFICATION_TYPE_GENERIC,
		                                       account, NULL, NULL);

		value = g_strdup_printf(_("Failed to load account '%s'"),
		                        account->username);
		purple_notification_set_title(notification, value);
		g_free(value);

		value = g_strdup_printf(_("Failed to find a protocol with id '%s'"),
		                        account->protocol_id);
		purple_notification_set_subtitle(notification, value);
		g_free(value);

		manager = purple_notification_manager_get_default();
		purple_notification_manager_add(manager, notification);
		g_clear_object(&notification);

		return;
	}

	purple_protocol_can_connect_async(protocol, account, NULL,
	                                  purple_account_can_connect_cb, account);
}

void
purple_account_disconnect(PurpleAccount *account)
{
	GError *error = NULL;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(!purple_account_is_disconnecting(account));
	g_return_if_fail(!purple_account_is_disconnected(account));

	purple_debug_info("account", "Disconnecting account %s (%p)\n",
	                  account->username, account);

	account->disconnecting = TRUE;

	if(!purple_connection_disconnect(account->gc, &error)) {
		g_warning("error while disconnecting account %s (%s): %s",
		          account->username,
		          purple_account_get_protocol_id(account),
		          (error != NULL) ? error->message : "unknown error");
		g_clear_error(&error);
	}

	purple_account_set_connection(account, NULL);

	account->disconnecting = FALSE;
}

gboolean
purple_account_is_disconnecting(PurpleAccount *account)
{
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), TRUE);

	return account->disconnecting;
}

void
purple_account_request_close_with_account(PurpleAccount *account) {
	PurpleNotificationManager *manager = NULL;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	manager = purple_notification_manager_get_default();
	purple_notification_manager_remove_with_account(manager, account, FALSE);
}

void
purple_account_request_password(PurpleAccount *account, GCallback ok_cb,
                                GCallback cancel_cb, void *user_data)
{
	char *primary;
	const char *username;
	PurpleRequestGroup *group;
	PurpleRequestField *field;
	PurpleRequestPage *page;

	/* Close any previous password request windows */
	purple_request_close_with_handle(account);

	username = purple_contact_info_get_username(PURPLE_CONTACT_INFO(account));
	primary = g_strdup_printf(_("Enter password for %s (%s)"), username,
	                          purple_account_get_protocol_name(account));

	page = purple_request_page_new();
	group = purple_request_group_new(NULL);
	purple_request_page_add_group(page, group);

	field = purple_request_field_string_new("password", _("Enter Password"),
	                                        NULL, FALSE);
	purple_request_field_string_set_masked(PURPLE_REQUEST_FIELD_STRING(field),
	                                       TRUE);
	purple_request_field_set_required(field, TRUE);
	purple_request_group_add_field(group, field);

	field = purple_request_field_bool_new("remember", _("Save password"),
	                                      FALSE);
	purple_request_group_add_field(group, field);

	purple_request_fields(account, NULL, primary, NULL, page, _("OK"), ok_cb,
	                      _("Cancel"), cancel_cb,
	                      purple_request_cpar_from_account(account),
	                      user_data);
	g_free(primary);
}

void
purple_account_set_user_info(PurpleAccount *account, const char *user_info) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_set_str(&account->user_info, user_info)) {
		g_object_notify_by_pspec(G_OBJECT(account),
		                         properties[PROP_USER_INFO]);

		purple_accounts_schedule_save();
	}
}

void
purple_account_set_buddy_icon_path(PurpleAccount *account, const char *path) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_set_str(&account->buddy_icon_path, path)) {
		g_object_notify_by_pspec(G_OBJECT(account),
		                         properties[PROP_BUDDY_ICON_PATH]);

		purple_accounts_schedule_save();
	}
}

void
purple_account_set_protocol_id(PurpleAccount *account, const char *protocol_id)
{
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(protocol_id != NULL);

	if(g_set_str(&account->protocol_id, protocol_id)) {
		g_object_notify_by_pspec(G_OBJECT(account),
		                         properties[PROP_PROTOCOL_ID]);

		purple_accounts_schedule_save();
	}
}

void
purple_account_set_connection(PurpleAccount *account, PurpleConnection *gc) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	/* If we got the same pointer, bail. */
	if(account->gc == gc) {
		return;
	}

	/* Remove our old signal handler. */
	if(PURPLE_IS_CONNECTION(account->gc)) {
		g_signal_handlers_disconnect_by_func(account->gc,
		                                     purple_account_connection_state_cb,
		                                     account);
	}

	if(g_set_object(&account->gc, gc)) {
		GObject *obj = G_OBJECT(account);

		if(PURPLE_IS_CONNECTION(account->gc)) {
			g_signal_connect(account->gc, "notify::state",
			                 G_CALLBACK(purple_account_connection_state_cb),
			                 account);
		}

		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_CONNECTION]);
		g_object_notify_by_pspec(obj, properties[PROP_CONNECTED]);
		g_object_thaw_notify(obj);
	} else {
		/* If the set didn't work, restore our old signal. */
		if(PURPLE_IS_CONNECTION(account->gc)) {
			g_signal_connect(account->gc, "notify::state",
			                 G_CALLBACK(purple_account_connection_state_cb),
			                 account);
		}
	}
}

void
purple_account_set_remember_password(PurpleAccount *account, gboolean value) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	account->remember_pass = value;

	g_object_notify_by_pspec(G_OBJECT(account),
	                         properties[PROP_REMEMBER_PASSWORD]);

	purple_accounts_schedule_save();
}

void
purple_account_set_enabled(PurpleAccount *account, gboolean value) {
	PurpleConnection *gc;
	gboolean was_enabled = FALSE;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	was_enabled = account->enabled;
	account->enabled = value;
	if(was_enabled != value) {
		g_object_notify_by_pspec(G_OBJECT(account), properties[PROP_ENABLED]);
	}

	gc = purple_account_get_connection(account);
	if(gc != NULL && _purple_connection_wants_to_die(gc)) {
		return;
	}

	if(value && purple_presence_is_online(account->presence)) {
		purple_account_connect(account);
	} else if (!value && !purple_account_is_disconnected(account)) {
		purple_account_disconnect(account);
	}

	purple_accounts_schedule_save();
}

void
purple_account_set_proxy_info(PurpleAccount *account, PurpleProxyInfo *info) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_set_object(&account->proxy_info, info)) {
		g_object_notify_by_pspec(G_OBJECT(account),
		                         properties[PROP_PROXY_INFO]);

		purple_accounts_schedule_save();
	}
}

void
purple_account_set_int(PurpleAccount *account, const char *name, int value) {
	PurpleAccountSetting *setting;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(name != NULL);

	setting = g_new0(PurpleAccountSetting, 1);

	g_value_init(&setting->value, G_TYPE_INT);
	g_value_set_int(&setting->value, value);

	g_hash_table_insert(account->settings, g_strdup(name), setting);

	purple_account_setting_changed_emit(account, name);

	purple_accounts_schedule_save();
}

void
purple_account_set_string(PurpleAccount *account, const char *name,
                          const char *value)
{
	PurpleAccountSetting *setting;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(name != NULL);

	setting = g_new0(PurpleAccountSetting, 1);

	g_value_init(&setting->value, G_TYPE_STRING);
	g_value_set_string(&setting->value, value);

	g_hash_table_insert(account->settings, g_strdup(name), setting);

	purple_account_setting_changed_emit(account, name);

	purple_accounts_schedule_save();
}

void
purple_account_set_bool(PurpleAccount *account, const char *name,
                        gboolean value)
{
	PurpleAccountSetting *setting;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(name != NULL);

	setting = g_new0(PurpleAccountSetting, 1);

	g_value_init(&setting->value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&setting->value, value);

	g_hash_table_insert(account->settings, g_strdup(name), setting);

	purple_account_setting_changed_emit(account, name);

	purple_accounts_schedule_save();
}

gboolean
purple_account_is_connected(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	return (purple_account_get_state(account) == PURPLE_CONNECTION_STATE_CONNECTED);
}

gboolean
purple_account_is_connecting(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	return (purple_account_get_state(account) == PURPLE_CONNECTION_STATE_CONNECTING);
}

gboolean
purple_account_is_disconnected(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	return (purple_account_get_state(account) == PURPLE_CONNECTION_STATE_DISCONNECTED);
}

const char *
purple_account_get_user_info(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->user_info;
}

const char *
purple_account_get_buddy_icon_path(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->buddy_icon_path;
}

const char *
purple_account_get_protocol_id(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->protocol_id;
}

PurpleProtocol *
purple_account_get_protocol(PurpleAccount *account) {
	PurpleProtocolManager *manager = NULL;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	manager = purple_protocol_manager_get_default();
	if(manager != NULL) {
		return purple_protocol_manager_find(manager, account->protocol_id);
	}

	return NULL;
}

const char *
purple_account_get_protocol_name(PurpleAccount *account) {
	PurpleProtocol *p;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	p = purple_account_get_protocol(account);

	return (p && purple_protocol_get_name(p) ?
	        _(purple_protocol_get_name(p)) : _("Unknown"));
}

PurpleConnection *
purple_account_get_connection(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->gc;
}

gboolean
purple_account_get_remember_password(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	return account->remember_pass;
}

gboolean
purple_account_get_enabled(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	return account->enabled;
}

PurpleProxyInfo *
purple_account_get_proxy_info(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->proxy_info;
}

PurplePresence *
purple_account_get_presence(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->presence;
}

int
purple_account_get_int(PurpleAccount *account, const char *name,
                       int default_value)
{
	PurpleAccountSetting *setting;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), default_value);
	g_return_val_if_fail(name    != NULL, default_value);

	setting = g_hash_table_lookup(account->settings, name);

	if(setting == NULL) {
		return default_value;
	}

	g_return_val_if_fail(G_VALUE_HOLDS_INT(&setting->value), default_value);

	return g_value_get_int(&setting->value);
}

const char *
purple_account_get_string(PurpleAccount *account, const char *name,
                          const char *default_value)
{
	PurpleAccountSetting *setting;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), default_value);
	g_return_val_if_fail(name != NULL, default_value);

	setting = g_hash_table_lookup(account->settings, name);

	if(setting == NULL) {
		return default_value;
	}

	g_return_val_if_fail(G_VALUE_HOLDS_STRING(&setting->value), default_value);

	return g_value_get_string(&setting->value);
}

gboolean
purple_account_get_bool(PurpleAccount *account, const char *name,
                        gboolean default_value)
{
	PurpleAccountSetting *setting;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), default_value);
	g_return_val_if_fail(name != NULL, default_value);

	setting = g_hash_table_lookup(account->settings, name);

	if(setting == NULL) {
		return default_value;
	}

	g_return_val_if_fail(G_VALUE_HOLDS_BOOLEAN(&setting->value), default_value);

	return g_value_get_boolean(&setting->value);
}

void
purple_account_change_password(PurpleAccount *account, const char *orig_pw,
                               const char *new_pw)
{
	PurpleCredentialManager *manager = NULL;
	PurpleProtocol *protocol = NULL;
	PurpleConnection *gc = purple_account_get_connection(account);

	/* just going to fire and forget this for now as not many protocols even
	 * implement the change password stuff.
	 */
	manager = purple_credential_manager_get_default();
	purple_credential_manager_write_password_async(manager, account, new_pw,
	                                               NULL, NULL, NULL);

	if(gc != NULL) {
		protocol = purple_connection_get_protocol(gc);
	}

	if(PURPLE_IS_PROTOCOL_SERVER(protocol)) {
		purple_protocol_server_change_passwd(PURPLE_PROTOCOL_SERVER(protocol),
		                                     gc, orig_pw, new_pw);
	}
}

const PurpleConnectionErrorInfo *
purple_account_get_error(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return account->error;
}

void
purple_account_set_error(PurpleAccount *account,
                         PurpleConnectionErrorInfo *info)
{
	PurpleNotificationManager *manager = NULL;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(info == account->error) {
		return;
	}

	g_clear_pointer(&account->error,
	                purple_connection_error_info_free);
	account->error = info;

	manager = purple_notification_manager_get_default();

	if(PURPLE_IS_NOTIFICATION(account->error_notification)) {
		purple_notification_manager_remove(manager,
		                                   account->error_notification);
		g_clear_object(&account->error_notification);
	}

	if(info != NULL) {
		account->error_notification =
			purple_notification_new_from_connection_error(account, info);

		purple_notification_manager_add(manager, account->error_notification);
	}

	g_object_notify_by_pspec(G_OBJECT(account), properties[PROP_ERROR]);

	purple_accounts_schedule_save();
}

void
purple_account_set_require_password(PurpleAccount *account,
                                    gboolean require_password)
{
	gboolean old = FALSE;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	old = account->require_password;
	account->require_password = require_password;

	if(old != require_password) {
		g_object_notify_by_pspec(G_OBJECT(account),
		                         properties[PROP_REQUIRE_PASSWORD]);
	}
}

gboolean
purple_account_get_require_password(PurpleAccount *account) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	return account->require_password;
}

void
purple_account_freeze_notify_settings(PurpleAccount *account) {
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	G_LOCK(setting_notify_lock);
	if(account->freeze_queue == NULL) {
		account->freeze_queue = g_slice_new0(PurpleAccountSettingFreezeQueue);
	}

	account->freeze_queue->ref_count++;

	G_UNLOCK(setting_notify_lock);
}

void
purple_account_thaw_notify_settings(PurpleAccount *account) {
	GSList *names = NULL;

	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	G_LOCK(setting_notify_lock);
	if(G_UNLIKELY(account->freeze_queue->ref_count == 0)) {
		G_UNLOCK(setting_notify_lock);

		g_critical("purple_account_settings_thaw_notify called for account %s "
		           "(%s) when not frozen",
		           purple_contact_info_get_username(PURPLE_CONTACT_INFO(account)),
		           purple_account_get_protocol_id(account));

		return;
	}

	account->freeze_queue->ref_count--;
	if(account->freeze_queue->ref_count > 0) {
		G_UNLOCK(setting_notify_lock);

		return;
	}

	/* This was the last ref, so fire off the signals. */
	names = account->freeze_queue->names;
	while(names != NULL) {
		char *name = names->data;

		g_signal_emit(account, signals[SIG_SETTING_CHANGED],
		              g_quark_from_string(name), name);

		names = g_slist_delete_link(names, names);
		g_free(name);
	}
	account->freeze_queue->names = names;

	purple_account_free_notify_settings(account);

	G_UNLOCK(setting_notify_lock);
}

