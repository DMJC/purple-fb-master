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

#include "account.h"
#include "request-datasheet.h"

#define PURPLE_DEFAULT_ACTION_NONE	-1

/**
 * PurpleRequestFieldType:
 * @PURPLE_REQUEST_FIELD_NONE: No field.
 * @PURPLE_REQUEST_FIELD_STRING: String field.
 * @PURPLE_REQUEST_FIELD_INTEGER: Integer field.
 * @PURPLE_REQUEST_FIELD_BOOLEAN: Boolean field.
 * @PURPLE_REQUEST_FIELD_CHOICE: Choice field (dropdown?).
 * @PURPLE_REQUEST_FIELD_LIST: List field.
 * @PURPLE_REQUEST_FIELD_LABEL: Label field.
 * @PURPLE_REQUEST_FIELD_IMAGE: Image field.
 * @PURPLE_REQUEST_FIELD_ACCOUNT: #PurpleAccount field.
 * @PURPLE_REQUEST_FIELD_DATASHEET: Datasheet field.
 *
 * A type of field.
 */
typedef enum
{
	PURPLE_REQUEST_FIELD_NONE,
	PURPLE_REQUEST_FIELD_STRING,
	PURPLE_REQUEST_FIELD_INTEGER,
	PURPLE_REQUEST_FIELD_BOOLEAN,
	PURPLE_REQUEST_FIELD_CHOICE,
	PURPLE_REQUEST_FIELD_LIST,
	PURPLE_REQUEST_FIELD_LABEL,
	PURPLE_REQUEST_FIELD_IMAGE,
	PURPLE_REQUEST_FIELD_ACCOUNT,
	PURPLE_REQUEST_FIELD_DATASHEET

} PurpleRequestFieldType;

typedef gboolean (*PurpleRequestFieldValidator)(PurpleRequestField *field,
	gchar **errmsg, gpointer user_data);

G_BEGIN_DECLS

/**************************************************************************/
/* Field API                                                              */
/**************************************************************************/

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
 * purple_request_field_destroy:
 * @field: The field to destroy.
 *
 * Destroys a field.
 */
void purple_request_field_destroy(PurpleRequestField *field);

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
PurpleRequestFieldType purple_request_field_get_field_type(const PurpleRequestField *field);

/**
 * purple_request_field_get_group:
 * @field: The field.
 *
 * Returns the group for the field.
 *
 * Returns: (transfer none): The UI data.
 */
PurpleRequestFieldGroup *purple_request_field_get_group(const PurpleRequestField *field);

/**
 * purple_request_field_get_id:
 * @field: The field.
 *
 * Returns the ID of a field.
 *
 * Returns: The ID
 */
const char *purple_request_field_get_id(const PurpleRequestField *field);

/**
 * purple_request_field_get_label:
 * @field: The field.
 *
 * Returns the label text of a field.
 *
 * Returns: The label text.
 */
const char *purple_request_field_get_label(const PurpleRequestField *field);

/**
 * purple_request_field_is_visible:
 * @field: The field.
 *
 * Returns whether or not a field is visible.
 *
 * Returns: TRUE if the field is visible. FALSE otherwise.
 */
gboolean purple_request_field_is_visible(const PurpleRequestField *field);

/**
 * purple_request_field_get_field_type_hint:
 * @field: The field.
 *
 * Returns the field's type hint.
 *
 * Returns: The field's type hint.
 */
const char *purple_request_field_get_field_type_hint(const PurpleRequestField *field);

/**
 * purple_request_field_get_tooltip:
 * @field: The field.
 *
 * Returns the field's tooltip.
 *
 * Returns: The field's tooltip.
 */
const char *purple_request_field_get_tooltip(const PurpleRequestField *field);

/**
 * purple_request_field_is_required:
 * @field: The field.
 *
 * Returns whether or not a field is required.
 *
 * Returns: TRUE if the field is required, or FALSE.
 */
gboolean purple_request_field_is_required(const PurpleRequestField *field);

/**
 * purple_request_field_is_filled:
 * @field: The field.
 *
 * Checks, if specified field has value.
 *
 * Returns: TRUE if the field has value, or FALSE.
 */
gboolean purple_request_field_is_filled(const PurpleRequestField *field);

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

/**
 * purple_request_field_get_ui_data:
 * @field: The field.
 *
 * Returns the ui_data for a field.
 *
 * Returns: The UI data.
 */
gpointer purple_request_field_get_ui_data(const PurpleRequestField *field);

/**
 * purple_request_field_set_ui_data:
 * @field:   The field.
 * @ui_data: The UI data.
 *
 * Sets the ui_data for a field.
 */
void purple_request_field_set_ui_data(PurpleRequestField *field,
                                      gpointer ui_data);

/**************************************************************************/
/* String Field API                                                       */
/**************************************************************************/

/**
 * purple_request_field_string_new:
 * @id:            The field ID.
 * @text:          The text label of the field.
 * @default_value: The optional default value.
 * @multiline:     Whether or not this should be a multiline string.
 *
 * Creates a string request field.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_string_new(const char *id,
												const char *text,
												const char *default_value,
												gboolean multiline);

/**
 * purple_request_field_string_set_default_value:
 * @field:         The field.
 * @default_value: The default value.
 *
 * Sets the default value in a string field.
 */
void purple_request_field_string_set_default_value(PurpleRequestField *field,
												 const char *default_value);

/**
 * purple_request_field_string_set_value:
 * @field: The field.
 * @value: The value.
 *
 * Sets the value in a string field.
 */
void purple_request_field_string_set_value(PurpleRequestField *field,
										 const char *value);

/**
 * purple_request_field_string_set_masked:
 * @field:  The field.
 * @masked: The masked value.
 *
 * Sets whether or not a string field is masked
 * (commonly used for password fields).
 */
void purple_request_field_string_set_masked(PurpleRequestField *field,
										  gboolean masked);

/**
 * purple_request_field_string_get_default_value:
 * @field: The field.
 *
 * Returns the default value in a string field.
 *
 * Returns: The default value.
 */
const char *purple_request_field_string_get_default_value(
		const PurpleRequestField *field);

/**
 * purple_request_field_string_get_value:
 * @field: The field.
 *
 * Returns the user-entered value in a string field.
 *
 * Returns: The value.
 */
const char *purple_request_field_string_get_value(const PurpleRequestField *field);

/**
 * purple_request_field_string_is_multiline:
 * @field: The field.
 *
 * Returns whether or not a string field is multi-line.
 *
 * Returns: %TRUE if the field is mulit-line, or %FALSE otherwise.
 */
gboolean purple_request_field_string_is_multiline(const PurpleRequestField *field);

/**
 * purple_request_field_string_is_masked:
 * @field: The field.
 *
 * Returns whether or not a string field is masked.
 *
 * Returns: %TRUE if the field is masked, or %FALSE otherwise.
 */
gboolean purple_request_field_string_is_masked(const PurpleRequestField *field);

/**************************************************************************/
/* Integer Field API                                                      */
/**************************************************************************/

/**
 * purple_request_field_int_new:
 * @id:            The field ID.
 * @text:          The text label of the field.
 * @default_value: The default value.
 * @lower_bound:   The lower bound.
 * @upper_bound:   The upper bound.
 *
 * Creates an integer field.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_int_new(const char *id,
	const char *text, int default_value, int lower_bound, int upper_bound);

/**
 * purple_request_field_int_set_default_value:
 * @field:         The field.
 * @default_value: The default value.
 *
 * Sets the default value in an integer field.
 */
void purple_request_field_int_set_default_value(PurpleRequestField *field,
											  int default_value);

/**
 * purple_request_field_int_set_lower_bound:
 * @field:       The field.
 * @lower_bound: The lower bound.
 *
 * Sets the lower bound in an integer field.
 */
void purple_request_field_int_set_lower_bound(PurpleRequestField *field, int lower_bound);

/**
 * purple_request_field_int_set_upper_bound:
 * @field:       The field.
 * @upper_bound: The upper bound.
 *
 * Sets the upper bound in an integer field.
 */
void purple_request_field_int_set_upper_bound(PurpleRequestField *field, int upper_bound);

/**
 * purple_request_field_int_set_value:
 * @field: The field.
 * @value: The value.
 *
 * Sets the value in an integer field.
 */
void purple_request_field_int_set_value(PurpleRequestField *field, int value);

/**
 * purple_request_field_int_get_default_value:
 * @field: The field.
 *
 * Returns the default value in an integer field.
 *
 * Returns: The default value.
 */
int purple_request_field_int_get_default_value(const PurpleRequestField *field);

/**
 * purple_request_field_int_get_lower_bound:
 * @field: The field.
 *
 * Returns the lower bound in an integer field.
 *
 * Returns: The lower bound.
 */
int purple_request_field_int_get_lower_bound(const PurpleRequestField *field);

/**
 * purple_request_field_int_get_upper_bound:
 * @field: The field.
 *
 * Returns the upper bound in an integer field.
 *
 * Returns: The upper bound.
 */
int purple_request_field_int_get_upper_bound(const PurpleRequestField *field);

/**
 * purple_request_field_int_get_value:
 * @field: The field.
 *
 * Returns the user-entered value in an integer field.
 *
 * Returns: The value.
 */
int purple_request_field_int_get_value(const PurpleRequestField *field);

/**************************************************************************/
/* Boolean Field API                                                      */
/**************************************************************************/

/**
 * purple_request_field_bool_new:
 * @id:            The field ID.
 * @text:          The text label of the field.
 * @default_value: The default value.
 *
 * Creates a boolean field.
 *
 * This is often represented as a checkbox.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_bool_new(const char *id,
											  const char *text,
											  gboolean default_value);

/**
 * purple_request_field_bool_set_default_value:
 * @field:         The field.
 * @default_value: The default value.
 *
 * Sets the default value in an boolean field.
 */
void purple_request_field_bool_set_default_value(PurpleRequestField *field,
											   gboolean default_value);

/**
 * purple_request_field_bool_set_value:
 * @field: The field.
 * @value: The value.
 *
 * Sets the value in an boolean field.
 */
void purple_request_field_bool_set_value(PurpleRequestField *field,
									   gboolean value);

/**
 * purple_request_field_bool_get_default_value:
 * @field: The field.
 *
 * Returns the default value in an boolean field.
 *
 * Returns: The default value.
 */
gboolean purple_request_field_bool_get_default_value(
		const PurpleRequestField *field);

/**
 * purple_request_field_bool_get_value:
 * @field: The field.
 *
 * Returns the user-entered value in an boolean field.
 *
 * Returns: The value.
 */
gboolean purple_request_field_bool_get_value(const PurpleRequestField *field);

/**************************************************************************/
/* Choice Field API                                                       */
/**************************************************************************/

/**
 * purple_request_field_choice_new:
 * @id:            The field ID.
 * @text:          The optional label of the field.
 * @default_value: The default choice.
 *
 * Creates a multiple choice field.
 *
 * This is often represented as a group of radio buttons.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *
purple_request_field_choice_new(const char *id, const char *text,
	gpointer default_value);

/**
 * purple_request_field_choice_add:
 * @field: The choice field.
 * @label: The choice label.
 * @data:  The choice value.
 *
 * Adds a choice to a multiple choice field.
 */
void
purple_request_field_choice_add(PurpleRequestField *field, const char *label,
	gpointer data);

/**
 * purple_request_field_choice_add_full:
 * @field: The choice field.
 * @label: The choice label.
 * @data:  The choice value.
 * @destroy: The value destroy function.
 *
 * Adds a choice to a multiple choice field with destructor for value.
 *
 * Since: 3.0.0
 */
void
purple_request_field_choice_add_full(PurpleRequestField *field, const char *label,
                                     gpointer data, GDestroyNotify destroy);

/**
 * purple_request_field_choice_set_default_value:
 * @field:         The field.
 * @default_value: The default value.
 *
 * Sets the default value in an choice field.
 */
void
purple_request_field_choice_set_default_value(PurpleRequestField *field,
	gpointer default_value);

/**
 * purple_request_field_choice_set_value:
 * @field: The field.
 * @value: The value.
 *
 * Sets the value in an choice field.
 */
void
purple_request_field_choice_set_value(PurpleRequestField *field,
	gpointer value);

/**
 * purple_request_field_choice_get_default_value:
 * @field: The field.
 *
 * Returns the default value in an choice field.
 *
 * Returns: The default value.
 */
gpointer
purple_request_field_choice_get_default_value(const PurpleRequestField *field);

/**
 * purple_request_field_choice_get_value:
 * @field: The field.
 *
 * Returns the user-entered value in an choice field.
 *
 * Returns: The value.
 */
gpointer
purple_request_field_choice_get_value(const PurpleRequestField *field);

/**
 * purple_request_field_choice_get_elements:
 * @field: The field.
 *
 * Returns a list of elements in a choice field.
 *
 * Returns: (element-type PurpleKeyValuePair) (transfer none): The list of pairs of {label, value}.
 */
GList *
purple_request_field_choice_get_elements(const PurpleRequestField *field);

/**************************************************************************/
/* List Field API                                                         */
/**************************************************************************/

/**
 * purple_request_field_list_new:
 * @id:   The field ID.
 * @text: The optional label of the field.
 *
 * Creates a multiple list item field.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_list_new(const char *id, const char *text);

/**
 * purple_request_field_list_set_multi_select:
 * @field:        The list field.
 * @multi_select: TRUE if multiple selection is enabled,
 *                     or FALSE otherwise.
 *
 * Sets whether or not a list field allows multiple selection.
 */
void purple_request_field_list_set_multi_select(PurpleRequestField *field,
											  gboolean multi_select);

/**
 * purple_request_field_list_get_multi_select:
 * @field: The list field.
 *
 * Returns whether or not a list field allows multiple selection.
 *
 * Returns: TRUE if multiple selection is enabled, or FALSE otherwise.
 */
gboolean purple_request_field_list_get_multi_select(
	const PurpleRequestField *field);

/**
 * purple_request_field_list_get_data:
 * @field: The list field.
 * @text:  The item text.
 *
 * Returns the data for a particular item.
 *
 * Returns: The data associated with the item.
 */
void *purple_request_field_list_get_data(const PurpleRequestField *field,
									   const char *text);

/**
 * purple_request_field_list_add_icon:
 * @field:     The list field.
 * @item:      The list item.
 * @icon_path: The path to icon file, or %NULL for no icon.
 * @data:      The associated data.
 *
 * Adds an item to a list field.
 */
void purple_request_field_list_add_icon(PurpleRequestField *field,
								 const char *item, const char* icon_path, void* data);

/**
 * purple_request_field_list_add_selected:
 * @field: The field.
 * @item:  The item to add.
 *
 * Adds a selected item to the list field.
 */
void purple_request_field_list_add_selected(PurpleRequestField *field,
										  const char *item);

/**
 * purple_request_field_list_clear_selected:
 * @field: The field.
 *
 * Clears the list of selected items in a list field.
 */
void purple_request_field_list_clear_selected(PurpleRequestField *field);

/**
 * purple_request_field_list_set_selected:
 * @field: The field.
 * @items: (element-type utf8) (transfer none): The list of selected items.
 *
 * Sets a list of selected items in a list field.
 */
void purple_request_field_list_set_selected(PurpleRequestField *field,
										  GList *items);

/**
 * purple_request_field_list_is_selected:
 * @field: The field.
 * @item:  The item.
 *
 * Returns whether or not a particular item is selected in a list field.
 *
 * Returns: TRUE if the item is selected. FALSE otherwise.
 */
gboolean purple_request_field_list_is_selected(const PurpleRequestField *field,
											 const char *item);

/**
 * purple_request_field_list_get_selected:
 * @field: The field.
 *
 * Returns a list of selected items in a list field.
 *
 * To retrieve the data for each item, use
 * purple_request_field_list_get_data().
 *
 * Returns: (element-type utf8) (transfer none): The list of selected items.
 */
GList *purple_request_field_list_get_selected(
	const PurpleRequestField *field);

/**
 * purple_request_field_list_get_items:
 * @field: The field.
 *
 * Returns a list of items in a list field.
 *
 * Returns: (element-type PurpleKeyValuePair) (transfer none): The list of items.
 */
GList *purple_request_field_list_get_items(const PurpleRequestField *field);

/**
 * purple_request_field_list_has_icons:
 * @field: The field.
 *
 * Indicates if list field has icons.
 *
 * Returns: TRUE if list field has icons, FALSE otherwise.
 *
 * Since: 3.0.0
 */
gboolean purple_request_field_list_has_icons(const PurpleRequestField *field);

/**************************************************************************/
/* Label Field API                                                        */
/**************************************************************************/

/**
 * purple_request_field_label_new:
 * @id:   The field ID.
 * @text: The label of the field.
 *
 * Creates a label field.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_label_new(const char *id,
											   const char *text);

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
/* Account Field API                                                      */
/**************************************************************************/

/**
 * purple_request_field_account_new:
 * @id:      The field ID.
 * @text:    The text label of the field.
 * @account: The optional default account.
 *
 * Creates an account field.
 *
 * By default, this field will not show offline accounts.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_account_new(const char *id,
												 const char *text,
												 PurpleAccount *account);

/**
 * purple_request_field_account_set_default_value:
 * @field:         The account field.
 * @default_value: The default account.
 *
 * Sets the default account on an account field.
 */
void purple_request_field_account_set_default_value(PurpleRequestField *field,
												  PurpleAccount *default_value);

/**
 * purple_request_field_account_set_value:
 * @field: The account field.
 * @value: The account.
 *
 * Sets the account in an account field.
 */
void purple_request_field_account_set_value(PurpleRequestField *field,
										  PurpleAccount *value);

/**
 * purple_request_field_account_set_show_all:
 * @field:    The account field.
 * @show_all: Whether or not to show all accounts.
 *
 * Sets whether or not to show all accounts in an account field.
 *
 * If TRUE, all accounts, online or offline, will be shown. If FALSE,
 * only online accounts will be shown.
 */
void purple_request_field_account_set_show_all(PurpleRequestField *field,
											 gboolean show_all);

/**
 * purple_request_field_account_set_filter:
 * @field:       The account field.
 * @filter_func: (scope notified): The account filter function.
 *
 * Sets the account filter function in an account field.
 *
 * This function will determine which accounts get displayed and which
 * don't.
 */
void purple_request_field_account_set_filter(PurpleRequestField *field,
										   PurpleFilterAccountFunc filter_func);

/**
 * purple_request_field_account_get_default_value:
 * @field: The field.
 *
 * Returns the default account in an account field.
 *
 * Returns: (transfer none): The default account.
 */
PurpleAccount *purple_request_field_account_get_default_value(
		const PurpleRequestField *field);

/**
 * purple_request_field_account_get_value:
 * @field: The field.
 *
 * Returns the user-entered account in an account field.
 *
 * Returns: (transfer none): The user-entered account.
 */
PurpleAccount *purple_request_field_account_get_value(
		const PurpleRequestField *field);

/**
 * purple_request_field_account_get_show_all:
 * @field: The account field.
 *
 * Returns whether or not to show all accounts in an account field.
 *
 * If TRUE, all accounts, online or offline, will be shown. If FALSE,
 * only online accounts will be shown.
 *
 * Returns: Whether or not to show all accounts.
 */
gboolean purple_request_field_account_get_show_all(
		const PurpleRequestField *field);

/**
 * purple_request_field_account_get_filter:
 * @field: The account field.
 *
 * Returns the account filter function in an account field.
 *
 * This function will determine which accounts get displayed and which
 * don't.
 *
 * Returns: (transfer none): The account filter function.
 */
PurpleFilterAccountFunc purple_request_field_account_get_filter(
		const PurpleRequestField *field);

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
