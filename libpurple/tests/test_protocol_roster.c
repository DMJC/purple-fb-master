/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include <purple.h>

#define TEST_PURPLE_PROTOCOL_ROSTER_DOMAIN \
	(g_quark_from_static_string("test-protocol-roster"))

/******************************************************************************
 * TestProtocolRosterEmpty implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleProtocolRosterEmpty,
                     test_purple_protocol_roster_empty,
                     TEST_PURPLE, PROTOCOL_ROSTER_EMPTY, PurpleProtocol)

struct _TestPurpleProtocolRosterEmpty {
	PurpleProtocol parent;
};

static void
test_purple_protocol_roster_empty_iface_init(G_GNUC_UNUSED PurpleProtocolRosterInterface *iface)
{
}

G_DEFINE_FINAL_TYPE_WITH_CODE(TestPurpleProtocolRosterEmpty,
                              test_purple_protocol_roster_empty,
                              PURPLE_TYPE_PROTOCOL,
                              G_IMPLEMENT_INTERFACE(PURPLE_TYPE_PROTOCOL_ROSTER,
                                                    test_purple_protocol_roster_empty_iface_init))

static void
test_purple_protocol_roster_empty_init(G_GNUC_UNUSED TestPurpleProtocolRosterEmpty *empty)
{
}

static void
test_purple_protocol_roster_empty_class_init(G_GNUC_UNUSED TestPurpleProtocolRosterEmptyClass *klass)
{
}

/******************************************************************************
 * TestProtocolRosterEmpty Tests
 *****************************************************************************/
static void
test_purple_protocol_roster_empty_add_cb(GObject *source, GAsyncResult *result,
                                         G_GNUC_UNUSED gpointer data)
{
	GError *error = NULL;
	gboolean success = FALSE;

	g_assert_true(PURPLE_IS_PROTOCOL_ROSTER(source));
	g_assert_true(G_IS_ASYNC_RESULT(result));

	success = purple_protocol_roster_add_finish(PURPLE_PROTOCOL_ROSTER(source),
	                                            result, &error);
	g_assert_error(error, PURPLE_PROTOCOL_ROSTER_ERROR, 0);
	g_clear_error(&error);
	g_assert_false(success);
}

static void
test_purple_protocol_roster_empty_add(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleProtocolRoster *roster = NULL;

	roster = g_object_new(test_purple_protocol_roster_empty_get_type(),
	                      NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_add_async(roster, account, contact, NULL,
	                                 test_purple_protocol_roster_empty_add_cb,
	                                 NULL);

	g_main_context_iteration(NULL, FALSE);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

static void
test_purple_protocol_roster_empty_update_cb(GObject *source,
                                            GAsyncResult *result,
                                            G_GNUC_UNUSED gpointer data)
{
	GError *error = NULL;
	gboolean success = FALSE;

	g_assert_true(PURPLE_IS_PROTOCOL_ROSTER(source));
	g_assert_true(G_IS_ASYNC_RESULT(result));

	success = purple_protocol_roster_update_finish(PURPLE_PROTOCOL_ROSTER(source),
	                                               result, &error);
	g_assert_error(error, PURPLE_PROTOCOL_ROSTER_ERROR, 0);
	g_clear_error(&error);
	g_assert_false(success);
}

static void
test_purple_protocol_roster_empty_update(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleProtocolRoster *roster = NULL;

	roster = g_object_new(test_purple_protocol_roster_empty_get_type(),
	                      NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_update_async(roster, account, contact, NULL,
	                                    test_purple_protocol_roster_empty_update_cb,
	                                    NULL);

	g_main_context_iteration(NULL, FALSE);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

static void
test_purple_protocol_roster_empty_remove_cb(GObject *source,
                                            GAsyncResult *result,
                                            G_GNUC_UNUSED gpointer data)
{
	GError *error = NULL;
	gboolean success = FALSE;

	g_assert_true(PURPLE_IS_PROTOCOL_ROSTER(source));
	g_assert_true(G_IS_ASYNC_RESULT(result));

	success = purple_protocol_roster_remove_finish(PURPLE_PROTOCOL_ROSTER(source),
	                                               result, &error);
	g_assert_error(error, PURPLE_PROTOCOL_ROSTER_ERROR, 0);
	g_clear_error(&error);
	g_assert_false(success);
}

static void
test_purple_protocol_roster_empty_remove(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleProtocolRoster *roster = NULL;

	roster = g_object_new(test_purple_protocol_roster_empty_get_type(),
	                      NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_remove_async(roster, account, contact, NULL,
	                                    test_purple_protocol_roster_empty_remove_cb,
	                                    NULL);

	g_main_context_iteration(NULL, FALSE);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

/******************************************************************************
 * TestProtocolRoster implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleProtocolRoster, test_purple_protocol_roster,
                     TEST_PURPLE, PROTOCOL_ROSTER, PurpleProtocol)

struct _TestPurpleProtocolRoster {
	PurpleProtocol parent;

	gboolean should_error;

	gboolean add_async;
	gboolean add_finish;
	gboolean update_async;
	gboolean update_finish;
	gboolean remove_async;
	gboolean remove_finish;
};

static void
test_purple_protocol_roster_add_async(PurpleProtocolRoster *r,
                                      G_GNUC_UNUSED PurpleAccount *account,
                                      G_GNUC_UNUSED PurpleContact *contact,
                                      GCancellable *cancellable,
                                      GAsyncReadyCallback callback,
                                      gpointer data)
{
	TestPurpleProtocolRoster *roster = TEST_PURPLE_PROTOCOL_ROSTER(r);
	GTask *task = NULL;

	roster->add_async = TRUE;

	task = g_task_new(r, cancellable, callback, data);
	if(roster->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_ROSTER_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_roster_add_finish(PurpleProtocolRoster *r,
                                       GAsyncResult *result, GError **error)
{
	TestPurpleProtocolRoster *roster = TEST_PURPLE_PROTOCOL_ROSTER(r);

	roster->add_finish = TRUE;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_roster_update_async(PurpleProtocolRoster *r,
                                         G_GNUC_UNUSED PurpleAccount *account,
                                         G_GNUC_UNUSED PurpleContact *contact,
                                         GCancellable *cancellable,
                                         GAsyncReadyCallback callback,
                                         gpointer data)
{
	TestPurpleProtocolRoster *roster = TEST_PURPLE_PROTOCOL_ROSTER(r);
	GTask *task = NULL;

	roster->update_async = TRUE;

	task = g_task_new(r, cancellable, callback, data);
	if(roster->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_ROSTER_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_roster_update_finish(PurpleProtocolRoster *r,
                                          GAsyncResult *result, GError **error)
{
	TestPurpleProtocolRoster *roster = TEST_PURPLE_PROTOCOL_ROSTER(r);

	roster->update_finish = TRUE;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_roster_remove_async(PurpleProtocolRoster *r,
                                         G_GNUC_UNUSED PurpleAccount *account,
                                         G_GNUC_UNUSED PurpleContact *contact,
                                         GCancellable *cancellable,
                                         GAsyncReadyCallback callback,
                                         gpointer data)
{
	TestPurpleProtocolRoster *roster = TEST_PURPLE_PROTOCOL_ROSTER(r);
	GTask *task = NULL;

	roster->remove_async = TRUE;

	task = g_task_new(r, cancellable, callback, data);
	if(roster->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_ROSTER_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_roster_remove_finish(PurpleProtocolRoster *r,
                                          GAsyncResult *result, GError **error)
{
	TestPurpleProtocolRoster *roster = TEST_PURPLE_PROTOCOL_ROSTER(r);

	roster->remove_finish = TRUE;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_roster_iface_init(PurpleProtocolRosterInterface *iface) {
	iface->add_async = test_purple_protocol_roster_add_async;
	iface->add_finish = test_purple_protocol_roster_add_finish;
	iface->update_async = test_purple_protocol_roster_update_async;
	iface->update_finish = test_purple_protocol_roster_update_finish;
	iface->remove_async = test_purple_protocol_roster_remove_async;
	iface->remove_finish = test_purple_protocol_roster_remove_finish;
}

G_DEFINE_FINAL_TYPE_WITH_CODE(TestPurpleProtocolRoster,
                              test_purple_protocol_roster,
                              PURPLE_TYPE_PROTOCOL,
                              G_IMPLEMENT_INTERFACE(PURPLE_TYPE_PROTOCOL_ROSTER,
                                                    test_purple_protocol_roster_iface_init))

static void
test_purple_protocol_roster_init(G_GNUC_UNUSED TestPurpleProtocolRoster *roster)
{
}

static void
test_purple_protocol_roster_class_init(G_GNUC_UNUSED TestPurpleProtocolRosterClass *klass)
{
}

/******************************************************************************
 * TestProtocolRoster Add test
 *****************************************************************************/
static void
test_purple_protocol_roster_add_cb(GObject *obj, GAsyncResult *res,
                                   G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolRoster *test_roster = TEST_PURPLE_PROTOCOL_ROSTER(obj);
	PurpleProtocolRoster *roster = PURPLE_PROTOCOL_ROSTER(obj);
	GError *error = NULL;
	gboolean result = FALSE;

	result = purple_protocol_roster_add_finish(roster, res, &error);
	if(test_roster->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_ROSTER_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}
}

static void
test_purple_protocol_roster_add(void) {
	TestPurpleProtocolRoster *roster = NULL;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	roster = g_object_new(test_purple_protocol_roster_get_type(), NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_add_async(PURPLE_PROTOCOL_ROSTER(roster), account,
	                                 contact, NULL,
	                                 test_purple_protocol_roster_add_cb,
	                                 NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_true(roster->add_async);
	g_assert_true(roster->add_finish);
	g_assert_false(roster->update_async);
	g_assert_false(roster->update_finish);
	g_assert_false(roster->remove_async);
	g_assert_false(roster->remove_finish);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

static void
test_purple_protocol_roster_add_error(void) {
	TestPurpleProtocolRoster *roster = NULL;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	roster = g_object_new(test_purple_protocol_roster_get_type(), NULL);
	roster->should_error = TRUE;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_add_async(PURPLE_PROTOCOL_ROSTER(roster), account,
	                                 contact, NULL,
	                                 test_purple_protocol_roster_add_cb,
	                                 NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_true(roster->add_async);
	g_assert_true(roster->add_finish);
	g_assert_false(roster->update_async);
	g_assert_false(roster->update_finish);
	g_assert_false(roster->remove_async);
	g_assert_false(roster->remove_finish);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

/******************************************************************************
 * TestProtocolRoster Update test
 *****************************************************************************/
static void
test_purple_protocol_roster_update_cb(GObject *obj, GAsyncResult *res,
                                      G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolRoster *test_roster = TEST_PURPLE_PROTOCOL_ROSTER(obj);
	PurpleProtocolRoster *roster = PURPLE_PROTOCOL_ROSTER(obj);
	GError *error = NULL;
	gboolean result = FALSE;

	result = purple_protocol_roster_update_finish(roster, res, &error);
	if(test_roster->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_ROSTER_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}
}

static void
test_purple_protocol_roster_update(void) {
	TestPurpleProtocolRoster *roster = NULL;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	roster = g_object_new(test_purple_protocol_roster_get_type(), NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_update_async(PURPLE_PROTOCOL_ROSTER(roster),
	                                    account, contact, NULL,
	                                    test_purple_protocol_roster_update_cb,
	                                    NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_false(roster->add_async);
	g_assert_false(roster->add_finish);
	g_assert_true(roster->update_async);
	g_assert_true(roster->update_finish);
	g_assert_false(roster->remove_async);
	g_assert_false(roster->remove_finish);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

static void
test_purple_protocol_roster_update_error(void) {
	TestPurpleProtocolRoster *roster = NULL;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	roster = g_object_new(test_purple_protocol_roster_get_type(), NULL);
	roster->should_error = TRUE;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_update_async(PURPLE_PROTOCOL_ROSTER(roster),
	                                    account, contact, NULL,
	                                    test_purple_protocol_roster_update_cb,
	                                    NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_false(roster->add_async);
	g_assert_false(roster->add_finish);
	g_assert_true(roster->update_async);
	g_assert_true(roster->update_finish);
	g_assert_false(roster->remove_async);
	g_assert_false(roster->remove_finish);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

/******************************************************************************
 * TestProtocolRoster Remove test
 *****************************************************************************/
static void
test_purple_protocol_roster_remove_cb(GObject *obj, GAsyncResult *res,
                                      G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolRoster *test_roster = TEST_PURPLE_PROTOCOL_ROSTER(obj);
	PurpleProtocolRoster *roster = PURPLE_PROTOCOL_ROSTER(obj);
	GError *error = NULL;
	gboolean result = FALSE;

	result = purple_protocol_roster_remove_finish(roster, res, &error);
	if(test_roster->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_ROSTER_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}
}

static void
test_purple_protocol_roster_remove(void) {
	TestPurpleProtocolRoster *roster = NULL;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	roster = g_object_new(test_purple_protocol_roster_get_type(), NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_remove_async(PURPLE_PROTOCOL_ROSTER(roster),
	                                    account, contact, NULL,
	                                    test_purple_protocol_roster_remove_cb,
	                                    contact);

	g_main_context_iteration(NULL, FALSE);

	g_assert_false(roster->add_async);
	g_assert_false(roster->add_finish);
	g_assert_false(roster->update_async);
	g_assert_false(roster->update_finish);
	g_assert_true(roster->remove_async);
	g_assert_true(roster->remove_finish);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

static void
test_purple_protocol_roster_remove_error(void) {
	TestPurpleProtocolRoster *roster = NULL;
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;

	roster = g_object_new(test_purple_protocol_roster_get_type(), NULL);
	roster->should_error = TRUE;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	purple_protocol_roster_remove_async(PURPLE_PROTOCOL_ROSTER(roster),
	                                    account, contact, NULL,
	                                    test_purple_protocol_roster_remove_cb,
	                                    contact);

	g_main_context_iteration(NULL, FALSE);

	g_assert_false(roster->add_async);
	g_assert_false(roster->add_finish);
	g_assert_false(roster->update_async);
	g_assert_false(roster->update_finish);
	g_assert_true(roster->remove_async);
	g_assert_true(roster->remove_finish);

	g_clear_object(&account);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(roster);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/protocol-roster/empty/add",
	                test_purple_protocol_roster_empty_add);
	g_test_add_func("/protocol-roster/empty/update",
	                test_purple_protocol_roster_empty_update);
	g_test_add_func("/protocol-roster/empty/remove",
	                test_purple_protocol_roster_empty_remove);

	g_test_add_func("/protocol-roster/add", test_purple_protocol_roster_add);
	g_test_add_func("/protocol-roster/add-error",
	                test_purple_protocol_roster_add_error);
	g_test_add_func("/protocol-roster/update",
	                test_purple_protocol_roster_update);
	g_test_add_func("/protocol-roster/update-error",
	                test_purple_protocol_roster_update_error);
	g_test_add_func("/protocol-roster/remove",
	                test_purple_protocol_roster_remove);
	g_test_add_func("/protocol-roster/remove-error",
	                test_purple_protocol_roster_remove_error);

	return g_test_run();
}
