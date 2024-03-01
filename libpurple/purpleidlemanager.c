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

#include "purpleidlemanager.h"
#include "purpleidlemanagerprivate.h"

#include "util.h"

enum {
	PROP_0,
	PROP_TIMESTAMP,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

struct _PurpleIdleManager {
	GObject parent;

	GHashTable *sources;
	char *active_source;
};

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleIdleManager, purple_idle_manager, G_TYPE_OBJECT)

static void
purple_idle_manager_finalize(GObject *obj) {
	PurpleIdleManager *manager = PURPLE_IDLE_MANAGER(obj);

	g_clear_pointer(&manager->sources, g_hash_table_destroy);
	g_clear_pointer(&manager->active_source, g_free);

	G_OBJECT_CLASS(purple_idle_manager_parent_class)->finalize(obj);
}

static void
purple_idle_manager_get_property(GObject *obj, guint param_id,
                                 GValue *value, GParamSpec *pspec)
{
	PurpleIdleManager *manager = PURPLE_IDLE_MANAGER(obj);

	switch(param_id) {
	case PROP_TIMESTAMP:
		g_value_set_boxed(value, purple_idle_manager_get_timestamp(manager));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_idle_manager_init(PurpleIdleManager *manager) {
	manager->sources = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
	                                         (GDestroyNotify)g_date_time_unref);
}

static void
purple_idle_manager_class_init(PurpleIdleManagerClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_idle_manager_finalize;
	obj_class->get_property = purple_idle_manager_get_property;

	/**
	 * PurpleIdleManager::timestamp:
	 *
	 * The aggregate of the oldest idle timestamp of all of the sources that
	 * are known.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TIMESTAMP] = g_param_spec_boxed(
		"timestamp", "timestamp",
		"The aggregate of the oldest timestamp of all sources.",
		G_TYPE_DATE_TIME,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Private API
 *****************************************************************************/
static PurpleIdleManager *default_manager = NULL;

void
purple_idle_manager_startup(void) {
	if(!PURPLE_IS_IDLE_MANAGER(default_manager)) {
		default_manager = g_object_new(PURPLE_TYPE_IDLE_MANAGER, NULL);
		g_object_add_weak_pointer(G_OBJECT(default_manager),
		                          (gpointer *)&default_manager);
	}
}

void
purple_idle_manager_shutdown(void) {
	g_clear_object(&default_manager);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleIdleManager *
purple_idle_manager_get_default(void) {
	return default_manager;
}

gboolean
purple_idle_manager_set_source(PurpleIdleManager *manager,
                               const char *source,
                               GDateTime *timestamp)
{
	GHashTableIter iter;
	GDateTime *oldest = NULL;
	const char *new_source = NULL;
	gpointer key;
	gpointer value;

	g_return_val_if_fail(PURPLE_IS_IDLE_MANAGER(manager), FALSE);
	g_return_val_if_fail(!purple_strempty(source), FALSE);

	/* We're adding/updating a source. */
	if(timestamp != NULL) {
		g_hash_table_insert(manager->sources, g_strdup(source),
		                    g_date_time_ref(timestamp));
	} else {
		g_hash_table_remove(manager->sources, source);
	}

	g_hash_table_iter_init(&iter, manager->sources);
	while(g_hash_table_iter_next(&iter, &key, &value)) {
		/* If we don't have an oldest yet, use this value. */
		if(oldest == NULL || g_date_time_compare(value, oldest) < 0) {
			oldest = value;
			new_source = key;
		}
	}

	/* Finally check if new_source matches old source. */
	if(!purple_strequal(new_source, manager->active_source)) {
		/* We have to set the active source before emitting the property change
		 * otherwise purple_idle_manager_get_timestamp will look up the wrong
		 * value.
		 */
		g_free(manager->active_source);
		manager->active_source = g_strdup(new_source);

		g_object_notify_by_pspec(G_OBJECT(manager),
		                         properties[PROP_TIMESTAMP]);

		return TRUE;
	}

	return FALSE;
}

GDateTime *
purple_idle_manager_get_timestamp(PurpleIdleManager *manager) {
	g_return_val_if_fail(PURPLE_IS_IDLE_MANAGER(manager), NULL);

	if(manager->active_source == NULL) {
		return NULL;
	}

	return g_hash_table_lookup(manager->sources, manager->active_source);
}
