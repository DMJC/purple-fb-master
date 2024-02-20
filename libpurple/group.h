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

#ifndef PURPLE_GROUP_H
#define PURPLE_GROUP_H

#include "countingnode.h"

#define PURPLE_TYPE_GROUP             (purple_group_get_type())
#define PURPLE_GROUP(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), PURPLE_TYPE_GROUP, PurpleGroup))
#define PURPLE_GROUP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), PURPLE_TYPE_GROUP, PurpleGroupClass))
#define PURPLE_IS_GROUP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_GROUP))
#define PURPLE_IS_GROUP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), PURPLE_TYPE_GROUP))
#define PURPLE_GROUP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), PURPLE_TYPE_GROUP, PurpleGroupClass))

typedef struct _PurpleGroup PurpleGroup;
typedef struct _PurpleGroupClass PurpleGroupClass;

#include "purpleaccount.h"
#include "purpleversion.h"

/**************************************************************************/
/* Data Structures                                                        */
/**************************************************************************/

/**
 * PurpleGroup:
 *
 * A group on the buddy list.
 *
 * A group is a counting node, which means it keeps track of the counts of the
 * chats and contacts under this group.
 *
 * Since: 2.0.0
 */
struct _PurpleGroup {
	PurpleCountingNode counting;
};

struct _PurpleGroupClass {
	PurpleCountingNodeClass counting_class;

	/*< private >*/
	void (*_purple_reserved1)(void);
	void (*_purple_reserved2)(void);
	void (*_purple_reserved3)(void);
	void (*_purple_reserved4)(void);
};

G_BEGIN_DECLS

/**************************************************************************/
/* Group API                                                              */
/**************************************************************************/

/**
 * purple_group_get_type:
 *
 * Returns: The #GType for the #PurpleGroup object.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GType purple_group_get_type(void);

/**
 * purple_group_new:
 * @name: The name of the new group
 *
 * Creates a new group
 *
 * You can't have more than one group with the same name.  Sorry.  If you pass
 * this the name of a group that already exists, it will return that group.
 *
 * Returns: A new group struct
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleGroup *purple_group_new(const char *name);

/**
 * purple_group_get_accounts:
 * @g: The group
 *
 * Returns a list of accounts that have buddies in this group
 *
 * Returns: (element-type PurpleAccount) (transfer container): A list of
 *          accounts, or %NULL if the group has no accounts.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
GSList *purple_group_get_accounts(PurpleGroup *g);

/**
 * purple_group_on_account:
 * @g:       The group to search through.
 * @account: The account.
 *
 * Determines whether an account owns any buddies in a given group
 *
 * Returns: TRUE if there are any buddies in the group, or FALSE otherwise.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_group_on_account(PurpleGroup *g, PurpleAccount *account);

/**
 * purple_group_set_name:
 * @group: The group.
 * @name:  The name of the group.
 *
 * Sets the name of a group.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_group_set_name(PurpleGroup *group, const char *name);

/**
 * purple_group_get_name:
 * @group: The group.
 *
 * Returns the name of a group.
 *
 * Returns: The name of the group.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_group_get_name(PurpleGroup *group);

G_END_DECLS

#endif /* PURPLE_GROUP_H */

