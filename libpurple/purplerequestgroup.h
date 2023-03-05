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

#ifndef PURPLE_REQUEST_GROUP_H
#define PURPLE_REQUEST_GROUP_H

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

/**
 * PurpleRequestFieldGroup:
 *
 * A group of fields with a title.
 */
typedef struct _PurpleRequestFieldGroup PurpleRequestFieldGroup;

#include "purplerequestfield.h"

G_BEGIN_DECLS

/**
 * purple_request_field_group_new:
 * @title: The optional title to give the group.
 *
 * Creates a fields group with an optional title.
 *
 * Returns: (transfer full): A new fields group
 */
PurpleRequestFieldGroup *purple_request_field_group_new(const char *title);

/**
 * purple_request_field_group_destroy:
 * @group: The group to destroy.
 *
 * Destroys a fields group.
 */
void purple_request_field_group_destroy(PurpleRequestFieldGroup *group);

/**
 * purple_request_field_group_add_field:
 * @group: The group to add the field to.
 * @field: The field to add to the group.
 *
 * Adds a field to the group.
 */
void purple_request_field_group_add_field(PurpleRequestFieldGroup *group,
										PurpleRequestField *field);

/**
 * purple_request_field_group_get_title:
 * @group: The group.
 *
 * Returns the title of a fields group.
 *
 * Returns: The title, if set.
 */
const char *purple_request_field_group_get_title(
		const PurpleRequestFieldGroup *group);

/**
 * purple_request_field_group_get_fields:
 * @group: The group.
 *
 * Returns a list of all fields in a group.
 *
 * Returns: (element-type PurpleRequestField) (transfer none): The list of fields in the group.
 */
GList *purple_request_field_group_get_fields(
		const PurpleRequestFieldGroup *group);

/**
 * purple_request_field_group_get_fields_list:
 * @group: The group.
 *
 * Returns a list of all fields in a group.
 *
 * Returns: (transfer none): The list of fields in the group.
 */
PurpleRequestFields *purple_request_field_group_get_fields_list(
		const PurpleRequestFieldGroup *group);

G_END_DECLS

#endif /* PURPLE_REQUEST_GROUP_H */
