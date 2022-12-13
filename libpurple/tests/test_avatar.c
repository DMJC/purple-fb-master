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

#include <glib.h>

#include <purple.h>

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_avatar_new(void) {
	PurpleAvatar *avatar = NULL;
	GdkPixbuf *pixbuf = NULL;

	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);
	avatar = purple_avatar_new("filename", pixbuf);

	g_assert_true(PURPLE_IS_AVATAR(avatar));

	g_assert_cmpstr(purple_avatar_get_filename(avatar), ==, "filename");
	g_assert_true(purple_avatar_get_pixbuf(avatar) == pixbuf);

	g_clear_object(&avatar);
	g_clear_object(&pixbuf);
}

static void
test_purple_avatar_properties(void) {
	PurpleAvatar *avatar = NULL;
	PurpleTags *tags = NULL;
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *pixbuf1 = NULL;
	gchar *filename = NULL;

	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);

	/* Use g_object_new so we can test setting properties by name. All of them
	 * call the setter methods, so by doing it this way we exercise more of the
	 * code.
	 */
	avatar = g_object_new(
		PURPLE_TYPE_AVATAR,
		"filename", "filename",
		"pixbuf", pixbuf,
		NULL);

	/* Now use g_object_get to read all of the properties. */
	g_object_get(avatar,
		"filename", &filename,
		"pixbuf", &pixbuf1,
		"tags", &tags,
		NULL);

	/* Compare all the things. */
	g_assert_cmpstr(filename, ==, "filename");
	g_assert_true(pixbuf1 == pixbuf);
	g_assert_nonnull(tags);

	/* Free/unref all the things. */
	g_clear_pointer(&filename, g_free);
	g_clear_object(&pixbuf1);
	g_clear_object(&tags);

	g_clear_object(&avatar);
	g_clear_object(&pixbuf);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/avatar/new",
	                test_purple_avatar_new);
	g_test_add_func("/avatar/properties",
	                test_purple_avatar_properties);

	return g_test_run();
}
