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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_REQUEST_PAGE_H
#define PURPLE_REQUEST_PAGE_H

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

/**
 * PurpleRequestFields:
 *
 * Multiple fields request data.
 */
typedef struct _PurpleRequestFields PurpleRequestFields;

#include "account.h"
#include "purplerequestgroup.h"
#include "purplerequestfield.h"

G_BEGIN_DECLS

/**
 * purple_request_fields_new:
 *
 * Creates a list of fields to pass to purple_request_fields().
 *
 * Returns: (transfer full): A PurpleRequestFields structure.
 */
PurpleRequestFields *purple_request_fields_new(void);

/**
 * purple_request_fields_destroy:
 * @fields: The list of fields to destroy.
 *
 * Destroys a list of fields.
 */
void purple_request_fields_destroy(PurpleRequestFields *fields);

/**
 * purple_request_fields_add_group:
 * @fields: The fields list.
 * @group:  The group to add.
 *
 * Adds a group of fields to the list.
 */
void purple_request_fields_add_group(PurpleRequestFields *fields, PurpleRequestGroup *group);

/**
 * purple_request_fields_get_groups:
 * @fields: The fields list.
 *
 * Returns a list of all groups in a field list.
 *
 * Returns: (element-type PurpleRequestGroup) (transfer none): A list of groups.
 */
GList *purple_request_fields_get_groups(const PurpleRequestFields *fields);

/**
 * purple_request_fields_exists:
 * @fields: The fields list.
 * @id:     The ID of the field.
 *
 * Returns whether or not the field with the specified ID exists.
 *
 * Returns: TRUE if the field exists, or FALSE.
 */
gboolean purple_request_fields_exists(const PurpleRequestFields *fields,
									const char *id);

/**
 * purple_request_fields_get_required:
 * @fields: The fields list.
 *
 * Returns a list of all required fields.
 *
 * Returns: (element-type PurpleRequestField) (transfer none): The list of required fields.
 */
const GList *purple_request_fields_get_required(
	const PurpleRequestFields *fields);

/**
 * purple_request_fields_get_validatable:
 * @fields: The fields list.
 *
 * Returns a list of all validated fields.
 *
 * Returns: (element-type PurpleRequestField) (transfer none): The list of validated fields.
 */
const GList *purple_request_fields_get_validatable(
	const PurpleRequestFields *fields);

/**
 * purple_request_fields_is_field_required:
 * @fields: The fields list.
 * @id:     The field ID.
 *
 * Returns whether or not a field with the specified ID is required.
 *
 * Returns: TRUE if the specified field is required, or FALSE.
 */
gboolean purple_request_fields_is_field_required(const PurpleRequestFields *fields,
											   const char *id);

/**
 * purple_request_fields_all_required_filled:
 * @fields: The fields list.
 *
 * Returns whether or not all required fields have values.
 *
 * Returns: TRUE if all required fields have values, or FALSE.
 */
gboolean purple_request_fields_all_required_filled(
	const PurpleRequestFields *fields);

/**
 * purple_request_fields_all_valid:
 * @fields: The fields list.
 *
 * Returns whether or not all fields are valid.
 *
 * Returns: TRUE if all fields are valid, or FALSE.
 */
gboolean purple_request_fields_all_valid(const PurpleRequestFields *fields);

/**
 * purple_request_fields_get_field:
 * @fields: The fields list.
 * @id:     The ID of the field.
 *
 * Return the field with the specified ID.
 *
 * Returns: (transfer none): The field, if found.
 */
PurpleRequestField *purple_request_fields_get_field(
		const PurpleRequestFields *fields, const char *id);

/**
 * purple_request_fields_get_string:
 * @fields: The fields list.
 * @id:     The ID of the field.
 *
 * Returns the string value of a field with the specified ID.
 *
 * Returns: The string value, if found, or %NULL otherwise.
 */
const char *purple_request_fields_get_string(const PurpleRequestFields *fields,
										   const char *id);

/**
 * purple_request_fields_get_integer:
 * @fields: The fields list.
 * @id:     The ID of the field.
 *
 * Returns the integer value of a field with the specified ID.
 *
 * Returns: The integer value, if found, or 0 otherwise.
 */
int purple_request_fields_get_integer(const PurpleRequestFields *fields,
									const char *id);

/**
 * purple_request_fields_get_bool:
 * @fields: The fields list.
 * @id:     The ID of the field.
 *
 * Returns the boolean value of a field with the specified ID.
 *
 * Returns: The boolean value, if found, or %FALSE otherwise.
 */
gboolean purple_request_fields_get_bool(const PurpleRequestFields *fields,
									  const char *id);

/**
 * purple_request_fields_get_choice:
 * @fields: The fields list.
 * @id:     The ID of the field.
 *
 * Returns the choice index of a field with the specified ID.
 *
 * Returns: The choice value, if found, or NULL otherwise.
 */
gpointer
purple_request_fields_get_choice(const PurpleRequestFields *fields,
	const char *id);

/**
 * purple_request_fields_get_account:
 * @fields: The fields list.
 * @id:     The ID of the field.
 *
 * Returns the account of a field with the specified ID.
 *
 * Returns: (transfer none): The account value, if found, or %NULL otherwise.
 */
PurpleAccount *purple_request_fields_get_account(const PurpleRequestFields *fields,
											 const char *id);

/**
 * purple_request_fields_get_ui_data:
 * @fields: The fields list.
 *
 * Returns the UI data associated with this object.
 *
 * Returns: The UI data associated with this object.  This is a
 *         convenience field provided to the UIs--it is not
 *         used by the libpurple core.
 */
gpointer purple_request_fields_get_ui_data(const PurpleRequestFields *fields);

/**
 * purple_request_fields_set_ui_data:
 * @fields: The fields list.
 * @ui_data: A pointer to associate with this object.
 *
 * Set the UI data associated with this object.
 */
void purple_request_fields_set_ui_data(PurpleRequestFields *fields, gpointer ui_data);

G_END_DECLS

#endif /* PURPLE_REQUEST_PAGE_H */
