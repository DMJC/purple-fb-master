/* pidgin
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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

#ifndef _PIDGINDIALOGS_H_
#define _PIDGINDIALOGS_H_
/**
 * SECTION:gtkdialogs
 * @section_id: pidgin-gtkdialogs
 * @short_description: <filename>gtkdialogs.h</filename>
 * @title: Pidgin Dialogs
 */

#include "pidgin.h"

#include <purple.h>

G_BEGIN_DECLS

/* Functions in gtkdialogs.c (these should actually stay in this file) */
void pidgin_dialogs_destroy_all(void);
/**
 * pidgin_dialogs_plugins_info:
 *
 * This should not be used by anything but Pidgin.
 *
 * Since: 2.10.0
 */
void pidgin_dialogs_plugins_info(void);
void pidgin_dialogs_im(void);
void pidgin_dialogs_im_with_user(PurpleAccount *account, const char *username);
void pidgin_dialogs_info(void);
void pidgin_dialogs_log(void);

void pidgin_dialogs_alias_buddy(PurpleBuddy *buddy);
void pidgin_dialogs_alias_chat(PurpleChat *chat);
void pidgin_dialogs_remove_buddy(PurpleBuddy *buddy);
void pidgin_dialogs_remove_group(PurpleGroup *group);
void pidgin_dialogs_remove_chat(PurpleChat *chat);
void pidgin_dialogs_remove_contact(PurpleContact *contact);
void pidgin_dialogs_merge_groups(PurpleGroup *group, const char *name);

G_END_DECLS

#endif /* _PIDGINDIALOGS_H_ */
