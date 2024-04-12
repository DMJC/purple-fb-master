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

#ifndef PURPLE_BUDDY_H
#define PURPLE_BUDDY_H

#define PURPLE_TYPE_BUDDY             (purple_buddy_get_type())
#define PURPLE_BUDDY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), PURPLE_TYPE_BUDDY, PurpleBuddy))
#define PURPLE_BUDDY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), PURPLE_TYPE_BUDDY, PurpleBuddyClass))
#define PURPLE_IS_BUDDY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_BUDDY))
#define PURPLE_IS_BUDDY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), PURPLE_TYPE_BUDDY))
#define PURPLE_BUDDY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), PURPLE_TYPE_BUDDY, PurpleBuddyClass))

typedef struct _PurpleBuddy PurpleBuddy;
typedef struct _PurpleBuddyClass PurpleBuddyClass;

#include "blistnode.h"
#include "buddyicon.h"
#include "chat.h"
#include "contact.h"
#include "group.h"
#include "media.h"
#include "purpleaccount.h"
#include "purplepresence.h"
#include "purpleversion.h"

/**
 * PURPLE_BUDDY_IS_ONLINE:
 * @b: The #PurpleBuddy instance.
 *
 * Checks whether @b is online or not.
 */
#define PURPLE_BUDDY_IS_ONLINE(b) \
	(PURPLE_IS_BUDDY(b) \
	&& purple_account_is_connected(purple_buddy_get_account(PURPLE_BUDDY(b))) \
	&& purple_presence_is_online(purple_buddy_get_presence(PURPLE_BUDDY(b))))

/**
 * PURPLE_BLIST_NODE_NAME:
 * @n: The #PurpleBlistNode instance.
 *
 * Gets the name of @n.
 */
#define PURPLE_BLIST_NODE_NAME(n) \
	(PURPLE_IS_CHAT(n) ? purple_chat_get_name(PURPLE_CHAT(n)) : \
	PURPLE_IS_BUDDY(n) ? purple_buddy_get_name(PURPLE_BUDDY(n)) : NULL)

/**************************************************************************/
/* Data Structures                                                        */
/**************************************************************************/
/**
 * PurpleBuddy:
 *
 * A buddy on the buddy list.
 *
 * Since: 2.0
 */
struct _PurpleBuddy {
	PurpleBlistNode node;
};

struct _PurpleBuddyClass {
	PurpleBlistNodeClass node_class;

	/*< private >*/
	void (*_purple_reserved1)(void);
	void (*_purple_reserved2)(void);
	void (*_purple_reserved3)(void);
	void (*_purple_reserved4)(void);
};

G_BEGIN_DECLS

/**************************************************************************/
/* Buddy API                                                              */
/**************************************************************************/

/**
 * purple_buddy_get_type:
 *
 * Returns: The #GType for the #PurpleBuddy object.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GType purple_buddy_get_type(void);

/**
 * purple_buddy_new:
 * @account:    The account this buddy will get added to
 * @name:       The name of the new buddy
 * @alias:      The alias of the new buddy (or NULL if unaliased)
 *
 * Creates a new buddy.
 *
 * This function only creates the #PurpleBuddy. Use purple_blist_add_buddy()
 * to add the buddy to the list and purple_account_add_buddy() to sync up
 * with the server.
 *
 * See purple_account_add_buddy(), purple_blist_add_buddy().
 *
 * Returns: A newly allocated buddy
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleBuddy *purple_buddy_new(PurpleAccount *account, const char *name, const char *alias);

/**
 * purple_buddy_get_id:
 * @buddy: The buddy.
 *
 * Gets the globally unique identifier of the buddy.
 *
 * Returns: The id of @buddy.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar *purple_buddy_get_id(PurpleBuddy *buddy);

/**
 * purple_buddy_set_icon:
 * @buddy: The buddy.
 * @icon:  The buddy icon.
 *
 * Sets a buddy's icon.
 *
 * This should only be called from within Purple. You probably want to
 * call purple_buddy_icon_set_data().
 *
 * See purple_buddy_icon_set_data().
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_buddy_set_icon(PurpleBuddy *buddy, PurpleBuddyIcon *icon);

/**
 * purple_buddy_get_icon:
 * @buddy: The buddy.
 *
 * Returns a buddy's icon.
 *
 * Returns: The buddy icon.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleBuddyIcon *purple_buddy_get_icon(PurpleBuddy *buddy);

/**
 * purple_buddy_get_account:
 * @buddy: The buddy.
 *
 * Returns a buddy's account.
 *
 * Returns: (transfer none): The account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleAccount *purple_buddy_get_account(PurpleBuddy *buddy);

/**
 * purple_buddy_set_name:
 * @buddy: The buddy.
 * @name:  The name.
 *
 * Sets a buddy's name
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_buddy_set_name(PurpleBuddy *buddy, const char *name);

/**
 * purple_buddy_get_name:
 * @buddy: The buddy.
 *
 * Returns a buddy's name
 *
 * Returns: The name.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_buddy_get_name(PurpleBuddy *buddy);

/**
 * purple_buddy_get_protocol_data:
 * @buddy: The buddy.
 *
 * Returns a buddy's protocol-specific data.
 *
 * This should only be called from the associated protocol.
 *
 * See purple_buddy_set_protocol_data().
 *
 * Returns:      The protocol data.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
gpointer purple_buddy_get_protocol_data(PurpleBuddy *buddy);

/**
 * purple_buddy_set_protocol_data:
 * @buddy: The buddy.
 * @data:  The data.
 *
 * Sets a buddy's protocol-specific data.
 *
 * This should only be called from the associated protocol.
 *
 * See purple_buddy_get_protocol_data().
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
void purple_buddy_set_protocol_data(PurpleBuddy *buddy, gpointer data);

/**
 * purple_buddy_get_contact:
 * @buddy: The buddy.
 *
 * Returns a buddy's contact.
 *
 * Returns: (transfer none): The buddy's contact.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleMetaContact *purple_buddy_get_contact(PurpleBuddy *buddy);

/**
 * purple_buddy_get_presence:
 * @buddy: The buddy.
 *
 * Returns a buddy's presence.
 *
 * Returns: (transfer none): The buddy's presence.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurplePresence *purple_buddy_get_presence(PurpleBuddy *buddy);

/**
 * purple_buddy_get_media_caps:
 * @buddy: The buddy.
 *
 * Gets the media caps from a buddy.
 *
 * Returns:      The media caps.
 *
 * Since: 2.7
 */
PURPLE_AVAILABLE_IN_2_7
PurpleMediaCaps purple_buddy_get_media_caps(PurpleBuddy *buddy);

/**
 * purple_buddy_set_media_caps:
 * @buddy:      The PurpleBuddy.
 * @media_caps: The PurpleMediaCaps.
 *
 * Sets the media caps for a buddy.
 *
 * Since: 2.7
 */
PURPLE_AVAILABLE_IN_2_7
void purple_buddy_set_media_caps(PurpleBuddy *buddy, PurpleMediaCaps media_caps);

/**
 * purple_buddy_get_alias_only:
 * @buddy:   The buddy whose alias will be returned.
 *
 * Returns the alias of a buddy.
 *
 * Returns:        The alias (if set), server alias (if set),
 *                or NULL.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_buddy_get_alias_only(PurpleBuddy *buddy);

/**
 * purple_buddy_set_server_alias:
 * @buddy:  The buddy.
 * @alias:  The server alias to be set.
 *
 * Sets the server alias for a buddy.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_buddy_set_server_alias(PurpleBuddy *buddy, const char *alias);

/**
 * purple_buddy_get_server_alias:
 * @buddy:  The buddy whose server alias will be returned
 *
 * Gets the server alias for a buddy.
 *
 * Returns:  The server alias, or NULL if it is not set.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_buddy_get_server_alias(PurpleBuddy *buddy);

/**
 * purple_buddy_get_contact_alias:
 * @buddy:  The buddy whose alias will be returned
 *
 * Returns the correct name to display for a buddy, taking the contact alias
 * into account. In order of precedence: the buddy's alias; the buddy's
 * contact alias; the buddy's server alias; the buddy's user name.
 *
 * Returns:       The appropriate name or alias, or NULL.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_buddy_get_contact_alias(PurpleBuddy *buddy);

/**
 * purple_buddy_get_alias:
 * @buddy:   The buddy whose alias will be returned.
 *
 * Returns the correct name to display for a buddy. In order of precedence:
 * the buddy's local alias; the buddy's server alias; the buddy's contact alias;
 * the buddy's user name.
 *
 * Returns:        The appropriate name or alias, or NULL
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_buddy_get_alias(PurpleBuddy *buddy);

/**
 * purple_buddy_set_local_alias:
 * @buddy:  The buddy
 * @alias:  The local alias for the buddy
 *
 * Sets the local alias for the buddy.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_buddy_set_local_alias(PurpleBuddy *buddy, const char *alias);

/**
 * purple_buddy_get_local_alias:
 * @buddy:  The buddy
 *
 * Returns the local alias for the buddy, or %NULL if none exists.
 *
 * Returns:       The local alias for the buddy
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_buddy_get_local_alias(PurpleBuddy *buddy);

/**
 * purple_buddy_get_group:
 * @buddy:   The buddy
 *
 * Returns the group of which the buddy is a member.
 *
 * Returns: (transfer none): The group or %NULL if the buddy is not in a group.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleGroup *purple_buddy_get_group(PurpleBuddy *buddy);

G_END_DECLS

#endif /* PURPLE_BUDDY_H */
