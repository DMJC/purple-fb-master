/* purple
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include <glib/gi18n-lib.h>

#include "glibcompat.h"
#include "purplerequestgroup.h"
#include "purpleprivate.h"

struct _PurpleRequestGroup {
	GObject parent;

	PurpleRequestFields *fields_list;

	char *title;

	GList *fields;
};

enum {
	PROP_0,
	PROP_TITLE,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_request_group_set_title(PurpleRequestGroup *group, const char *title) {
	g_free(group->title);
	group->title = g_strdup(title);

	g_object_notify_by_pspec(G_OBJECT(group), properties[PROP_TITLE]);
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_request_group_get_item_type(G_GNUC_UNUSED GListModel *model) {
	return PURPLE_TYPE_REQUEST_FIELD;
}

static guint
purple_request_group_get_n_items(GListModel *model) {
	PurpleRequestGroup *group = PURPLE_REQUEST_GROUP(model);

	return g_list_length(group->fields);
}

static gpointer
purple_request_group_get_item(G_GNUC_UNUSED GListModel *model, guint index) {
	PurpleRequestGroup *group = PURPLE_REQUEST_GROUP(model);

	return g_list_nth_data(group->fields, index);
}

static void
purple_request_group_list_model_init(GListModelInterface *iface) {
	iface->get_item_type = purple_request_group_get_item_type;
	iface->get_item = purple_request_group_get_item;
	iface->get_n_items = purple_request_group_get_n_items;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_TYPE_WITH_CODE(PurpleRequestGroup, purple_request_group, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                              purple_request_group_list_model_init))

static void
purple_request_group_get_property(GObject *obj, guint param_id, GValue *value,
                                  GParamSpec *pspec)
{
	PurpleRequestGroup *group = PURPLE_REQUEST_GROUP(obj);

	switch(param_id) {
		case PROP_TITLE:
			g_value_set_string(value, purple_request_group_get_title(group));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_request_group_set_property(GObject *obj, guint param_id,
                                  const GValue *value, GParamSpec *pspec)
{
	PurpleRequestGroup *group = PURPLE_REQUEST_GROUP(obj);

	switch(param_id) {
		case PROP_TITLE:
			purple_request_group_set_title(group, g_value_get_string(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_request_group_finalize(GObject *obj) {
	PurpleRequestGroup *group = PURPLE_REQUEST_GROUP(obj);

	g_free(group->title);

	g_list_free_full(group->fields, g_object_unref);

	G_OBJECT_CLASS(purple_request_group_parent_class)->finalize(obj);
}

static void
purple_request_group_init(G_GNUC_UNUSED PurpleRequestGroup *group) {
}

static void
purple_request_group_class_init(PurpleRequestGroupClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_request_group_finalize;
	obj_class->get_property = purple_request_group_get_property;
	obj_class->set_property = purple_request_group_set_property;

	/**
	 * PurpleRequestGroup:title:
	 *
	 * The title of the field group.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TITLE] = g_param_spec_string(
		"title", "title",
		"The title of the field group.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleRequestGroup *
purple_request_group_new(const char *title) {
	return g_object_new(PURPLE_TYPE_REQUEST_GROUP,
	                    "title", title,
	                    NULL);
}

void
_purple_request_group_set_field_list(PurpleRequestGroup *group,
                                     PurpleRequestFields *fields)
{
	g_return_if_fail(PURPLE_IS_REQUEST_GROUP(group));

	group->fields_list = fields;
}

void
_purple_request_group_set_field_required(PurpleRequestGroup *group,
                                         PurpleRequestField *field,
                                         gboolean required)
{
	g_return_if_fail(PURPLE_IS_REQUEST_GROUP(group));

	_purple_request_field_list_set_field_required(group->fields_list, field,
	                                              required);
}

void
_purple_request_group_set_field_validator(PurpleRequestGroup *group,
                                          PurpleRequestField *field,
                                          gboolean validator)
{
	g_return_if_fail(PURPLE_IS_REQUEST_GROUP(group));

	_purple_request_field_list_set_field_validator(group->fields_list, field,
	                                               validator);
}

void
purple_request_group_add_field(PurpleRequestGroup *group,
                               PurpleRequestField *field)
{
	guint position;

	g_return_if_fail(PURPLE_IS_REQUEST_GROUP(group));
	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	position = g_list_length(group->fields);
	group->fields = g_list_append(group->fields, field);

	if(group->fields_list != NULL) {
		_purple_request_field_list_add_field(group->fields_list, field);
	}

	_purple_request_field_set_group(field, group);

	g_list_model_items_changed(G_LIST_MODEL(group), position, 0, 1);
}

const char *
purple_request_group_get_title(PurpleRequestGroup *group)
{
	g_return_val_if_fail(PURPLE_IS_REQUEST_GROUP(group), NULL);

	return group->title;
}

GList *
purple_request_group_get_fields(PurpleRequestGroup *group)
{
	g_return_val_if_fail(PURPLE_IS_REQUEST_GROUP(group), NULL);

	return group->fields;
}

PurpleRequestFields *
purple_request_group_get_fields_list(PurpleRequestGroup *group)
{
	g_return_val_if_fail(PURPLE_IS_REQUEST_GROUP(group), NULL);

	return group->fields_list;
}
