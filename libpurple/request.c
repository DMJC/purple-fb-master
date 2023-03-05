/* purple
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include <glib/gi18n-lib.h>

#include "glibcompat.h"
#include "notify.h"
#include "purplemarkup.h"
#include "request.h"
#include "debug.h"
#include "purplekeyvaluepair.h"
#include "purpleprivate.h"

static PurpleRequestUiOps *request_ui_ops = NULL;
static GList *handles = NULL;

typedef struct
{
	GDestroyNotify cb;
	gpointer data;
} PurpleRequestCloseNotified;

typedef struct
{
	PurpleRequestType type;
	void *handle;
	void *ui_handle;
	GSList *notify_on_close;
} PurpleRequestInfo;

struct _PurpleRequestFields
{
	GList *groups;

	GHashTable *fields;

	GList *required_fields;

	GList *validated_fields;

	void *ui_data;
};

struct _PurpleRequestCommonParameters {
	PurpleAccount *account;
	PurpleConversation *conv;

	PurpleRequestIconType icon_type;
	gconstpointer icon_data;
	gsize icon_size;

	gboolean html;

	gboolean compact;

	PurpleRequestHelpCb help_cb;
	gpointer help_data;

	GSList *extra_actions;

	gpointer parent_from;
};

PurpleRequestCommonParameters *
purple_request_cpar_new(void)
{
	return g_rc_box_new0(PurpleRequestCommonParameters);
}

PurpleRequestCommonParameters *
purple_request_cpar_from_connection(PurpleConnection *gc)
{
	if (gc == NULL)
		return purple_request_cpar_new();
	return purple_request_cpar_from_account(
		purple_connection_get_account(gc));
}

PurpleRequestCommonParameters *
purple_request_cpar_from_account(PurpleAccount *account)
{
	PurpleRequestCommonParameters *cpar;

	cpar = purple_request_cpar_new();
	purple_request_cpar_set_account(cpar, account);

	return cpar;
}

PurpleRequestCommonParameters *
purple_request_cpar_from_conversation(PurpleConversation *conv)
{
	PurpleRequestCommonParameters *cpar;
	PurpleAccount *account = NULL;

	if (conv != NULL) {
		account = purple_connection_get_account(
			purple_conversation_get_connection(conv));
	}

	cpar = purple_request_cpar_new();
	purple_request_cpar_set_account(cpar, account);
	purple_request_cpar_set_conversation(cpar, conv);

	return cpar;
}

void
purple_request_cpar_ref(PurpleRequestCommonParameters *cpar)
{
	g_return_if_fail(cpar != NULL);

	g_rc_box_acquire(cpar);
}

static void
purple_request_cpar_destroy(PurpleRequestCommonParameters *cpar) {
	g_slist_free_full(cpar->extra_actions,
	                  (GDestroyNotify)purple_key_value_pair_free);
}

void
purple_request_cpar_unref(PurpleRequestCommonParameters *cpar)
{
	if(cpar == NULL) {
		return;
	}

	g_rc_box_release_full(cpar, (GDestroyNotify)purple_request_cpar_destroy);
}

void
purple_request_cpar_set_account(PurpleRequestCommonParameters *cpar,
	PurpleAccount *account)
{
	g_return_if_fail(cpar != NULL);

	cpar->account = account;
}

PurpleAccount *
purple_request_cpar_get_account(PurpleRequestCommonParameters *cpar)
{
	if (cpar == NULL)
		return NULL;

	return cpar->account;
}

void
purple_request_cpar_set_conversation(PurpleRequestCommonParameters *cpar,
	PurpleConversation *conv)
{
	g_return_if_fail(cpar != NULL);

	cpar->conv = conv;
}

PurpleConversation *
purple_request_cpar_get_conversation(PurpleRequestCommonParameters *cpar)
{
	if (cpar == NULL)
		return NULL;

	return cpar->conv;
}

void
purple_request_cpar_set_icon(PurpleRequestCommonParameters *cpar,
	PurpleRequestIconType icon_type)
{
	g_return_if_fail(cpar != NULL);

	cpar->icon_type = icon_type;
}

PurpleRequestIconType
purple_request_cpar_get_icon(PurpleRequestCommonParameters *cpar)
{
	if (cpar == NULL)
		return PURPLE_REQUEST_ICON_DEFAULT;

	return cpar->icon_type;
}

void
purple_request_cpar_set_custom_icon(PurpleRequestCommonParameters *cpar,
	gconstpointer icon_data, gsize icon_size)
{
	g_return_if_fail(cpar != NULL);
	g_return_if_fail((icon_data == NULL) == (icon_size == 0));

	cpar->icon_data = icon_data;
	cpar->icon_size = icon_size;
}

gconstpointer
purple_request_cpar_get_custom_icon(PurpleRequestCommonParameters *cpar,
	gsize *icon_size)
{
	if (cpar == NULL) {
		if (icon_size != NULL)
			*icon_size = 0;
		return NULL;
	}

	if (icon_size != NULL)
		*icon_size = cpar->icon_size;
	return cpar->icon_data;
}

void
purple_request_cpar_set_html(PurpleRequestCommonParameters *cpar,
	gboolean enabled)
{
	g_return_if_fail(cpar != NULL);

	cpar->html = enabled;
}

gboolean
purple_request_cpar_is_html(PurpleRequestCommonParameters *cpar)
{
	if (cpar == NULL)
		return FALSE;

	return cpar->html;
}

void
purple_request_cpar_set_compact(PurpleRequestCommonParameters *cpar,
	gboolean compact)
{
	g_return_if_fail(cpar != NULL);

	cpar->compact = compact;
}

gboolean
purple_request_cpar_is_compact(PurpleRequestCommonParameters *cpar)
{
	if (cpar == NULL)
		return FALSE;

	return cpar->compact;
}

void
purple_request_cpar_set_help_cb(PurpleRequestCommonParameters *cpar,
	PurpleRequestHelpCb cb, gpointer user_data)
{
	g_return_if_fail(cpar != NULL);

	cpar->help_cb = cb;
	cpar->help_data = cb ? user_data : NULL;
}

PurpleRequestHelpCb
purple_request_cpar_get_help_cb(PurpleRequestCommonParameters *cpar,
	gpointer *user_data)
{
	if (cpar == NULL)
		return NULL;

	if (user_data != NULL)
		*user_data = cpar->help_data;
	return cpar->help_cb;
}

void
purple_request_cpar_set_extra_actions(PurpleRequestCommonParameters *cpar, ...)
{
	va_list args;
	GSList *extra = NULL;

	g_slist_free_full(cpar->extra_actions, (GDestroyNotify)purple_key_value_pair_free);

	va_start(args, cpar);

	while (TRUE) {
		const gchar *label;
		PurpleRequestFieldsCb cb;
		PurpleKeyValuePair *extra_action;

		label = va_arg(args, const gchar*);
		if (label == NULL)
			break;
		cb = va_arg(args, PurpleRequestFieldsCb);

		extra_action = purple_key_value_pair_new(label, cb);

		extra = g_slist_append(extra, extra_action);
	}

	va_end(args);

	cpar->extra_actions = extra;
}

GSList *
purple_request_cpar_get_extra_actions(PurpleRequestCommonParameters *cpar)
{
	if (cpar == NULL)
		return NULL;

	return cpar->extra_actions;
}

void
purple_request_cpar_set_parent_from(PurpleRequestCommonParameters *cpar,
	gpointer ui_handle)
{
	g_return_if_fail(cpar != NULL);

	cpar->parent_from = ui_handle;
}

gpointer
purple_request_cpar_get_parent_from(PurpleRequestCommonParameters *cpar)
{
	if (cpar == NULL)
		return NULL;

	return cpar->parent_from;
}

static PurpleRequestInfo *
purple_request_info_from_ui_handle(void *ui_handle)
{
	GList *it;

	g_return_val_if_fail(ui_handle != NULL, NULL);

	for (it = handles; it != NULL; it = g_list_next(it)) {
		PurpleRequestInfo *info = it->data;

		if (info->ui_handle == ui_handle)
			return info;
	}

	return NULL;
}

PurpleRequestFields *
purple_request_fields_new(void)
{
	PurpleRequestFields *fields;

	fields = g_new0(PurpleRequestFields, 1);

	fields->fields = g_hash_table_new_full(g_str_hash, g_str_equal,
										   g_free, NULL);

	return fields;
}

void
purple_request_fields_destroy(PurpleRequestFields *fields)
{
	g_return_if_fail(fields != NULL);

	g_list_free_full(fields->groups,
	                 (GDestroyNotify)purple_request_field_group_destroy);
	g_list_free(fields->required_fields);
	g_list_free(fields->validated_fields);
	g_hash_table_destroy(fields->fields);
	g_free(fields);
}

void
_purple_request_field_list_set_field_required(PurpleRequestFields *fields,
                                              PurpleRequestField *field,
                                              gboolean required)
{
	g_return_if_fail(fields != NULL);

	if(required) {
		fields->required_fields = g_list_append(fields->required_fields, field);
	} else {
		fields->required_fields = g_list_remove(fields->required_fields, field);
	}
}

void
_purple_request_field_list_set_field_validator(PurpleRequestFields *fields,
                                               PurpleRequestField *field,
                                               gboolean validator)
{
	g_return_if_fail(fields != NULL);

	fields->validated_fields = g_list_remove(fields->validated_fields, field);
	if(validator) {
		fields->validated_fields = g_list_append(fields->validated_fields,
		                                         field);
	}
}

void
_purple_request_field_list_add_field(PurpleRequestFields *fields,
                                     PurpleRequestField *field)
{
	g_return_if_fail(fields != NULL);

	g_hash_table_insert(fields->fields,
	                    g_strdup(purple_request_field_get_id(field)), field);

	if(purple_request_field_is_required(field)) {
		fields->required_fields = g_list_append(fields->required_fields,
		                                        field);
	}

	if(purple_request_field_is_validatable(field)) {
		fields->validated_fields = g_list_append(fields->validated_fields,
		                                         field);
	}
}

void
purple_request_fields_add_group(PurpleRequestFields *fields,
							  PurpleRequestFieldGroup *group)
{
	GList *l;
	PurpleRequestField *field;

	g_return_if_fail(fields != NULL);
	g_return_if_fail(group  != NULL);

	fields->groups = g_list_append(fields->groups, group);

	_purple_request_field_group_set_field_list(group, fields);

	for (l = purple_request_field_group_get_fields(group);
		 l != NULL;
		 l = l->next) {

		field = l->data;

		g_hash_table_insert(fields->fields,
			g_strdup(purple_request_field_get_id(field)), field);

		if (purple_request_field_is_required(field)) {
			fields->required_fields =
				g_list_append(fields->required_fields, field);
		}

		if (purple_request_field_is_validatable(field)) {
			fields->validated_fields =
				g_list_append(fields->validated_fields, field);
		}
	}
}

GList *
purple_request_fields_get_groups(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->groups;
}

gboolean
purple_request_fields_exists(const PurpleRequestFields *fields, const char *id)
{
	g_return_val_if_fail(fields != NULL, FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	return (g_hash_table_lookup(fields->fields, id) != NULL);
}

const GList *
purple_request_fields_get_required(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->required_fields;
}

const GList *
purple_request_fields_get_validatable(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->validated_fields;
}

gboolean
purple_request_fields_is_field_required(const PurpleRequestFields *fields,
									  const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return FALSE;

	return purple_request_field_is_required(field);
}

gboolean
purple_request_fields_all_required_filled(const PurpleRequestFields *fields)
{
	GList *l;

	g_return_val_if_fail(fields != NULL, FALSE);

	for (l = fields->required_fields; l != NULL; l = l->next)
	{
		PurpleRequestField *field = (PurpleRequestField *)l->data;

		if (!purple_request_field_is_filled(field))
			return FALSE;
	}

	return TRUE;
}

gboolean
purple_request_fields_all_valid(const PurpleRequestFields *fields)
{
	GList *l;

	g_return_val_if_fail(fields != NULL, FALSE);

	for (l = fields->validated_fields; l != NULL; l = l->next)
	{
		PurpleRequestField *field = (PurpleRequestField *)l->data;

		if (!purple_request_field_is_valid(field, NULL))
			return FALSE;
	}

	return TRUE;
}

PurpleRequestField *
purple_request_fields_get_field(const PurpleRequestFields *fields, const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	field = g_hash_table_lookup(fields->fields, id);

	g_return_val_if_fail(field != NULL, NULL);

	return field;
}

const char *
purple_request_fields_get_string(const PurpleRequestFields *fields, const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return NULL;

	return purple_request_field_string_get_value(field);
}

int
purple_request_fields_get_integer(const PurpleRequestFields *fields,
								const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, 0);
	g_return_val_if_fail(id     != NULL, 0);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return 0;

	return purple_request_field_int_get_value(field);
}

gboolean
purple_request_fields_get_bool(const PurpleRequestFields *fields, const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return FALSE;

	return purple_request_field_bool_get_value(field);
}

gpointer
purple_request_fields_get_choice(const PurpleRequestFields *fields,
	const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id != NULL, NULL);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return NULL;

	return purple_request_field_choice_get_value(field);
}

PurpleAccount *
purple_request_fields_get_account(const PurpleRequestFields *fields,
								const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(fields != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	if ((field = purple_request_fields_get_field(fields, id)) == NULL)
		return NULL;

	return purple_request_field_account_get_value(field);
}

gpointer purple_request_fields_get_ui_data(const PurpleRequestFields *fields)
{
	g_return_val_if_fail(fields != NULL, NULL);

	return fields->ui_data;
}

void purple_request_fields_set_ui_data(PurpleRequestFields *fields, gpointer ui_data)
{
	g_return_if_fail(fields != NULL);

	fields->ui_data = ui_data;
}

/* -- */

static gchar *
purple_request_strip_html_custom(const gchar *html)
{
	gchar *tmp, *ret;

	tmp = purple_strreplace(html, "\n", "<br>");
	ret = purple_markup_strip_html(tmp);
	g_free(tmp);

	return ret;
}

static gchar **
purple_request_strip_html(PurpleRequestCommonParameters *cpar,
	const char **primary, const char **secondary)
{
	PurpleRequestUiOps *ops = purple_request_get_ui_ops();
	gchar **ret;

	if (!purple_request_cpar_is_html(cpar))
		return NULL;
	if (ops->features & PURPLE_REQUEST_FEATURE_HTML)
		return NULL;

	ret = g_new0(gchar*, 3);
	*primary = ret[0] = purple_request_strip_html_custom(*primary);
	*secondary = ret[1] = purple_request_strip_html_custom(*secondary);

	return ret;
}

void *
purple_request_input(void *handle, const char *title, const char *primary,
				   const char *secondary, const char *default_value,
				   gboolean multiline, gboolean masked, gchar *hint,
				   const char *ok_text, GCallback ok_cb,
				   const char *cancel_text, GCallback cancel_cb,
				   PurpleRequestCommonParameters *cpar,
				   void *user_data)
{
	PurpleRequestUiOps *ops;

	if (G_UNLIKELY(ok_text == NULL || ok_cb == NULL)) {
		purple_request_cpar_unref(cpar);
		g_warn_if_fail(ok_text != NULL);
		g_warn_if_fail(ok_cb != NULL);
		g_return_val_if_reached(NULL);
	}

	ops = purple_request_get_ui_ops();

	if (ops != NULL && ops->request_input != NULL) {
		PurpleRequestInfo *info;
		gchar **tmp;

		tmp = purple_request_strip_html(cpar, &primary, &secondary);

		info            = g_new0(PurpleRequestInfo, 1);
		info->type      = PURPLE_REQUEST_INPUT;
		info->handle    = handle;
		info->ui_handle = ops->request_input(title, primary, secondary,
			default_value, multiline, masked, hint, ok_text, ok_cb,
			cancel_text, cancel_cb, cpar, user_data);

		handles = g_list_append(handles, info);

		g_strfreev(tmp);
		purple_request_cpar_unref(cpar);
		return info->ui_handle;
	}

	purple_request_cpar_unref(cpar);
	return NULL;
}

void *
purple_request_choice(void *handle, const char *title, const char *primary,
	const char *secondary, gpointer default_value, const char *ok_text,
	GCallback ok_cb, const char *cancel_text, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data, ...)
{
	void *ui_handle;
	va_list args;

	if (G_UNLIKELY(ok_text == NULL || ok_cb == NULL)) {
		purple_request_cpar_unref(cpar);
		g_warn_if_fail(ok_text != NULL);
		g_warn_if_fail(ok_cb != NULL);
		g_return_val_if_reached(NULL);
	}

	va_start(args, user_data);
	ui_handle = purple_request_choice_varg(handle, title, primary, secondary,
					     default_value, ok_text, ok_cb,
					     cancel_text, cancel_cb,
					     cpar, user_data, args);
	va_end(args);

	return ui_handle;
}

void *
purple_request_choice_varg(void *handle, const char *title, const char *primary,
	const char *secondary, gpointer default_value, const char *ok_text,
	GCallback ok_cb, const char *cancel_text, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data, va_list choices)
{
	PurpleRequestUiOps *ops;

	if (G_UNLIKELY(ok_text == NULL || ok_cb == NULL ||
		cancel_text == NULL))
	{
		purple_request_cpar_unref(cpar);
		g_warn_if_fail(ok_text != NULL);
		g_warn_if_fail(ok_cb != NULL);
		g_warn_if_fail(cancel_text != NULL);
		g_return_val_if_reached(NULL);
	}

	ops = purple_request_get_ui_ops();

	if (ops != NULL && ops->request_choice != NULL) {
		PurpleRequestInfo *info;
		gchar **tmp;

		tmp = purple_request_strip_html(cpar, &primary, &secondary);

		info            = g_new0(PurpleRequestInfo, 1);
		info->type      = PURPLE_REQUEST_CHOICE;
		info->handle    = handle;
		info->ui_handle = ops->request_choice(title, primary, secondary,
			default_value, ok_text, ok_cb, cancel_text, cancel_cb,
			cpar, user_data, choices);

		handles = g_list_append(handles, info);

		g_strfreev(tmp);
		purple_request_cpar_unref(cpar);
		return info->ui_handle;
	}

	purple_request_cpar_unref(cpar);
	return NULL;
}

void *
purple_request_action(void *handle, const char *title, const char *primary,
	const char *secondary, int default_action,
	PurpleRequestCommonParameters *cpar, void *user_data,
	size_t action_count, ...)
{
	void *ui_handle;
	va_list args;

	va_start(args, action_count);
	ui_handle = purple_request_action_varg(handle, title, primary,
		secondary, default_action, cpar, user_data, action_count, args);
	va_end(args);

	return ui_handle;
}

void *
purple_request_action_varg(void *handle, const char *title, const char *primary,
	const char *secondary, int default_action,
	PurpleRequestCommonParameters *cpar, void *user_data,
	size_t action_count, va_list actions)
{
	PurpleRequestUiOps *ops;

	ops = purple_request_get_ui_ops();

	if (ops != NULL && ops->request_action != NULL) {
		PurpleRequestInfo *info;
		gchar **tmp;

		tmp = purple_request_strip_html(cpar, &primary, &secondary);

		info            = g_new0(PurpleRequestInfo, 1);
		info->type      = PURPLE_REQUEST_ACTION;
		info->handle    = handle;
		info->ui_handle = ops->request_action(title, primary, secondary,
			default_action, cpar, user_data, action_count, actions);

		handles = g_list_append(handles, info);

		g_strfreev(tmp);
		purple_request_cpar_unref(cpar);
		return info->ui_handle;
	}

	purple_request_cpar_unref(cpar);
	return NULL;
}

void *
purple_request_wait(void *handle, const char *title, const char *primary,
	const char *secondary, gboolean with_progress,
	PurpleRequestCancelCb cancel_cb, PurpleRequestCommonParameters *cpar,
	void *user_data)
{
	PurpleRequestUiOps *ops;

	if (primary == NULL)
		primary = _("Please wait...");

	ops = purple_request_get_ui_ops();

	if (ops != NULL && ops->request_wait != NULL) {
		PurpleRequestInfo *info;
		gchar **tmp;

		tmp = purple_request_strip_html(cpar, &primary, &secondary);

		info            = g_new0(PurpleRequestInfo, 1);
		info->type      = PURPLE_REQUEST_WAIT;
		info->handle    = handle;
		info->ui_handle = ops->request_wait(title, primary, secondary,
			with_progress, cancel_cb, cpar, user_data);

		handles = g_list_append(handles, info);

		g_strfreev(tmp);
		purple_request_cpar_unref(cpar);
		return info->ui_handle;
	}

	if (cpar == NULL)
		cpar = purple_request_cpar_new();
	if (purple_request_cpar_get_icon(cpar) == PURPLE_REQUEST_ICON_DEFAULT)
		purple_request_cpar_set_icon(cpar, PURPLE_REQUEST_ICON_WAIT);

	return purple_request_action(handle, title, primary, secondary,
		PURPLE_DEFAULT_ACTION_NONE, cpar, user_data,
		cancel_cb ? 1 : 0, _("Cancel"), cancel_cb);
}

void
purple_request_wait_pulse(void *ui_handle)
{
	PurpleRequestUiOps *ops;

	ops = purple_request_get_ui_ops();

	if (ops == NULL || ops->request_wait_update == NULL)
		return;

	ops->request_wait_update(ui_handle, TRUE, 0.0);
}

void
purple_request_wait_progress(void *ui_handle, gfloat fraction)
{
	PurpleRequestUiOps *ops;

	ops = purple_request_get_ui_ops();

	if (ops == NULL || ops->request_wait_update == NULL)
		return;

	if (fraction < 0.0 || fraction > 1.0) {
		purple_debug_warning("request", "Fraction parameter out of "
			"range: %f", fraction);
		if (fraction < 0.0)
			fraction = 0.0;
		else /* if (fraction > 1.0) */
			fraction = 1.0;
	}

	ops->request_wait_update(ui_handle, FALSE, fraction);
}

static void
purple_request_fields_strip_html(PurpleRequestFields *fields)
{
	GList *itg;

	for (itg = fields->groups; itg != NULL; itg = g_list_next(itg)) {
		PurpleRequestFieldGroup *group = itg->data;
		GList *itf;

		for (itf = purple_request_field_group_get_fields(group);
		     itf != NULL;
		     itf = g_list_next(itf))
		{
			PurpleRequestField *field = itf->data;
			const char *old_label;
			gchar *new_label;

			old_label = purple_request_field_get_label(field);
			new_label = purple_request_strip_html_custom(old_label);
			if(g_strcmp0(new_label, old_label) != 0) {
				purple_request_field_set_label(field, new_label);
			}
			g_free(new_label);
		}
	}
}

void *
purple_request_fields(void *handle, const char *title, const char *primary,
	const char *secondary, PurpleRequestFields *fields, const char *ok_text,
	GCallback ok_cb, const char *cancel_text, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data)
{
	PurpleRequestUiOps *ops;

	if (G_UNLIKELY(fields == NULL ||
		((ok_text == NULL) != (ok_cb == NULL)) ||
		cancel_text == NULL))
	{
		purple_request_cpar_unref(cpar);
		g_warn_if_fail(fields != NULL);
		g_warn_if_fail((ok_text == NULL) != (ok_cb == NULL));
		g_warn_if_fail(cancel_text != NULL);
		g_return_val_if_reached(NULL);
	}

	ops = purple_request_get_ui_ops();

	if (purple_request_cpar_is_html(cpar) &&
		!((ops->features & PURPLE_REQUEST_FEATURE_HTML)))
	{
		purple_request_fields_strip_html(fields);
	}

	if (ops != NULL && ops->request_fields != NULL) {
		PurpleRequestInfo *info;
		gchar **tmp;

		tmp = purple_request_strip_html(cpar, &primary, &secondary);

		info            = g_new0(PurpleRequestInfo, 1);
		info->type      = PURPLE_REQUEST_FIELDS;
		info->handle    = handle;
		info->ui_handle = ops->request_fields(title, primary, secondary,
			fields, ok_text, ok_cb, cancel_text, cancel_cb,
			cpar, user_data);

		handles = g_list_append(handles, info);

		g_strfreev(tmp);
		purple_request_cpar_unref(cpar);
		return info->ui_handle;
	}

	purple_request_cpar_unref(cpar);
	return NULL;
}

void *
purple_request_file(void *handle, const char *title, const char *filename,
	gboolean savedialog, GCallback ok_cb, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data)
{
	PurpleRequestUiOps *ops;

	ops = purple_request_get_ui_ops();

	if (ops != NULL && ops->request_file != NULL) {
		PurpleRequestInfo *info;

		info            = g_new0(PurpleRequestInfo, 1);
		info->type      = PURPLE_REQUEST_FILE;
		info->handle    = handle;
		info->ui_handle = ops->request_file(title, filename, savedialog,
			ok_cb, cancel_cb, cpar, user_data);
		handles = g_list_append(handles, info);

		purple_request_cpar_unref(cpar);
		return info->ui_handle;
	}

	purple_request_cpar_unref(cpar);
	return NULL;
}

void *
purple_request_folder(void *handle, const char *title, const char *dirname,
	GCallback ok_cb, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data)
{
	PurpleRequestUiOps *ops;

	ops = purple_request_get_ui_ops();

	if (ops != NULL && ops->request_file != NULL) {
		PurpleRequestInfo *info;

		info            = g_new0(PurpleRequestInfo, 1);
		info->type      = PURPLE_REQUEST_FOLDER;
		info->handle    = handle;
		info->ui_handle = ops->request_folder(title, dirname, ok_cb,
			cancel_cb, cpar, user_data);
		handles = g_list_append(handles, info);

		purple_request_cpar_unref(cpar);
		return info->ui_handle;
	}

	purple_request_cpar_unref(cpar);
	return NULL;
}

gboolean
purple_request_is_valid_ui_handle(void *ui_handle, PurpleRequestType *type)
{
	PurpleRequestInfo *info;

	if (ui_handle == NULL)
		return FALSE;

	info = purple_request_info_from_ui_handle(ui_handle);

	if (info == NULL)
		return FALSE;

	if (type != NULL)
		*type = info->type;

	return TRUE;
}

void
purple_request_add_close_notify(void *ui_handle, GDestroyNotify notify,
	gpointer notify_data)
{
	PurpleRequestInfo *info;
	PurpleRequestCloseNotified *notified;

	g_return_if_fail(ui_handle != NULL);
	g_return_if_fail(notify != NULL);

	info = purple_request_info_from_ui_handle(ui_handle);
	g_return_if_fail(info != NULL);

	notified = g_new0(PurpleRequestCloseNotified, 1);
	notified->cb = notify;
	notified->data = notify_data;

	info->notify_on_close = g_slist_append(info->notify_on_close, notified);
}

static void
purple_request_close_info(PurpleRequestInfo *info)
{
	PurpleRequestUiOps *ops;
	GSList *it;

	ops = purple_request_get_ui_ops();

	purple_notify_close_with_handle(info->ui_handle);
	purple_request_close_with_handle(info->ui_handle);

	if (ops != NULL && ops->close_request != NULL)
		ops->close_request(info->type, info->ui_handle);

	for (it = info->notify_on_close; it; it = g_slist_next(it)) {
		PurpleRequestCloseNotified *notify = it->data;

		notify->cb(notify->data);
	}

	g_slist_free_full(info->notify_on_close, g_free);
	g_free(info);
}

void
purple_request_close(G_GNUC_UNUSED PurpleRequestType type, void *ui_handle)
{
	GList *l;

	g_return_if_fail(ui_handle != NULL);

	for (l = handles; l != NULL; l = l->next) {
		PurpleRequestInfo *info = l->data;

		if (info->ui_handle == ui_handle) {
			handles = g_list_delete_link(handles, l);
			purple_request_close_info(info);
			break;
		}
	}
}

void
purple_request_close_with_handle(void *handle)
{
	GList *l, *l_next;

	g_return_if_fail(handle != NULL);

	for (l = handles; l != NULL; l = l_next) {
		PurpleRequestInfo *info = l->data;

		l_next = l->next;

		if (info->handle == handle) {
			handles = g_list_delete_link(handles, l);
			purple_request_close_info(info);
		}
	}
}

void
purple_request_set_ui_ops(PurpleRequestUiOps *ops)
{
	request_ui_ops = ops;
}

PurpleRequestUiOps *
purple_request_get_ui_ops(void)
{
	return request_ui_ops;
}

/**************************************************************************
 * GBoxed code
 **************************************************************************/
static PurpleRequestUiOps *
purple_request_ui_ops_copy(PurpleRequestUiOps *ops)
{
	PurpleRequestUiOps *ops_new;

	g_return_val_if_fail(ops != NULL, NULL);

	ops_new = g_new(PurpleRequestUiOps, 1);
	*ops_new = *ops;

	return ops_new;
}

GType
purple_request_ui_ops_get_type(void)
{
	static GType type = 0;

	if (type == 0) {
		type = g_boxed_type_register_static("PurpleRequestUiOps",
				(GBoxedCopyFunc)purple_request_ui_ops_copy,
				(GBoxedFreeFunc)g_free);
	}

	return type;
}
