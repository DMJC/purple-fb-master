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
#include "purplerequestpage.h"
#include "purpleprivate.h"

struct _PurpleRequestFields
{
	GList *groups;

	GHashTable *fields;

	GList *required_fields;

	GList *validated_fields;

	void *ui_data;
};

PurpleRequestFields *
purple_request_fields_new(void)
{
	PurpleRequestFields *fields;

	fields = g_new0(PurpleRequestFields, 1);

	fields->fields = g_hash_table_new_full(g_str_hash, g_str_equal,
										   g_free, NULL);

	return fields;
}

void
purple_request_fields_destroy(PurpleRequestFields *fields)
{
	g_return_if_fail(fields != NULL);

	g_list_free_full(fields->groups, g_object_unref);
	g_list_free(fields->required_fields);
	g_list_free(fields->validated_fields);
	g_hash_table_destroy(fields->fields);
	g_free(fields);
}

void
_purple_request_field_list_set_field_required(PurpleRequestFields *fields,
                                              PurpleRequestField *field,
                                              gboolean required)
{
	g_return_if_fail(fields != NULL);

	if(required) {
		fields->required_fields = g_list_append(fields->required_fields, field);
	} else {
		fields->required_fields = g_list_remove(fields->required_fields, field);
	}
}

void
_purple_request_field_list_set_field_validator(PurpleRequestFields *fields,
                                               PurpleRequestField *field,
                                               gboolean validator)
{
	g_return_if_fail(fields != NULL);

	fields->validated_fields = g_list_remove(fields->validated_fields, field);
	if(validator) {
		fields->validated_fields = g_list_append(fields->validated_fields,
		                                         field);
	}
}

void
_purple_request_field_list_add_field(PurpleRequestFields *fields,
                                     PurpleRequestField *field)
{
	g_return_if_fail(fields != NULL);

	g_hash_table_insert(fields->fields,
	                    g_strdup(purple_request_field_get_id(field)), field);

	if(purple_request_field_is_required(field)) {
		fields->required_fields = g_list_append(fields->required_fields,
		                                        field);
	}

	if(purple_request_field_is_validatable(field)) {
		fields->validated_fields = g_list_append(fields->validated_fields,
		                                         field);
	}
}

void
purple_request_fields_add_group(PurpleRequestFields *fields,
                                PurpleRequestGroup *group)
{
	GList *l;
	PurpleRequestField *field;

	g_return_if_fail(fields != NULL);
	g_return_if_fail(PURPLE_IS_REQUEST_GROUP(group));

	fields->groups = g_list_append(fields->groups, group);

	_purple_request_group_set_field_list(group, fields);

	for (l = purple_request_group_get_fields(group);
		 l != NULL;
		 l = l->next) {

		field = l->data;

		g_hash_table_insert(fields->fields,
			g_strdup(purple_request_field_get_id(field)), field);

		if (purple_request_field_is_required(field)) {
			fields->required_fields =
				g_list_append(fields->required_fields, field);
		}

		if (purple_request_field_is_validatable(field)) {
			fields->validated_fields =
				g_list_append(fields->validated_fields, field);
		}
	}
}

GList *
purple_request_fields_get_groups(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->groups;
}

gboolean
purple_request_fields_exists(const PurpleRequestFields *fields, const char *id)
{
	g_return_val_if_fail(fields != NULL, FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	return (g_hash_table_lookup(fields->fields, id) != NULL);
}

const GList *
purple_request_fields_get_required(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->required_fields;
}

const GList *
purple_request_fields_get_validatable(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->validated_fields;
}

gboolean
purple_request_fields_is_field_required(const PurpleRequestFields *fields,
									  const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return FALSE;

	return purple_request_field_is_required(field);
}

gboolean
purple_request_fields_all_required_filled(const PurpleRequestFields *fields)
{
	GList *l;

	g_return_val_if_fail(fields != NULL, FALSE);

	for (l = fields->required_fields; l != NULL; l = l->next)
	{
		PurpleRequestField *field = PURPLE_REQUEST_FIELD(l->data);

		if (!purple_request_field_is_filled(field))
			return FALSE;
	}

	return TRUE;
}

gboolean
purple_request_fields_all_valid(const PurpleRequestFields *fields)
{
	GList *l;

	g_return_val_if_fail(fields != NULL, FALSE);

	for (l = fields->validated_fields; l != NULL; l = l->next)
	{
		PurpleRequestField *field = PURPLE_REQUEST_FIELD(l->data);

		if (!purple_request_field_is_valid(field, NULL))
			return FALSE;
	}

	return TRUE;
}

PurpleRequestField *
purple_request_fields_get_field(const PurpleRequestFields *fields, const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	field = g_hash_table_lookup(fields->fields, id);

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	return field;
}

const char *
purple_request_fields_get_string(const PurpleRequestFields *fields, const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return NULL;

	return purple_request_field_string_get_value(field);
}

int
purple_request_fields_get_integer(const PurpleRequestFields *fields,
								const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, 0);
	g_return_val_if_fail(id     != NULL, 0);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return 0;

	return purple_request_field_int_get_value(field);
}

gboolean
purple_request_fields_get_bool(const PurpleRequestFields *fields, const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return FALSE;

	return purple_request_field_bool_get_value(field);
}

gpointer
purple_request_fields_get_choice(const PurpleRequestFields *fields,
	const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id != NULL, NULL);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return NULL;

	return purple_request_field_choice_get_value(field);
}

PurpleAccount *
purple_request_fields_get_account(const PurpleRequestFields *fields,
								const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return NULL;

	return purple_request_field_account_get_value(field);
}

gpointer purple_request_fields_get_ui_data(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->ui_data;
}

void purple_request_fields_set_ui_data(PurpleRequestFields *fields, gpointer ui_data)
{
	g_return_if_fail(fields != NULL);

	fields->ui_data = ui_data;
}
