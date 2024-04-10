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

#ifndef PURPLE_CONTACT_H
#define PURPLE_CONTACT_H

#include <glib.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "purplecontactinfo.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PURPLE_CONTACT_DOMAIN:
 *
 * An error domain for [class@Contact].
 *
 * Since: 3.0
 */
#define PURPLE_CONTACT_DOMAIN \
    (g_quark_from_static_string("purple-contact")) \
    PURPLE_AVAILABLE_MACRO_IN_3_0

#define PURPLE_TYPE_CONTACT (purple_contact_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleContact, purple_contact, PURPLE, CONTACT,
                     PurpleContactInfo)

#include "purpleaccount.h"
#include "purpleconversation.h"

/**
 * PurpleContact:
 *
 * A contact is a concrete representation of a user in that it contains all of
 * the contact information as well as a [class@Purple.Account] to use that
 * contact information with.
 */

/**
 * purple_contact_new:
 * @account: The [class@Purple.Account] this contact is from.
 * @id: (nullable): The id of the contact.
 *
 * Creates a new [class@Purple.Contact].
 *
 * If @id is %NULL, an ID will be randomly generated.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleContact *purple_contact_new(PurpleAccount *account, const gchar *id);

/**
 * purple_contact_get_account:
 * @contact: The instance.
 *
 * Gets the [class@Purple.Account] that @contact belongs to.
 *
 * Returns: (transfer none): The [class@Purple.Account] that @contact belongs
 *          to.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleAccount *purple_contact_get_account(PurpleContact *contact);

/**
 * purple_contact_find_dm:
 * @contact: The instance.
 * @create: Whether or not to create a new DM if one can't be found.
 *
 * Attempts to find a conversation for @contact in the default
 * [class@ContactManager] by using @contact's username.
 *
 * If no existing direct messages exists for @contact and @create is %TRUE,
 * then a new direct message will be created and registered in the default
 * [class@ConversationManager].
 *
 * Returns: (transfer none) (nullable): The conversation or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversation *purple_contact_find_dm(PurpleContact *contact, gboolean create);

/**
 * purple_contact_create_dm_async:
 * @contact: The instance.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is
 *            satisfied.
 * @data: User data to pass to @callback.
 *
 * Asks the protocol to create a direct message with @contact.
 *
 * The protocol may return an existing conversation.
 *
 * Since: 3.0
 */
void purple_contact_create_dm_async(PurpleContact *contact, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_contact_create_dm_finish:
 * @contact: The instance.
 * @result: The result.
 * @error: The return address for a #GError.
 *
 * Finishes a previous call to [method@Contact.create_dm_async].
 *
 * Returns: (transfer full): The conversation if successful, otherwise %NULL
 *          with @error set.
 *
 * Since: 3.0
 */
PurpleConversation *purple_contact_create_dm_finish(PurpleContact *contact, GAsyncResult *result, GError **error);

G_END_DECLS

#endif /* PURPLE_CONTACT_H */
