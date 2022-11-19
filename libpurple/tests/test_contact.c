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

#include "test_ui.h"

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_contact_new(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, "id");

	g_assert_true(purple_contact_get_account(contact) == account);
	g_assert_cmpstr(purple_contact_get_id(contact), ==, "id");

	g_clear_object(&contact);
	g_clear_object(&account);
}

static void
test_purple_contact_properties(void) {
	PurpleAccount *account = NULL;
	PurpleAccount *account1 = NULL;
	PurpleContact *contact = NULL;
	PurpleContactPermission permission;
	PurplePerson *person = NULL;
	PurplePerson *person1 = NULL;
	PurplePresence *presence1 = NULL;
	PurpleTags *tags = NULL;
	GdkPixbuf *avatar = NULL;
	GdkPixbuf *avatar1 = NULL;
	gchar *id = NULL;
	gchar *username = NULL;
	gchar *display_name = NULL;
	gchar *alias = NULL;

	account = purple_account_new("test", "test");
	avatar = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);
	person = purple_person_new();

	/* Use g_object_new so we can test setting properties by name. All of them
	 * call the setter methods, so by doing it this way we exercise more of the
	 * code.
	 */
	contact = g_object_new(
		PURPLE_TYPE_CONTACT,
		"account", account,
		"id", "id1",
		"username", "username",
		"display-name", "display-name",
		"alias", "alias",
		"avatar", avatar,
		"person", person,
		"permission", PURPLE_CONTACT_PERMISSION_ALLOW,
		NULL);

	/* Now use g_object_get to read all of the properties. */
	g_object_get(contact,
		"id", &id,
		"account", &account1,
		"username", &username,
		"display-name", &display_name,
		"alias", &alias,
		"avatar", &avatar1,
		"presence", &presence1,
		"tags", &tags,
		"person", &person1,
		"permission", &permission,
		NULL);

	/* Compare all the things. */
	g_assert_cmpstr(id, ==, "id1");
	g_assert_true(account1 == account);
	g_assert_cmpstr(username, ==, "username");
	g_assert_cmpstr(display_name, ==, "display-name");
	g_assert_cmpstr(alias, ==, "alias");
	g_assert_true(avatar1 == avatar);
	g_assert_nonnull(presence1);
	g_assert_nonnull(tags);
	g_assert_true(person1 == person);
	g_assert_true(permission == PURPLE_CONTACT_PERMISSION_ALLOW);

	/* Free/unref all the things. */
	g_clear_pointer(&id, g_free);
	g_clear_object(&account1);
	g_clear_pointer(&username, g_free);
	g_clear_pointer(&display_name, g_free);
	g_clear_pointer(&alias, g_free);
	g_clear_object(&avatar1);
	g_clear_object(&presence1);
	g_clear_object(&tags);
	g_clear_object(&person);
	g_clear_object(&person1);

	g_clear_object(&avatar);
	g_clear_object(&contact);
	g_clear_object(&account);
}

/******************************************************************************
 * get_name_for_display tests
 *****************************************************************************/
static void
test_purple_contact_get_name_for_display_person_with_alias(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurplePerson *person = NULL;
	const char *alias = NULL;

	account = purple_account_new("test", "test");

	person = purple_person_new();
	purple_person_set_alias(person, "this is the alias");

	contact = purple_contact_new(account, NULL);
	purple_contact_set_person(contact, person);

	alias = purple_contact_get_name_for_display(contact);
	g_assert_cmpstr(alias, ==, "this is the alias");

	g_clear_object(&account);
	g_clear_object(&contact);
	g_clear_object(&person);
}

static void
test_purple_contact_get_name_for_display_contact_with_alias(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurplePerson *person = NULL;
	const char *alias = NULL;

	account = purple_account_new("test", "test");
	person = purple_person_new();

	contact = purple_contact_new(account, NULL);
	purple_contact_set_person(contact, person);

	purple_contact_set_alias(contact, "this is the alias");

	alias = purple_contact_get_name_for_display(contact);
	g_assert_cmpstr(alias, ==, "this is the alias");

	g_clear_object(&account);
	g_clear_object(&contact);
	g_clear_object(&person);
}

static void
test_purple_contact_get_name_for_display_contact_with_display_name(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurplePerson *person = NULL;
	const char *alias = NULL;

	account = purple_account_new("test", "test");
	person = purple_person_new();

	contact = purple_contact_new(account, NULL);
	purple_contact_set_person(contact, person);

	purple_contact_set_display_name(contact, "this is the display name");

	alias = purple_contact_get_name_for_display(contact);
	g_assert_cmpstr(alias, ==, "this is the display name");

	g_clear_object(&account);
	g_clear_object(&contact);
	g_clear_object(&person);
}

static void
test_purple_contact_get_name_for_display_username_fallback(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurplePerson *person = NULL;
	const char *alias = NULL;

	account = purple_account_new("test", "test");
	person = purple_person_new();

	contact = purple_contact_new(account, NULL);
	purple_contact_set_username(contact, "username");
	purple_contact_set_person(contact, person);

	alias = purple_contact_get_name_for_display(contact);
	g_assert_cmpstr(alias, ==, "username");

	g_clear_object(&account);
	g_clear_object(&contact);
	g_clear_object(&person);
}

static void
test_purple_contact_get_name_for_display_id_fallback(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurplePerson *person = NULL;
	const char *alias = NULL;

	account = purple_account_new("test", "test");
	person = purple_person_new();

	contact = purple_contact_new(account, "id");
	purple_contact_set_person(contact, person);

	alias = purple_contact_get_name_for_display(contact);
	g_assert_cmpstr(alias, ==, "id");

	g_clear_object(&account);
	g_clear_object(&contact);
	g_clear_object(&person);
}

/******************************************************************************
 * purple_contact_compare tests
 *****************************************************************************/
static void
test_purple_contact_compare_not_null__null(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	g_assert_cmpint(purple_contact_compare(contact, NULL), ==, -1);

	g_clear_object(&account);
	g_clear_object(&contact);
}

static void
test_purple_contact_compare_null__not_null(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	g_assert_cmpint(purple_contact_compare(NULL, contact), ==, 1);

	g_clear_object(&account);
	g_clear_object(&contact);
}

static void
test_purple_contact_compare_null__null(void) {
	g_assert_cmpint(purple_contact_compare(NULL, NULL), ==, 0);
}

static void
test_purple_contact_compare_person__no_person(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact_a = NULL;
	PurpleContact *contact_b = NULL;
	PurplePerson *person = NULL;

	account = purple_account_new("test", "test");

	contact_a = purple_contact_new(account, NULL);
	person = purple_person_new();
	purple_contact_set_person(contact_a, person);

	contact_b = purple_contact_new(account, NULL);

	g_assert_cmpint(purple_contact_compare(contact_a, contact_b), ==, -1);

	g_clear_object(&account);
	g_clear_object(&contact_a);
	g_clear_object(&contact_b);
	g_clear_object(&person);
}

static void
test_purple_contact_compare_no_person__person(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact_a = NULL;
	PurpleContact *contact_b = NULL;
	PurplePerson *person = NULL;

	account = purple_account_new("test", "test");

	contact_a = purple_contact_new(account, NULL);

	contact_b = purple_contact_new(account, NULL);
	person = purple_person_new();
	purple_contact_set_person(contact_b, person);

	g_assert_cmpint(purple_contact_compare(contact_a, contact_b), ==, 1);

	g_clear_object(&account);
	g_clear_object(&contact_a);
	g_clear_object(&contact_b);
	g_clear_object(&person);
}

static void
test_purple_contact_compare_name__name(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact_a = NULL;
	PurpleContact *contact_b = NULL;

	account = purple_account_new("test", "test");

	contact_a = purple_contact_new(account, NULL);
	purple_contact_set_username(contact_a, "aaa");

	contact_b = purple_contact_new(account, NULL);
	purple_contact_set_username(contact_b, "zzz");

	g_assert_cmpint(purple_contact_compare(contact_a, contact_b), ==, -1);
	g_assert_cmpint(purple_contact_compare(contact_b, contact_a), ==, 1);

	purple_contact_set_username(contact_b, "aaa");
	g_assert_cmpint(purple_contact_compare(contact_b, contact_a), ==, 0);

	g_clear_object(&account);
	g_clear_object(&contact_a);
	g_clear_object(&contact_b);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	test_ui_purple_init();

	g_test_add_func("/contact/new",
	                test_purple_contact_new);
	g_test_add_func("/contact/properties",
	                test_purple_contact_properties);

	g_test_add_func("/contact/get_name_for_display/person_with_alias",
	                test_purple_contact_get_name_for_display_person_with_alias);
	g_test_add_func("/contact/get_name_for_display/contact_with_alias",
	                test_purple_contact_get_name_for_display_contact_with_alias);
	g_test_add_func("/contact/get_name_for_display/contact_with_display_name",
	                test_purple_contact_get_name_for_display_contact_with_display_name);
	g_test_add_func("/contact/get_name_for_display/username_fallback",
	                test_purple_contact_get_name_for_display_username_fallback);
	g_test_add_func("/contact/get_name_for_display/id_fallback",
	                test_purple_contact_get_name_for_display_id_fallback);

	g_test_add_func("/contact/compare/not_null__null",
	                test_purple_contact_compare_not_null__null);
	g_test_add_func("/contact/compare/null__not_null",
	                test_purple_contact_compare_null__not_null);
	g_test_add_func("/contact/compare/null__null",
	                test_purple_contact_compare_null__null);
	g_test_add_func("/contact/compare/person__no_person",
	                test_purple_contact_compare_person__no_person);
	g_test_add_func("/contact/compare/no_person__person",
	                test_purple_contact_compare_no_person__person);
	g_test_add_func("/contact/compare/name__name",
	                test_purple_contact_compare_name__name);

	return g_test_run();
}
