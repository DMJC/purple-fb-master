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
#include "purplerequestpage.h"
#include "request/purplerequestfieldaccount.h"
#include "request/purplerequestfieldstring.h"
#include "purpleprivate.h"

struct _PurpleRequestPage {
	GObject parent;

	GList *groups;

	GHashTable *fields;

	GList *required_fields;

	GList *validated_fields;

	void *ui_data;
};

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_request_page_get_item_type(G_GNUC_UNUSED GListModel *model) {
	return PURPLE_TYPE_REQUEST_GROUP;
}

static guint
purple_request_page_get_n_items(GListModel *model) {
	PurpleRequestPage *page = PURPLE_REQUEST_PAGE(model);

	return g_list_length(page->groups);
}

static gpointer
purple_request_page_get_item(GListModel *model, guint index) {
	PurpleRequestPage *page = PURPLE_REQUEST_PAGE(model);
	PurpleRequestGroup *group = NULL;

	group = g_list_nth_data(page->groups, index);
	if(PURPLE_IS_REQUEST_GROUP(group)) {
		g_object_ref(group);
	}

	return group;
}

static void
purple_request_page_list_model_init(GListModelInterface *iface) {
	iface->get_item_type = purple_request_page_get_item_type;
	iface->get_item = purple_request_page_get_item;
	iface->get_n_items = purple_request_page_get_n_items;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_TYPE_WITH_CODE(PurpleRequestPage, purple_request_page, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                              purple_request_page_list_model_init))

static void
purple_request_page_finalize(GObject *obj) {
	PurpleRequestPage *page = PURPLE_REQUEST_PAGE(obj);

	g_list_free_full(page->groups, g_object_unref);
	g_list_free(page->required_fields);
	g_list_free(page->validated_fields);
	g_hash_table_destroy(page->fields);

	G_OBJECT_CLASS(purple_request_page_parent_class)->finalize(obj);
}

static void
purple_request_page_init(PurpleRequestPage *page) {
	page->fields = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

static void
purple_request_page_class_init(PurpleRequestPageClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_request_page_finalize;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleRequestPage *
purple_request_page_new(void) {
	return g_object_new(PURPLE_TYPE_REQUEST_PAGE, NULL);
}

void
_purple_request_page_set_field_required(PurpleRequestPage *page,
                                        PurpleRequestField *field,
                                        gboolean required)
{
	g_return_if_fail(PURPLE_IS_REQUEST_PAGE(page));

	if(required) {
		page->required_fields = g_list_append(page->required_fields, field);
	} else {
		page->required_fields = g_list_remove(page->required_fields, field);
	}
}

void
_purple_request_page_set_field_validator(PurpleRequestPage *page,
                                         PurpleRequestField *field,
                                         gboolean validator)
{
	g_return_if_fail(PURPLE_IS_REQUEST_PAGE(page));

	page->validated_fields = g_list_remove(page->validated_fields, field);
	if(validator) {
		page->validated_fields = g_list_append(page->validated_fields, field);
	}
}

void
_purple_request_page_add_field(PurpleRequestPage *page,
                               PurpleRequestField *field)
{
	g_return_if_fail(PURPLE_IS_REQUEST_PAGE(page));

	g_hash_table_insert(page->fields,
	                    g_strdup(purple_request_field_get_id(field)), field);

	if(purple_request_field_is_required(field)) {
		page->required_fields = g_list_append(page->required_fields, field);
	}

	if(purple_request_field_is_validatable(field)) {
		page->validated_fields = g_list_append(page->validated_fields, field);
	}
}

void
purple_request_page_add_group(PurpleRequestPage *page,
                              PurpleRequestGroup *group)
{
	guint position;
	GList *l;
	PurpleRequestField *field;

	g_return_if_fail(PURPLE_IS_REQUEST_PAGE(page));
	g_return_if_fail(PURPLE_IS_REQUEST_GROUP(group));

	position = g_list_length(page->groups);
	page->groups = g_list_append(page->groups, group);

	_purple_request_group_set_page(group, page);

	for (l = purple_request_group_get_fields(group);
		 l != NULL;
		 l = l->next) {

		field = l->data;

		g_hash_table_insert(page->fields,
		                    g_strdup(purple_request_field_get_id(field)),
		                    field);

		if (purple_request_field_is_required(field)) {
			page->required_fields = g_list_append(page->required_fields,
			                                      field);
		}

		if (purple_request_field_is_validatable(field)) {
			page->validated_fields = g_list_append(page->validated_fields,
			                                       field);
		}
	}

	g_list_model_items_changed(G_LIST_MODEL(page), position, 0, 1);
}

GList *
purple_request_page_get_groups(PurpleRequestPage *page) {
	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), NULL);

	return page->groups;
}

gboolean
purple_request_page_exists(PurpleRequestPage *page, const char *id) {
	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	return (g_hash_table_lookup(page->fields, id) != NULL);
}

const GList *
purple_request_page_get_required(PurpleRequestPage *page) {
	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), NULL);

	return page->required_fields;
}

const GList *
purple_request_page_get_validatable(PurpleRequestPage *page) {
	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), NULL);

	return page->validated_fields;
}

gboolean
purple_request_page_is_field_required(PurpleRequestPage *page, const char *id)
{
	PurpleRequestField *field;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	if((field = purple_request_page_get_field(page, id)) == NULL) {
		return FALSE;
	}

	return purple_request_field_is_required(field);
}

gboolean
purple_request_page_all_required_filled(PurpleRequestPage *page) {
	GList *l;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), FALSE);

	for(l = page->required_fields; l != NULL; l = l->next) {
		PurpleRequestField *field = PURPLE_REQUEST_FIELD(l->data);

		if (!purple_request_field_is_filled(field))
			return FALSE;
	}

	return TRUE;
}

gboolean
purple_request_page_all_valid(PurpleRequestPage *page) {
	GList *l;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), FALSE);

	for(l = page->validated_fields; l != NULL; l = l->next) {
		PurpleRequestField *field = PURPLE_REQUEST_FIELD(l->data);

		if (!purple_request_field_is_valid(field, NULL))
			return FALSE;
	}

	return TRUE;
}

PurpleRequestField *
purple_request_page_get_field(PurpleRequestPage *page, const char *id) {
	PurpleRequestField *field;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	field = g_hash_table_lookup(page->fields, id);

	g_return_val_if_fail(PURPLE_IS_REQUEST_FIELD(field), NULL);

	return field;
}

const char *
purple_request_page_get_string(PurpleRequestPage *page, const char *id) {
	PurpleRequestField *field;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	field = purple_request_page_get_field(page, id);
	if(!PURPLE_IS_REQUEST_FIELD_STRING(field)) {
		return NULL;
	}

	return purple_request_field_string_get_value(PURPLE_REQUEST_FIELD_STRING(field));
}

int
purple_request_page_get_integer(PurpleRequestPage *page, const char *id) {
	PurpleRequestField *field;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), 0);
	g_return_val_if_fail(id     != NULL, 0);

	if((field = purple_request_page_get_field(page, id)) == NULL) {
		return 0;
	}

	return purple_request_field_int_get_value(field);
}

gboolean
purple_request_page_get_bool(PurpleRequestPage *page, const char *id) {
	PurpleRequestField *field;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), FALSE);
	g_return_val_if_fail(id     != NULL, FALSE);

	if((field = purple_request_page_get_field(page, id)) == NULL) {
		return FALSE;
	}

	return purple_request_field_bool_get_value(field);
}

gpointer
purple_request_page_get_choice(PurpleRequestPage *page, const char *id) {
	PurpleRequestField *field;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), NULL);
	g_return_val_if_fail(id != NULL, NULL);

	if((field = purple_request_page_get_field(page, id)) == NULL) {
		return NULL;
	}

	return purple_request_field_choice_get_value(field);
}

PurpleAccount *
purple_request_page_get_account(PurpleRequestPage *page, const char *id) {
	PurpleRequestField *field;

	g_return_val_if_fail(PURPLE_IS_REQUEST_PAGE(page), NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	field = purple_request_page_get_field(page, id);
	if(!PURPLE_IS_REQUEST_FIELD_ACCOUNT(field)) {
		return NULL;
	}

	return purple_request_field_account_get_value(PURPLE_REQUEST_FIELD_ACCOUNT(field));
}
