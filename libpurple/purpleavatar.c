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

#include "purpleavatar.h"

struct _PurpleAvatar {
	GObject parent;

	char *filename;
	GdkPixbuf *pixbuf;
	PurpleTags *tags;
};

enum {
	PROP_0,
	PROP_FILENAME,
	PROP_PIXBUF,
	PROP_TAGS,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE(PurpleAvatar, purple_avatar, G_TYPE_OBJECT)

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_avatar_get_property(GObject *obj, guint param_id, GValue *value,
                           GParamSpec *pspec)
{
	PurpleAvatar *avatar = PURPLE_AVATAR(obj);

	switch(param_id) {
		case PROP_FILENAME:
			g_value_set_string(value, purple_avatar_get_filename(avatar));
			break;
		case PROP_PIXBUF:
			g_value_set_object(value, purple_avatar_get_pixbuf(avatar));
			break;
		case PROP_TAGS:
			g_value_set_object(value, purple_avatar_get_tags(avatar));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_avatar_set_property(GObject *obj, guint param_id, const GValue *value,
                           GParamSpec *pspec)
{
	PurpleAvatar *avatar = PURPLE_AVATAR(obj);

	switch(param_id) {
		case PROP_FILENAME:
			purple_avatar_set_filename(avatar, g_value_get_string(value));
			break;
		case PROP_PIXBUF:
			purple_avatar_set_pixbuf(avatar, g_value_get_object(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_avatar_finalize(GObject *obj) {
	PurpleAvatar *avatar = PURPLE_AVATAR(obj);

	g_clear_pointer(&avatar->filename, g_free);
	g_clear_object(&avatar->pixbuf);
	g_clear_object(&avatar->tags);

	G_OBJECT_CLASS(purple_avatar_parent_class)->finalize(obj);
}

static void
purple_avatar_init(PurpleAvatar *avatar) {
	avatar->tags = purple_tags_new();
}

static void
purple_avatar_class_init(PurpleAvatarClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_avatar_finalize;
	obj_class->get_property = purple_avatar_get_property;
	obj_class->set_property = purple_avatar_set_property;

	/**
	 * PurpleAvatar:filename:
	 *
	 * The filename to save/load the avatar to/from.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_FILENAME] = g_param_spec_string(
		"filename", "filename",
		"The filename to save/load the avatar from.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAvatar:pixbuf:
	 *
	 * The [class@GdkPixbuf.Pixbuf]. This may be %NULL if
	 * [method@Purple.Avatar.load] has not yet been called.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_PIXBUF] = g_param_spec_object(
		"pixbuf", "pixbuf",
		"The pixbuf of the avatar.",
		GDK_TYPE_PIXBUF,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAvatar:tags:
	 *
	 * The [class@Purple.Tags] for the avatar.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TAGS] = g_param_spec_object(
		"tags", "tags",
		"The tags for the avatar.",
		PURPLE_TYPE_TAGS,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleAvatar *
purple_avatar_new(const char *filename, GdkPixbuf *pixbuf) {
	return g_object_new(
		PURPLE_TYPE_AVATAR,
		"filename", filename,
		"pixbuf", pixbuf,
		NULL);
}

const char *
purple_avatar_get_filename(PurpleAvatar *avatar) {
	g_return_val_if_fail(PURPLE_IS_AVATAR(avatar), NULL);

	return avatar->filename;
}

void
purple_avatar_set_filename(PurpleAvatar *avatar, const char *filename) {
	g_return_if_fail(PURPLE_IS_AVATAR(avatar));

	g_free(avatar->filename);
	avatar->filename = g_strdup(filename);

	g_object_notify_by_pspec(G_OBJECT(avatar), properties[PROP_FILENAME]);
}

GdkPixbuf *
purple_avatar_get_pixbuf(PurpleAvatar *avatar) {
	g_return_val_if_fail(PURPLE_IS_AVATAR(avatar), NULL);

	return avatar->pixbuf;
}

void
purple_avatar_set_pixbuf(PurpleAvatar *avatar, GdkPixbuf *pixbuf) {
	g_return_if_fail(PURPLE_IS_AVATAR(avatar));

	if(g_set_object(&avatar->pixbuf, pixbuf)) {
		g_object_notify_by_pspec(G_OBJECT(avatar), properties[PROP_PIXBUF]);
	}
}

PurpleTags *
purple_avatar_get_tags(PurpleAvatar *avatar) {
	g_return_val_if_fail(PURPLE_IS_AVATAR(avatar), NULL);

	return avatar->tags;
}

gboolean
purple_avatar_load(PurpleAvatar *avatar, GError **error) {
	GError *local_error = NULL;

	g_return_val_if_fail(PURPLE_IS_AVATAR(avatar), FALSE);

	g_clear_object(&avatar->pixbuf);

	avatar->pixbuf = gdk_pixbuf_new_from_file(avatar->filename, &local_error);

	if(avatar->pixbuf == NULL) {
		g_propagate_error(error, local_error);

		return FALSE;
	}

	return TRUE;
}

gboolean
purple_avatar_save(PurpleAvatar *avatar, GError **error) {
	g_return_val_if_fail(PURPLE_IS_AVATAR(avatar), FALSE);

	return gdk_pixbuf_save(avatar->pixbuf, avatar->filename, "png", error,
	                       "quality", "100", NULL);
}
