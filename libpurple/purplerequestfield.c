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
#include "request/purplerequestfieldstring.h"
#include "debug.h"
#include "purplekeyvaluepair.h"
#include "purpleprivate.h"

typedef struct {
	PurpleRequestFieldType type;
	PurpleRequestGroup *group;

	char *id;
	char *label;
	char *type_hint;

	gboolean visible;
	gboolean required;
	gboolean sensitive;

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
                                PurpleRequestGroup *group)
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

	if(PURPLE_IS_REQUEST_GROUP(priv->group)) {
		_purple_request_group_set_field_required(priv->group, field,
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

PurpleRequestGroup *
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

	if(PURPLE_IS_REQUEST_FIELD_STRING(field)) {
		PurpleRequestFieldString *sfield = PURPLE_REQUEST_FIELD_STRING(field);
		return !purple_strempty(purple_request_field_string_get_value(sfield));
	}

	return TRUE;
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

	if(PURPLE_IS_REQUEST_GROUP(priv->group)) {
		_purple_request_group_set_field_validator(priv->group, field,
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
