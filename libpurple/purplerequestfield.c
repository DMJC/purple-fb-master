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
#include "request.h"
#include "debug.h"
#include "purpleaccountmanager.h"
#include "purplekeyvaluepair.h"
#include "purpleprivate.h"

typedef struct {
	PurpleRequestFieldType type;
	PurpleRequestFieldGroup *group;

	char *id;
	char *label;
	char *type_hint;

	gboolean visible;
	gboolean required;
	gboolean sensitive;

	union {
		struct {
			gboolean multiline;
			gboolean masked;
			char *default_value;
			char *value;
		} string;

		struct {
			int default_value;
			int value;
			int lower_bound;
			int upper_bound;
		} integer;

		struct {
			gboolean default_value;
			gboolean value;
		} boolean;

		struct {
			gpointer default_value;
			gpointer value;

			GList *elements;
		} choice;

		struct {
			GList *items;
			gboolean has_icons;
			GHashTable *item_data;
			GList *selected;
			GHashTable *selected_table;

			gboolean multiple_selection;
		} list;

		struct {
			PurpleAccount *default_account;
			PurpleAccount *account;
			gboolean show_all;

			PurpleFilterAccountFunc filter_func;
		} account;

		struct {
			unsigned int scale_x;
			unsigned int scale_y;
			char *buffer;
			gsize size;
		} image;

		struct {
			PurpleRequestDatasheet *sheet;
		} datasheet;
	} u;

	void *ui_data;
	char *tooltip;

	PurpleRequestFieldValidator validator;
	void *validator_data;
} PurpleRequestFieldPrivate;

enum {
	PROP_0,
	PROP_ID,
	PROP_LABEL,
	PROP_VISIBLE,
	PROP_SENSITIVE,
	PROP_TYPE_HINT,
	PROP_TOOLTIP,
	PROP_REQUIRED,
	PROP_IS_VALIDATABLE,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(PurpleRequestField, purple_request_field, G_TYPE_OBJECT)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_request_field_set_id(PurpleRequestField *field, const char *id) {
	PurpleRequestFieldPrivate *priv = NULL;

	priv = purple_request_field_get_instance_private(field);

	g_free(priv->id);
	priv->id = g_strdup(id);

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_ID]);
}

static void
purple_request_field_set_field_type(PurpleRequestField *field,
                                    PurpleRequestFieldType type)
{
	PurpleRequestFieldPrivate *priv = NULL;

	priv = purple_request_field_get_instance_private(field);

	priv->type = type;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_request_field_get_property(GObject *obj, guint param_id, GValue *value,
                                  GParamSpec *pspec)
{
	PurpleRequestField *field = PURPLE_REQUEST_FIELD(obj);

	switch(param_id) {
		case PROP_ID:
			g_value_set_string(value, purple_request_field_get_id(field));
			break;
		case PROP_LABEL:
			g_value_set_string(value, purple_request_field_get_label(field));
			break;
		case PROP_VISIBLE:
			g_value_set_boolean(value, purple_request_field_is_visible(field));
			break;
		case PROP_SENSITIVE:
			g_value_set_boolean(value,
			                    purple_request_field_is_sensitive(field));
			break;
		case PROP_TYPE_HINT:
			g_value_set_string(value,
			                   purple_request_field_get_type_hint(field));
			break;
		case PROP_TOOLTIP:
			g_value_set_string(value, purple_request_field_get_tooltip(field));
			break;
		case PROP_REQUIRED:
			g_value_set_boolean(value,
			                    purple_request_field_is_required(field));
			break;
		case PROP_IS_VALIDATABLE:
			g_value_set_boolean(value,
			                    purple_request_field_is_validatable(field));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_request_field_set_property(GObject *obj, guint param_id,
                                  const GValue *value, GParamSpec *pspec)
{
	PurpleRequestField *field = PURPLE_REQUEST_FIELD(obj);

	switch(param_id) {
		case PROP_ID:
			purple_request_field_set_id(field, g_value_get_string(value));
			break;
		case PROP_LABEL:
			purple_request_field_set_label(field, g_value_get_string(value));
			break;
		case PROP_VISIBLE:
			purple_request_field_set_visible(field,
			                                 g_value_get_boolean(value));
			break;
		case PROP_SENSITIVE:
			purple_request_field_set_sensitive(field,
			                                   g_value_get_boolean(value));
			break;
		case PROP_TYPE_HINT:
			purple_request_field_set_type_hint(field,
			                                   g_value_get_string(value));
			break;
		case PROP_TOOLTIP:
			purple_request_field_set_tooltip(field, g_value_get_string(value));
			break;
		case PROP_REQUIRED:
			purple_request_field_set_required(field,
			                                  g_value_get_boolean(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_request_field_finalize(GObject *obj) {
	PurpleRequestField *field = PURPLE_REQUEST_FIELD(obj);
	PurpleRequestFieldPrivate *priv = NULL;

	priv = purple_request_field_get_instance_private(field);

	g_free(priv->id);
	g_free(priv->label);
	g_free(priv->type_hint);
	g_free(priv->tooltip);

	if(priv->type == PURPLE_REQUEST_FIELD_STRING) {
		g_free(priv->u.string.default_value);
		g_free(priv->u.string.value);
	} else if(priv->type == PURPLE_REQUEST_FIELD_CHOICE) {
		g_list_free_full(priv->u.choice.elements,
		                 (GDestroyNotify)purple_key_value_pair_free);
	} else if(priv->type == PURPLE_REQUEST_FIELD_LIST) {
		g_list_free_full(priv->u.list.items,
		                 (GDestroyNotify)purple_key_value_pair_free);
		g_list_free_full(priv->u.list.selected, g_free);
		g_hash_table_destroy(priv->u.list.item_data);
		g_hash_table_destroy(priv->u.list.selected_table);
	} else if(priv->type == PURPLE_REQUEST_FIELD_DATASHEET) {
		purple_request_datasheet_free(priv->u.datasheet.sheet);
	} else if(priv->type == PURPLE_REQUEST_FIELD_IMAGE) {
		g_free(priv->u.image.buffer);
	}

	G_OBJECT_CLASS(purple_request_field_parent_class)->finalize(obj);
}

static void
purple_request_field_init(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	priv = purple_request_field_get_instance_private(field);

	priv->visible = TRUE;
	priv->sensitive = TRUE;
}

static void
purple_request_field_class_init(PurpleRequestFieldClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_request_field_finalize;
	obj_class->get_property = purple_request_field_get_property;
	obj_class->set_property = purple_request_field_set_property;

	/**
	 * PurpleRequestField:id:
	 *
	 * The ID of the field.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", "id",
		"The ID of the field.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleRequestField:label:
	 *
	 * The label of the field.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_LABEL] = g_param_spec_string(
		"label", "label",
		"The label of the field.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleRequestField:visible:
	 *
	 * Whether the field should be visible.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_VISIBLE] = g_param_spec_boolean(
		"visible", "visible",
		"Whether the field should be visible.",
		TRUE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleRequestField:sensitive:
	 *
	 * Whether the field should be sensitive.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_SENSITIVE] = g_param_spec_boolean(
		"sensitive", "sensitive",
		"Whether the field should be sensitive.",
		TRUE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleRequestField:type-hint:
	 *
	 * The type hint of the field.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TYPE_HINT] = g_param_spec_string(
		"type-hint", "type-hint",
		"The type hint of the field.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleRequestField:tooltip:
	 *
	 * The tooltip of the field.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TOOLTIP] = g_param_spec_string(
		"tooltip", "tooltip",
		"The tooltip of the field.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleRequestField:required:
	 *
	 * Whether the field is required to complete the request.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_REQUIRED] = g_param_spec_boolean(
		"required", "required",
		"Whether the field is required to complete the request.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleRequestField:is-validatable:
	 *
	 * Whether the field can be validated by the requestor.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_IS_VALIDATABLE] = g_param_spec_boolean(
		"is-validatable", "is-validatable",
		"Whether the field can be validated by the requestor.",
		FALSE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleRequestField *
purple_request_field_new(const char *id, const char *text,
                         PurpleRequestFieldType type)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id != NULL, NULL);
	g_return_val_if_fail(type != PURPLE_REQUEST_FIELD_NONE, NULL);

	field = g_object_new(PURPLE_TYPE_REQUEST_FIELD,
	                     "id", id,
	                     "label", text,
	                     NULL);
	purple_request_field_set_field_type(field, type);

	return field;
}

void
_purple_request_field_set_group(PurpleRequestField *field,
                                PurpleRequestFieldGroup *group)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	priv->group = group;
}

void
purple_request_field_set_label(PurpleRequestField *field, const char *label)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	g_free(priv->label);
	priv->label = g_strdup(label);

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_LABEL]);
}

void
purple_request_field_set_visible(PurpleRequestField *field, gboolean visible)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	priv->visible = visible;

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_VISIBLE]);
}

void
purple_request_field_set_type_hint(PurpleRequestField *field,
								 const char *type_hint)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	g_free(priv->type_hint);
	priv->type_hint = g_strdup(type_hint);

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_TYPE_HINT]);
}

void
purple_request_field_set_tooltip(PurpleRequestField *field, const char *tooltip)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	g_free(priv->tooltip);
	priv->tooltip = g_strdup(tooltip);

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_TOOLTIP]);
}

void
purple_request_field_set_required(PurpleRequestField *field, gboolean required)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	if(priv->required == required) {
		return;
	}

	priv->required = required;

	if(priv->group != NULL) {
		_purple_request_field_group_set_field_required(priv->group, field,
		                                               required);
	}

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_REQUIRED]);
}

PurpleRequestFieldType
purple_request_field_get_field_type(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field),
	                     PURPLE_REQUEST_FIELD_NONE);

	priv = purple_request_field_get_instance_private(field);

	return priv->type;
}

PurpleRequestFieldGroup *
purple_request_field_get_group(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);

	return priv->group;
}

const char *
purple_request_field_get_id(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);

	return priv->id;
}

const char *
purple_request_field_get_label(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);

	return priv->label;
}

gboolean
purple_request_field_is_visible(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);

	return priv->visible;
}

const char *
purple_request_field_get_type_hint(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);

	return priv->type_hint;
}

const char *
purple_request_field_get_tooltip(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);

	return priv->tooltip;
}

gboolean
purple_request_field_is_required(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);

	return priv->required;
}

gboolean
purple_request_field_is_filled(PurpleRequestField *field) {
	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	switch (purple_request_field_get_field_type(field))
	{
		case PURPLE_REQUEST_FIELD_STRING:
			return (purple_request_field_string_get_value(field) != NULL &&
				*(purple_request_field_string_get_value(field)) != '\0');
		default:
			return TRUE;
	}
}

void
purple_request_field_set_validator(PurpleRequestField *field,
	PurpleRequestFieldValidator validator, void *user_data)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	priv->validator = validator;
	priv->validator_data = validator ? user_data : NULL;

	if(priv->group != NULL) {
		_purple_request_field_group_set_field_validator(priv->group, field,
		                                                validator != NULL);
	}

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_IS_VALIDATABLE]);
}

gboolean
purple_request_field_is_validatable(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);

	return priv->validator != NULL;
}

gboolean
purple_request_field_is_valid(PurpleRequestField *field, gchar **errmsg)
{
	PurpleRequestFieldPrivate *priv = NULL;
	gboolean valid;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);

	if(!priv->validator) {
		return TRUE;
	}

	if (!purple_request_field_is_required(field) &&
		!purple_request_field_is_filled(field))
		return TRUE;

	valid = priv->validator(field, errmsg, priv->validator_data);

	if (valid && errmsg)
		*errmsg = NULL;

	return valid;
}

void
purple_request_field_set_sensitive(PurpleRequestField *field,
	gboolean sensitive)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);

	priv->sensitive = sensitive;

	g_object_notify_by_pspec(G_OBJECT(field), properties[PROP_SENSITIVE]);
}

gboolean
purple_request_field_is_sensitive(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);

	return priv->sensitive;
}

PurpleRequestField *
purple_request_field_string_new(const char *id, const char *text,
							  const char *default_value, gboolean multiline)
{
	PurpleRequestField *field;
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_STRING);
	priv = purple_request_field_get_instance_private(field);

	priv->u.string.multiline = multiline;

	purple_request_field_string_set_default_value(field, default_value);
	purple_request_field_string_set_value(field, default_value);

	return field;
}

void
purple_request_field_string_set_default_value(PurpleRequestField *field,
											const char *default_value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING);

	g_free(priv->u.string.default_value);
	priv->u.string.default_value = g_strdup(default_value);
}

void
purple_request_field_string_set_value(PurpleRequestField *field, const char *value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING);

	g_free(priv->u.string.value);
	priv->u.string.value = g_strdup(value);
}

void
purple_request_field_string_set_masked(PurpleRequestField *field, gboolean masked)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING);

	priv->u.string.masked = masked;
}

const char *
purple_request_field_string_get_default_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING, NULL);

	return priv->u.string.default_value;
}

const char *
purple_request_field_string_get_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING, NULL);

	return priv->u.string.value;
}

gboolean
purple_request_field_string_is_multiline(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

	return priv->u.string.multiline;
}

gboolean
purple_request_field_string_is_masked(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

	return priv->u.string.masked;
}

PurpleRequestField *
purple_request_field_int_new(const char *id, const char *text,
	int default_value, int lower_bound, int upper_bound)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_INTEGER);

	purple_request_field_int_set_lower_bound(field, lower_bound);
	purple_request_field_int_set_upper_bound(field, upper_bound);
	purple_request_field_int_set_default_value(field, default_value);
	purple_request_field_int_set_value(field, default_value);

	return field;
}

void
purple_request_field_int_set_default_value(PurpleRequestField *field,
										 int default_value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER);

	priv->u.integer.default_value = default_value;
}

void
purple_request_field_int_set_lower_bound(PurpleRequestField *field,
	int lower_bound)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER);

	priv->u.integer.lower_bound = lower_bound;
}

void
purple_request_field_int_set_upper_bound(PurpleRequestField *field,
	int upper_bound)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER);

	priv->u.integer.upper_bound = upper_bound;
}

void
purple_request_field_int_set_value(PurpleRequestField *field, int value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER);

	if(value < priv->u.integer.lower_bound ||
	   value > priv->u.integer.upper_bound)
	{
		purple_debug_error("request", "Int value out of bounds\n");
		return;
	}

	priv->u.integer.value = value;
}

int
purple_request_field_int_get_default_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), 0);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return priv->u.integer.default_value;
}

int
purple_request_field_int_get_lower_bound(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), 0);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return priv->u.integer.lower_bound;
}

int
purple_request_field_int_get_upper_bound(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), 0);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return priv->u.integer.upper_bound;
}

int
purple_request_field_int_get_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), 0);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return priv->u.integer.value;
}

PurpleRequestField *
purple_request_field_bool_new(const char *id, const char *text,
							gboolean default_value)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_BOOLEAN);

	purple_request_field_bool_set_default_value(field, default_value);
	purple_request_field_bool_set_value(field, default_value);

	return field;
}

void
purple_request_field_bool_set_default_value(PurpleRequestField *field,
										  gboolean default_value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_BOOLEAN);

	priv->u.boolean.default_value = default_value;
}

void
purple_request_field_bool_set_value(PurpleRequestField *field, gboolean value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_BOOLEAN);

	priv->u.boolean.value = value;
}

gboolean
purple_request_field_bool_get_default_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_BOOLEAN, FALSE);

	return priv->u.boolean.default_value;
}

gboolean
purple_request_field_bool_get_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_BOOLEAN, FALSE);

	return priv->u.boolean.value;
}

PurpleRequestField *
purple_request_field_choice_new(const char *id, const char *text,
	gpointer default_value)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_CHOICE);

	purple_request_field_choice_set_default_value(field, default_value);
	purple_request_field_choice_set_value(field, default_value);

	return field;
}

void
purple_request_field_choice_add(PurpleRequestField *field, const char *label,
	gpointer value)
{
	purple_request_field_choice_add_full(field, label, value, NULL);
}

void
purple_request_field_choice_add_full(PurpleRequestField *field, const char *label,
                                     gpointer value, GDestroyNotify destroy)
{
	PurpleKeyValuePair *choice;
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));
	g_return_if_fail(label != NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_CHOICE);

	choice = purple_key_value_pair_new_full(label, value, destroy);

	priv->u.choice.elements = g_list_append(priv->u.choice.elements, choice);
}

void
purple_request_field_choice_set_default_value(PurpleRequestField *field,
	gpointer default_value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_CHOICE);

	priv->u.choice.default_value = default_value;
}

void
purple_request_field_choice_set_value(PurpleRequestField *field, gpointer value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_CHOICE);

	priv->u.choice.value = value;
}

gpointer
purple_request_field_choice_get_default_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_CHOICE, NULL);

	return priv->u.choice.default_value;
}

gpointer
purple_request_field_choice_get_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_CHOICE, NULL);

	return priv->u.choice.value;
}

GList *
purple_request_field_choice_get_elements(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_CHOICE, NULL);

	return priv->u.choice.elements;
}

PurpleRequestField *
purple_request_field_list_new(const char *id, const char *text)
{
	PurpleRequestField *field;
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(id   != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_LIST);
	priv = purple_request_field_get_instance_private(field);

	priv->u.list.item_data = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                               g_free, NULL);

	priv->u.list.selected_table = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                                    g_free, NULL);

	return field;
}

void
purple_request_field_list_set_multi_select(PurpleRequestField *field,
										 gboolean multi_select)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST);

	priv->u.list.multiple_selection = multi_select;
}

gboolean
purple_request_field_list_get_multi_select(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST, FALSE);

	return priv->u.list.multiple_selection;
}

void *
purple_request_field_list_get_data(PurpleRequestField *field, const char *text)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);
	g_return_val_if_fail(text  != NULL, NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST, NULL);

	return g_hash_table_lookup(priv->u.list.item_data, text);
}

void
purple_request_field_list_add_icon(PurpleRequestField *field, const char *item, const char* icon_path,
							void *data)
{
	PurpleKeyValuePair *kvp;
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));
	g_return_if_fail(item  != NULL);
	g_return_if_fail(data  != NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST);

	priv->u.list.has_icons = priv->u.list.has_icons || (icon_path != NULL);
	kvp = purple_key_value_pair_new_full(item, g_strdup(icon_path), g_free);
	priv->u.list.items = g_list_append(priv->u.list.items, kvp);
	g_hash_table_insert(priv->u.list.item_data, g_strdup(item), data);
}

void
purple_request_field_list_add_selected(PurpleRequestField *field, const char *item)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));
	g_return_if_fail(item  != NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST);

	if(!purple_request_field_list_get_multi_select(field) &&
	   priv->u.list.selected != NULL)
	{
		purple_debug_warning("request",
						   "More than one item added to non-multi-select "
						   "field %s\n",
						   purple_request_field_get_id(field));
		return;
	}

	priv->u.list.selected = g_list_append(priv->u.list.selected,
	                                      g_strdup(item));

	g_hash_table_add(priv->u.list.selected_table, g_strdup(item));
}

void
purple_request_field_list_clear_selected(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST);

	if(priv->u.list.selected != NULL) {
		g_list_free_full(priv->u.list.selected, g_free);
		priv->u.list.selected = NULL;
	}

	g_hash_table_remove_all(priv->u.list.selected_table);
}

void
purple_request_field_list_set_selected(PurpleRequestField *field, GList *items)
{
	PurpleRequestFieldPrivate *priv = NULL;
	GList *l;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));
	g_return_if_fail(items != NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST);

	purple_request_field_list_clear_selected(field);

	if (!purple_request_field_list_get_multi_select(field) && items->next) {
		purple_debug_warning("request",
						   "More than one item added to non-multi-select "
						   "field %s\n",
						   purple_request_field_get_id(field));
		return;
	}

	for (l = items; l != NULL; l = l->next) {
		char *selected = l->data;
		priv->u.list.selected = g_list_append(priv->u.list.selected,
		                                      g_strdup(selected));
		g_hash_table_add(priv->u.list.selected_table, g_strdup(selected));
	}
}

gboolean
purple_request_field_list_is_selected(PurpleRequestField *field,
                                      const char *item)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);
	g_return_val_if_fail(item  != NULL, FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST, FALSE);

	return g_hash_table_contains(priv->u.list.selected_table, item);
}

GList *
purple_request_field_list_get_selected(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST, NULL);

	return priv->u.list.selected;
}

GList *
purple_request_field_list_get_items(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST, NULL);

	return priv->u.list.items;
}

gboolean
purple_request_field_list_has_icons(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_LIST, FALSE);

	return priv->u.list.has_icons;
}

PurpleRequestField *
purple_request_field_label_new(const char *id, const char *text)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_LABEL);

	return field;
}

PurpleRequestField *
purple_request_field_image_new(const char *id, const char *text, const char *buf, gsize size)
{
	PurpleRequestField *field;

	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);
	g_return_val_if_fail(buf  != NULL, NULL);
	g_return_val_if_fail(size > 0, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_IMAGE);
	priv = purple_request_field_get_instance_private(field);

	priv->u.image.buffer = g_memdup2(buf, size);
	priv->u.image.size = size;
	priv->u.image.scale_x = 1;
	priv->u.image.scale_y = 1;

	return field;
}

void
purple_request_field_image_set_scale(PurpleRequestField *field, unsigned int x, unsigned int y)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_IMAGE);

	priv->u.image.scale_x = x;
	priv->u.image.scale_y = y;
}

const char *
purple_request_field_image_get_buffer(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_IMAGE, NULL);

	return priv->u.image.buffer;
}

gsize
purple_request_field_image_get_size(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), 0);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_IMAGE, 0);

	return priv->u.image.size;
}

unsigned int
purple_request_field_image_get_scale_x(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), 0);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_IMAGE, 0);

	return priv->u.image.scale_x;
}

unsigned int
purple_request_field_image_get_scale_y(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), 0);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_IMAGE, 0);

	return priv->u.image.scale_y;
}

PurpleRequestField *
purple_request_field_account_new(const char *id, const char *text,
							   PurpleAccount *account)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_ACCOUNT);

	if(account == NULL) {
		PurpleAccountManager *manager = purple_account_manager_get_default();
		GList *accounts = purple_account_manager_get_connected(manager);

		if(accounts != NULL) {
			account = accounts->data;
			g_list_free(accounts);
		}
	}

	purple_request_field_account_set_default_value(field, account);
	purple_request_field_account_set_value(field, account);

	return field;
}

void
purple_request_field_account_set_default_value(PurpleRequestField *field,
											 PurpleAccount *default_value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	priv->u.account.default_account = default_value;
}

void
purple_request_field_account_set_value(PurpleRequestField *field,
									 PurpleAccount *value)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	priv->u.account.account = value;
}

void
purple_request_field_account_set_show_all(PurpleRequestField *field,
										gboolean show_all)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	if(priv->u.account.show_all == show_all) {
		return;
	}

	priv->u.account.show_all = show_all;

	if(!show_all) {
		PurpleAccountManager *manager = purple_account_manager_get_default();
		GList *accounts = purple_account_manager_get_connected(manager);

		if(purple_account_is_connected(priv->u.account.default_account)) {
			purple_request_field_account_set_default_value(field,
			                                               accounts->data);
		}

		if(purple_account_is_connected(priv->u.account.account)) {
			purple_request_field_account_set_value(field,
			                                       accounts->data);
		}

		g_list_free(accounts);
	}
}

void
purple_request_field_account_set_filter(PurpleRequestField *field,
									  PurpleFilterAccountFunc filter_func)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	priv = purple_request_field_get_instance_private(field);
	g_return_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	priv->u.account.filter_func = filter_func;
}

PurpleAccount *
purple_request_field_account_get_default_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT, NULL);

	return priv->u.account.default_account;
}

PurpleAccount *
purple_request_field_account_get_value(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT, NULL);

	return priv->u.account.account;
}

gboolean
purple_request_field_account_get_show_all(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT, FALSE);

	return priv->u.account.show_all;
}

PurpleFilterAccountFunc
purple_request_field_account_get_filter(PurpleRequestField *field) {
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_ACCOUNT, FALSE);

	return priv->u.account.filter_func;
}

PurpleRequestField *
purple_request_field_datasheet_new(const char *id,
	const gchar *text, PurpleRequestDatasheet *sheet)
{
	PurpleRequestField *field;
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(id != NULL, NULL);
	g_return_val_if_fail(sheet != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_DATASHEET);
	priv = purple_request_field_get_instance_private(field);

	priv->u.datasheet.sheet = sheet;

	return field;
}

PurpleRequestDatasheet *
purple_request_field_datasheet_get_sheet(PurpleRequestField *field)
{
	PurpleRequestFieldPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_DATASHEET, NULL);

	return priv->u.datasheet.sheet;
}

/* -- */

gboolean
purple_request_field_email_validator(PurpleRequestField *field, gchar **errmsg,
                                     G_GNUC_UNUSED gpointer user_data)
{
	PurpleRequestFieldPrivate *priv = NULL;
	const char *value;

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

	value = purple_request_field_string_get_value(field);

	if (value != NULL && purple_email_is_valid(value))
		return TRUE;

	if (errmsg)
		*errmsg = g_strdup(_("Invalid email address"));
	return FALSE;
}

gboolean
purple_request_field_alphanumeric_validator(PurpleRequestField *field,
	gchar **errmsg, void *allowed_characters)
{
	PurpleRequestFieldPrivate *priv = NULL;
	const char *value;
	gchar invalid_char = '\0';

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), FALSE);

	priv = purple_request_field_get_instance_private(field);
	g_return_val_if_fail(priv->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

	value = purple_request_field_string_get_value(field);

	g_return_val_if_fail(value != NULL, FALSE);

	if (allowed_characters)
	{
		gchar *value_r = g_strdup(value);
		g_strcanon(value_r, allowed_characters, '\0');
		invalid_char = value[strlen(value_r)];
		g_free(value_r);
	}
	else
	{
		while (value)
		{
			if (!g_ascii_isalnum(*value))
			{
				invalid_char = *value;
				break;
			}
			value++;
		}
	}
	if (!invalid_char)
		return TRUE;

	if (errmsg)
		*errmsg = g_strdup_printf(_("Invalid character '%c'"),
			invalid_char);
	return FALSE;
}
