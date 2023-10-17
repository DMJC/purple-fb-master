/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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

#include <glib/gi18n-lib.h>

#include <purple.h>

#include "pidgin/pidginaccountrow.h"

struct _PidginAccountRow {
	AdwComboRow parent;

	GtkFilterListModel *filter;
};

enum {
	PROP_0,
	PROP_ACCOUNT,
	PROP_FILTER,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE(PidginAccountRow, pidgin_account_row, ADW_TYPE_COMBO_ROW)

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_account_row_changed_cb(G_GNUC_UNUSED GObject *obj,
                              G_GNUC_UNUSED GParamSpec *pspec,
                              gpointer data)
{
	g_object_notify_by_pspec(G_OBJECT(data), properties[PROP_ACCOUNT]);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_account_row_get_property(GObject *obj, guint param_id, GValue *value,
                                GParamSpec *pspec)
{
	PidginAccountRow *row = PIDGIN_ACCOUNT_ROW(obj);

	switch(param_id) {
		case PROP_ACCOUNT:
			g_value_set_object(value, pidgin_account_row_get_account(row));
			break;
		case PROP_FILTER:
			g_value_set_object(value, pidgin_account_row_get_filter(row));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
pidgin_account_row_set_property(GObject *obj, guint param_id,
                                const GValue *value, GParamSpec *pspec)
{
	PidginAccountRow *row = PIDGIN_ACCOUNT_ROW(obj);

	switch(param_id) {
		case PROP_ACCOUNT:
			pidgin_account_row_set_account(row, g_value_get_object(value));
			break;
		case PROP_FILTER:
			pidgin_account_row_set_filter(row, g_value_get_object(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
pidgin_account_row_init(PidginAccountRow *row) {
	GListModel *model = NULL;

	gtk_widget_init_template(GTK_WIDGET(row));

	model = purple_account_manager_get_default_as_model();
	gtk_filter_list_model_set_model(row->filter, model);
}

static void
pidgin_account_row_class_init(PidginAccountRowClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->get_property = pidgin_account_row_get_property;
	obj_class->set_property = pidgin_account_row_set_property;

	/**
	 * PidginAccountRow:account:
	 *
	 * The [class@Purple.Account] that is selected.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", "account",
		"The account that is selected.",
		PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PidginAccountRow:filter:
	 *
	 * The filter to use on the list of accounts.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_FILTER] = g_param_spec_object(
	        "filter", "filter",
	        "The filter to be applied on the list of accounts.",
	        GTK_TYPE_FILTER,
	        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/account-row.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginAccountRow,
	                                     filter);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_account_row_changed_cb);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_account_row_new(void) {
	return g_object_new(PIDGIN_TYPE_ACCOUNT_ROW, NULL);
}

PurpleAccount *
pidgin_account_row_get_account(PidginAccountRow *row) {
	g_return_val_if_fail(PIDGIN_IS_ACCOUNT_ROW(row), NULL);

	return adw_combo_row_get_selected_item(ADW_COMBO_ROW(row));
}

void
pidgin_account_row_set_account(PidginAccountRow *row, PurpleAccount *account) {
	GListModel *model = NULL;
	guint n_items = 0;

	g_return_if_fail(PIDGIN_IS_ACCOUNT_ROW(row));

	model = adw_combo_row_get_model(ADW_COMBO_ROW(row));
	g_return_if_fail(G_IS_LIST_MODEL(model));

	n_items = g_list_model_get_n_items(model);
	for(guint position = 0; position < n_items; position++) {
		PurpleAccount *account1 = g_list_model_get_item(model, position);

		if(account1 == account) {
			/* NOTE: Property notification occurs in 'changed' signal
			 * callback.
			 */
			adw_combo_row_set_selected(ADW_COMBO_ROW(row), position);

			g_clear_object(&account1);

			return;
		}

		g_clear_object(&account1);
	}
}

GtkFilter *
pidgin_account_row_get_filter(PidginAccountRow *row) {
	g_return_val_if_fail(PIDGIN_IS_ACCOUNT_ROW(row), NULL);

	return gtk_filter_list_model_get_filter(row->filter);
}

void
pidgin_account_row_set_filter(PidginAccountRow *row, GtkFilter *filter) {
	g_return_if_fail(PIDGIN_IS_ACCOUNT_ROW(row));

	gtk_filter_list_model_set_filter(row->filter, filter);
	g_object_notify_by_pspec(G_OBJECT(row), properties[PROP_FILTER]);
}

