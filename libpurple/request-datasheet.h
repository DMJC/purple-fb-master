/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_REQUEST_DATA_H
#define PURPLE_REQUEST_DATA_H

#include <glib.h>

#include "purpleversion.h"

/**
 * PurpleRequestDatasheet:
 *
 * Request Datasheet is a UI abstracted table.
 */
typedef struct _PurpleRequestDatasheet PurpleRequestDatasheet;

/**
 * PurpleRequestDatasheetRecord:
 *
 * A data sheet record.
 */
typedef struct _PurpleRequestDatasheetRecord PurpleRequestDatasheetRecord;

/**
 * PurpleRequestDatasheetAction:
 *
 * A data sheet action.
 */
typedef struct _PurpleRequestDatasheetAction PurpleRequestDatasheetAction;

/**
 * PurpleRequestDatasheetActionCb:
 * @rec: The record.
 * @user_data: User data.
 *
 * A callback function for actions in a data sheet.
 *
 * Since: 3.0
 */
typedef void (*PurpleRequestDatasheetActionCb)(
	PurpleRequestDatasheetRecord *rec, gpointer user_data);

/**
 * PurpleRequestDatasheetActionCheckCb:
 * @rec: The record.
 * @user_data: User data.
 *
 * A callback function for checking if an action is enabled or not.
 *
 * Returns: %TRUE if the actions should be enabled, otherwise %FALSE.
 *
 * Since: 3.0
 */
typedef gboolean (*PurpleRequestDatasheetActionCheckCb)(
	PurpleRequestDatasheetRecord *rec, gpointer user_data);

/**
 * PurpleRequestDatasheetColumnType:
 * @PURPLE_REQUEST_DATASHEET_COLUMN_STRING: The column displays strings.
 * @PURPLE_REQUEST_DATASHEET_COLUMN_IMAGE: The column displays images.
 *
 * The type of data to be shown in a column.
 */

typedef enum
{
	PURPLE_REQUEST_DATASHEET_COLUMN_STRING,
	PURPLE_REQUEST_DATASHEET_COLUMN_IMAGE
} PurpleRequestDatasheetColumnType;

G_BEGIN_DECLS

/**************************************************************************/
/* Datasheet API                                                          */
/**************************************************************************/

/**
 * purple_request_datasheet_new:
 *
 * Creates new Datasheet.
 *
 * Returns: (transfer full): The new datasheet.
 */
PURPLE_AVAILABLE_IN_3_0
PurpleRequestDatasheet *
purple_request_datasheet_new(void);

/**
 * purple_request_datasheet_free:
 * @sheet: The datasheet.
 *
 * Destroys datasheet with all its contents.
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_free(PurpleRequestDatasheet *sheet);

/**
 * purple_request_datasheet_add_column:
 * @sheet: The datasheet.
 * @type:  The column type.
 * @title: The column title (may be %NULL).
 *
 * Adds a column to the datasheet.
 *
 * You cannot add a column if datasheet contains any data.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_add_column(PurpleRequestDatasheet *sheet,
	PurpleRequestDatasheetColumnType type, const gchar *title);

/**
 * purple_request_datasheet_get_column_count:
 * @sheet: The datasheet.
 *
 * Returns the column count of datasheet.
 *
 * Returns: The column count.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
guint
purple_request_datasheet_get_column_count(PurpleRequestDatasheet *sheet);

/**
 * purple_request_datasheet_get_column_type:
 * @sheet:  The datasheet.
 * @col_no: The column number (0 is the first one).
 *
 * Returns the column type for a datasheet.
 *
 * Returns: The column type.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleRequestDatasheetColumnType
purple_request_datasheet_get_column_type(PurpleRequestDatasheet *sheet,
	guint col_no);

/**
 * purple_request_datasheet_get_column_title:
 * @sheet:  The datasheet.
 * @col_no: The column number (0 is the first one).
 *
 * Returns the column title for a datasheet.
 *
 * Returns: The column title.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar *
purple_request_datasheet_get_column_title(PurpleRequestDatasheet *sheet,
	guint col_no);

/**
 * purple_request_datasheet_get_records:
 * @sheet: The datasheet.
 *
 * Returns the list of records in a datasheet.
 *
 * You shouldn't modify datasheet's data while iterating through it.
 *
 * Returns: (element-type PurpleRequestDatasheetRecord) (transfer none): The list of records.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const GList *
purple_request_datasheet_get_records(PurpleRequestDatasheet *sheet);

/**
 * purple_request_datasheet_add_action:
 * @sheet:  The datasheet.
 * @action: The action.
 *
 * Adds an action to the datasheet.
 *
 * Action object is owned by the datasheet since this call.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_add_action(PurpleRequestDatasheet *sheet,
	PurpleRequestDatasheetAction *action);

/**
 * purple_request_datasheet_get_actions:
 * @sheet: The datasheet.
 *
 * Returns the list of actions in a datasheet.
 *
 * Returns: (element-type PurpleRequestDatasheetAction) (transfer none): The list of actions.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const GList *
purple_request_datasheet_get_actions(PurpleRequestDatasheet *sheet);


/**************************************************************************/
/* Datasheet actions API                                                  */
/**************************************************************************/

/**
 * purple_request_datasheet_action_new:
 *
 * Creates new datasheet action.
 *
 * Returns: (transfer full): The new action.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleRequestDatasheetAction *
purple_request_datasheet_action_new(void);

/**
 * purple_request_datasheet_action_free:
 * @act: The action.
 *
 * Destroys the datasheet action.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_action_free(PurpleRequestDatasheetAction *act);

/**
 * purple_request_datasheet_action_set_label:
 * @act:   The action.
 * @label: The label.
 *
 * Sets the localized label for the action.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_action_set_label(PurpleRequestDatasheetAction *act,
	const gchar *label);

/**
 * purple_request_datasheet_action_get_label:
 * @act: The action.
 *
 * Gets the label of action.
 *
 * Returns: The localized label text.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar*
purple_request_datasheet_action_get_label(PurpleRequestDatasheetAction *act);

/**
 * purple_request_datasheet_action_set_cb:
 * @act:       The action.
 * @cb: (scope notified):       The callback function.
 * @user_data: The data to be passed to the callback function.
 *
 * Sets the callback for the action.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_action_set_cb(PurpleRequestDatasheetAction *act,
	PurpleRequestDatasheetActionCb cb, gpointer user_data);

/**
 * purple_request_datasheet_action_call:
 * @act: The action.
 * @rec: The user selected record.
 *
 * Calls the callback of the action.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_action_call(PurpleRequestDatasheetAction *act,
	PurpleRequestDatasheetRecord *rec);

/**
 * purple_request_datasheet_action_set_sens_cb:
 * @act:       The action.
 * @cb: (scope notified):       The callback function, may be %NULL.
 * @user_data: The data to be passed to the callback function.
 *
 * Sets the sensitivity checker for the action.
 *
 * If there is no callback set, default is used: the action is enabled, if any
 * record is active.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_action_set_sens_cb(
	PurpleRequestDatasheetAction *act,
	PurpleRequestDatasheetActionCheckCb cb, gpointer user_data);

/**
 * purple_request_datasheet_action_is_sensitive:
 * @act: The action.
 * @rec: The record.
 *
 * Checks, if the action is enabled for the active record.
 *
 * Returns: %TRUE, if the action is enabled, %FALSE otherwise.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean
purple_request_datasheet_action_is_sensitive(PurpleRequestDatasheetAction *act,
	PurpleRequestDatasheetRecord *rec);


/**************************************************************************/
/* Datasheet record API                                                   */
/**************************************************************************/

/**
 * purple_request_datasheet_record_get_key:
 * @rec: The record.
 *
 * Returns the key of a record.
 *
 * Returns: The key.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gpointer
purple_request_datasheet_record_get_key(
	const PurpleRequestDatasheetRecord *rec);

/**
 * purple_request_datasheet_record_get_datasheet:
 * @rec: The record.
 *
 * Returns the datasheet of a record.
 *
 * Returns: (transfer none): The datasheet.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleRequestDatasheet *
purple_request_datasheet_record_get_datasheet(
	PurpleRequestDatasheetRecord *rec);

/**
 * purple_request_datasheet_record_find:
 * @sheet: The datasheet.
 * @key:   The key.
 *
 * Looks up for a record in datasheet.
 *
 * Returns: (transfer none): The record if found, %NULL otherwise.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleRequestDatasheetRecord *
purple_request_datasheet_record_find(PurpleRequestDatasheet *sheet,
	gpointer key);

/**
 * purple_request_datasheet_record_add:
 * @sheet: The datasheet.
 * @key:   The key.
 *
 * Adds a record to the datasheet.
 *
 * If the specified key already exists in datasheet, old record is returned.
 *
 * Returns: (transfer full): The record.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleRequestDatasheetRecord *
purple_request_datasheet_record_add(PurpleRequestDatasheet *sheet,
	gpointer key);

/**
 * purple_request_datasheet_record_remove:
 * @sheet: The datasheet.
 * @key:   The key.
 *
 * Removes a record from a datasheet.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_record_remove(PurpleRequestDatasheet *sheet,
	gpointer key);

/**
 * purple_request_datasheet_record_remove_all:
 * @sheet: The datasheet.
 *
 * Removes all records from a datasheet.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_record_remove_all(PurpleRequestDatasheet *sheet);

/**
 * purple_request_datasheet_record_mark_all_for_rem:
 * @sheet: The datasheet.
 *
 * Marks all records for removal. Record will be unmarked, when touched with
 * purple_request_datasheet_record_add.
 *
 * See purple_request_datasheet_record_add().
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_record_mark_all_for_rem(PurpleRequestDatasheet *sheet);

/**
 * purple_request_datasheet_record_remove_marked:
 * @sheet: The datasheet.
 *
 * Removes all marked records.
 *
 * See purple_request_datasheet_record_mark_all_for_rem().
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_record_remove_marked(PurpleRequestDatasheet *sheet);

/**
 * purple_request_datasheet_record_set_string_data:
 * @rec:    The record.
 * @col_no: The column.
 * @data:   The data.
 *
 * Sets data for a string column of specified record.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_record_set_string_data(
	PurpleRequestDatasheetRecord *rec, guint col_no, const gchar *data);

/**
 * purple_request_datasheet_record_set_image_data:
 * @rec:      The record.
 * @col_no:   The column.
 * @stock_id: The stock identifier of a image.
 *
 * Sets data for a image column of specified record.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void
purple_request_datasheet_record_set_image_data(
	PurpleRequestDatasheetRecord *rec, guint col_no, const gchar *stock_id);

/**
 * purple_request_datasheet_record_get_string_data:
 * @rec:    The record.
 * @col_no: The column.
 *
 * Returns data for a string column of specified record.
 *
 * Returns: The data.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar *
purple_request_datasheet_record_get_string_data(
	const PurpleRequestDatasheetRecord *rec, guint col_no);

/**
 * purple_request_datasheet_record_get_image_data:
 * @rec:    The record.
 * @col_no: The column.
 *
 * Returns data for an image column of specified record.
 *
 * Returns: The stock id of an image.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar *
purple_request_datasheet_record_get_image_data(
	const PurpleRequestDatasheetRecord *rec, guint col_no);

G_END_DECLS

#endif /* PURPLE_REQUEST_DATA_H */
