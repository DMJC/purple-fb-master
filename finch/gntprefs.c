/*
 * finch
 *
 * Finch is the legal property of its developers, whose names are too numerous
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

#include <purple.h>

#include <gnt.h>

#include "gntprefs.h"
#include "gntrequest.h"


#include <string.h>

static struct {
	GList *freestrings;  /* strings to be freed when the pref-window is closed */
	gboolean showing;
	GntWidget *window;
} pref_request;

void
finch_prefs_init(void)
{
	purple_prefs_add_none("/finch");

	purple_prefs_add_none("/finch/plugins");
	purple_prefs_add_path_list("/finch/plugins/loaded", NULL);
	purple_prefs_add_path_list("/finch/plugins/seen", NULL);

	purple_prefs_add_none("/finch/conversations");
	purple_prefs_add_bool("/finch/conversations/timestamps", TRUE);
	purple_prefs_add_bool("/finch/conversations/notify_typing", FALSE);

	purple_prefs_add_none("/finch/filelocations");
	purple_prefs_add_path("/finch/filelocations/last_save_folder", "");
	purple_prefs_add_path("/finch/filelocations/last_save_folder", "");
}

void
finch_prefs_update_old(void)
{
}

typedef struct
{
	PurplePrefType type;
	const char *pref;
	const char *label;
	GList *(*lv)(void);   /* If the value is to be selected from a number of choices */
} Prefs;

static GList *
get_idle_options(void)
{
	GList *list = NULL;
	list = g_list_append(list, (char *)_("Based on keyboard use"));
	list = g_list_append(list, "system");
	list = g_list_append(list, (char*)_("From last sent message"));
	list = g_list_append(list, "purple");
	list = g_list_append(list, (char*)_("Never"));
	list = g_list_append(list, "never");
	return list;
}

static GList *
get_status_titles(void)
{
	GList *list = NULL;
	GList *iter;
	for (iter = purple_savedstatuses_get_all(); iter; iter = iter->next) {
		char *str;
		if (purple_savedstatus_is_transient(iter->data))
			continue;
		str = g_strdup_printf("%ld", purple_savedstatus_get_creation_time(iter->data));
		list = g_list_append(list, (char*)purple_savedstatus_get_title(iter->data));
		list = g_list_append(list, str);
		pref_request.freestrings = g_list_prepend(pref_request.freestrings, str);
	}
	return list;
}

static void
get_credential_provider_options_helper(PurpleCredentialProvider *provider,
                                       gpointer data)
{
	GList **list = data;
	const gchar *value = NULL;

	value = purple_credential_provider_get_name(provider);
	*list = g_list_append(*list, (gpointer)value);

	value = purple_credential_provider_get_id(provider);
	*list = g_list_append(*list, (gpointer)value);
}

static GList *
get_credential_provider_options(void) {
	PurpleCredentialManager *manager = NULL;
	GList *list = NULL;

	manager = purple_credential_manager_get_default();
	purple_credential_manager_foreach(manager,
	                                  get_credential_provider_options_helper,
	                                  &list);

	return list;
}

static PurpleRequestField *
get_pref_field(Prefs *prefs)
{
	PurpleRequestField *field = NULL;

	if (prefs->lv == NULL)
	{
		switch (prefs->type)
		{
			case PURPLE_PREF_BOOLEAN:
				field = purple_request_field_bool_new(prefs->pref, _(prefs->label),
						purple_prefs_get_bool(prefs->pref));
				break;
			case PURPLE_PREF_INT:
				field = purple_request_field_int_new(prefs->pref, _(prefs->label),
						purple_prefs_get_int(prefs->pref), INT_MIN, INT_MAX);
				break;
			case PURPLE_PREF_STRING:
				field = purple_request_field_string_new(prefs->pref, _(prefs->label),
						purple_prefs_get_string(prefs->pref), FALSE);
				break;
			default:
				break;
		}
	}
	else
	{
		GList *list = prefs->lv(), *iter;
		if (list)
			field = purple_request_field_list_new(prefs->pref, _(prefs->label));
		for (iter = list; iter; iter = iter->next)
		{
			gboolean select = FALSE;
			const char *data = iter->data;
			int idata;
			iter = iter->next;
			switch (prefs->type)
			{
				case PURPLE_PREF_BOOLEAN:
					if (sscanf(iter->data, "%d", &idata) != 1)
						idata = FALSE;
					if (purple_prefs_get_bool(prefs->pref) == idata)
						select = TRUE;
					break;
				case PURPLE_PREF_INT:
					if (sscanf(iter->data, "%d", &idata) != 1)
						idata = 0;
					if (purple_prefs_get_int(prefs->pref) == idata)
						select = TRUE;
					break;
				case PURPLE_PREF_STRING:
					if (purple_strequal(purple_prefs_get_string(prefs->pref), iter->data))
						select = TRUE;
					break;
				default:
					break;
			}
			purple_request_field_list_add_icon(field, data, NULL, iter->data);
			if (select)
				purple_request_field_list_add_selected(field, data);
		}
		g_list_free(list);
	}
	return field;
}

static Prefs blist[] =
{
	{PURPLE_PREF_BOOLEAN, "/finch/blist/idletime", N_("Show Idle Time"), NULL},
	{PURPLE_PREF_BOOLEAN, "/finch/blist/showoffline", N_("Show Offline Buddies"), NULL},
	{PURPLE_PREF_NONE, NULL, NULL, NULL}
};

static Prefs convs[] =
{
	{PURPLE_PREF_BOOLEAN, "/finch/conversations/timestamps", N_("Show Timestamps"), NULL},
	{PURPLE_PREF_BOOLEAN, "/finch/conversations/notify_typing", N_("Notify buddies when you are typing"), NULL},
	{PURPLE_PREF_NONE, NULL, NULL, NULL}
};

static Prefs idle[] =
{
	{PURPLE_PREF_STRING, "/purple/away/idle_reporting", N_("Report Idle time"), get_idle_options},
	{PURPLE_PREF_BOOLEAN, "/purple/away/away_when_idle", N_("Change status when idle"), NULL},
	{PURPLE_PREF_INT, "/purple/away/mins_before_away", N_("Minutes before changing status"), NULL},
	{PURPLE_PREF_INT, "/purple/savedstatus/idleaway", N_("Change status to"), get_status_titles},
	{PURPLE_PREF_NONE, NULL, NULL, NULL},
};

static Prefs credentials[] =
{
	{PURPLE_PREF_STRING, "/purple/credentials/active-provider", N_("Provider"), get_credential_provider_options},
	{PURPLE_PREF_NONE, NULL, NULL, NULL},
};

static void
free_strings(void)
{
	g_list_free_full(pref_request.freestrings, g_free);
	pref_request.freestrings = NULL;
	pref_request.showing = FALSE;
}

static void
save_cb(void *data, PurpleRequestFields *allfields)
{
	finch_request_save_in_prefs(data, allfields);
	free_strings();
}

static void
add_pref_group(PurpleRequestFields *fields, const char *title, Prefs *prefs)
{
	PurpleRequestField *field;
	PurpleRequestGroup *group;
	int i;

	group = purple_request_group_new(title);
	purple_request_fields_add_group(fields, group);
	for (i = 0; prefs[i].pref; i++)
	{
		field = get_pref_field(prefs + i);
		if (field)
			purple_request_group_add_field(group, field);
	}
}

void
finch_prefs_show_all(void)
{
	PurpleRequestFields *fields;

	if (pref_request.showing) {
		gnt_window_present(pref_request.window);
		return;
	}

	fields = purple_request_fields_new();

	add_pref_group(fields, _("Buddy List"), blist);
	add_pref_group(fields, _("Conversations"), convs);
	add_pref_group(fields, _("Idle"), idle);
	add_pref_group(fields, _("Credentials"), credentials);

	pref_request.showing = TRUE;
	pref_request.window = purple_request_fields(NULL, _("Preferences"), NULL, NULL, fields,
			_("Save"), G_CALLBACK(save_cb), _("Cancel"), free_strings,
			NULL, NULL);
}
