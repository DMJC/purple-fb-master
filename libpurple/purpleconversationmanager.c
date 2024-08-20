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

#include "purpleconversationmanager.h"

#include "core.h"
#include "purplecontact.h"
#include "purpleprivate.h"
#include "purpleui.h"
#include "util.h"

enum {
	PROP_0,
	PROP_FILENAME,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_REGISTERED,
	SIG_UNREGISTERED,
	SIG_CONVERSATION_CHANGED,
	SIG_PRESENT_CONVERSATION,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

struct _PurpleConversationManager {
	GObject parent;

	char *filename;

	GHashTable *conversations;
};

static PurpleConversationManager *default_manager = NULL;

G_DEFINE_FINAL_TYPE(PurpleConversationManager, purple_conversation_manager,
                    G_TYPE_OBJECT)

typedef gboolean (*PurpleConversationManagerCompareFunc)(PurpleConversation *conversation, gpointer userdata);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_conversation_manager_set_filename(PurpleConversationManager *manager,
                                         const char *filename)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager));

	if(g_set_str(&manager->filename, filename)) {
		g_object_notify_by_pspec(G_OBJECT(manager), properties[PROP_FILENAME]);
	}
}

static gboolean
purple_conversation_has_id(PurpleConversation *conversation, gpointer data) {
	const char *needle = data;
	const char *haystack = NULL;

	if(!PURPLE_IS_CONVERSATION(conversation)) {
		return FALSE;
	}

	haystack = purple_conversation_get_id(conversation);

	return purple_strequal(needle, haystack);
}

static PurpleConversation *
purple_conversation_manager_find_internal(PurpleConversationManager *manager,
                                          PurpleAccount *account,
                                          const gchar *name,
                                          PurpleConversationManagerCompareFunc func,
                                          gpointer userdata)
{
	GHashTableIter iter;
	gpointer key;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	g_hash_table_iter_init(&iter, manager->conversations);
	while(g_hash_table_iter_next(&iter, &key, NULL)) {
		PurpleConversation *conversation = PURPLE_CONVERSATION(key);

		if(name != NULL) {
			const gchar *conv_name = purple_conversation_get_name(conversation);

			if(!purple_strequal(conv_name, name)) {
				continue;
			}
		}

		if(purple_conversation_get_account(conversation) != account) {
			continue;
		}

		if(func != NULL && !func(conversation, userdata)) {
			continue;
		}

		return conversation;
	}

	return NULL;
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_conversation_manager_conversation_changed_cb(GObject *source,
                                                    GParamSpec *pspec,
                                                    gpointer data)
{
	g_signal_emit(data, signals[SIG_CONVERSATION_CHANGED],
	              g_param_spec_get_name_quark(pspec),
	              source, pspec);
}

static void
purple_conversation_manager_present_conversation_cb(PurpleConversation *conversation,
                                                    gpointer data)
{
	g_signal_emit(data, signals[SIG_PRESENT_CONVERSATION], 0, conversation);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_conversation_manager_init(PurpleConversationManager *manager) {
	manager->conversations = g_hash_table_new_full(g_direct_hash,
	                                               g_direct_equal,
	                                               g_object_unref, NULL);
}

static void
purple_conversation_manager_finalize(GObject *obj) {
	PurpleConversationManager *manager = PURPLE_CONVERSATION_MANAGER(obj);

	g_hash_table_destroy(manager->conversations);
	g_clear_pointer(&manager->filename, g_free);

	G_OBJECT_CLASS(purple_conversation_manager_parent_class)->finalize(obj);
}

static void
purple_conversation_manager_get_property(GObject *obj, guint param_id,
                                         GValue *value, GParamSpec *pspec)
{
	PurpleConversationManager *manager = PURPLE_CONVERSATION_MANAGER(obj);

	switch(param_id) {
	case PROP_FILENAME:
		g_value_set_string(value,
		                   purple_conversation_manager_get_filename(manager));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_conversation_manager_set_property(GObject *obj, guint param_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
	PurpleConversationManager *manager = PURPLE_CONVERSATION_MANAGER(obj);

	switch(param_id) {
	case PROP_FILENAME:
		purple_conversation_manager_set_filename(manager,
		                                         g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_conversation_manager_class_init(PurpleConversationManagerClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_conversation_manager_finalize;
	obj_class->get_property = purple_conversation_manager_get_property;
	obj_class->set_property = purple_conversation_manager_set_property;

	/**
	 * PurpleConversationManager:filename:
	 *
	 * The filename that the manager should save its contents to.
	 *
	 * If this is %NULL, no serialization will be performed.
	 *
	 * Since: 3.0
	 */
	properties[PROP_FILENAME] = g_param_spec_string(
		"filename", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleConversationManager::registered:
	 * @manager: The manager.
	 * @conversation: The conversation that was registered.
	 *
	 * Emitted after @conversation has been registered with @manager.
	 *
	 * Since: 3.0
	 */
	signals[SIG_REGISTERED] = g_signal_new_class_handler(
		"registered",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_CONVERSATION);

	/**
	 * PurpleConversationManager::unregistered:
	 * @manager: The manager.
	 * @conversation: The conversation that was unregistered.
	 *
	 * Emitted after @conversation has been unregistered from @manager.
	 *
	 * Since: 3.0
	 */
	signals[SIG_UNREGISTERED] = g_signal_new_class_handler(
		"unregistered",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_CONVERSATION);

	/**
	 * PurpleConversationManager::conversation-changed:
	 * @manager: The account manager instance.
	 * @conversation: The conversation that was changed.
	 * @pspec: The [class@GObject.ParamSpec] for the property that changed.
	 *
	 * This is a propagation of the notify signal from @conversation. This
	 * means that your callback will be called for any conversation that
	 * @manager knows about.
	 *
	 * This also supports details, so you can specify the signal name as
	 * something like `conversation-changed::title` and your callback will only
	 * be called when [property@Conversation:title] has been changed.
	 *
	 * Since: 3.0
	 */
	signals[SIG_CONVERSATION_CHANGED] = g_signal_new_class_handler(
		"conversation-changed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		PURPLE_TYPE_CONVERSATION,
		G_TYPE_PARAM);

	/**
	 * PurpleConversationManager::present-conversation:
	 * @manager: The instance.
	 * @conversation: The conversation that should be presented.
	 *
	 * This is a propagation of [signal@Conversation::present]. This means that
	 * your callback will be called for any conversation that @manager knows
	 * about.
	 *
	 * Since: 3.0
	 */
	signals[SIG_PRESENT_CONVERSATION] = g_signal_new_class_handler(
		"present-conversation",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_CONVERSATION);
}

/******************************************************************************
 * Private API
 *****************************************************************************/
void
purple_conversation_manager_startup(void) {
	if(default_manager == NULL) {
		PurpleUi *ui = purple_core_get_ui();

		default_manager = purple_ui_get_conversation_manager(ui);
		if(PURPLE_IS_CONVERSATION_MANAGER(default_manager)) {
			g_object_add_weak_pointer(G_OBJECT(default_manager),
			                          (gpointer *)&default_manager);
		}
	}
}

void
purple_conversation_manager_shutdown(void) {
	g_clear_object(&default_manager);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleConversationManager *
purple_conversation_manager_new(const char *filename) {
	return g_object_new(
		PURPLE_TYPE_CONVERSATION_MANAGER,
		"filename", filename,
		NULL);
}

PurpleConversationManager *
purple_conversation_manager_get_default(void) {
	return default_manager;
}

const char *
purple_conversation_manager_get_filename(PurpleConversationManager *manager) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), NULL);

	return manager->filename;
}

gboolean
purple_conversation_manager_register(PurpleConversationManager *manager,
                                     PurpleConversation *conversation)
{
	gboolean registered = FALSE;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	/* g_hash_table_add calls the key_destroy_func if the key already exists
	 * which means we don't need to worry about the reference we're creating
	 * during the addition.
	 */
	registered = g_hash_table_add(manager->conversations,
	                              g_object_ref(conversation));

	if(registered) {
		/* Register our signals that need to be propagated. */
		g_signal_connect_object(conversation, "notify",
		                        G_CALLBACK(purple_conversation_manager_conversation_changed_cb),
		                        manager, 0);
		g_signal_connect_object(conversation, "present",
		                        G_CALLBACK(purple_conversation_manager_present_conversation_cb),
		                        manager, 0);

		/* Tell everyone about the new conversation. */
		g_signal_emit(manager, signals[SIG_REGISTERED], 0, conversation);
	}

	return registered;
}

gboolean
purple_conversation_manager_unregister(PurpleConversationManager *manager,
                                       PurpleConversation *conversation)
{
	gboolean unregistered = FALSE;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	/* Make sure we have a reference in case we need to emit signals. */
	g_object_ref(conversation);

	unregistered = g_hash_table_remove(manager->conversations, conversation);
	if(unregistered) {
		/* Disconnect all the signals we added for propagation. */
		g_signal_handlers_disconnect_by_func(conversation,
		                                     purple_conversation_manager_conversation_changed_cb,
		                                     manager);
		g_signal_handlers_disconnect_by_func(conversation,
		                                     purple_conversation_manager_present_conversation_cb,
		                                     manager);

		/* Tell everyone about the unregistered conversation. */
		g_signal_emit(manager, signals[SIG_UNREGISTERED], 0, conversation);
	}

	g_object_unref(conversation);

	return unregistered;
}

gboolean
purple_conversation_manager_is_registered(PurpleConversationManager *manager,
                                          PurpleConversation *conversation)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return g_hash_table_contains(manager->conversations, conversation);
}

void
purple_conversation_manager_foreach(PurpleConversationManager *manager,
                                    PurpleConversationManagerForeachFunc func,
                                    gpointer data)
{
	GHashTableIter iter;
	gpointer key;

	g_return_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager));
	g_return_if_fail(func != NULL);

	g_hash_table_iter_init(&iter, manager->conversations);
	while(g_hash_table_iter_next(&iter, &key, NULL)) {
		func(PURPLE_CONVERSATION(key), data);
	}
}

GList *
purple_conversation_manager_get_all(PurpleConversationManager *manager) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), NULL);

	return g_hash_table_get_keys(manager->conversations);
}


PurpleConversation *
purple_conversation_manager_find(PurpleConversationManager *manager,
                                 PurpleAccount *account, const gchar *name)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), NULL);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);
	g_return_val_if_fail(name != NULL, NULL);

	return purple_conversation_manager_find_internal(manager, account, name,
	                                                 NULL, NULL);
}

PurpleConversation *
purple_conversation_manager_find_dm(PurpleConversationManager *manager,
                                    PurpleContact *contact)
{
	PurpleAccount *contact_account = NULL;
	PurpleContactInfo *info = NULL;
	GHashTableIter iter;
	gpointer key;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), NULL);
	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	info = PURPLE_CONTACT_INFO(contact);
	contact_account = purple_contact_get_account(contact);

	g_hash_table_iter_init(&iter, manager->conversations);
	while(g_hash_table_iter_next(&iter, &key, NULL)) {
		PurpleAccount *conversation_account = NULL;
		PurpleConversation *conversation = key;
		PurpleConversationMembers *members = NULL;

		conversation_account = purple_conversation_get_account(conversation);
		if(conversation_account != contact_account) {
			continue;
		}

		if(!purple_conversation_is_dm(conversation)) {
			continue;
		}

		members = purple_conversation_get_members(conversation);
		if(purple_conversation_members_has_member(members, info, NULL)) {
			return conversation;
		}
	}

	return NULL;
}

PurpleConversation *
purple_conversation_manager_find_with_id(PurpleConversationManager *manager,
                                         PurpleAccount *account,
                                         const char *id)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MANAGER(manager), NULL);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return purple_conversation_manager_find_internal(manager, account, NULL,
	                                                 purple_conversation_has_id,
	                                                 (gpointer)id);
}
