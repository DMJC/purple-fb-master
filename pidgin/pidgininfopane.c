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

#include "pidgin/pidgininfopane.h"

struct _PidginInfoPane {
	GtkBox parent;

	char *title;
	char *subtitle;

	GtkWidget *child;
};

enum {
	PROP_0,
	PROP_TITLE,
	PROP_SUBTITLE,
	PROP_CHILD,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_FINAL_TYPE(PidginInfoPane, pidgin_info_pane, GTK_TYPE_BOX)

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static char *
pidgin_info_pane_linkify_text_cb(GObject *self, const char *subtitle,
                                 G_GNUC_UNUSED gpointer data)
{
	GtkWidget *label = GTK_WIDGET(self);
	char *ret = NULL;

	ret = purple_markup_linkify(subtitle);

	gtk_widget_set_visible(label, !purple_strempty(ret));

	return ret;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_info_pane_get_property(GObject *obj, guint param_id, GValue *value,
                              GParamSpec *pspec)
{
	PidginInfoPane *pane = PIDGIN_INFO_PANE(obj);

	switch(param_id) {
		case PROP_TITLE:
			g_value_set_string(value, pidgin_info_pane_get_title(pane));
			break;
		case PROP_SUBTITLE:
			g_value_set_string(value, pidgin_info_pane_get_subtitle(pane));
			break;
		case PROP_CHILD:
			g_value_set_object(value, pidgin_info_pane_get_child(pane));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
pidgin_info_pane_set_property(GObject *obj, guint param_id, const GValue *value,
                              GParamSpec *pspec)
{
	PidginInfoPane *pane = PIDGIN_INFO_PANE(obj);

	switch(param_id) {
		case PROP_TITLE:
			pidgin_info_pane_set_title(pane, g_value_get_string(value));
			break;
		case PROP_SUBTITLE:
			pidgin_info_pane_set_subtitle(pane, g_value_get_string(value));
			break;
		case PROP_CHILD:
			pidgin_info_pane_set_child(pane, g_value_get_object(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
pidgin_info_pane_finalize(GObject *obj) {
	PidginInfoPane *pane = PIDGIN_INFO_PANE(obj);

	g_clear_pointer(&pane->title, g_free);
	g_clear_pointer(&pane->subtitle, g_free);

	G_OBJECT_CLASS(pidgin_info_pane_parent_class)->finalize(obj);
}

static void
pidgin_info_pane_init(PidginInfoPane *pane) {
	gtk_widget_init_template(GTK_WIDGET(pane));
}

static void
pidgin_info_pane_class_init(PidginInfoPaneClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->finalize = pidgin_info_pane_finalize;
	obj_class->get_property = pidgin_info_pane_get_property;
	obj_class->set_property = pidgin_info_pane_set_property;

	/**
	 * PidginInfoPane:title:
	 *
	 * The title to display.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TITLE] = g_param_spec_string(
		"title", "title",
		"The title for the info pane.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PidginInfoPane:subtitle:
	 *
	 * The sub title to display.
	 *
	 * Since: 3.0
	 */
	properties[PROP_SUBTITLE] = g_param_spec_string(
		"subtitle", "subtitle",
		"The subtitle for the info pane.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PidginInfoPane:child:
	 *
	 * The child widget to display at the end of the info pane.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CHILD] = g_param_spec_object(
		"child", "child",
		"A child widget to display at the end of the info pane.",
		GTK_TYPE_WIDGET,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	gtk_widget_class_set_template_from_resource(widget_class,
	                                            "/im/pidgin/Pidgin3/infopane.ui");

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_info_pane_linkify_text_cb);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_info_pane_new(void) {
	return g_object_new(PIDGIN_TYPE_INFO_PANE, NULL);
}

const char *
pidgin_info_pane_get_title(PidginInfoPane *pane) {
	g_return_val_if_fail(PIDGIN_IS_INFO_PANE(pane), NULL);

	return pane->title;
}

void
pidgin_info_pane_set_title(PidginInfoPane *pane, const char *title) {
	g_return_if_fail(PIDGIN_IS_INFO_PANE(pane));

	if(!purple_strequal(pane->title, title)) {
		g_free(pane->title);
		pane->title = g_strdup(title);

		g_object_notify_by_pspec(G_OBJECT(pane), properties[PROP_TITLE]);
	}
}

const char *
pidgin_info_pane_get_subtitle(PidginInfoPane *pane) {
	g_return_val_if_fail(PIDGIN_IS_INFO_PANE(pane), NULL);

	return pane->subtitle;
}

void
pidgin_info_pane_set_subtitle(PidginInfoPane *pane, const char *subtitle) {
	g_return_if_fail(PIDGIN_IS_INFO_PANE(pane));

	if(!purple_strequal(pane->subtitle, subtitle)) {
		g_free(pane->subtitle);
		pane->subtitle = g_strdup(subtitle);

		g_object_notify_by_pspec(G_OBJECT(pane), properties[PROP_SUBTITLE]);
	}
}

GtkWidget *
pidgin_info_pane_get_child(PidginInfoPane *pane) {
	g_return_val_if_fail(PIDGIN_IS_INFO_PANE(pane), NULL);

	return pane->child;
}

void
pidgin_info_pane_set_child(PidginInfoPane *pane, GtkWidget *widget) {
	g_return_if_fail(PIDGIN_IS_INFO_PANE(pane));
	g_return_if_fail(widget == NULL || gtk_widget_get_parent(widget) == NULL);

	if(pane->child == widget) {
		return;
	}

	g_clear_object(&pane->child);

	if(GTK_IS_WIDGET(widget)) {
		pane->child = g_object_ref_sink(widget);
		gtk_box_append(GTK_BOX(pane), pane->child);

		/* This might not be the best place for this, but this helps keep the
		 * user interface consistent and doesn't force users to do it.
		 */
		gtk_widget_set_margin_end(pane->child, 6);
	}

	g_object_notify_by_pspec(G_OBJECT(pane), properties[PROP_CHILD]);
}
