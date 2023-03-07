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

struct _PurpleRequestFieldGroup
{
	PurpleRequestFields *fields_list;

	char *title;

	GList *fields;
};

PurpleRequestFieldGroup *
purple_request_field_group_new(const char *title)
{
	PurpleRequestFieldGroup *group;

	group = g_new0(PurpleRequestFieldGroup, 1);

	group->title = g_strdup(title);

	return group;
}

void
purple_request_field_group_destroy(PurpleRequestFieldGroup *group)
{
	g_return_if_fail(group != NULL);

	g_free(group->title);

	g_list_free_full(group->fields, g_object_unref);

	g_free(group);
}

void
_purple_request_field_group_set_field_list(PurpleRequestFieldGroup *group,
                                           PurpleRequestFields *fields)
{
	g_return_if_fail(group != NULL);

	group->fields_list = fields;
}

void
_purple_request_field_group_set_field_required(PurpleRequestFieldGroup *group,
                                               PurpleRequestField *field,
                                               gboolean required)
{
	g_return_if_fail(group != NULL);

	_purple_request_field_list_set_field_required(group->fields_list, field,
	                                              required);
}

void
_purple_request_field_group_set_field_validator(PurpleRequestFieldGroup *group,
                                                PurpleRequestField *field,
                                                gboolean validator)
{
	g_return_if_fail(group != NULL);

	_purple_request_field_list_set_field_validator(group->fields_list, field,
	                                               validator);
}

void
purple_request_field_group_add_field(PurpleRequestFieldGroup *group,
								   PurpleRequestField *field)
{
	g_return_if_fail(group != NULL);
	g_return_if_fail(PURPLE_IS_REQUEST_FIELD(field));

	group->fields = g_list_append(group->fields, field);

	if(group->fields_list != NULL) {
		_purple_request_field_list_add_field(group->fields_list, field);
	}

	_purple_request_field_set_group(field, group);
}

const char *
purple_request_field_group_get_title(const PurpleRequestFieldGroup *group)
{
	g_return_val_if_fail(group != NULL, NULL);

	return group->title;
}

GList *
purple_request_field_group_get_fields(const PurpleRequestFieldGroup *group)
{
	g_return_val_if_fail(group != NULL, NULL);

	return group->fields;
}

PurpleRequestFields *
purple_request_field_group_get_fields_list(const PurpleRequestFieldGroup *group)
{
	g_return_val_if_fail(group != NULL, NULL);

	return group->fields_list;
}
