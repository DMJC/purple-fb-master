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
#include <glib/gstdio.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include "purplepresencemanager.h"
#include "purplepresencemanagerprivate.h"

#include "core.h"
#include "purpleui.h"
#include "util.h"

#define MANAGER_SCHEMA_ID "im.pidgin.Purple.PresenceManager"
#define SAVED_PRESENCE_SCHEMA_ID "im.pidgin.Purple.SavedPresence"

enum {
	PROP_0,
	PROP_ITEM_TYPE,
	PROP_N_ITEMS,
	PROP_PATH,
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

	char *path;
	GSettingsBackend *backend;
	GSettings *settings;

	PurpleSavedPresence *active;
	GPtrArray *presences;
};

static PurplePresenceManager *default_manager = NULL;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static inline char *
purple_presence_manager_get_path_for_id(PurplePresenceManager *manager,
                                        const char *id)
{
	char *filename = NULL;
	char *basename = NULL;

	basename = g_strdup_printf("%s.ini", id);
	filename = g_build_filename(manager->path, basename, NULL);
	g_clear_pointer(&basename, g_free);

	return filename;
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

static PurpleSavedPresence *
purple_presence_manager_add(PurplePresenceManager *manager, const char *id) {
	PurpleSavedPresence *presence = NULL;
	GSettingsBackend *backend = NULL;
	GSettings *settings = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), NULL);
	g_return_val_if_fail(!purple_strempty(id), NULL);

	/* Figure out which settings backend to use. */
	if(!purple_strempty(manager->path)) {
		char *filename = NULL;

		filename = purple_presence_manager_get_path_for_id(manager, id);
		backend = g_keyfile_settings_backend_new(filename, "/", NULL);
		g_clear_pointer(&filename, g_free);
	} else {
		backend = g_memory_settings_backend_new();
	}

	/* Create the settings object with the determined backend. */
	settings = g_settings_new_with_backend(SAVED_PRESENCE_SCHEMA_ID, backend);
	g_clear_object(&backend);

	/* Create the presence. */
	presence = g_object_new(
		PURPLE_TYPE_SAVED_PRESENCE,
		"id", id,
		"settings", settings,
		NULL);

	g_ptr_array_add(manager->presences, g_object_ref(presence));

	g_signal_emit(manager, signals[SIG_ADDED], 0, presence);
	g_list_model_items_changed(G_LIST_MODEL(manager),
	                           manager->presences->len - 1, 0, 1);

	return presence;
}

static void
purple_presence_manager_set_path(PurplePresenceManager *manager,
                                 const char *path)
{
	g_return_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager));

	if(g_set_str(&manager->path, path)) {
		g_object_notify_by_pspec(G_OBJECT(manager), properties[PROP_PATH]);
	}
}

static void
purple_presence_manager_load_saved_presences(PurplePresenceManager *manager) {
	PurpleSavedPresence *presence = NULL;
	const char *id = NULL;

	/* Load the presences from disk. */
	if(manager->path != NULL) {
		GDir *dir = NULL;
		GError *error = NULL;
		const char *basename = NULL;

		dir = g_dir_open(manager->path, 0, &error);
		if(error != NULL) {
			g_warning("failed to open directory '%s': %s", manager->path,
			          error->message);
		} else {
			GPatternSpec *pattern = NULL;

			/* We use ?* to make sure we have at least one character before the
			 * .ini.
			 */
			pattern = g_pattern_spec_new("?*.ini");

			while((basename = g_dir_read_name(dir)) != NULL) {
				if(purple_strequal(basename, "manager.ini")) {
					continue;
				}

				if(g_pattern_spec_match_string(pattern, basename)) {
					PurpleSavedPresence *presence = NULL;
					char *id = NULL;

					id = g_strndup(basename, strlen(basename) - 4);
					presence = purple_presence_manager_add(manager, id);
					g_clear_pointer(&id, g_free);
					g_clear_object(&presence);
				}
			}
			g_dir_close(dir);

			g_clear_pointer(&pattern, g_pattern_spec_free);
		}
	}

	/* Make sure we have our default available presence. */
	id = "ffffffff-ffff-ffff-ffff-ffffffffffff";
	presence = purple_presence_manager_find_with_id(manager, id, NULL);
	if(!PURPLE_IS_SAVED_PRESENCE(presence)) {
		presence = purple_presence_manager_add(manager, id);
		purple_saved_presence_set_name(presence, _("Available"));
		purple_saved_presence_set_primitive(presence,
		                                    PURPLE_PRESENCE_PRIMITIVE_AVAILABLE);
		g_clear_object(&presence);
	}

	/* Make sure we have our default offline presence as well. */
	id = "00000000-0000-0000-0000-000000000000";
	presence = purple_presence_manager_find_with_id(manager, id, NULL);
	if(!PURPLE_IS_SAVED_PRESENCE(presence)) {
		presence = purple_presence_manager_add(manager, id);
		purple_saved_presence_set_name(presence, _("Offline"));
		purple_saved_presence_set_primitive(presence,
		                                    PURPLE_PRESENCE_PRIMITIVE_OFFLINE);
		g_clear_object(&presence);
	}
}

/******************************************************************************
 * GSettings Mappings
 *****************************************************************************/
static gboolean
purple_presence_manager_get_active_mapping(GValue *value, GVariant *variant,
                                           gpointer data)
{
	PurplePresenceManager *manager = data;
	PurpleSavedPresence *presence = NULL;
	const char *id = NULL;

	/* Get the id of the saved presence from gsettings. */
	id = g_variant_get_string(variant, NULL);

	presence = purple_presence_manager_find_with_id(manager, id, NULL);
	if(PURPLE_IS_SAVED_PRESENCE(presence)) {
		g_value_set_object(value, G_OBJECT(presence));

		return TRUE;
	}

	return FALSE;
}

static GVariant *
purple_presence_manager_set_active_mapping(const GValue *value,
                                           const GVariantType *expected_type,
                                           G_GNUC_UNUSED gpointer data)
{
	PurpleSavedPresence *presence = NULL;
	const char *id = NULL;

	if(!g_variant_type_equal(expected_type, G_VARIANT_TYPE_STRING)) {
		return NULL;
	}

	presence = g_value_get_object(value);
	if(!PURPLE_IS_SAVED_PRESENCE(presence)) {
		return NULL;
	}

	id = purple_saved_presence_get_id(presence);

	return g_variant_new_string(id);
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
G_DEFINE_FINAL_TYPE_WITH_CODE(PurplePresenceManager, purple_presence_manager,
                              G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                                    purple_presence_manager_list_model_init))

static void
purple_presence_manager_constructed(GObject *obj) {
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(obj);

	G_OBJECT_CLASS(purple_presence_manager_parent_class)->constructed(obj);

	if(manager->path == NULL) {
		manager->backend = g_memory_settings_backend_new();
	} else {
		char *filename = NULL;

		filename = g_build_filename(manager->path, "manager.ini", NULL);
		manager->backend = g_keyfile_settings_backend_new(filename, "/", NULL);
		g_clear_pointer(&filename, g_free);
	}

	manager->settings = g_settings_new_with_backend(MANAGER_SCHEMA_ID,
	                                                manager->backend);

	purple_presence_manager_load_saved_presences(manager);

	g_settings_bind_with_mapping(manager->settings, "active", manager,
	                             "active-presence", G_SETTINGS_BIND_DEFAULT,
	                             purple_presence_manager_get_active_mapping,
	                             purple_presence_manager_set_active_mapping,
	                             manager, NULL);
}

static void
purple_presence_manager_finalize(GObject *obj) {
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(obj);

	g_clear_pointer(&manager->path, g_free);

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
	case PROP_ITEM_TYPE:
		g_value_set_gtype(value,
		                  purple_presence_manager_get_item_type(G_LIST_MODEL(manager)));
		break;
	case PROP_N_ITEMS:
		g_value_set_uint(value,
		                 purple_presence_manager_get_n_items(G_LIST_MODEL(manager)));
		break;
	case PROP_PATH:
		g_value_set_string(value, purple_presence_manager_get_path(manager));
		break;
	case PROP_ACTIVE:
		g_value_set_object(value, purple_presence_manager_get_active(manager));
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
	case PROP_PATH:
		purple_presence_manager_set_path(manager,
		                                 g_value_get_string(value));
		break;
	case PROP_ACTIVE:
		purple_presence_manager_set_active(manager,
		                                   g_value_get_object(value));
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
	 * PurplePresenceManager:item-type:
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
	 * PurplePresenceManager:n-items:
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
	 * PurplePresenceManager:path:
	 *
	 * The directory path where settings should be stored. If this is %NULL
	 * settings will not be saved to disk.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PATH] = g_param_spec_string(
		"path", "path",
		"The directory path where settings should be stored.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresenceManager:active-presence:
	 *
	 * The [class@SavedPresence] that is currently active.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ACTIVE] = g_param_spec_object(
		"active-presence", "active-presence",
		"The active presence.",
		PURPLE_TYPE_SAVED_PRESENCE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurplePresenceManager::added:
	 * @self: The instance.
	 * @presence: The presence that was added.
	 *
	 * Emitted when a [class@SavedPresence] is successfully added to the
	 * manager.
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
		PURPLE_TYPE_SAVED_PRESENCE);

	/**
	 * PurplePresenceManager::removed:
	 * @self: The instance.
	 * @presence: The presence that was removed.
	 *
	 * Emitted when a [class@SavedPresence] is successfully removed from the
	 * manager.
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
		PURPLE_TYPE_SAVED_PRESENCE);
}

/******************************************************************************
 * Private API
 *****************************************************************************/
void
purple_presence_manager_startup(void) {
	if(default_manager == NULL) {
		PurpleUi *ui = purple_core_get_ui();

		default_manager = purple_ui_get_presence_manager(ui);
		if(PURPLE_IS_PRESENCE_MANAGER(default_manager)) {
			g_object_add_weak_pointer(G_OBJECT(default_manager),
			                          (gpointer *)&default_manager);
		}
	}
}

void
purple_presence_manager_shutdown(void) {
	g_clear_object(&default_manager);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurplePresenceManager *
purple_presence_manager_get_default(void) {
	return default_manager;
}

GListModel *
purple_presence_manager_get_default_as_model(void) {
	if(PURPLE_IS_PRESENCE_MANAGER(default_manager)) {
		return G_LIST_MODEL(default_manager);
	}

	return NULL;
}

PurplePresenceManager *
purple_presence_manager_new(const char *path) {
	return g_object_new(PURPLE_TYPE_PRESENCE_MANAGER,
	                    "path", path,
	                    NULL);
}

const char *
purple_presence_manager_get_path(PurplePresenceManager *manager) {
	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), NULL);

	return manager->path;
}

PurpleSavedPresence *
purple_presence_manager_get_active(PurplePresenceManager *manager) {
	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), NULL);

	return manager->active;
}

gboolean
purple_presence_manager_set_active(PurplePresenceManager *manager,
                                   PurpleSavedPresence *presence)
{
	PurpleSavedPresence *found = NULL;
	const char *id = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(presence), FALSE);

	/* We need to make sure the manager knows about the passed in presence. */
	id = purple_saved_presence_get_id(presence);
	found = purple_presence_manager_find_with_id(manager, id, NULL);
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(found), FALSE);

	if(g_set_object(&manager->active, presence)) {
		g_object_notify_by_pspec(G_OBJECT(manager), properties[PROP_ACTIVE]);
	}

	return TRUE;
}

PurpleSavedPresence *
purple_presence_manager_create(PurplePresenceManager *manager) {
	PurpleSavedPresence *presence = NULL;
	char *id = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE_MANAGER(manager), FALSE);

	id = g_uuid_string_random();
	presence = purple_presence_manager_add(manager, id);
	g_free(id);

	return presence;
}

gboolean
purple_presence_manager_remove(PurplePresenceManager *manager, const char *id)
{
	PurpleSavedPresence *presence = NULL;
	char *filename = NULL;
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

	filename = purple_presence_manager_get_path_for_id(manager, id);
	g_remove(filename);
	g_clear_pointer(&filename, g_free);

	g_signal_emit(manager, signals[SIG_REMOVED], 0, presence);
	g_list_model_items_changed(G_LIST_MODEL(manager), index, 1, 0);

	/* Unref the presence as we are now done with it. */
	g_object_unref(presence);

	return TRUE;
}
