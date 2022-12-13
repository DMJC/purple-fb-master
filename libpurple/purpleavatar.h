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

#ifndef PURPLE_AVATAR_H
#define PURPLE_AVATAR_H

#include <glib.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libpurple/purpletags.h>

G_BEGIN_DECLS

#define PURPLE_TYPE_AVATAR (purple_avatar_get_type())
G_DECLARE_FINAL_TYPE(PurpleAvatar, purple_avatar, PURPLE, AVATAR, GObject)

/**
 * PurpleAvatar:
 *
 * A representation of an Avatar that is used for accounts, contacts, and
 * conversations.
 *
 * Since: 3.0.0
 */

/**
 * purple_avatar_new:
 * @filename: (nullable): The filename for the avatar.
 * @pixbuf: (nullable): The [class@GdkPixbuf.Pixbuf] to use.
 *
 * Creates a new avatar with @filename and @pixbuf.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0.0
 */
PurpleAvatar *purple_avatar_new(const char *filename, GdkPixbuf *pixbuf);

/**
 * purple_avatar_get_filename:
 * @avatar: The instance.
 *
 * Gets the base filename of @avatar.
 *
 * Returns: The base filename of @avatar.
 *
 * Since: 3.0.0
 */
const char *purple_avatar_get_filename(PurpleAvatar *avatar);

/**
 * purple_avatar_set_filename:
 * @avatar: The instance.
 * @filename: The new filename.
 *
 * Sets the filename of @avatar to @filename.
 *
 * Since: 3.0.0
 */
void purple_avatar_set_filename(PurpleAvatar *avatar, const char *filename);

/**
 * purple_avatar_get_pixbuf:
 * @avatar: The instance.
 *
 * Gets the [class@GdkPixbuf.Pixbuf] of @avatar.
 *
 * Returns: The pixbuf of the avatar which could be %NULL.
 *
 * Since: 3.0.0
 */
GdkPixbuf *purple_avatar_get_pixbuf(PurpleAvatar *avatar);

/**
 * purple_avatar_set_pixbuf:
 * @avatar: The instance.
 * @pixbuf: (nullable): The new [class@GdkPixbuf.Pixbuf].
 *
 * Sets the [class@GdkPixbuf.Pixbuf] for @avatar to @pixbuf. If @pixbuf is
 * %NULL, the pixbuf will be cleared.
 *
 * Since: 3.0.0
 */
void purple_avatar_set_pixbuf(PurpleAvatar *avatar, GdkPixbuf *pixbuf);

/**
 * purple_avatar_get_tags:
 * @avatar: The instance.
 *
 * Gets the [class@Purple.Tags] for @avatar.
 *
 * Returns: (transfer none): The [class@Purple.Tags] for @avatar.
 *
 * Since: 3.0.0
 */
PurpleTags *purple_avatar_get_tags(PurpleAvatar *avatar);

/**
 * purple_avatar_load:
 * @avatar: The instance.
 * @error: Return address for a #GError, or %NULL.
 *
 * Attempts to load @avatar from disk from [property@Purple.Avatar:filename].
 * If successful, %TRUE is returned, otherwise %FALSE will be returned with
 * @error potentially set.
 *
 * Returns: %TRUE on success or %FALSE on error with @error potentialy set.
 *
 * Since: 3.0.0
 */
gboolean purple_avatar_load(PurpleAvatar *avatar, GError **error);

/**
 * purple_avatar_save:
 * @avatar: The instance.
 * @error: Return address for a #GError, or %NULL.
 *
 * Attempts to save @avatar to disk using [property@Purple.Avatar:filename].
 * If successful, %TRUE is returned, otherwise %FALSE will be returned with
 * @error potentially set.
 *
 * Returns: %TRUE on success or %FALSE on error with @error potentialy set.
 *
 * Since: 3.0.0
 */
gboolean purple_avatar_save(PurpleAvatar *avatar, GError **error);

G_END_DECLS

#endif /* PURPLE_AVATAR_H */
