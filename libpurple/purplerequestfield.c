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

struct _PurpleRequestField
{
	PurpleRequestFieldType type;
	PurpleRequestFieldGroup *group;

	char *id;
	char *label;
	char *type_hint;

	gboolean visible;
	gboolean required;
	gboolean sensitive;

	union
	{
		struct
		{
			gboolean multiline;
			gboolean masked;
			char *default_value;
			char *value;

		} string;

		struct
		{
			int default_value;
			int value;
			int lower_bound;
			int upper_bound;
		} integer;

		struct
		{
			gboolean default_value;
			gboolean value;

		} boolean;

		struct
		{
			gpointer default_value;
			gpointer value;

			GList *elements;
		} choice;

		struct
		{
			GList *items;
			gboolean has_icons;
			GHashTable *item_data;
			GList *selected;
			GHashTable *selected_table;

			gboolean multiple_selection;

		} list;

		struct
		{
			PurpleAccount *default_account;
			PurpleAccount *account;
			gboolean show_all;

			PurpleFilterAccountFunc filter_func;

		} account;

		struct
		{
			unsigned int scale_x;
			unsigned int scale_y;
			char *buffer;
			gsize size;
		} image;

		struct
		{
			PurpleRequestDatasheet *sheet;
		} datasheet;
	} u;

	void *ui_data;
	char *tooltip;

	PurpleRequestFieldValidator validator;
	void *validator_data;
};

gpointer
purple_request_field_get_ui_data(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);

	return field->ui_data;
}

void
purple_request_field_set_ui_data(PurpleRequestField *field,
                                 gpointer ui_data)
{
	g_return_if_fail(field != NULL);

	field->ui_data = ui_data;
}

PurpleRequestField *
purple_request_field_new(const char *id, const char *text,
					   PurpleRequestFieldType type)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(type != PURPLE_REQUEST_FIELD_NONE, NULL);

	field = g_new0(PurpleRequestField, 1);

	field->id   = g_strdup(id);
	field->type = type;

	purple_request_field_set_label(field, text);
	purple_request_field_set_visible(field, TRUE);
	purple_request_field_set_sensitive(field, TRUE);

	return field;
}

void
purple_request_field_destroy(PurpleRequestField *field)
{
	g_return_if_fail(field != NULL);

	g_free(field->id);
	g_free(field->label);
	g_free(field->type_hint);
	g_free(field->tooltip);

	if(field->type == PURPLE_REQUEST_FIELD_STRING) {
		g_free(field->u.string.default_value);
		g_free(field->u.string.value);
	} else if(field->type == PURPLE_REQUEST_FIELD_CHOICE) {
		g_list_free_full(field->u.choice.elements, (GDestroyNotify)purple_key_value_pair_free);
	} else if(field->type == PURPLE_REQUEST_FIELD_LIST) {
		g_list_free_full(field->u.list.items, (GDestroyNotify)purple_key_value_pair_free);
		g_list_free_full(field->u.list.selected, g_free);
		g_hash_table_destroy(field->u.list.item_data);
		g_hash_table_destroy(field->u.list.selected_table);
	} else if(field->type == PURPLE_REQUEST_FIELD_DATASHEET) {
		purple_request_datasheet_free(field->u.datasheet.sheet);
	} else if(field->type == PURPLE_REQUEST_FIELD_IMAGE) {
		g_free(field->u.image.buffer);
	}

	g_free(field);
}

void
_purple_request_field_set_group(PurpleRequestField *field,
                                PurpleRequestFieldGroup *group)
{
	g_return_if_fail(field != NULL);

	field->group = group;
}

void
purple_request_field_set_label(PurpleRequestField *field, const char *label)
{
	g_return_if_fail(field != NULL);

	g_free(field->label);
	field->label = g_strdup(label);
}

void
purple_request_field_set_visible(PurpleRequestField *field, gboolean visible)
{
	g_return_if_fail(field != NULL);

	field->visible = visible;
}

void
purple_request_field_set_type_hint(PurpleRequestField *field,
								 const char *type_hint)
{
	g_return_if_fail(field != NULL);

	g_free(field->type_hint);
	field->type_hint = g_strdup(type_hint);
}

void
purple_request_field_set_tooltip(PurpleRequestField *field, const char *tooltip)
{
	g_return_if_fail(field != NULL);

	g_free(field->tooltip);
	field->tooltip = g_strdup(tooltip);
}

void
purple_request_field_set_required(PurpleRequestField *field, gboolean required)
{
	g_return_if_fail(field != NULL);

	if (field->required == required)
		return;

	field->required = required;

	if(field->group != NULL) {
		_purple_request_field_group_set_field_required(field->group, field,
		                                               required);
	}
}

PurpleRequestFieldType
purple_request_field_get_field_type(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, PURPLE_REQUEST_FIELD_NONE);

	return field->type;
}

PurpleRequestFieldGroup *
purple_request_field_get_group(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);

	return field->group;
}

const char *
purple_request_field_get_id(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);

	return field->id;
}

const char *
purple_request_field_get_label(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);

	return field->label;
}

gboolean
purple_request_field_is_visible(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);

	return field->visible;
}

const char *
purple_request_field_get_field_type_hint(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);

	return field->type_hint;
}

const char *
purple_request_field_get_tooltip(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);

	return field->tooltip;
}

gboolean
purple_request_field_is_required(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);

	return field->required;
}

gboolean
purple_request_field_is_filled(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);

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
	g_return_if_fail(field != NULL);

	field->validator = validator;
	field->validator_data = validator ? user_data : NULL;

	if(field->group != NULL) {
		_purple_request_field_group_set_field_validator(field->group, field,
		                                                validator != NULL);
	}
}

gboolean
purple_request_field_is_validatable(PurpleRequestField *field)
{
	g_return_val_if_fail(field != NULL, FALSE);

	return field->validator != NULL;
}

gboolean
purple_request_field_is_valid(PurpleRequestField *field, gchar **errmsg)
{
	gboolean valid;

	g_return_val_if_fail(field != NULL, FALSE);

	if (!field->validator)
		return TRUE;

	if (!purple_request_field_is_required(field) &&
		!purple_request_field_is_filled(field))
		return TRUE;

	valid = field->validator(field, errmsg, field->validator_data);

	if (valid && errmsg)
		*errmsg = NULL;

	return valid;
}

void
purple_request_field_set_sensitive(PurpleRequestField *field,
	gboolean sensitive)
{
	g_return_if_fail(field != NULL);

	field->sensitive = sensitive;
}

gboolean
purple_request_field_is_sensitive(PurpleRequestField *field)
{
	g_return_val_if_fail(field != NULL, FALSE);

	return field->sensitive;
}

PurpleRequestField *
purple_request_field_string_new(const char *id, const char *text,
							  const char *default_value, gboolean multiline)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_STRING);

	field->u.string.multiline = multiline;

	purple_request_field_string_set_default_value(field, default_value);
	purple_request_field_string_set_value(field, default_value);

	return field;
}

void
purple_request_field_string_set_default_value(PurpleRequestField *field,
											const char *default_value)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING);

	g_free(field->u.string.default_value);
	field->u.string.default_value = g_strdup(default_value);
}

void
purple_request_field_string_set_value(PurpleRequestField *field, const char *value)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING);

	g_free(field->u.string.value);
	field->u.string.value = g_strdup(value);
}

void
purple_request_field_string_set_masked(PurpleRequestField *field, gboolean masked)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING);

	field->u.string.masked = masked;
}

const char *
purple_request_field_string_get_default_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING, NULL);

	return field->u.string.default_value;
}

const char *
purple_request_field_string_get_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING, NULL);

	return field->u.string.value;
}

gboolean
purple_request_field_string_is_multiline(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

	return field->u.string.multiline;
}

gboolean
purple_request_field_string_is_masked(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

	return field->u.string.masked;
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
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER);

	field->u.integer.default_value = default_value;
}

void
purple_request_field_int_set_lower_bound(PurpleRequestField *field,
	int lower_bound)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER);

	field->u.integer.lower_bound = lower_bound;
}

void
purple_request_field_int_set_upper_bound(PurpleRequestField *field,
	int upper_bound)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER);

	field->u.integer.upper_bound = upper_bound;
}

void
purple_request_field_int_set_value(PurpleRequestField *field, int value)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER);

	if (value < field->u.integer.lower_bound ||
		value > field->u.integer.upper_bound) {
		purple_debug_error("request", "Int value out of bounds\n");
		return;
	}

	field->u.integer.value = value;
}

int
purple_request_field_int_get_default_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, 0);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return field->u.integer.default_value;
}

int
purple_request_field_int_get_lower_bound(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, 0);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return field->u.integer.lower_bound;
}

int
purple_request_field_int_get_upper_bound(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, 0);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return field->u.integer.upper_bound;
}

int
purple_request_field_int_get_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, 0);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_INTEGER, 0);

	return field->u.integer.value;
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
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_BOOLEAN);

	field->u.boolean.default_value = default_value;
}

void
purple_request_field_bool_set_value(PurpleRequestField *field, gboolean value)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_BOOLEAN);

	field->u.boolean.value = value;
}

gboolean
purple_request_field_bool_get_default_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_BOOLEAN, FALSE);

	return field->u.boolean.default_value;
}

gboolean
purple_request_field_bool_get_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_BOOLEAN, FALSE);

	return field->u.boolean.value;
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

	g_return_if_fail(field != NULL);
	g_return_if_fail(label != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_CHOICE);

	choice = purple_key_value_pair_new_full(label, value, destroy);

	field->u.choice.elements = g_list_append(field->u.choice.elements,
		choice);
}

void
purple_request_field_choice_set_default_value(PurpleRequestField *field,
	gpointer default_value)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_CHOICE);

	field->u.choice.default_value = default_value;
}

void
purple_request_field_choice_set_value(PurpleRequestField *field, gpointer value)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_CHOICE);

	field->u.choice.value = value;
}

gpointer
purple_request_field_choice_get_default_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_CHOICE, NULL);

	return field->u.choice.default_value;
}

gpointer
purple_request_field_choice_get_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_CHOICE, NULL);

	return field->u.choice.value;
}

GList *
purple_request_field_choice_get_elements(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_CHOICE, NULL);

	return field->u.choice.elements;
}

PurpleRequestField *
purple_request_field_list_new(const char *id, const char *text)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id   != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_LIST);

	field->u.list.item_data = g_hash_table_new_full(g_str_hash, g_str_equal,
													g_free, NULL);

	field->u.list.selected_table =
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return field;
}

void
purple_request_field_list_set_multi_select(PurpleRequestField *field,
										 gboolean multi_select)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST);

	field->u.list.multiple_selection = multi_select;
}

gboolean
purple_request_field_list_get_multi_select(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST, FALSE);

	return field->u.list.multiple_selection;
}

void *
purple_request_field_list_get_data(PurpleRequestField *field, const char *text)
{
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(text  != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST, NULL);

	return g_hash_table_lookup(field->u.list.item_data, text);
}

void
purple_request_field_list_add_icon(PurpleRequestField *field, const char *item, const char* icon_path,
							void *data)
{
	PurpleKeyValuePair *kvp;

	g_return_if_fail(field != NULL);
	g_return_if_fail(item  != NULL);
	g_return_if_fail(data  != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST);

	field->u.list.has_icons = field->u.list.has_icons || (icon_path != NULL);
	kvp = purple_key_value_pair_new_full(item, g_strdup(icon_path), g_free);
	field->u.list.items = g_list_append(field->u.list.items, kvp);
	g_hash_table_insert(field->u.list.item_data, g_strdup(item), data);
}

void
purple_request_field_list_add_selected(PurpleRequestField *field, const char *item)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(item  != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST);

	if (!purple_request_field_list_get_multi_select(field) &&
		field->u.list.selected != NULL)
	{
		purple_debug_warning("request",
						   "More than one item added to non-multi-select "
						   "field %s\n",
						   purple_request_field_get_id(field));
		return;
	}

	field->u.list.selected = g_list_append(field->u.list.selected,
										   g_strdup(item));

	g_hash_table_add(field->u.list.selected_table, g_strdup(item));
}

void
purple_request_field_list_clear_selected(PurpleRequestField *field)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST);

	if (field->u.list.selected != NULL)
	{
		g_list_free_full(field->u.list.selected, g_free);
		field->u.list.selected = NULL;
	}

	g_hash_table_remove_all(field->u.list.selected_table);
}

void
purple_request_field_list_set_selected(PurpleRequestField *field, GList *items)
{
	GList *l;

	g_return_if_fail(field != NULL);
	g_return_if_fail(items != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST);

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
		field->u.list.selected = g_list_append(field->u.list.selected,
		                                       g_strdup(selected));
		g_hash_table_add(field->u.list.selected_table, g_strdup(selected));
	}
}

gboolean
purple_request_field_list_is_selected(PurpleRequestField *field,
                                      const char *item)
{
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(item  != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST, FALSE);

	return g_hash_table_contains(field->u.list.selected_table, item);
}

GList *
purple_request_field_list_get_selected(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST, NULL);

	return field->u.list.selected;
}

GList *
purple_request_field_list_get_items(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST, NULL);

	return field->u.list.items;
}

gboolean
purple_request_field_list_has_icons(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_LIST, FALSE);

	return field->u.list.has_icons;
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

	g_return_val_if_fail(id   != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);
	g_return_val_if_fail(buf  != NULL, NULL);
	g_return_val_if_fail(size > 0, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_IMAGE);

	field->u.image.buffer  = g_memdup2(buf, size);
	field->u.image.size    = size;
	field->u.image.scale_x = 1;
	field->u.image.scale_y = 1;

	return field;
}

void
purple_request_field_image_set_scale(PurpleRequestField *field, unsigned int x, unsigned int y)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_IMAGE);

	field->u.image.scale_x = x;
	field->u.image.scale_y = y;
}

const char *
purple_request_field_image_get_buffer(PurpleRequestField *field)
{
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_IMAGE, NULL);

	return field->u.image.buffer;
}

gsize
purple_request_field_image_get_size(PurpleRequestField *field)
{
	g_return_val_if_fail(field != NULL, 0);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_IMAGE, 0);

	return field->u.image.size;
}

unsigned int
purple_request_field_image_get_scale_x(PurpleRequestField *field)
{
	g_return_val_if_fail(field != NULL, 0);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_IMAGE, 0);

	return field->u.image.scale_x;
}

unsigned int
purple_request_field_image_get_scale_y(PurpleRequestField *field)
{
	g_return_val_if_fail(field != NULL, 0);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_IMAGE, 0);

	return field->u.image.scale_y;
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
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	field->u.account.default_account = default_value;
}

void
purple_request_field_account_set_value(PurpleRequestField *field,
									 PurpleAccount *value)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	field->u.account.account = value;
}

void
purple_request_field_account_set_show_all(PurpleRequestField *field,
										gboolean show_all)
{
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	if (field->u.account.show_all == show_all)
		return;

	field->u.account.show_all = show_all;

	if(!show_all) {
		PurpleAccountManager *manager = purple_account_manager_get_default();
		GList *accounts = purple_account_manager_get_connected(manager);

		if (purple_account_is_connected(field->u.account.default_account))
		{
			purple_request_field_account_set_default_value(field,
			                                               accounts->data);
		}

		if (purple_account_is_connected(field->u.account.account))
		{
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
	g_return_if_fail(field != NULL);
	g_return_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT);

	field->u.account.filter_func = filter_func;
}

PurpleAccount *
purple_request_field_account_get_default_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT, NULL);

	return field->u.account.default_account;
}

PurpleAccount *
purple_request_field_account_get_value(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT, NULL);

	return field->u.account.account;
}

gboolean
purple_request_field_account_get_show_all(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT, FALSE);

	return field->u.account.show_all;
}

PurpleFilterAccountFunc
purple_request_field_account_get_filter(PurpleRequestField *field) {
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_ACCOUNT, FALSE);

	return field->u.account.filter_func;
}

PurpleRequestField *
purple_request_field_datasheet_new(const char *id,
	const gchar *text, PurpleRequestDatasheet *sheet)
{
	PurpleRequestField *field;

	g_return_val_if_fail(id != NULL, NULL);
	g_return_val_if_fail(sheet != NULL, NULL);

	field = purple_request_field_new(id, text, PURPLE_REQUEST_FIELD_DATASHEET);

	field->u.datasheet.sheet = sheet;

	return field;
}

PurpleRequestDatasheet *
purple_request_field_datasheet_get_sheet(PurpleRequestField *field)
{
	g_return_val_if_fail(field != NULL, NULL);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_DATASHEET, NULL);

	return field->u.datasheet.sheet;
}

/* -- */

gboolean
purple_request_field_email_validator(PurpleRequestField *field, gchar **errmsg,
                                     G_GNUC_UNUSED gpointer user_data)
{
	const char *value;

	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

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
	const char *value;
	gchar invalid_char = '\0';

	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(field->type == PURPLE_REQUEST_FIELD_STRING, FALSE);

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
