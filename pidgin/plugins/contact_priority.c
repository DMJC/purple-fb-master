/*
 * Contact priority settings plugin.
 *
 * Copyright (C) 2003 Etan Reisner, <deryni9@users.sourceforge.net>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "internal.h"
#include "pidgin.h"
#include "gtkutils.h"
#include "pidginplugininfo.h"
#include "pidginaccountchooser.h"

#include <purple.h>


#define CONTACT_PRIORITY_PLUGIN_ID "gtk-contact-priority"

static void
select_account(GtkWidget *chooser, gpointer data)
{
	PurpleAccount *account = pidgin_account_chooser_get_selected(chooser);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data),
	                          (gdouble)purple_account_get_int(account, "score", 0));
}

static void
account_update(GtkWidget *widget, GtkWidget *chooser)
{
	PurpleAccount *account = NULL;

	account = pidgin_account_chooser_get_selected(chooser);
	purple_account_set_int(account, "score", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget)));
}

static void
pref_update(GtkWidget *widget, char *pref)
{
	if (purple_prefs_get_pref_type(pref) == PURPLE_PREF_INT)
		purple_prefs_set_int(pref, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget)));
	if (purple_prefs_get_pref_type(pref) == PURPLE_PREF_BOOLEAN)
		purple_prefs_set_bool(pref, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static struct PurpleContactPriorityStatuses
{
	const char *id;
	const char *description;
} const statuses[] =
{
	{ "idle",          N_("Buddy is idle") },
	{ "away",          N_("Buddy is away") },
	{ "extended_away", N_("Buddy is \"extended\" away") },
#if 0
	/* Not used yet. */
	{ "mobile",        N_("Buddy is mobile") },
#endif
	{ "offline",       N_("Buddy is offline") },
	{ NULL, NULL }
};

static GtkWidget *
get_config_frame(PurplePlugin *plugin)
{
	GtkWidget *ret = NULL, *hbox = NULL, *frame = NULL, *vbox = NULL;
	GtkWidget *label = NULL, *spin = NULL, *check = NULL;
	GtkWidget *chooser = NULL;
	GtkAdjustment *adj = NULL;
	GtkSizeGroup *sg = NULL;
	PurpleAccount *account = NULL;
	int i;

	gboolean last_match = purple_prefs_get_bool("/purple/contact/last_match");

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	ret = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
	gtk_container_set_border_width(GTK_CONTAINER(ret), 12);

	frame = pidgin_make_frame(ret, _("Point values to use when..."));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	/* Status Spinboxes */
	for (i = 0 ; statuses[i].id != NULL && statuses[i].description != NULL ; i++)
	{
		char *pref = g_strconcat("/purple/status/scores/", statuses[i].id, NULL);

		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

		label = gtk_label_new_with_mnemonic(_(statuses[i].description));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		gtk_size_group_add_widget(sg, label);
		gtk_label_set_xalign(GTK_LABEL(label), 0);
		gtk_label_set_yalign(GTK_LABEL(label), 0);

		adj = GTK_ADJUSTMENT(gtk_adjustment_new(purple_prefs_get_int(pref),
		                                        -500, 500, 1, 1, 1));
		spin = gtk_spin_button_new(adj, 1, 0);
		g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_update), pref);
		gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);

		g_free(pref);
	}

	/* Explanation */
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("The buddy with the <i>largest score</i> is the buddy who will have priority in the contact.\n"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* Last match */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	check = gtk_check_button_new_with_label(_("Use last buddy when scores are equal"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), last_match);
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_update), "/purple/contact/last_match");
	gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);

	frame = pidgin_make_frame(ret, _("Point values to use for account..."));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	/* Account */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* make this here so I can use it in the option menu callback, we'll
	 * actually set it up later */
	adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, -500, 500, 1, 1, 1));
	spin = gtk_spin_button_new(adj, 1, 0);

	chooser = pidgin_account_chooser_new(NULL, TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), chooser, FALSE, FALSE, 0);
	g_signal_connect(chooser, "changed", G_CALLBACK(select_account), spin);

	/* this is where we set up the spin button we made above */
	account = pidgin_account_chooser_get_selected(chooser);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin),
	                          (gdouble)purple_account_get_int(account, "score", 0));
	gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(spin), GTK_ADJUSTMENT(adj));
	g_signal_connect(G_OBJECT(spin), "value-changed",
	                 G_CALLBACK(account_update), chooser);
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);

	gtk_widget_show_all(ret);
	g_object_unref(sg);

	return ret;
}

static PidginPluginInfo *
plugin_query(GError **error)
{
	const gchar * const authors[] = {
		"Etan Reisner <deryni@eden.rutgers.edu>",
		NULL
	};

	return pidgin_plugin_info_new(
		"id",                   CONTACT_PRIORITY_PLUGIN_ID,
		"name",                 N_("Contact Priority"),
		"version",              DISPLAY_VERSION,
		"category",             N_("Utility"),
		"summary",              N_("Allows for controlling the values associated with different buddy states."),
		"description",          N_("Allows for changing the point values of idle/away/offline states for buddies in contact priority computations."),
		"authors",              authors,
		"website",              PURPLE_WEBSITE,
		"abi-version",          PURPLE_ABI_VERSION,
		"gtk-config-frame-cb",  get_config_frame,
		NULL
	);
}

static gboolean
plugin_load(PurplePlugin *plugin, GError **error)
{
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
	return TRUE;
}

PURPLE_PLUGIN_INIT(contactpriority, plugin_query, plugin_load, plugin_unload);
