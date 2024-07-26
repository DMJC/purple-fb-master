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

#include "purpleprotocolcontacts.h"

#include "util.h"

/******************************************************************************
 * Default Implementations
 *****************************************************************************/
static guint
purple_protocol_contacts_default_get_minimum_search_length(G_GNUC_UNUSED PurpleProtocolContacts *protocol_contacts)
{
	return 3;
}

/******************************************************************************
 * GInterface Implementation
 *****************************************************************************/
G_DEFINE_INTERFACE(PurpleProtocolContacts, purple_protocol_contacts,
                   PURPLE_TYPE_PROTOCOL)

static void
purple_protocol_contacts_default_init(G_GNUC_UNUSED PurpleProtocolContactsInterface *iface)
{
	iface->get_minimum_search_length = purple_protocol_contacts_default_get_minimum_search_length;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
gboolean
purple_protocol_contacts_implements_search(PurpleProtocolContacts *protocol_contacts)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts),
	                     FALSE);

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);

	if(iface->get_minimum_search_length == NULL) {
		return FALSE;
	}

	if(iface->search_async == NULL) {
		return FALSE;
	}

	if(iface->search_finish == NULL) {
		return FALSE;
	}

	return TRUE;
}

guint
purple_protocol_contacts_get_minimum_search_length(PurpleProtocolContacts *protocol_contacts)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts), 3);

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);
	if(iface != NULL && iface->get_minimum_search_length != NULL) {
		return iface->get_minimum_search_length(protocol_contacts);
	}

	return 3;
}

void
purple_protocol_contacts_search_async(PurpleProtocolContacts *protocol_contacts,
                                      PurpleAccount *account,
                                      const char *text,
                                      GCancellable *cancellable,
                                      GAsyncReadyCallback callback,
                                      gpointer data)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(!purple_strempty(text));

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);
	if(iface != NULL && iface->search_async != NULL) {
		iface->search_async(protocol_contacts, account, text, cancellable,
		                    callback, data);
	} else {
		g_task_report_new_error(G_OBJECT(protocol_contacts), callback, data,
		                        purple_protocol_contacts_search_async,
		                        PURPLE_PROTOCOL_CONTACTS_DOMAIN, 0,
		                        "%s does not implement search_async",
		                        G_OBJECT_TYPE_NAME(protocol_contacts));
	}
}

GListModel *
purple_protocol_contacts_search_finish(PurpleProtocolContacts *protocol_contacts,
                                       GAsyncResult *result,
                                       GError **error)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts), NULL);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), NULL);

	if(g_async_result_is_tagged(result, purple_protocol_contacts_search_async))
	{
		return g_task_propagate_pointer(G_TASK(result), error);
	}

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);
	if(iface != NULL && iface->search_finish != NULL) {
		GListModel *ret = NULL;

		ret = iface->search_finish(protocol_contacts, result, error);
		if(G_IS_LIST_MODEL(ret)) {
			GType type = G_TYPE_INVALID;

			type = g_list_model_get_item_type(G_LIST_MODEL(ret));
			if(g_type_is_a(type, PURPLE_TYPE_CONTACT_INFO)) {
				return ret;
			}

			/* The GListModel we got back doesn't have an item type that is
			 * PurpleContactInfo or a subclass of it.
			 */
			g_clear_object(&ret);

			g_set_error(error, PURPLE_PROTOCOL_CONTACTS_DOMAIN, 0,
			            "%s returned a list of type %s which is not "
			            "PurpleContactInfo or a subclass of",
			            G_OBJECT_TYPE_NAME(protocol_contacts),
			            g_type_name(type));
		}

		return ret;
	}

	g_warning("purple_protocol_contacts_search_finish called without calling "
	          "purple_protocol_contacts_search_async");

	return NULL;
}

void
purple_protocol_contacts_get_profile_async(PurpleProtocolContacts *protocol_contacts,
                                           PurpleContactInfo *info,
                                           GCancellable *cancellable,
                                           GAsyncReadyCallback callback,
                                           gpointer data)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts));
	g_return_if_fail(PURPLE_IS_CONTACT_INFO(info));

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);
	if(iface != NULL && iface->get_profile_async != NULL) {
		iface->get_profile_async(protocol_contacts, info, cancellable,
		                         callback, data);
	} else {
		g_task_report_new_error(G_OBJECT(protocol_contacts), callback, data,
		                        purple_protocol_contacts_get_profile_async,
		                        PURPLE_PROTOCOL_CONTACTS_DOMAIN, 0,
		                        "%s does not implement get_profile_async",
		                        G_OBJECT_TYPE_NAME(protocol_contacts));
	}
}

char *
purple_protocol_contacts_get_profile_finish(PurpleProtocolContacts *protocol_contacts,
                                            GAsyncResult *result,
                                            GError **error)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts), NULL);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), NULL);

	if(g_async_result_is_tagged(result,
	                            purple_protocol_contacts_get_profile_async))
	{
		return g_task_propagate_pointer(G_TASK(result), error);
	}

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);
	if(iface != NULL && iface->get_profile_finish != NULL) {
		return iface->get_profile_finish(protocol_contacts, result, error);
	}

	g_warning("purple_protocol_contacts_get_profile_finish without calling "
	          "purple_protocol_contacts_get_profile_async");

	return NULL;
}

GActionGroup *
purple_protocol_contacts_get_actions(PurpleProtocolContacts *protocol_contacts,
                                     PurpleContactInfo *info)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts), NULL);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(info), NULL);

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);
	if(iface != NULL && iface->get_actions != NULL) {
		return iface->get_actions(protocol_contacts, info);
	}

	return NULL;
}

GMenuModel *
purple_protocol_contacts_get_menu(PurpleProtocolContacts *protocol_contacts,
                                  PurpleContactInfo *info)
{
	PurpleProtocolContactsInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONTACTS(protocol_contacts), NULL);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(info), NULL);

	iface = PURPLE_PROTOCOL_CONTACTS_GET_IFACE(protocol_contacts);
	if(iface != NULL && iface->get_menu != NULL) {
		return iface->get_menu(protocol_contacts, info);
	}

	return NULL;
}
