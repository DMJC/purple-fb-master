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

#include <adwaita.h>

#include "pidginawayprefs.h"
#include "pidginiconname.h"
#include "pidginprefsinternal.h"

struct _PidginAwayPrefs {
	AdwPreferencesPage parent;

	GtkWidget *idle_reporting;
	GtkWidget *mins_before_away;
	GtkWidget *idle_row;
	GtkWidget *away_when_idle;
	GtkWidget *startup_current_status;
	GtkWidget *startup_row;
};

G_DEFINE_FINAL_TYPE(PidginAwayPrefs, pidgin_away_prefs,
                    ADW_TYPE_PREFERENCES_PAGE)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gchar *
idle_reporting_expression_cb(GObject *self, G_GNUC_UNUSED gpointer data) {
	const gchar *text = "";
	const gchar *value = NULL;

	value = gtk_string_object_get_string(GTK_STRING_OBJECT(self));
	if(purple_strequal(value, "none")) {
		text = _("Never");
	} else if(purple_strequal(value, "purple")) {
		text = _("From last sent message");
	} else if(purple_strequal(value, "system")) {
		text = _("Based on keyboard or mouse use");
	}

	return g_strdup(text);
}

static char *
idle_presence_icon_name_from_primitive(G_GNUC_UNUSED GObject *self,
                                       PurplePresencePrimitive primitive,
                                       G_GNUC_UNUSED gpointer data)
{
	const char *icon_name = NULL;

	icon_name = pidgin_icon_name_from_presence_primitive(primitive,
	                                                     "icon-missing");

	return g_strdup(icon_name);
}

static gboolean
idle_get_reporting_mapping(GValue *value, GVariant *variant,
                           G_GNUC_UNUSED gpointer data)
{
	const char *id = NULL;

	id = g_variant_get_string(variant, NULL);
	if(purple_strequal(id, "none")) {
		g_value_set_uint(value, 0);
	} else if(purple_strequal(id, "purple")) {
		g_value_set_uint(value, 1);
	} else if(purple_strequal(id, "system")) {
		g_value_set_uint(value, 2);
	} else {
		return FALSE;
	}

	return TRUE;
}

static GVariant *
idle_set_reporting_mapping(const GValue *value, const GVariantType *expected,
                           G_GNUC_UNUSED gpointer data)
{
	const char *id = NULL;

	if(!g_variant_type_equal(expected, G_VARIANT_TYPE_STRING)) {
		return NULL;
	}

	switch(g_value_get_uint(value)) {
	case 0:
		id = "none";
		break;
	case 1:
		id = "purple";
		break;
	case 2:
		id = "system";
		break;
	}

	if(id != NULL) {
		return g_variant_new_string(id);
	}

	return NULL;
}

static gboolean
idle_get_saved_presence_mapping(GValue *value, GVariant *variant,
                                G_GNUC_UNUSED gpointer data)
{
	GListModel *model = NULL;
	const char *name = NULL;
	guint n_items = 0;

	model = purple_presence_manager_get_default_as_model();

	name = g_variant_get_string(variant, NULL);

	n_items = g_list_model_get_n_items(model);
	for(guint i = 0; i < n_items; i++) {
		PurpleSavedPresence *presence = NULL;

		presence = g_list_model_get_item(model, i);
		if(PURPLE_IS_SAVED_PRESENCE(presence)) {
			const char *presence_name = NULL;

			presence_name = purple_saved_presence_get_name(presence);
			if(purple_strequal(presence_name, name)) {
				g_value_set_uint(value, i);

				g_clear_object(&presence);

				return TRUE;
			}
		}

		g_clear_object(&presence);
	}

	g_value_set_uint(value, 0);

	return TRUE;
}

static GVariant *
idle_set_saved_presence_mapping(const GValue *value,
                                const GVariantType *expected,
                                G_GNUC_UNUSED gpointer data)
{
	PurpleSavedPresence *presence = NULL;
	GListModel *model = NULL;
	guint position = 0;

	if(!g_variant_type_equal(expected, G_VARIANT_TYPE_STRING)) {
		return NULL;
	}

	model = purple_presence_manager_get_default_as_model();
	position = g_value_get_uint(value);
	presence = g_list_model_get_item(model, position);

	if(PURPLE_IS_SAVED_PRESENCE(presence)) {
		GVariant *ret = NULL;
		const char *name = NULL;

		name = purple_saved_presence_get_name(presence);
		ret = g_variant_new_string(name);

		g_clear_object(&presence);

		return ret;
	}

	return NULL;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_away_prefs_class_init(PidginAwayPrefsClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/Prefs/away.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginAwayPrefs,
	                                     idle_reporting);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        idle_reporting_expression_cb);
	gtk_widget_class_bind_template_child(widget_class, PidginAwayPrefs,
	                                     mins_before_away);
	gtk_widget_class_bind_template_child(widget_class, PidginAwayPrefs,
	                                     away_when_idle);
	gtk_widget_class_bind_template_child(widget_class, PidginAwayPrefs,
	                                     idle_row);
	gtk_widget_class_bind_template_child(widget_class, PidginAwayPrefs,
	                                     startup_current_status);
	gtk_widget_class_bind_template_child(widget_class, PidginAwayPrefs,
	                                     startup_row);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        idle_presence_icon_name_from_primitive);
}

static void
pidgin_away_prefs_init(PidginAwayPrefs *prefs)
{
	GListModel *model = NULL;
	GSettings *settings = NULL;
	gpointer backend = NULL;

	gtk_widget_init_template(GTK_WIDGET(prefs));

	backend = purple_core_get_settings_backend();
	model = purple_presence_manager_get_default_as_model();

	/* Finish setting up our idle preferences. */
	adw_combo_row_set_model(ADW_COMBO_ROW(prefs->idle_row), model);

	settings = g_settings_new_with_backend("im.pidgin.Purple.Idle", backend);

	g_settings_bind_with_mapping(settings, "method",
	                             prefs->idle_reporting, "selected",
	                             G_SETTINGS_BIND_DEFAULT,
	                             idle_get_reporting_mapping,
	                             idle_set_reporting_mapping,
	                             NULL, NULL);
	g_settings_bind(settings, "duration", prefs->mins_before_away,
	                "value", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(settings, "change-presence", prefs->away_when_idle,
	                "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind_with_mapping(settings, "saved-presence",
	                             prefs->idle_row, "selected",
	                             G_SETTINGS_BIND_DEFAULT,
	                             idle_get_saved_presence_mapping,
	                             idle_set_saved_presence_mapping,
	                             NULL, NULL);

	g_clear_object(&settings);

	/* Finish setting up the startup presence preferences. */
	adw_combo_row_set_model(ADW_COMBO_ROW(prefs->startup_row), model);

	settings = g_settings_new_with_backend("im.pidgin.Purple.Startup",
	                                       backend);

	g_settings_bind(settings, "use-previous-presence",
	                prefs->startup_current_status, "active",
	                G_SETTINGS_BIND_DEFAULT);
	g_settings_bind_with_mapping(settings, "saved-presence",
	                             prefs->startup_row, "selected",
	                             G_SETTINGS_BIND_DEFAULT,
	                             idle_get_saved_presence_mapping,
	                             idle_set_saved_presence_mapping,
	                             NULL, NULL);

	g_clear_object(&settings);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_away_prefs_new(void) {
	return g_object_new(PIDGIN_TYPE_AWAY_PREFS, NULL);
}
