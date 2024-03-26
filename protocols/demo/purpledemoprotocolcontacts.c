/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpledemoprotocolcontacts.h"

/******************************************************************************
 * PurpleProtocolContacts Implementation
 *****************************************************************************/
static char *
purple_demo_protocol_contacts_get_profile_finish(G_GNUC_UNUSED PurpleProtocolContacts *contacts,
                                                 GAsyncResult *result,
                                                 GError **error)
{
	g_return_val_if_fail(G_IS_TASK(result), NULL);

	return g_task_propagate_pointer(G_TASK(result), error);
}

static void
purple_demo_protocol_contacts_get_profile_async(PurpleProtocolContacts *contacts,
                                                PurpleContactInfo *info,
                                                GCancellable *cancellable,
                                                GAsyncReadyCallback callback,
                                                gpointer data)
{
	GTask *task = NULL;
	const char *profile = NULL;

	task = g_task_new(contacts, cancellable, callback, data);

	profile = g_object_get_data(G_OBJECT(info), "demo-profile");
	g_task_return_pointer(task, g_strdup(profile), g_free);
	g_clear_object(&task);
}

void
purple_demo_protocol_contacts_init(PurpleProtocolContactsInterface *iface) {
	iface->get_profile_async = purple_demo_protocol_contacts_get_profile_async;
	iface->get_profile_finish = purple_demo_protocol_contacts_get_profile_finish;
}
