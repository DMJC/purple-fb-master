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
 * Callbacks
 *****************************************************************************/
static void
test_purple_message_notify_counter_cb(G_GNUC_UNUSED GObject *obj,
                                      G_GNUC_UNUSED GParamSpec *pspec,
                                      gpointer data)
{
	guint *counter = data;

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_message_properties(void) {
	PurpleMessage *message = NULL;
	PurpleMessageContentType content_type = 0;
	PurpleMessageFlags flags = 0;
	GDateTime *timestamp = NULL;
	GDateTime *timestamp1 = NULL;
	GDateTime *delivered_at1 = NULL;
	GDateTime *edited_at1 = NULL;
	GError *error = NULL;
	GError *error1 = NULL;
	char *id = NULL;
	char *author = NULL;
	char *author_alias = NULL;
	char *author_name_color = NULL;
	char *recipient = NULL;
	char *contents = NULL;
	gboolean action = FALSE;
	gboolean delivered = FALSE;
	gboolean edited = FALSE;

	timestamp = g_date_time_new_from_unix_utc(911347200);
	error = g_error_new(g_quark_from_static_string("test-message"), 0,
	                    "delivery failed");

	/* We don't set delivered-at here because delivered will set it for us so
	 * it's impossible to check that here, which is why we have separate tests
	 * to do that check. We do verify that delivered-at was set though.
	 */
	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"id", "id",
		"action", TRUE,
		"author", "author",
		"author-alias", "alias",
		"author-name-color", "purple",
		"delivered", TRUE,
		"edited", TRUE,
		"recipient", "pidgy",
		"contents", "Now that is a big door",
		"content-type", PURPLE_MESSAGE_CONTENT_TYPE_MARKDOWN,
		"timestamp", timestamp,
		"flags", PURPLE_MESSAGE_SYSTEM,
		"error", error,
		NULL);

	g_object_get(
		message,
		"id", &id,
		"action", &action,
		"author", &author,
		"author-alias", &author_alias,
		"author-name-color", &author_name_color,
		"delivered", &delivered,
		"delivered-at", &delivered_at1,
		"edited", &edited,
		"edited-at", &edited_at1,
		"recipient", &recipient,
		"contents", &contents,
		"content-type", &content_type,
		"timestamp", &timestamp1,
		"flags", &flags,
		"error", &error1,
		NULL);

	g_assert_cmpstr(id, ==, "id");
	g_assert_true(action);
	g_assert_cmpstr(author, ==, "author");
	g_assert_cmpstr(author_alias, ==, "alias");
	g_assert_cmpstr(author_name_color, ==, "purple");
	g_assert_true(delivered);
	g_assert_nonnull(delivered_at1);
	g_assert_true(edited);
	g_assert_nonnull(edited_at1);
	g_assert_cmpstr(recipient, ==, "pidgy");
	g_assert_cmpstr(contents, ==, "Now that is a big door");
	g_assert_cmpint(content_type, ==, PURPLE_MESSAGE_CONTENT_TYPE_MARKDOWN);
	g_assert_true(g_date_time_equal(timestamp1, timestamp));
	g_assert_cmpint(flags, ==, PURPLE_MESSAGE_SYSTEM);
	g_assert_error(error1, error->domain, error->code);

	g_clear_pointer(&id, g_free);
	g_clear_pointer(&author, g_free);
	g_clear_pointer(&author_alias, g_free);
	g_clear_pointer(&author_name_color, g_free);
	g_clear_pointer(&delivered_at1, g_date_time_unref);
	g_clear_pointer(&edited_at1, g_date_time_unref);
	g_clear_pointer(&recipient, g_free);
	g_clear_pointer(&contents, g_free);
	g_clear_pointer(&timestamp, g_date_time_unref);
	g_clear_pointer(&timestamp1, g_date_time_unref);
	g_clear_error(&error1);
	g_clear_object(&message);
}

static void
test_purple_message_delivered_set_delivered_at(void) {
	PurpleMessage *message = NULL;
	guint delivered_counter = 0;
	guint delivered_at_counter = 0;

	message = g_object_new(PURPLE_TYPE_MESSAGE, NULL);
	g_signal_connect(message, "notify::delivered",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &delivered_counter);
	g_signal_connect(message, "notify::delivered-at",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &delivered_at_counter);

	/* The default delivery state is FALSE, so setting it to true, should call
	 * the notify signals and set delivered-at.
	 */
	purple_message_set_delivered(message, TRUE);
	g_assert_true(purple_message_get_delivered(message));
	g_assert_nonnull(purple_message_get_delivered_at(message));
	g_assert_cmpuint(delivered_counter, ==, 1);
	g_assert_cmpuint(delivered_at_counter, ==, 1);

	/* Now clear everything and verify it's empty. */
	delivered_counter = 0;
	delivered_at_counter = 0;

	purple_message_set_delivered(message, FALSE);
	g_assert_false(purple_message_get_delivered(message));
	g_assert_null(purple_message_get_delivered_at(message));
	g_assert_cmpuint(delivered_counter, ==, 1);
	g_assert_cmpuint(delivered_at_counter, ==, 1);

	g_clear_object(&message);
}

static void
test_purple_message_delivered_at_set_delivered(void) {
	PurpleMessage *message = NULL;
	GDateTime *delivered_at = NULL;
	GDateTime *delivered_at1 = NULL;
	guint delivered_counter = 0;
	guint delivered_at_counter = 0;

	message = g_object_new(PURPLE_TYPE_MESSAGE, NULL);
	g_signal_connect(message, "notify::delivered",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &delivered_counter);
	g_signal_connect(message, "notify::delivered-at",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &delivered_at_counter);

	/* The default value for delivered-at is NULL, so setting it to non-null
	 * should emit the signals and everything.
	 */
	delivered_at = g_date_time_new_now_utc();
	purple_message_set_delivered_at(message, delivered_at);
	g_assert_true(purple_message_get_delivered(message));
	delivered_at1 = purple_message_get_delivered_at(message);
	g_assert_nonnull(delivered_at1);
	g_assert_true(g_date_time_equal(delivered_at1, delivered_at));
	g_assert_cmpuint(delivered_counter, ==, 1);
	g_assert_cmpuint(delivered_at_counter, ==, 1);

	/* Now clear everything and make sure it's all good. */
	delivered_counter = 0;
	delivered_at_counter = 0;
	purple_message_set_delivered_at(message, NULL);
	g_assert_false(purple_message_get_delivered(message));
	g_assert_null(purple_message_get_delivered_at(message));
	g_assert_cmpuint(delivered_counter, ==, 1);
	g_assert_cmpuint(delivered_at_counter, ==, 1);

	g_clear_pointer(&delivered_at, g_date_time_unref);
	g_clear_object(&message);
}

static void
test_purple_message_edited_set_edited_at(void) {
	PurpleMessage *message = NULL;
	guint edited_counter = 0;
	guint edited_at_counter = 0;

	message = g_object_new(PURPLE_TYPE_MESSAGE, NULL);
	g_signal_connect(message, "notify::edited",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &edited_counter);
	g_signal_connect(message, "notify::edited-at",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &edited_at_counter);

	/* The default edit state is FALSE, so setting it to true, should call
	 * the notify signals and set edited-at.
	 */
	purple_message_set_edited(message, TRUE);
	g_assert_true(purple_message_get_edited(message));
	g_assert_nonnull(purple_message_get_edited_at(message));
	g_assert_cmpuint(edited_counter, ==, 1);
	g_assert_cmpuint(edited_at_counter, ==, 1);

	/* Now clear everything and verify it's empty. */
	edited_counter = 0;
	edited_at_counter = 0;

	purple_message_set_edited(message, FALSE);
	g_assert_false(purple_message_get_edited(message));
	g_assert_null(purple_message_get_edited_at(message));
	g_assert_cmpuint(edited_counter, ==, 1);
	g_assert_cmpuint(edited_at_counter, ==, 1);

	g_clear_object(&message);
}

static void
test_purple_message_edited_at_set_edited(void) {
	PurpleMessage *message = NULL;
	GDateTime *edited_at = NULL;
	GDateTime *edited_at1 = NULL;
	guint edited_counter = 0;
	guint edited_at_counter = 0;

	message = g_object_new(PURPLE_TYPE_MESSAGE, NULL);
	g_signal_connect(message, "notify::edited",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &edited_counter);
	g_signal_connect(message, "notify::edited-at",
	                 G_CALLBACK(test_purple_message_notify_counter_cb),
	                 &edited_at_counter);

	/* The default value for edited-at is NULL, so setting it to non-null
	 * should emit the signals and everything.
	 */
	edited_at = g_date_time_new_now_utc();
	purple_message_set_edited_at(message, edited_at);
	g_assert_true(purple_message_get_edited(message));
	edited_at1 = purple_message_get_edited_at(message);
	g_assert_nonnull(edited_at1);
	g_assert_true(g_date_time_equal(edited_at1, edited_at));
	g_assert_cmpuint(edited_counter, ==, 1);
	g_assert_cmpuint(edited_at_counter, ==, 1);

	/* Now clear everything and make sure it's all good. */
	edited_counter = 0;
	edited_at_counter = 0;
	purple_message_set_edited_at(message, NULL);
	g_assert_false(purple_message_get_edited(message));
	g_assert_null(purple_message_get_edited_at(message));
	g_assert_cmpuint(edited_counter, ==, 1);
	g_assert_cmpuint(edited_at_counter, ==, 1);

	g_clear_pointer(&edited_at, g_date_time_unref);
	g_clear_object(&message);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/message/properties",
	                test_purple_message_properties);

	g_test_add_func("/message/delivered-sets-delivered-at",
	                test_purple_message_delivered_set_delivered_at);
	g_test_add_func("/message/delivered-at-sets-delivered",
	                test_purple_message_delivered_at_set_delivered);

	g_test_add_func("/message/edited-sets-edited-at",
	                test_purple_message_edited_set_edited_at);
	g_test_add_func("/message/edited-at-sets-edited",
	                test_purple_message_edited_at_set_edited);

	return g_test_run();
}
