/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PURPLE_CONTACT_H
#define PURPLE_CONTACT_H

#include <glib.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libpurple/purplepresence.h>
#include <libpurple/purpletags.h>

G_BEGIN_DECLS

#define PURPLE_TYPE_CONTACT (purple_contact_get_type())
G_DECLARE_FINAL_TYPE(PurpleContact, purple_contact, PURPLE, CONTACT, GObject)

/**
 * PurpleContactPermission:
 * @PURPLE_CONTACT_PERMISSION_UNSET: The value is unset.
 * @PURPLE_CONTACT_PERMISSION_ALLOW: The contact is allowed to contact the
 *                                   user.
 * @PURPLE_CONTACT_PERMISSION_DENY: The contact is not allowed to contact the
 *                                  user.
 *
 * A representation of whether or not a contact has permission to contact the
 * user.
 *
 * Since: 3.0.0
 */
typedef enum {
	PURPLE_CONTACT_PERMISSION_UNSET = 0,
	PURPLE_CONTACT_PERMISSION_ALLOW,
	PURPLE_CONTACT_PERMISSION_DENY,
} PurpleContactPermission;

#include <libpurple/account.h>
#include <libpurple/purpleperson.h>

/**
 * PurpleContact:
 *
 * A representation of a user. Contacts are used everywhere you need to refer to
 * a user. Be it a chat, an direct message, a file transfer, etc.
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
 * Since: 3.0.0
 */
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
 * Since: 3.0.0
 */
PurpleAccount *purple_contact_get_account(PurpleContact *contact);

/**
 * purple_contact_get_id:
 * @contact: The instance.
 *
 * Gets the id of @contact.
 *
 * If a protocol would like to set this, it should call
 * [ctor@GObject.Object.new] and pass in the id attribute manually.
 *
 * Returns: The id of the contact.
 *
 * Since: 3.0.0
 */
const gchar *purple_contact_get_id(PurpleContact *contact);

/**
 * purple_contact_set_id:
 * @contact: The instance.
 * @id: The new identifier.
 *
 * Sets the identifier of @contact to @id. Note, this should be used rarely if
 * at all. The main intent of this, is for protocols to update the id of
 * [property@Purple.Account:contact] when an account is connected if it is
 * missing.
 *
 * Since: 3.0.0
 */
void purple_contact_set_id(PurpleContact *contact, const char *id);

/**
 * purple_contact_get_username:
 * @contact: The instance.
 *
 * Gets the username of @contact.
 *
 * Returns: The username of @contact.
 *
 * Since: 3.0.0
 */
const gchar *purple_contact_get_username(PurpleContact *contact);

/**
 * purple_contact_set_username:
 * @contact: The instance.
 * @username: The new username.
 *
 * Sets the username of @contact to @username.
 *
 * This is primarily used by protocol plugins like IRC when a user changes
 * their "nick" which is their username.
 *
 * Since: 3.0.0
 */
void purple_contact_set_username(PurpleContact *contact, const gchar *username);

/**
 * purple_contact_get_display_name:
 * @contact: The instance.
 *
 * Gets the display name for @contact. The display name is typically set by the
 * contact and is handled by the protocol plugin.
 *
 * Returns: (nullable): The display name of @contact if one is set, otherwise
 *          %NULL will be returned.
 *
 * Since: 3.0.0
 */
const gchar *purple_contact_get_display_name(PurpleContact *contact);

/**
 * purple_contact_set_display_name:
 * @contact: The instance.
 * @display_name: (nullable): The new displayname.
 *
 * Sets the display name of @contact to @display_name.
 *
 * This should primarily only be used by protocol plugins and everyone else
 * should be using [method@Purple.Contact.set_alias].
 *
 * Since: 3.0.0
 */
void purple_contact_set_display_name(PurpleContact *contact, const gchar *display_name);

/**
 * purple_contact_get_alias:
 * @contact: The instance.
 *
 * Gets the alias for @contact.
 *
 * Returns: (nullable): The alias of @contact if one is set, otherwise %NULL.
 *
 * Since: 3.0.0
 */
const gchar *purple_contact_get_alias(PurpleContact *contact);

/**
 * purple_contact_set_alias:
 * @contact: The instance.
 * @alias: (nullable): The new alias.
 *
 * Sets the alias of @contact to @alias.
 *
 * Protocol plugins may use this value to synchronize across instances.
 *
 * Since: 3.0.0
 */
void purple_contact_set_alias(PurpleContact *contact, const gchar *alias);

/**
 * purple_contact_get_avatar:
 * @contact: The instance.
 *
 * Gets the avatar for @contact if one is set.
 *
 * Returns: (transfer none): The avatar if set, otherwise %NULL.
 *
 * Since: 3.0.0
 */
GdkPixbuf *purple_contact_get_avatar(PurpleContact *contact);

/**
 * purple_contact_set_avatar:
 * @contact: The instance.
 * @avatar: (nullable): The new avatar to set.
 *
 * Sets the avatar for @contact to @avatar. If @avatar is %NULL an existing
 * avatar will be removed.
 *
 * Typically this should only called by the protocol plugin.
 *
 * Since: 3.0.0
 */
void purple_contact_set_avatar(PurpleContact *contact, GdkPixbuf *avatar);

/**
 * purple_contact_get_presence:
 * @contact: The instance.
 *
 * Gets the [class@Purple.Presence] for @contact.
 *
 * Returns: (transfer none) (nullable): The presence for @contact if one is
 *          set, otherwise %NULL.
 *
 * Since: 3.0.0
 */
PurplePresence *purple_contact_get_presence(PurpleContact *contact);

/**
 * purple_contact_get_tags:
 * @contact: The instance.
 *
 * Gets the [class@Purple.Tags] instance for @contact.
 *
 * Returns: (transfer none): The tags for @contact.
 *
 * Since: 3.0.0
 */
PurpleTags *purple_contact_get_tags(PurpleContact *contact);

/**
 * purple_contact_set_person:
 * @contact: The instance.
 * @person: (nullable): The new [class@Purple.Person] or %NULL.
 *
 * Sets the person that @contact belongs to to @person.
 *
 * Since: 3.0.0
 */
void purple_contact_set_person(PurpleContact *contact, PurplePerson *person);

/**
 * purple_contact_get_person:
 * @contact: The instance.
 *
 * Gets the [class@Purple.Person] that @contact belongs to.
 *
 * Returns: (transfer none) (nullable): The [class@Purple.Person] that @contact
 *          belongs to, or %NULL.
 *
 * Since: 3.0.0
 */
PurplePerson *purple_contact_get_person(PurpleContact *contact);

/**
 * purple_contact_get_permission:
 * @contact: The instance.
 *
 * Gets the [enum@Purple.ContactPermission] for @contact.
 *
 * Returns: The permission for @contact.
 *
 * Since: 3.0.0
 */
PurpleContactPermission purple_contact_get_permission(PurpleContact *contact);

/**
 * purple_contact_set_permission:
 * @contact: The instance.
 * @permission: The new permission of the contact.
 *
 * Sets the permission of @contact to @permission.
 *
 * Since: 3.0.0
 */
void purple_contact_set_permission(PurpleContact *contact, PurpleContactPermission permission);

/**
 * purple_contact_get_name_for_display:
 * @contact: The instance.
 *
 * Gets the name that should be displayed for @contact.
 *
 * If @contact is associated with a [class@Purple.Person], the value of
 * [property@Purple.Person:alias] will be returned if it is set.
 *
 * Otherwise, this will return the first set property from the following list:
 *
 *  * [property@Purple.Contact:alias]
 *  * [property@Purple.Contact:display-name]
 *  * [property@Purple.Contact:username]
 *  * [property@Purple.Contact:id]
 *
 * Returns: (transfer none): The name to display for @contact.
 *
 * Since: 3.0.0
 */
const char *purple_contact_get_name_for_display(PurpleContact *contact);

/**
 * purple_contact_compare:
 * @a: The first instance.
 * @b: The second instance.
 *
 * Compares contacts @a and @b
 *
 * Returns: less than 0 if @a should be sorted before @b, 0 if they sorted
 *          equally, and greater than 0 if @a should be sorted after @b.
 *
 * Since: 3.0.0
 */
int purple_contact_compare(PurpleContact *a, PurpleContact *b);

G_END_DECLS

#endif /* PURPLE_CONTACT_H */
