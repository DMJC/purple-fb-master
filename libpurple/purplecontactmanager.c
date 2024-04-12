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

#include <birb.h>

#include "purplecontactmanager.h"

#include "purplegdkpixbuf.h"
#include "purpleprivate.h"
#include "purpleprotocolroster.h"
#include "util.h"

enum {
	PROP_0,
	PROP_ITEM_TYPE,
	PROP_N_ITEMS,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_ADDED,
	SIG_REMOVED,
	SIG_PERSON_ADDED,
	SIG_PERSON_REMOVED,
	SIG_POPULATE_MENU,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

struct _PurpleContactManager {
	GObject parent;

	GHashTable *accounts;

	GPtrArray *people;
};

static PurpleContactManager *default_manager = NULL;

/* Necessary prototype. */
static void purple_contact_manager_contact_person_changed_cb(GObject *obj,
                                                             GParamSpec *pspec,
                                                             gpointer data);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
purple_contact_manager_find_with_username_helper(gconstpointer a,
                                                 gconstpointer b)
{
	PurpleContactInfo *info_a = PURPLE_CONTACT_INFO((gpointer)a);
	PurpleContactInfo *info_b = PURPLE_CONTACT_INFO((gpointer)b);
	const gchar *username_a = NULL;
	const gchar *username_b = NULL;

	username_a = purple_contact_info_get_username(info_a);
	username_b = purple_contact_info_get_username(info_b);

	return purple_strequal(username_a, username_b);
}

static gboolean
purple_contact_manager_find_with_id_helper(gconstpointer a, gconstpointer b) {
	PurpleContactInfo *info_a = PURPLE_CONTACT_INFO((gpointer)a);
	PurpleContactInfo *info_b = PURPLE_CONTACT_INFO((gpointer)b);
	const gchar *id_a = NULL;
	const gchar *id_b = NULL;

	id_a = purple_contact_info_get_id(info_a);
	id_b = purple_contact_info_get_id(info_b);

	return purple_strequal(id_a, id_b);
}

static void
purple_contact_manager_protocol_roster_update(PurpleContact *contact) {
	PurpleAccount *account = NULL;

	account = purple_contact_get_account(contact);
	if(PURPLE_IS_ACCOUNT(account)) {
		PurpleProtocol *protocol = NULL;

		protocol = purple_account_get_protocol(account);
		if(PURPLE_IS_PROTOCOL_ROSTER(protocol)) {
			purple_protocol_roster_update_async(PURPLE_PROTOCOL_ROSTER(protocol),
			                                    account, contact, NULL, NULL,
			                                    NULL);
		}
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_contact_manager_contact_update_cb(GObject *obj,
                                         G_GNUC_UNUSED GParamSpec *pspec,
                                         G_GNUC_UNUSED gpointer data)
{
	purple_contact_manager_protocol_roster_update(PURPLE_CONTACT(obj));
}

static void
purple_contact_manager_contact_person_changed_cb(GObject *obj,
                                                 G_GNUC_UNUSED GParamSpec *pspec,
                                                 gpointer data)
{
	PurpleContact *contact = PURPLE_CONTACT(obj);
	PurpleContactManager *manager = data;
	PurplePerson *person = NULL;

	person = purple_contact_info_get_person(PURPLE_CONTACT_INFO(contact));
	/* If the person is now NULL, we leaving the existing person in place as
	 * we don't want to potentially delete user data.
	 */
	if(!PURPLE_IS_PERSON(person)) {
		return;
	}

	/* At this point the person changed or is new so we need to add the new
	 * person.
	 */
	purple_contact_manager_add_person(manager, person);

	/* Finally tell the ProtocolRoster about this change. */
	purple_contact_manager_protocol_roster_update(contact);
}

static void
purple_contact_manager_contact_populate_menu_cb(PurpleContactInfo *info,
                                                GMenu *menu,
                                                gpointer data)
{
	g_signal_emit(data, signals[SIG_POPULATE_MENU], 0, info, menu);
}

static void
purple_contact_manager_tags_changed_cb(G_GNUC_UNUSED PurpleTags *tags,
                                       G_GNUC_UNUSED const char *tag,
                                       G_GNUC_UNUSED const char *name,
                                       G_GNUC_UNUSED const char *value,
                                       gpointer data)
{
	purple_contact_manager_protocol_roster_update(data);
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_contact_manager_get_item_type(G_GNUC_UNUSED GListModel *list) {
	return PURPLE_TYPE_PERSON;
}

static guint
purple_contact_manager_get_n_items(GListModel *list) {
	PurpleContactManager *manager = PURPLE_CONTACT_MANAGER(list);

	return manager->people->len;
}

static gpointer
purple_contact_manager_get_item(GListModel *list, guint position) {
	PurpleContactManager *manager = PURPLE_CONTACT_MANAGER(list);
	PurpleContact *contact = NULL;

	if(position < manager->people->len) {
		contact = g_object_ref(g_ptr_array_index(manager->people, position));
	}

	return contact;
}

static void
pidgin_contact_manager_list_model_iface_init(GListModelInterface *iface) {
	iface->get_item_type = purple_contact_manager_get_item_type;
	iface->get_n_items = purple_contact_manager_get_n_items;
	iface->get_item = purple_contact_manager_get_item;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE_WITH_CODE(PurpleContactManager, purple_contact_manager,
                              G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                                    pidgin_contact_manager_list_model_iface_init))

static void
purple_contact_manager_dispose(GObject *obj) {
	PurpleContactManager *manager = NULL;

	manager = PURPLE_CONTACT_MANAGER(obj);

	g_hash_table_remove_all(manager->accounts);

	if(manager->people != NULL) {
		g_ptr_array_free(manager->people, TRUE);
		manager->people = NULL;
	}

	G_OBJECT_CLASS(purple_contact_manager_parent_class)->dispose(obj);
}

static void
purple_contact_manager_finalize(GObject *obj) {
	PurpleContactManager *manager = NULL;

	manager = PURPLE_CONTACT_MANAGER(obj);

	g_clear_pointer(&manager->accounts, g_hash_table_destroy);

	G_OBJECT_CLASS(purple_contact_manager_parent_class)->finalize(obj);
}

static void
purple_contact_manager_get_property(GObject *obj, guint param_id,
                                    GValue *value, GParamSpec *pspec)
{
	PurpleContactManager *manager = PURPLE_CONTACT_MANAGER(obj);

	switch(param_id) {
	case PROP_ITEM_TYPE:
		g_value_set_gtype(value,
		                  purple_contact_manager_get_item_type(G_LIST_MODEL(manager)));
		break;
	case PROP_N_ITEMS:
		g_value_set_uint(value,
		                 purple_contact_manager_get_n_items(G_LIST_MODEL(manager)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_contact_manager_init(PurpleContactManager *manager) {
	manager->accounts = g_hash_table_new_full(g_direct_hash, g_direct_equal,
	                                          g_object_unref, g_object_unref);

	/* 100 Seems like a reasonable default of the number people on your contact
	 * list. - gk 20221109
	 */
	manager->people = g_ptr_array_new_full(100,
	                                       (GDestroyNotify)g_object_unref);
}

static void
purple_contact_manager_class_init(PurpleContactManagerClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->dispose = purple_contact_manager_dispose;
	obj_class->finalize = purple_contact_manager_finalize;
	obj_class->get_property = purple_contact_manager_get_property;

	/**
	 * PurpleContactManager:item-type:
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
	 * PurpleContactManager:n-items:
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

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleContactManager::added:
	 * @manager: The instance.
	 * @contact: The [class@Purple.Contact] that was registered.
	 *
	 * Emitted after @contact has been added to @manager.
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
		PURPLE_TYPE_CONTACT);

	/**
	 * PurpleContactManager::removed:
	 * @manager: The instance.
	 * @contact: The [class@Purple.Contact] that was removed.
	 *
	 * Emitted after @contact has been removed from @manager.
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
		PURPLE_TYPE_CONTACT);

	/**
	 * PurpleContactManager::person-added:
	 * @manager: The instance.
	 * @person: The [class@Purple.Person] that was added.
	 *
	 * Emitted after @person has been added to @manager. This is typically done
	 * when a contact is added via [method@Purple.ContactManager.add] but can
	 * also happen if [method@Purple.ContactManager.add_person] is called.
	 *
	 * Since: 3.0
	 */
	signals[SIG_PERSON_ADDED] = g_signal_new_class_handler(
		"person-added",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_PERSON);

	/**
	 * PurpleContactManager::person-removed:
	 * @manager: The instance.
	 * @person: The [class@Purple.Person] that was removed.
	 *
	 * Emitted after @person has been removed from @manager. This typically
	 * happens when [method@Purple.ContactManager.remove_person] is called.
	 *
	 * Since: 3.0
	 */
	signals[SIG_PERSON_REMOVED] = g_signal_new_class_handler(
		"person-removed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_PERSON);

	/**
	 * PurpleContactManager::populate-menu:
	 * @manager: The instance.
	 * @info: The [class@ContactInfo] instance.
	 * @menu: The [class@Gio.Menu] to populate.
	 *
	 * This is a propagation of [signal@ContactInfo::populate-menu]. This means
	 * that your callback will be called for any contact info to populate its
	 * context menu.
	 *
	 * Since: 3.0
	 */
	signals[SIG_POPULATE_MENU] = g_signal_new_class_handler(
		"populate-menu",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		PURPLE_TYPE_CONTACT_INFO,
		BIRB_TYPE_ACTION_MENU);
}

/******************************************************************************
 * Private API
 *****************************************************************************/
void
purple_contact_manager_startup(void) {
	if(default_manager == NULL) {
		default_manager = g_object_new(PURPLE_TYPE_CONTACT_MANAGER, NULL);
		g_object_add_weak_pointer(G_OBJECT(default_manager),
		                          (gpointer)&default_manager);
	}
}

void
purple_contact_manager_shutdown(void) {
	g_clear_object(&default_manager);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleContactManager *
purple_contact_manager_get_default(void) {
	if(G_UNLIKELY(!PURPLE_IS_CONTACT_MANAGER(default_manager))) {
		g_warning("The default contact manager was unexpectedly NULL");
	}

	return default_manager;
}

void
purple_contact_manager_add(PurpleContactManager *manager,
                           PurpleContact *contact)
{
	PurpleAccount *account = NULL;
	GListStore *contacts = NULL;
	gboolean added = FALSE;

	g_return_if_fail(PURPLE_IS_CONTACT_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_CONTACT(contact));

	account = purple_contact_get_account(contact);
	contacts = g_hash_table_lookup(manager->accounts, account);
	if(!G_IS_LIST_STORE(contacts)) {
		contacts = g_list_store_new(PURPLE_TYPE_CONTACT);
		g_hash_table_insert(manager->accounts, g_object_ref(account), contacts);

		g_list_store_append(contacts, contact);

		added = TRUE;
	} else {
		if(g_list_store_find(contacts, contact, NULL)) {
			PurpleContactInfo *info = PURPLE_CONTACT_INFO(contact);
			const gchar *username = purple_contact_info_get_username(info);
			const gchar *id = purple_contact_info_get_id(info);

			g_warning("double add detected for contact %s:%s", id, username);

			return;
		}

		g_list_store_append(contacts, contact);
		added = TRUE;
	}

	if(added) {
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(contact);
		PurplePerson *person = purple_contact_info_get_person(info);
		PurpleTags *tags = NULL;

		/* If the contact already has a person, add the person to our list of
		 * people.
		 */
		if(PURPLE_IS_PERSON(person)) {
			purple_contact_manager_add_person(manager, person);
		}

		tags = purple_contact_info_get_tags(info);

		/* Add some notify signals to track changes. */
		g_signal_connect_object(info, "notify::alias",
		                        G_CALLBACK(purple_contact_manager_contact_update_cb),
		                        manager, 0);
		g_signal_connect_object(info, "notify::permission",
		                        G_CALLBACK(purple_contact_manager_contact_update_cb),
		                        manager, 0);
		g_signal_connect_object(info, "notify::person",
		                        G_CALLBACK(purple_contact_manager_contact_person_changed_cb),
		                        manager, 0);
		g_signal_connect_object(info, "populate-menu",
		                        G_CALLBACK(purple_contact_manager_contact_populate_menu_cb),
		                        manager, 0);

		g_signal_connect_object(tags, "added",
		                        G_CALLBACK(purple_contact_manager_tags_changed_cb),
		                        contact, 0);
		g_signal_connect_object(tags, "removed",
		                        G_CALLBACK(purple_contact_manager_tags_changed_cb),
		                        contact, 0);

		g_signal_emit(manager, signals[SIG_ADDED], 0, contact);
	}
}

gboolean
purple_contact_manager_remove(PurpleContactManager *manager,
                              PurpleContact *contact)
{
	PurpleAccount *account = NULL;
	GListStore *contacts = NULL;
	guint position = 0;

	g_return_val_if_fail(PURPLE_IS_CONTACT_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), FALSE);

	account = purple_contact_get_account(contact);
	contacts = g_hash_table_lookup(manager->accounts, account);
	if(!G_IS_LIST_STORE(contacts)) {
		return FALSE;
	}

	if(g_list_store_find(contacts, contact, &position)) {
		PurpleTags *tags = NULL;
		gboolean removed = FALSE;
		guint len = 0;

		/* Ref the contact to make sure that the instance is valid when we emit
		 * the removed signal.
		 */
		g_object_ref(contact);

		len = g_list_model_get_n_items(G_LIST_MODEL(contacts));
		g_list_store_remove(contacts, position);
		if(g_list_model_get_n_items(G_LIST_MODEL(contacts)) < len) {
			removed = TRUE;
		}

		/* Remove the signals for the contact's tags changing as we're no
		 * longer tracking the contact they belong to.
		 */
		tags = purple_contact_info_get_tags(PURPLE_CONTACT_INFO(contact));
		g_signal_handlers_disconnect_by_func(tags,
		                                     purple_contact_manager_tags_changed_cb,
		                                     contact);

		if(removed) {
			g_signal_emit(manager, signals[SIG_REMOVED], 0, contact);
		}

		g_object_unref(contact);

		return removed;
	}

	return FALSE;
}

gboolean
purple_contact_manager_remove_all(PurpleContactManager *manager,
                                  PurpleAccount *account)
{
	GListStore *contacts = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	/* If there are any contacts for this account, manually iterate them and
	 * emit the removed signal. This is more efficient than calling remove on
	 * each one individually as that would require updating the backing
	 * GListStore for each individual removal.
	 */
	contacts = g_hash_table_lookup(manager->accounts, account);
	if(G_IS_LIST_STORE(contacts)) {
		guint n_items = g_list_model_get_n_items(G_LIST_MODEL(contacts));
		for(guint i = 0; i < n_items; i++) {
			PurpleContact *contact = NULL;

			contact = g_list_model_get_item(G_LIST_MODEL(contacts), i);

			g_signal_emit(manager, signals[SIG_REMOVED], 0, contact);

			g_clear_object(&contact);
		}
	}

	return g_hash_table_remove(manager->accounts, account);
}

GListModel *
purple_contact_manager_get_all(PurpleContactManager *manager,
                               PurpleAccount *account)
{
	g_return_val_if_fail(PURPLE_IS_CONTACT_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);

	return g_hash_table_lookup(manager->accounts, account);
}

PurpleContact *
purple_contact_manager_find_with_username(PurpleContactManager *manager,
                                          PurpleAccount *account,
                                          const gchar *username)
{
	PurpleContact *needle = NULL;
	GListStore *contacts = NULL;
	guint position = 0;
	gboolean found = FALSE;

	g_return_val_if_fail(PURPLE_IS_CONTACT_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);
	g_return_val_if_fail(username != NULL, FALSE);

	contacts = g_hash_table_lookup(manager->accounts, account);
	if(!G_IS_LIST_STORE(contacts)) {
		return NULL;
	}

	needle = purple_contact_new(account, NULL);
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(needle), username);
	found = g_list_store_find_with_equal_func(contacts, needle,
	                                          purple_contact_manager_find_with_username_helper,
	                                          &position);
	g_clear_object(&needle);

	if(found) {
		return g_list_model_get_item(G_LIST_MODEL(contacts), position);
	}

	return NULL;
}

PurpleContact *
purple_contact_manager_find_with_id(PurpleContactManager *manager,
                                    PurpleAccount *account, const gchar *id)
{
	PurpleContact *needle = NULL;
	GListStore *contacts = NULL;
	guint position = 0;
	gboolean found = FALSE;

	g_return_val_if_fail(PURPLE_IS_CONTACT_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);

	contacts = g_hash_table_lookup(manager->accounts, account);
	if(!G_IS_LIST_STORE(contacts)) {
		return NULL;
	}

	needle = purple_contact_new(account, id);
	found = g_list_store_find_with_equal_func(contacts, needle,
	                                          purple_contact_manager_find_with_id_helper,
	                                          &position);
	g_clear_object(&needle);

	if(found) {
		return g_list_model_get_item(G_LIST_MODEL(contacts), position);
	}

	return NULL;
}

void
purple_contact_manager_add_person(PurpleContactManager *manager,
                                  PurplePerson *person)
{
	g_return_if_fail(PURPLE_IS_CONTACT_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_PERSON(person));

	/* If the person is already known, bail. */
	if(g_ptr_array_find(manager->people, person, NULL)) {
		return;
	}

	/* Add the person and emit our signals. */
	g_ptr_array_add(manager->people, g_object_ref(person));
	g_list_model_items_changed(G_LIST_MODEL(manager), manager->people->len - 1,
	                           0, 1);
	g_signal_emit(manager, signals[SIG_PERSON_ADDED], 0, person);
}

void
purple_contact_manager_remove_person(PurpleContactManager *manager,
                                     PurplePerson *person,
                                     gboolean remove_contacts)
{
	guint index = 0;

	g_return_if_fail(PURPLE_IS_CONTACT_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_PERSON(person));

	if(!g_ptr_array_find(manager->people, person, &index)) {
		return;
	}

	if(remove_contacts) {
		guint n = g_list_model_get_n_items(G_LIST_MODEL(person));

		for(guint i = 0; i < n; i++) {
			PurpleContact *contact = NULL;

			contact = g_list_model_get_item(G_LIST_MODEL(person), i);
			if(PURPLE_IS_CONTACT(contact)) {
				purple_contact_manager_remove(manager, contact);
				g_object_unref(contact);
			}
		}
	}

	/* Add a ref to the person, so we can emit the removed signal after it
	 * was actually removed, as our GPtrArray may be holding the last
	 * reference.
	 */
	g_object_ref(person);

	g_ptr_array_remove_index(manager->people, index);

	g_list_model_items_changed(G_LIST_MODEL(manager), index, 1, 0);

	/* Emit the removed signal and clear our temporary reference. */
	g_signal_emit(manager, signals[SIG_PERSON_REMOVED], 0, person);
	g_object_unref(person);
}

