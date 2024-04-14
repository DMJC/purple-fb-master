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

#include "purpleaccountmanager.h"
#include "purpleprivate.h"

#include "accounts.h"
#include "core.h"
#include "purpleaccount.h"
#include "util.h"

enum {
	PROP_0,
	PROP_ITEM_TYPE,
	PROP_N_ITEMS,
	PROP_ONLINE,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_ADDED,
	SIG_REMOVED,
	SIG_ACCOUNT_CHANGED,
	SIG_ACCOUNT_SETTING_CHANGED,
	SIG_ACCOUNT_CONNECTED,
	SIG_ACCOUNT_DISCONNECTED,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

struct _PurpleAccountManager {
	GObject parent;

	gboolean online;
	GPtrArray *accounts;
};

static PurpleAccountManager *default_manager = NULL;

/******************************************************************************
 * Callbacks
 *****************************************************************************/

/* We monitor the enabled property for all accounts in the manager, to
 * automatically connect or disconnect them.
 */
static void
purple_account_manager_account_enabled_cb(GObject *source,
                                          G_GNUC_UNUSED GParamSpec *pspec,
                                          gpointer data)
{
	PurpleAccount *account = PURPLE_ACCOUNT(source);
	gboolean online = FALSE;

	online = purple_account_manager_get_online(data);

	if(purple_account_get_enabled(account) && online) {
		purple_account_connect(account);
	} else {
		if(purple_account_is_connected(account)) {
			purple_account_disconnect(account);
		}
	}
}

/* This is the callback for the notify signal on accounts. It re-emits the
 * signal as coming from the manager and passes the account as a parameter to
 * the callback. It supports details as well so you can use
 * g_signal_connect(manager, "account-changed::enabled", ...) to just get
 * called when an account's enabled property changes.
 */
static void
purple_account_manager_account_notify_cb(GObject *source, GParamSpec *pspec,
                                         gpointer data)
{
	g_signal_emit(data, signals[SIG_ACCOUNT_CHANGED],
	              g_param_spec_get_name_quark(pspec),
	              source, pspec);
}

static void
purple_account_manager_account_setting_changed_cb(PurpleAccount *account,
                                                  const char *name,
                                                  gpointer data)
{
	g_signal_emit(data, signals[SIG_ACCOUNT_SETTING_CHANGED],
	              g_quark_from_string(name), account, name);
}

static void
purple_account_manager_account_connected_cb(PurpleAccount *account,
                                            gpointer data)
{
	g_signal_emit(data, signals[SIG_ACCOUNT_CONNECTED], 0, account);
}

static void
purple_account_manager_account_disconnected_cb(PurpleAccount *account,
                                               gpointer data)
{
	g_signal_emit(data, signals[SIG_ACCOUNT_DISCONNECTED], 0, account);
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_account_manager_get_item_type(G_GNUC_UNUSED GListModel *list) {
	return PURPLE_TYPE_ACCOUNT;
}

static guint
purple_account_manager_get_n_items(GListModel *list) {
	PurpleAccountManager *manager = PURPLE_ACCOUNT_MANAGER(list);

	return manager->accounts->len;
}

static gpointer
purple_account_manager_get_item(GListModel *list, guint position) {
	PurpleAccountManager *manager = PURPLE_ACCOUNT_MANAGER(list);
	PurpleAccount *account = NULL;

	if(position < manager->accounts->len) {
		account = g_ptr_array_index(manager->accounts, position);
		g_object_ref(account);
	}

	return account;
}

static void
purple_account_manager_list_model_init(GListModelInterface *iface) {
	iface->get_item_type = purple_account_manager_get_item_type;
	iface->get_n_items = purple_account_manager_get_n_items;
	iface->get_item = purple_account_manager_get_item;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE_WITH_CODE(PurpleAccountManager, purple_account_manager,
                              G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                                    purple_account_manager_list_model_init))

static void
purple_account_manager_finalize(GObject *obj) {
	PurpleAccountManager *manager = PURPLE_ACCOUNT_MANAGER(obj);

	if(manager->accounts != NULL) {
		g_ptr_array_free(manager->accounts, TRUE);
	}

	G_OBJECT_CLASS(purple_account_manager_parent_class)->finalize(obj);
}

static void
purple_account_manager_get_property(GObject *obj, guint param_id,
                                    GValue *value, GParamSpec *pspec)
{
	PurpleAccountManager *manager = PURPLE_ACCOUNT_MANAGER(obj);

	switch(param_id) {
	case PROP_ITEM_TYPE:
		g_value_set_gtype(value,
		                  purple_account_manager_get_item_type(G_LIST_MODEL(manager)));
		break;
	case PROP_N_ITEMS:
		g_value_set_uint(value,
		                 purple_account_manager_get_n_items(G_LIST_MODEL(manager)));
		break;
	case PROP_ONLINE:
		g_value_set_boolean(value, purple_account_manager_get_online(manager));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_account_manager_set_property(GObject *obj, guint param_id,
                                    const GValue *value, GParamSpec *pspec)
{
	PurpleAccountManager *manager = PURPLE_ACCOUNT_MANAGER(obj);

	switch(param_id) {
	case PROP_ONLINE:
		purple_account_manager_set_online(manager, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_account_manager_init(PurpleAccountManager *manager) {
	manager->online = FALSE;

	manager->accounts = g_ptr_array_new_full(0, (GDestroyNotify)g_object_unref);
}

static void
purple_account_manager_class_init(PurpleAccountManagerClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_account_manager_finalize;
	obj_class->get_property = purple_account_manager_get_property;
	obj_class->set_property = purple_account_manager_set_property;

	/**
	 * PurpleAccountManager:item-type:
	 *
	 * The type of items. See [iface@Gio.ListModel.get_item_type].
	 *
	 * Since: 3.0
	 */
	properties[PROP_ITEM_TYPE] = g_param_spec_gtype(
		"item-type", "item-type",
		"The type of the contained items.",
		G_TYPE_OBJECT,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccountManager:n-items:
	 *
	 * The number of items. See [iface@Gio.ListModel.get_n_items].
	 *
	 * Since: 3.0
	 */
	properties[PROP_N_ITEMS] = g_param_spec_uint(
		"n-items", "n-items",
		"The number of contained items.",
		0, G_MAXUINT, 0,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAccountManager:online:
	 *
	 * Controls whether or not accounts that the manager knows about should be
	 * online or offline.
	 *
	 * While this is set to %TRUE, any account added via
	 * [method@AccountManager.add] that has [property@Account:enabled] set to
	 * %TRUE, will automatically have [method@Account.connect] called on it.
	 *
	 * When this property changes, all accounts that the manager knows about
	 * with [property@Account:enabled] set to %TRUE will have
	 * [method@Account.connect] or [method@Account.disconnect] called on them
	 * to keep their state synchronized.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ONLINE] = g_param_spec_boolean(
		"online", "online",
		"Whether or not known accounts are online or offline.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleAccountManager::added:
	 * @manager: The account manager instance.
	 * @account: The account that was added.
	 *
	 * Emitted after @account was added to @manager.
	 *
	 * Since: 3.0
	 */
	signals[SIG_ADDED] = g_signal_new_class_handler(
		"added",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_ACCOUNT);

	/**
	 * PurpleAccountManager::removed:
	 * @manager: The account manager instance.
	 * @account: The account that was removed.
	 *
	 * Emitted after @account was removed from @manager.
	 *
	 * Since: 3.0
	 */
	signals[SIG_REMOVED] = g_signal_new_class_handler(
		"removed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_ACCOUNT);

	/**
	 * PurpleAccountManager::account-changed:
	 * @manager: The account manager instance.
	 * @account: The account that was changed.
	 * @pspec: The [class@GObject.ParamSpec] for the property that changed.
	 *
	 * This is a propagation of the notify signal from @account. This means
	 * that your callback will be called for any account that @manager knows
	 * about.
	 *
	 * This also supports details, so you can specify the signal name as
	 * something like `account-changed::enabled` and your callback will only
	 * be called when the enabled property of @account has been changed.
	 *
	 * Since: 3.0
	 */
	signals[SIG_ACCOUNT_CHANGED] = g_signal_new_class_handler(
		"account-changed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		PURPLE_TYPE_ACCOUNT,
		G_TYPE_PARAM);

	/**
	 * PurpleAccountManager::account-setting-changed:
	 * @manager: The account manager instance.
	 * @account: The account that was changed.
	 * @name: The name of the setting that was changed.
	 *
	 * This is a propagation of [signal@Purple.Account::setting-changed]
	 * signal. This means that your callback will be called for any account
	 * that @manager knows about.
	 *
	 * This also supports details, so you can specify the signal name as
	 * something like `account-setting-changed::use-tls` and your callback will
	 * only be called when the `use-tls` setting of @account has been changed.
	 *
	 * Since: 3.0
	 */
	signals[SIG_ACCOUNT_SETTING_CHANGED] = g_signal_new_class_handler(
		"account-setting-changed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		PURPLE_TYPE_ACCOUNT,
		G_TYPE_STRING);

	/**
	 * PurpleAccountManager::account-connected:
	 * @manager: The account manager instance.
	 * @account: The account that was connected.
	 *
	 * This is a propagation of [signal@Purple.Account::connected] signal. This
	 * means that your callback will be called for any account that @manager
	 * knows about.
	 *
	 * Since: 3.0
	 */
	signals[SIG_ACCOUNT_CONNECTED] = g_signal_new_class_handler(
		"account-connected",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_ACCOUNT);

	/**
	 * PurpleAccountManager::account-disconnected:
	 * @manager: The account manager instance.
	 * @account: The account that was disconnected.
	 *
	 * This is a propagation of [signal@Purple.Account::disconnected] signal.
	 * This means that your callback will be called for any account that
	 * @manager knows about.
	 *
	 * Since: 3.0
	 */
	signals[SIG_ACCOUNT_DISCONNECTED] = g_signal_new_class_handler(
		"account-disconnected",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_ACCOUNT);
}

/******************************************************************************
 * Private API
 *****************************************************************************/
void
purple_account_manager_startup(void) {
	if(!PURPLE_IS_ACCOUNT_MANAGER(default_manager)) {
		default_manager = g_object_new(PURPLE_TYPE_ACCOUNT_MANAGER, NULL);
		g_object_add_weak_pointer(G_OBJECT(default_manager),
		                          (gpointer *)&default_manager);
	}
}

void
purple_account_manager_shutdown(void) {
	g_clear_object(&default_manager);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleAccountManager *
purple_account_manager_get_default(void) {
	return default_manager;
}

GListModel *
purple_account_manager_get_default_as_model(void) {
	if(PURPLE_IS_ACCOUNT_MANAGER(default_manager)) {
		return G_LIST_MODEL(default_manager);
	}

	return NULL;
}

void
purple_account_manager_add(PurpleAccountManager *manager,
                           PurpleAccount *account)
{
	g_return_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	/* If the manager already knows about the account, we do nothing. */
	if(g_ptr_array_find(manager->accounts, account, NULL)) {
		return;
	}

	/* Since the manager doesn't know about the account, put the new account
	 * at the start of the list as that's likely to be the first one in user
	 * interfaces and the most likely to have configuration issues as it's a
	 * new account.
	 */
	g_ptr_array_insert(manager->accounts, 0, g_object_ref(account));

	g_signal_connect_object(account, "notify::enabled",
	                        G_CALLBACK(purple_account_manager_account_enabled_cb),
	                        manager, 0);

	/* Connect to the signals of the account that we want to propagate. */
	g_signal_connect_object(account, "notify",
	                        G_CALLBACK(purple_account_manager_account_notify_cb),
	                        manager, 0);
	g_signal_connect_object(account, "setting-changed",
	                        G_CALLBACK(purple_account_manager_account_setting_changed_cb),
	                        manager, 0);
	g_signal_connect_object(account, "connected",
	                        G_CALLBACK(purple_account_manager_account_connected_cb),
	                        manager, 0);
	g_signal_connect_object(account, "disconnected",
	                        G_CALLBACK(purple_account_manager_account_disconnected_cb),
	                        manager, 0);

	purple_accounts_schedule_save();

	g_signal_emit(manager, signals[SIG_ADDED], 0, account);
	g_list_model_items_changed(G_LIST_MODEL(manager), 0, 0, 1);

	if(manager->online && purple_account_get_enabled(account)) {
		purple_account_connect(account);
	}
}

void
purple_account_manager_remove(PurpleAccountManager *manager,
                              PurpleAccount *account)
{
	guint index = 0;

	g_return_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_ptr_array_find(manager->accounts, account, &index)) {
		g_ptr_array_steal_index(manager->accounts, index);
		g_list_model_items_changed(G_LIST_MODEL(manager), index, 1, 0);
	}

	/* Disconnect all the signals we added for the account. */
	g_signal_handlers_disconnect_by_func(account,
	                                     purple_account_manager_account_enabled_cb,
	                                     manager);
	g_signal_handlers_disconnect_by_func(account,
	                                     purple_account_manager_account_notify_cb,
	                                     manager);
	g_signal_handlers_disconnect_by_func(account,
	                                     purple_account_manager_account_setting_changed_cb,
	                                     manager);
	g_signal_handlers_disconnect_by_func(account,
	                                     purple_account_manager_account_connected_cb,
	                                     manager);
	g_signal_handlers_disconnect_by_func(account,
	                                     purple_account_manager_account_disconnected_cb,
	                                     manager);

	/* Save the list. */
	purple_accounts_schedule_save();

	/* Clearing the error ensures that account-error-changed is emitted,
	 * which is the end of the guarantee that the error's pointer is valid.
	 */
	purple_account_set_error(account, NULL);

	g_signal_emit(manager, signals[SIG_REMOVED], 0, account);

	/* Since we stole the index from the GPtrArray, we need to unref it
	 * ourselves.
	 */
	g_object_unref(account);
}

GList *
purple_account_manager_get_enabled(PurpleAccountManager *manager) {
	GList *enabled = NULL;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager), NULL);

	for(guint index = 0; index < manager->accounts->len; index++) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);

		if(purple_account_get_enabled(account)) {
			enabled = g_list_append(enabled, account);
		}
	}

	return enabled;
}

GList *
purple_account_manager_get_disabled(PurpleAccountManager *manager) {
	GList *disabled = NULL;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager), NULL);

	for(guint index = 0; index < manager->accounts->len; index++) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);

		if(!purple_account_get_enabled(account)) {
			disabled = g_list_append(disabled, account);
		}
	}

	return disabled;
}

GList *
purple_account_manager_get_connected(PurpleAccountManager *manager) {
	GList *connected = NULL;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager), NULL);

	for(guint index = 0; index < manager->accounts->len; index++) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);
		PurpleConnection *connection = NULL;

		connection = purple_account_get_connection(account);
		if(PURPLE_IS_CONNECTION(connection) &&
		   PURPLE_CONNECTION_IS_CONNECTED(connection))
		{
			connected = g_list_append(connected, account);
		}
	}

	return connected;
}

void
purple_account_manager_reorder(PurpleAccountManager *manager,
                               PurpleAccount *account,
                               guint new_index)
{
	guint index = 0;

	g_return_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_ptr_array_find(manager->accounts, account, &index)) {
		g_ptr_array_steal_index(manager->accounts, index);
		g_list_model_items_changed(G_LIST_MODEL(manager), index, 1, 0);

		/* If new_index is greater than the current index, we need to
		 * decrement new_index by 1 to account for the move as we'll be
		 * inserting into a list with one less item.
		 */
		if(new_index > index) {
			new_index--;
		}
	} else {
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(account);

		g_critical("Unregistered account (%s) found during reorder!",
		           purple_contact_info_get_username(info));

		return;
	}

	/* Insert the account into its new position. */
	g_ptr_array_insert(manager->accounts, new_index, account);
	g_list_model_items_changed(G_LIST_MODEL(manager), new_index, 0, 1);

	purple_accounts_schedule_save();
}

PurpleAccount *
purple_account_manager_find_by_id(PurpleAccountManager *manager,
                                  const gchar *id)
{
	g_return_val_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager), NULL);
	g_return_val_if_fail(id != NULL, NULL);

	for(guint index = 0; index < manager->accounts->len; index++) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(account);

		if(purple_strequal(purple_contact_info_get_id(info), id)) {
			return g_object_ref(account);
		}
	}

	return NULL;
}

PurpleAccount *
purple_account_manager_find(PurpleAccountManager *manager,
                            const gchar *username, const gchar *protocol_id)
{
	g_return_val_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager), NULL);
	g_return_val_if_fail(username != NULL, NULL);
	g_return_val_if_fail(protocol_id != NULL, NULL);

	for(guint index = 0; index < manager->accounts->len; index++) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(account);
		gchar *normalized = NULL;
		const gchar *existing_protocol_id = NULL;
		const gchar *existing_username = NULL;
		const gchar *existing_normalized = NULL;

		/* Check if the protocol id matches what the user asked for. */
		existing_protocol_id = purple_account_get_protocol_id(account);
		if(!purple_strequal(existing_protocol_id, protocol_id)) {
			continue;
		}

		/* Finally verify the username. */
		existing_username = purple_contact_info_get_username(info);
		normalized = g_strdup(purple_normalize(account, username));
		existing_normalized = purple_normalize(account, existing_username);

		if(purple_strequal(existing_normalized, normalized)) {
			g_free(normalized);

			return g_object_ref(account);
		}
		g_free(normalized);
	}

	return NULL;
}

PurpleAccount *
purple_account_manager_find_custom(PurpleAccountManager *manager,
                                   GEqualFunc func, gconstpointer data)
{
	guint index = 0;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager), NULL);
	g_return_val_if_fail(func != NULL, NULL);

	if(g_ptr_array_find_with_equal_func(manager->accounts, data, func, &index)) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);

		return g_object_ref(account);
	}

	return NULL;
}

void
purple_account_manager_foreach(PurpleAccountManager *manager,
                               PurpleAccountManagerForeachFunc callback,
                               gpointer data)
{
	g_return_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager));
	g_return_if_fail(callback != NULL);

	for(guint index = 0; index < manager->accounts->len; index++) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);
		callback(account, data);
	}
}

gboolean
purple_account_manager_get_online(PurpleAccountManager *manager) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager), FALSE);

	return manager->online;
}

void
purple_account_manager_set_online(PurpleAccountManager *manager,
                                  gboolean online)
{
	g_return_if_fail(PURPLE_IS_ACCOUNT_MANAGER(manager));

	if(manager->online == online) {
		return;
	}

	manager->online = online;

	for(guint index = 0; index < manager->accounts->len; index++) {
		PurpleAccount *account = g_ptr_array_index(manager->accounts, index);
		PurplePresence *presence = NULL;
		gboolean enabled = FALSE;
		gboolean online = FALSE;

		enabled = purple_account_get_enabled(account);
		if(!enabled) {
			continue;
		}

		if(manager->online) {
			if(!purple_account_is_connected(account) &&
			   !purple_account_is_connecting(account))
			{
				presence = purple_account_get_presence(account);
				online = purple_presence_is_online(presence);

				if(!online) {
					purple_account_connect(account);
				}
			}
		} else {
			if(!purple_account_is_disconnected(account) &&
			   !purple_account_is_disconnecting(account))
			{
				purple_account_disconnect(account);
			}
		}
	}

	g_object_notify_by_pspec(G_OBJECT(manager), properties[PROP_ONLINE]);
}
