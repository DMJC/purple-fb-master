/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
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
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include "purplepresencemanager.h"
#include "purplesavedpresenceprivate.h"
#include "util.h"

#define MANAGER_SCHEMA_ID "im.pidgin.Purple.PresenceManager"
#define PRESENCE_SCHEMA_ID "im.pidgin.Purple.SavedPresence"
#define MANAGER_PATH "/purple/presence-manager"

enum {
	PROP_0,
	PROP_FILENAME,
	PROP_ACTIVE,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_ADDED,
	SIG_REMOVED,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

struct _PurplePresenceManager {
	GObject parent;

	char *filename;
	GSettingsBackend *backend;
	GSettings *settings;

	PurpleSavedPresence *active;
	GPtrArray *presences;
};

/******************************************************************************
 * Helpers
 *****************************************************************************/
static inline char *
purple_presence_manager_path_for_presence(const char *id) {
	return g_strdup_printf("%s/presences/%s/", MANAGER_PATH, id);
}

static gboolean
purple_presence_manager_id_equal(gconstpointer a, gconstpointer b) {
	PurpleSavedPresence *presence_a = PURPLE_SAVED_PRESENCE((gpointer)a);
	PurpleSavedPresence *presence_b = PURPLE_SAVED_PRESENCE((gpointer)b);
	const char *id_a = NULL;
	const char *id_b = NULL;

	id_a = purple_saved_presence_get_id(presence_a);
	id_b = purple_saved_presence_get_id(presence_b);

	return purple_strequal(id_a, id_b);
}

static PurpleSavedPresence *
purple_presence_manager_find_with_id(PurplePresenceManager *manager,
                                     const char *id, guint *_index)
{
	PurpleSavedPresence *needle = NULL;
	PurpleSavedPresence *ret = NULL;
	gboolean found = FALSE;
	guint index = 0;

	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), NULL);
	g_return_val_if_fail(!purple_strempty(id), NULL);

	needle = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, "id", id, NULL);

	found = g_ptr_array_find_with_equal_func(manager->presences, needle,
	                                         purple_presence_manager_id_equal,
	                                         &index);

	if(found) {
		ret = g_ptr_array_index(manager->presences, index);
		if(_index) {
			*_index = index;
		}
	}

	g_clear_object(&needle);

	return ret;
}

static void
purple_presence_manager_update_presences_setting(PurplePresenceManager *manager)
{
	GStrvBuilder *builder = NULL;
	GStrv presences = NULL;

	builder = g_strv_builder_new();
	for(guint i = 0; i < manager->presences->len; i++) {
		PurpleSavedPresence *presence = NULL;
		const char *id = NULL;

		presence = g_ptr_array_index(manager->presences, i);
		id = purple_saved_presence_get_id(presence);
		g_strv_builder_add(builder, id);
	}

	presences = g_strv_builder_end(builder);
	g_settings_set_strv(manager->settings, "presences",
	                    (const char * const *)presences);
	g_strfreev(presences);
	g_strv_builder_unref(builder);
}

static inline void
purple_presence_manager_add(PurplePresenceManager *manager,
                            PurpleSavedPresence *presence)
{
	g_return_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_SAVED_PRESENCE(presence));

	g_ptr_array_add(manager->presences, g_object_ref(presence));
	purple_presence_manager_update_presences_setting(manager);

	g_signal_emit(manager, signals[SIG_ADDED], 0, presence);
	g_list_model_items_changed(G_LIST_MODEL(manager),
	                           manager->presences->len - 1, 0, 1);
}

static void
purple_presence_manager_set_filename(PurplePresenceManager *manager,
                                     const char *filename)
{
	g_return_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager));

	if(!purple_strequal(filename, manager->filename)) {
		g_free(manager->filename);
		manager->filename = g_strdup(filename);

		g_object_notify_by_pspec(G_OBJECT(manager), properties[PROP_FILENAME]);
	}
}

static void
purple_presence_manager_load_saved_presences(PurplePresenceManager *manager) {
	GStrv ids = NULL;

	ids = g_settings_get_strv(manager->settings, "presences");
	for(int i = 0; ids[i] != NULL; i++) {
		PurpleSavedPresence *presence = NULL;
		GSettings *settings = NULL;
		char *path = NULL;

		path = purple_presence_manager_path_for_presence(ids[i]);
		settings = g_settings_new_with_backend_and_path(PRESENCE_SCHEMA_ID,
		                                                manager->backend,
		                                                path);
		g_clear_pointer(&path, g_free);

		presence = g_object_new(
			PURPLE_TYPE_SAVED_PRESENCE,
			"id", ids[i],
			"settings", settings,
			NULL);

		purple_presence_manager_add(manager, presence);

		g_clear_object(&presence);
	}

	g_strfreev(ids);
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_presence_manager_get_item_type(G_GNUC_UNUSED GListModel *list) {
	return PURPLE_TYPE_SAVED_PRESENCE;
}

static guint
purple_presence_manager_get_n_items(GListModel *list) {
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(list);

	return manager->presences->len;
}

static gpointer
purple_presence_manager_get_item(GListModel *list, guint position) {
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(list);
	PurpleSavedPresence *presence = NULL;

	if(position < manager->presences->len) {
		presence = g_ptr_array_index(manager->presences, position);
		g_object_ref(presence);
	}

	return presence;
}

static void
purple_presence_manager_list_model_init(GListModelInterface *iface) {
	iface->get_item_type = purple_presence_manager_get_item_type;
	iface->get_n_items = purple_presence_manager_get_n_items;
	iface->get_item = purple_presence_manager_get_item;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_TYPE_EXTENDED(PurplePresenceManager, purple_presence_manager,
                       G_TYPE_OBJECT, G_TYPE_FLAG_FINAL,
                       G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                             purple_presence_manager_list_model_init))

static void
purple_presence_manager_constructed(GObject *obj) {
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(obj);
	char *active_id = NULL;

	G_OBJECT_CLASS(purple_presence_manager_parent_class)->constructed(obj);

	if(manager->filename == NULL) {
		manager->backend = g_memory_settings_backend_new();
	} else {
		manager->backend = g_keyfile_settings_backend_new(manager->filename,
		                                                  "/", NULL);
	}

	manager->settings = g_settings_new_with_backend(MANAGER_SCHEMA_ID,
	                                                manager->backend);

	purple_presence_manager_load_saved_presences(manager);

	active_id = g_settings_get_string(manager->settings, "active");
	if(!purple_strempty(active_id)) {
		if(!purple_presence_manager_set_active(manager, active_id)) {
			g_warning("failed to set the active saved presence to %s",
			          active_id);
		}
	}
	g_clear_pointer(&active_id, g_free);
}

static void
purple_presence_manager_finalize(GObject *obj) {
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(obj);

	g_clear_pointer(&manager->filename, g_free);

	g_clear_object(&manager->active);
	if(manager->presences != NULL) {
		g_ptr_array_free(manager->presences, TRUE);
		manager->presences = NULL;
	}

	g_clear_object(&manager->backend);
	g_clear_object(&manager->settings);

	G_OBJECT_CLASS(purple_presence_manager_parent_class)->finalize(obj);
}

static void
purple_presence_manager_get_property(GObject *obj, guint param_id,
                                     GValue *value, GParamSpec *pspec)
{
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(obj);

	switch(param_id) {
		case PROP_FILENAME:
			g_value_set_string(value,
			                   purple_presence_manager_get_filename(manager));
			break;
		case PROP_ACTIVE:
			g_value_set_object(value,
			                   purple_presence_manager_get_active(manager));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_presence_manager_set_property(GObject *obj, guint param_id,
                                     const GValue *value, GParamSpec *pspec)
{
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(obj);

	switch(param_id) {
		case PROP_FILENAME:
			purple_presence_manager_set_filename(manager,
			                                     g_value_get_string(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_presence_manager_init(PurplePresenceManager *manager) {
	manager->presences = g_ptr_array_new_full(0,
	                                          (GDestroyNotify)g_object_unref);
}

static void
purple_presence_manager_class_init(PurplePresenceManagerClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->constructed = purple_presence_manager_constructed;
	obj_class->finalize = purple_presence_manager_finalize;
	obj_class->get_property = purple_presence_manager_get_property;
	obj_class->set_property = purple_presence_manager_set_property;

	/**
	 * PurplePresenceManager:filename:
	 *
	 * The filename where settings should be stored. If this is %NULL settings
	 * will not be saved to disk.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_FILENAME] = g_param_spec_string(
		"filename", "filename",
		"The name of the file where settings should be stored.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresenceManager:active-presence:
	 *
	 * The [class@SavedPresence] that is currently active.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ACTIVE] = g_param_spec_object(
		"active-presence", "active-presence",
		"The active presence.",
		PURPLE_TYPE_SAVED_PRESENCE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurplePresenceManager::added:
	 * @self: The instance.
	 * @presence: The presence that was added.
	 *
	 * Emitted when a [class@SavedPresence] is successfully added to the
	 * manager.
	 *
	 * Since: 3.0.0
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
		PURPLE_TYPE_SAVED_PRESENCE);

	/**
	 * PurplePresenceManager::removed:
	 * @self: The instance.
	 * @presence: The presence that was removed.
	 *
	 * Emitted when a [class@SavedPresence] is successfully removed from the
	 * manager.
	 *
	 * Since: 3.0.0
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
		PURPLE_TYPE_SAVED_PRESENCE);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurplePresenceManager *
purple_presence_manager_new(const char *filename) {
	return g_object_new(PURPLE_TYPE_PRESENCE_MANAGER,
	                    "filename", filename,
	                    NULL);
}

const char *
purple_presence_manager_get_filename(PurplePresenceManager *manager) {
	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), NULL);

	return manager->filename;
}

PurpleSavedPresence *
purple_presence_manager_get_active(PurplePresenceManager *manager) {
	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), NULL);

	return manager->active;
}

gboolean
purple_presence_manager_set_active(PurplePresenceManager *manager,
                                   const char *id)
{
	PurpleSavedPresence *presence = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), FALSE);

	if(id != NULL) {
		presence = purple_presence_manager_find_with_id(manager, id, NULL);
		if(!PURPLE_IS_SAVED_PRESENCE(presence)) {
			return FALSE;
		}
	}

	if(g_set_object(&manager->active, presence)) {
		g_object_notify_by_pspec(G_OBJECT(manager), properties[PROP_ACTIVE]);

		g_settings_set_string(manager->settings, "active", id);
	}

	return TRUE;
}

PurpleSavedPresence *
purple_presence_manager_create(PurplePresenceManager *manager) {
	PurpleSavedPresence *presence = NULL;
	GSettings *settings = NULL;
	char *id = NULL;
	char *path = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), FALSE);

	id = g_uuid_string_random();
	path = purple_presence_manager_path_for_presence(id);
	settings = g_settings_new_with_backend_and_path(PRESENCE_SCHEMA_ID,
	                                                manager->backend, path);
	g_free(path);

	presence = g_object_new(
		PURPLE_TYPE_SAVED_PRESENCE,
		"id", id,
		"settings", settings,
		NULL);

	g_clear_pointer(&id, g_free);
	g_clear_object(&settings);

	purple_presence_manager_add(manager, presence);

	return presence;
}

gboolean
purple_presence_manager_remove(PurplePresenceManager *manager, const char *id)
{
	PurpleSavedPresence *presence = NULL;
	guint index;

	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), FALSE);
	g_return_val_if_fail(!purple_strempty(id), FALSE);

	presence = purple_presence_manager_find_with_id(manager, id, &index);
	if(!PURPLE_IS_SAVED_PRESENCE(presence)) {
		return FALSE;
	}

	/* Add a reference to the presence so we can emit the signal that it was
	 * removed.
	 */
	g_object_ref(presence);

	g_ptr_array_remove_index(manager->presences, index);
	purple_presence_manager_update_presences_setting(manager);
	purple_saved_presence_reset(presence);

	g_signal_emit(manager, signals[SIG_REMOVED], 0, presence);
	g_list_model_items_changed(G_LIST_MODEL(manager), index, 1, 0);

	/* Unref the presence as we are now done with it. */
	g_object_unref(presence);

	return TRUE;
}
