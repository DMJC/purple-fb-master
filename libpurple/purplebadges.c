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

#include <gio/gio.h>

#include "purplebadges.h"

#include "util.h"

struct _PurpleBadges {
	GObject parent;

	GPtrArray *badges;
};

enum {
	PROP_0,
	PROP_ITEM_TYPE,
	PROP_N_ITEMS,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
purple_badges_equal(gconstpointer a, gconstpointer b) {
	PurpleBadge *badge1 = (gpointer)a;
	PurpleBadge *badge2 = (gpointer)b;

	return purple_badge_equal(badge1, badge2);
}

static int
purple_badges_compare(gconstpointer a, gconstpointer b) {
	PurpleBadge *badge1 = (gpointer)a;
	PurpleBadge *badge2 = (gpointer)b;

	return purple_badge_compare(badge1, badge2);
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_badges_get_item_type(G_GNUC_UNUSED GListModel *model) {
	return PURPLE_TYPE_BADGE;
}

static guint
purple_badges_get_n_items(GListModel *model) {
	PurpleBadges *badges = PURPLE_BADGES(model);

	return badges->badges->len;
}

static gpointer
purple_badges_get_item(GListModel *model, guint position) {
	PurpleBadges *badges = PURPLE_BADGES(model);
	PurpleBadge *badge = NULL;

	if(position < badges->badges->len) {
		badge = g_ptr_array_index(badges->badges, position);

		if(PURPLE_IS_BADGE(badge)) {
			g_object_ref(badge);
		}
	}

	return badge;
}

static void
purple_badges_list_model_init(GListModelInterface *iface) {
	iface->get_item_type = purple_badges_get_item_type;
	iface->get_n_items = purple_badges_get_n_items;
	iface->get_item = purple_badges_get_item;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE_WITH_CODE(PurpleBadges, purple_badges, G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                                    purple_badges_list_model_init))

static void
purple_badges_finalize(GObject *obj) {
	PurpleBadges *badges = PURPLE_BADGES(obj);

	g_clear_pointer(&badges->badges, g_ptr_array_unref);

	G_OBJECT_CLASS(purple_badges_parent_class)->finalize(obj);
}

static void
purple_badges_get_property(GObject *obj, guint param_id, GValue *value,
                           GParamSpec *pspec)
{
	GListModel *model = G_LIST_MODEL(obj);

	switch(param_id) {
	case PROP_ITEM_TYPE:
		g_value_set_gtype(value, g_list_model_get_item_type(model));
		break;
	case PROP_N_ITEMS:
		g_value_set_uint(value, g_list_model_get_n_items(model));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_badges_init(PurpleBadges *badges) {
	badges->badges = g_ptr_array_new_full(0, g_object_unref);
}

static void
purple_badges_class_init(PurpleBadgesClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_badges_finalize;
	obj_class->get_property = purple_badges_get_property;

	/**
	 * PurpleBadges:item-type:
	 *
	 * The type of items. See [iface@Gio.ListModel.get_item_type].
	 *
	 * Since: 3.0
	 */
	properties[PROP_ITEM_TYPE] = g_param_spec_gtype(
		"item-type", NULL, NULL,
		PURPLE_TYPE_BADGE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadges:n-items:
	 *
	 * The number of items. See [iface@Gio.ListModel.get_n_items].
	 *
	 * Since: 3.0
	 */
	properties[PROP_N_ITEMS] = g_param_spec_uint(
		"n-items", NULL, NULL,
		0, G_MAXUINT, 0,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleBadges *
purple_badges_new(void) {
	return g_object_new(PURPLE_TYPE_BADGES, NULL);
}

gboolean
purple_badges_add_badge(PurpleBadges *badges, PurpleBadge *badge) {
	gboolean found = FALSE;
	guint len = 0;

	g_return_val_if_fail(PURPLE_IS_BADGES(badges), FALSE);
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), FALSE);

	found = g_ptr_array_find_with_equal_func(badges->badges, badge,
	                                         purple_badges_equal, NULL);

	if(found) {
		return FALSE;
	}

	/* Store length before adding to make the math easier to understand. */
	len = badges->badges->len;

	g_ptr_array_add(badges->badges, g_object_ref(badge));
	g_ptr_array_sort_values(badges->badges, purple_badges_compare);

	g_list_model_items_changed(G_LIST_MODEL(badges), 0, len, len + 1);

	return TRUE;
}

gboolean
purple_badges_remove_badge(PurpleBadges *badges, const char *id) {
	PurpleBadge *needle = NULL;
	gboolean found = FALSE;
	guint index = 0;

	g_return_val_if_fail(PURPLE_IS_BADGES(badges), FALSE);
	g_return_val_if_fail(!purple_strempty(id), FALSE);

	/* We only need the id, but the other values can't be null. */
	needle = purple_badge_new(id, 0, "folder-saved-search-symbolic", "🔍");

	found = g_ptr_array_find_with_equal_func(badges->badges, needle,
	                                         purple_badges_equal, &index);
	g_clear_object(&needle);

	if(!found) {
		return FALSE;
	}

	g_ptr_array_remove_index(badges->badges, index);

	g_list_model_items_changed(G_LIST_MODEL(badges), index, 1, 0);

	return TRUE;
}
