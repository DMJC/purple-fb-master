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

#ifndef PURPLE_REQUEST_FIELD_H
#define PURPLE_REQUEST_FIELD_H

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

/**
 * PurpleRequestField:
 *
 * A request field.
 */
typedef struct _PurpleRequestField PurpleRequestField;

#include "request.h"
#include "request-datasheet.h"

#define PURPLE_DEFAULT_ACTION_NONE	-1

/**
 * PurpleRequestFieldClass:
 *
 * #PurpleRequestFieldClass defines the interface for a request field.
 *
 * Since: 3.0.0
 */
struct _PurpleRequestFieldClass {
	/*< private >*/
	GObjectClass parent_class;

	/*< public >*/

	/*< private >*/
	gpointer reserved[4];
};

/**
 * PurpleRequestFieldType:
 * @PURPLE_REQUEST_FIELD_NONE: No field.
 * @PURPLE_REQUEST_FIELD_IMAGE: Image field.
 * @PURPLE_REQUEST_FIELD_DATASHEET: Datasheet field.
 *
 * A type of field.
 */
typedef enum
{
	PURPLE_REQUEST_FIELD_NONE,
	PURPLE_REQUEST_FIELD_IMAGE,
	PURPLE_REQUEST_FIELD_DATASHEET

} PurpleRequestFieldType;

typedef gboolean (*PurpleRequestFieldValidator)(PurpleRequestField *field,
	gchar **errmsg, gpointer user_data);

G_BEGIN_DECLS

/**************************************************************************/
/* Field API                                                              */
/**************************************************************************/

#define PURPLE_TYPE_REQUEST_FIELD (purple_request_field_get_type())
G_DECLARE_DERIVABLE_TYPE(PurpleRequestField, purple_request_field,
                         PURPLE, REQUEST_FIELD, GObject)

/**
 * purple_request_field_new:
 * @id:   The field ID.
 * @text: The text label of the field.
 * @type: The type of field.
 *
 * Creates a field of the specified type.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_new(const char *id, const char *text,
										 PurpleRequestFieldType type);

/**
 * purple_request_field_set_label:
 * @field: The field.
 * @label: The text label.
 *
 * Sets the label text of a field.
 */
void purple_request_field_set_label(PurpleRequestField *field, const char *label);

/**
 * purple_request_field_set_visible:
 * @field:   The field.
 * @visible: TRUE if visible, or FALSE if not.
 *
 * Sets whether or not a field is visible.
 */
void purple_request_field_set_visible(PurpleRequestField *field, gboolean visible);

/**
 * purple_request_field_set_type_hint:
 * @field:     The field.
 * @type_hint: The type hint.
 *
 * Sets the type hint for the field.
 *
 * This is optionally used by the UIs to provide such features as
 * auto-completion for type hints like "account" and "screenname".
 */
void purple_request_field_set_type_hint(PurpleRequestField *field,
									  const char *type_hint);

/**
 * purple_request_field_set_tooltip:
 * @field:     The field.
 * @tooltip:   The tooltip text.
 *
 * Sets the tooltip for the field.
 *
 * This is optionally used by the UIs to provide a tooltip for
 * the field.
 */
void purple_request_field_set_tooltip(PurpleRequestField *field,
									const char *tooltip);

/**
 * purple_request_field_set_required:
 * @field:    The field.
 * @required: TRUE if required, or FALSE.
 *
 * Sets whether or not a field is required.
 */
void purple_request_field_set_required(PurpleRequestField *field,
									 gboolean required);

/**
 * purple_request_field_get_field_type:
 * @field: The field.
 *
 * Returns the type of a field.
 *
 * Returns: The field's type.
 */
PurpleRequestFieldType purple_request_field_get_field_type(PurpleRequestField *field);

/**
 * purple_request_field_get_group:
 * @field: The field.
 *
 * Returns the group for the field.
 *
 * Returns: (transfer none): The UI data.
 */
PurpleRequestGroup *purple_request_field_get_group(PurpleRequestField *field);

/**
 * purple_request_field_get_id:
 * @field: The field.
 *
 * Returns the ID of a field.
 *
 * Returns: The ID
 */
const char *purple_request_field_get_id(PurpleRequestField *field);

/**
 * purple_request_field_get_label:
 * @field: The field.
 *
 * Returns the label text of a field.
 *
 * Returns: The label text.
 */
const char *purple_request_field_get_label(PurpleRequestField *field);

/**
 * purple_request_field_is_visible:
 * @field: The field.
 *
 * Returns whether or not a field is visible.
 *
 * Returns: TRUE if the field is visible. FALSE otherwise.
 */
gboolean purple_request_field_is_visible(PurpleRequestField *field);

/**
 * purple_request_field_get_type_hint:
 * @field: The field.
 *
 * Returns the field's type hint.
 *
 * Returns: The field's type hint.
 */
const char *purple_request_field_get_type_hint(PurpleRequestField *field);

/**
 * purple_request_field_get_tooltip:
 * @field: The field.
 *
 * Returns the field's tooltip.
 *
 * Returns: The field's tooltip.
 */
const char *purple_request_field_get_tooltip(PurpleRequestField *field);

/**
 * purple_request_field_is_required:
 * @field: The field.
 *
 * Returns whether or not a field is required.
 *
 * Returns: TRUE if the field is required, or FALSE.
 */
gboolean purple_request_field_is_required(PurpleRequestField *field);

/**
 * purple_request_field_is_filled:
 * @field: The field.
 *
 * Checks, if specified field has value.
 *
 * Returns: TRUE if the field has value, or FALSE.
 */
gboolean purple_request_field_is_filled(PurpleRequestField *field);

/**
 * purple_request_field_set_validator:
 * @field:     The field.
 * @validator: (scope notified): The validator callback, NULL to disable validation.
 * @user_data: The data to pass to the callback.
 *
 * Sets validator for a single field.
 */
void purple_request_field_set_validator(PurpleRequestField *field,
	PurpleRequestFieldValidator validator, void *user_data);

/**
 * purple_request_field_is_validatable:
 * @field: The field.
 *
 * Returns whether or not field has validator set.
 *
 * Returns: TRUE if the field has validator, or FALSE.
 */
gboolean purple_request_field_is_validatable(PurpleRequestField *field);

/**
 * purple_request_field_is_valid:
 * @field:  The field.
 * @errmsg: If non-NULL, the memory area, where the pointer to validation
 *         failure message will be set.
 *
 * Checks, if specified field is valid.
 *
 * If detailed message about failure reason is needed, there is an option to
 * return (via errmsg argument) pointer to newly allocated error message.
 * It must be freed with g_free after use.
 *
 * Note: empty, not required fields are valid.
 *
 * Returns: TRUE, if the field is valid, FALSE otherwise.
 */
gboolean purple_request_field_is_valid(PurpleRequestField *field, gchar **errmsg);

/**
 * purple_request_field_set_sensitive:
 * @field:     The field.
 * @sensitive: TRUE if the field should be sensitive for user input.
 *
 * Sets field editable.
 */
void purple_request_field_set_sensitive(PurpleRequestField *field,
	gboolean sensitive);

/**
 * purple_request_field_is_sensitive:
 * @field: The field.
 *
 * Checks, if field is editable.
 *
 * Returns: TRUE, if the field is sensitive for user input.
 */
gboolean purple_request_field_is_sensitive(PurpleRequestField *field);

/**************************************************************************/
/* Image Field API                                                        */
/**************************************************************************/

/**
 * purple_request_field_image_new:
 * @id:   The field ID.
 * @text: The label of the field.
 * @buf:  The image data.
 * @size: The size of the data in @buf.
 *
 * Creates an image field.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_image_new(const char *id, const char *text,
											   const char *buf, gsize size);

/**
 * purple_request_field_image_set_scale:
 * @field: The image field.
 * @x:     The x scale factor.
 * @y:     The y scale factor.
 *
 * Sets the scale factors of an image field.
 */
void purple_request_field_image_set_scale(PurpleRequestField *field, unsigned int x, unsigned int y);

/**
 * purple_request_field_image_get_buffer:
 * @field: The image field.
 *
 * Returns pointer to the image.
 *
 * Returns: Pointer to the image.
 */
const char *purple_request_field_image_get_buffer(PurpleRequestField *field);

/**
 * purple_request_field_image_get_size:
 * @field: The image field.
 *
 * Returns size (in bytes) of the image.
 *
 * Returns: Size of the image.
 */
gsize purple_request_field_image_get_size(PurpleRequestField *field);

/**
 * purple_request_field_image_get_scale_x:
 * @field: The image field.
 *
 * Returns X scale coefficient of the image.
 *
 * Returns: X scale coefficient of the image.
 */
unsigned int purple_request_field_image_get_scale_x(PurpleRequestField *field);

/**
 * purple_request_field_image_get_scale_y:
 * @field: The image field.
 *
 * Returns Y scale coefficient of the image.
 *
 * Returns: Y scale coefficient of the image.
 */
unsigned int purple_request_field_image_get_scale_y(PurpleRequestField *field);

/**************************************************************************/
/* Datasheet Field API                                                    */
/**************************************************************************/

/**
 * purple_request_field_datasheet_new:
 * @id:    The field ID.
 * @text:  The label of the field, may be %NULL.
 * @sheet: The datasheet.
 *
 * Creates a datasheet item field.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_datasheet_new(const char *id,
	const gchar *text, PurpleRequestDatasheet *sheet);

/**
 * purple_request_field_datasheet_get_sheet:
 * @field: The field.
 *
 * Returns a datasheet for a field.
 *
 * Returns: (transfer none): The datasheet object.
 */
PurpleRequestDatasheet *purple_request_field_datasheet_get_sheet(
	PurpleRequestField *field);

/**************************************************************************/
/* Validators for request fields.                                         */
/**************************************************************************/

/**
 * purple_request_field_email_validator:
 * @field: The field.
 * @errmsg: (out) (optional): destination for error message.
 * @user_data: Ignored.
 *
 * Validates a field which should contain an email address.
 *
 * See purple_request_field_set_validator().
 *
 * Returns: TRUE, if field contains valid email address.
 */
gboolean purple_request_field_email_validator(PurpleRequestField *field,
	gchar **errmsg, void *user_data);

/**
 * purple_request_field_alphanumeric_validator:
 * @field: The field.
 * @errmsg: (allow-none): destination for error message.
 * @allowed_characters: (allow-none): allowed character list
 *                      (NULL-terminated string).
 *
 * Validates a field which should contain alphanumeric content.
 *
 * See purple_request_field_set_validator().
 *
 * Returns: TRUE, if field contains only alphanumeric characters.
 */
gboolean purple_request_field_alphanumeric_validator(PurpleRequestField *field,
	gchar **errmsg, void *allowed_characters);

G_END_DECLS

#endif /* PURPLE_REQUEST_FIELD_H */
