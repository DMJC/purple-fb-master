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
 * TestPurpleCredentialProvider Implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleCredentialProvider,
                     test_purple_credential_provider,
                     TEST_PURPLE, CREDENTIAL_PROVIDER,
                     PurpleCredentialProvider)

struct _TestPurpleCredentialProvider {
	PurpleCredentialProvider parent;
};

G_DEFINE_FINAL_TYPE(TestPurpleCredentialProvider,
                    test_purple_credential_provider,
                    PURPLE_TYPE_CREDENTIAL_PROVIDER)

static void
test_purple_credential_provider_read_password_async(PurpleCredentialProvider *p,
                                                    G_GNUC_UNUSED PurpleAccount *account,
                                                    GCancellable *cancellable,
                                                    GAsyncReadyCallback callback,
                                                    gpointer data)
{
	GTask *task = NULL;

	task = g_task_new(p, cancellable, callback, data);
	g_task_return_pointer(task, g_strdup("password"), g_free);
	g_clear_object(&task);
}

static char *
test_purple_credential_provider_read_password_finish(PurpleCredentialProvider *p,
                                                     GAsyncResult *result,
                                                     GError **error)
{
	g_return_val_if_fail(g_task_is_valid(result, p), NULL);

	return g_task_propagate_pointer(G_TASK(result), error);
}

static void
test_purple_credential_provider_write_password_async(PurpleCredentialProvider *p,
                                                     G_GNUC_UNUSED PurpleAccount *account,
                                                     G_GNUC_UNUSED const char *password,
                                                     GCancellable *cancellable,
                                                     GAsyncReadyCallback callback,
                                                     gpointer data)
{
	GTask *task = NULL;

	task = g_task_new(p, cancellable, callback, data);
	g_task_return_boolean(task, TRUE);
	g_clear_object(&task);
}

static gboolean
test_purple_credential_provider_write_password_finish(PurpleCredentialProvider *p,
                                                      GAsyncResult *result,
                                                      GError **error)
{
	g_return_val_if_fail(g_task_is_valid(result, p), FALSE);

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_credential_provider_clear_password_async(PurpleCredentialProvider *p,
                                                     G_GNUC_UNUSED PurpleAccount *account,
                                                     GCancellable *cancellable,
                                                     GAsyncReadyCallback callback,
                                                     gpointer data)
{
	GTask *task = NULL;

	task = g_task_new(p, cancellable, callback, data);
	g_task_return_boolean(task, TRUE);
	g_clear_object(&task);
}

static gboolean
test_purple_credential_provider_clear_password_finish(PurpleCredentialProvider *p,
                                                      GAsyncResult *result,
                                                      GError **error)
{
	g_return_val_if_fail(g_task_is_valid(result, p), FALSE);

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_credential_provider_init(G_GNUC_UNUSED TestPurpleCredentialProvider *provider) {
}

static void
test_purple_credential_provider_class_init(TestPurpleCredentialProviderClass *klass)
{
	PurpleCredentialProviderClass *provider_class = PURPLE_CREDENTIAL_PROVIDER_CLASS(klass);

	provider_class->read_password_async = test_purple_credential_provider_read_password_async;
	provider_class->read_password_finish = test_purple_credential_provider_read_password_finish;
	provider_class->write_password_async = test_purple_credential_provider_write_password_async;
	provider_class->write_password_finish = test_purple_credential_provider_write_password_finish;
	provider_class->clear_password_async = test_purple_credential_provider_clear_password_async;
	provider_class->clear_password_finish = test_purple_credential_provider_clear_password_finish;
}

static PurpleCredentialProvider *
test_purple_credential_provider_new(void) {
	return g_object_new(
		test_purple_credential_provider_get_type(),
		"id", "test-provider",
		"name", "Test Provider",
		NULL);
}

/******************************************************************************
 * Registration Tests
 *****************************************************************************/
static void
test_purple_credential_manager_registration(void) {
	PurpleCredentialManager *manager = NULL;
	PurpleCredentialProvider *provider = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);
	g_assert_true(PURPLE_IS_CREDENTIAL_MANAGER(manager));

	provider = test_purple_credential_provider_new();

	/* Register the first time cleanly. */
	result = purple_credential_manager_register(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	/* Register again and verify the error. */
	result = purple_credential_manager_register(manager, provider, &error);
	g_assert_error(error, PURPLE_CREDENTIAL_MANAGER_DOMAIN, 0);
	g_clear_error(&error);

	g_assert_false(result);

	/* Unregister the provider. */
	result = purple_credential_manager_unregister(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	/* Unregister the provider again and verify the error. */
	result = purple_credential_manager_unregister(manager, provider, &error);
	g_assert_error(error, PURPLE_CREDENTIAL_MANAGER_DOMAIN, 0);
	g_clear_error(&error);

	g_assert_false(result);

	/* Final clean ups. */
	g_clear_object(&provider);
	g_clear_object(&manager);
}

/******************************************************************************
 * Set Active Tests
 *****************************************************************************/
static void
test_purple_credential_manager_set_active_null(void) {
	PurpleCredentialManager *manager = NULL;
	GError *error = NULL;
	gboolean ret = FALSE;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);

	ret = purple_credential_manager_set_active(manager, NULL, &error);

	g_assert_no_error(error);
	g_assert_true(ret);

	g_clear_object(&manager);
}

static void
test_purple_credential_manager_set_active_non_existent(void) {
	PurpleCredentialManager *manager = NULL;
	GError *error = NULL;
	gboolean ret = FALSE;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);

	ret = purple_credential_manager_set_active(manager, "foo", &error);

	g_assert_error(error, PURPLE_CREDENTIAL_MANAGER_DOMAIN, 0);
	g_clear_error(&error);

	g_assert_false(ret);

	g_clear_object(&manager);
}

static void
test_purple_credential_manager_set_active_normal(void) {
	PurpleCredentialManager *manager = NULL;
	PurpleCredentialProvider *provider = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);

	/* Create the provider and register it in the manager. */
	provider = test_purple_credential_provider_new();
	result = purple_credential_manager_register(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	/* Set the provider as active and verify it was successful. */
	result = purple_credential_manager_set_active(manager, "test-provider",
	                                              &error);
	g_assert_no_error(error);
	g_assert_true(result);

	/* Verify that unregistering the provider fails. */
	result = purple_credential_manager_unregister(manager, provider, &error);
	g_assert_error(error, PURPLE_CREDENTIAL_MANAGER_DOMAIN, 0);
	g_clear_error(&error);
	g_assert_false(result);

	/* Now unset the active provider. */
	result = purple_credential_manager_set_active(manager, NULL, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	/* Finally unregister the provider now that it's no longer active. */
	result = purple_credential_manager_unregister(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	/* And our final cleanup. */
	g_clear_object(&provider);
	g_clear_object(&manager);
}

/******************************************************************************
 * No Provider Tests
 *****************************************************************************/
static void
test_purple_credential_manager_no_provider_read_password_cb(GObject *obj,
                                                            GAsyncResult *res,
                                                            G_GNUC_UNUSED gpointer data)
{
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	GError *error = NULL;
	char *password = NULL;

	password = purple_credential_manager_read_password_finish(manager, res,
	                                                          &error);
	g_assert_error(error, PURPLE_CREDENTIAL_MANAGER_DOMAIN, 0);
	g_clear_error(&error);
	g_assert_null(password);
}

static void
test_purple_credential_manager_no_provider_read_password_async(void) {
	PurpleAccount *account = NULL;
	PurpleCredentialManager *manager = NULL;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);

	account = purple_account_new("test", "test");
	purple_account_set_remember_password(account, TRUE);

	purple_credential_manager_read_password_async(manager, account, NULL,
	                                              test_purple_credential_manager_no_provider_read_password_cb,
	                                              NULL);
	g_main_context_iteration(NULL, FALSE);

	g_assert_finalize_object(manager);
	g_clear_object(&account);
}

static void
test_purple_credential_manager_no_provider_write_password_cb(GObject *obj,
                                                             GAsyncResult *res,
                                                             G_GNUC_UNUSED gpointer data)
{
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	GError *error = NULL;
	gboolean result = FALSE;

	result = purple_credential_manager_write_password_finish(manager, res,
	                                                         &error);
	g_assert_error(error, PURPLE_CREDENTIAL_MANAGER_DOMAIN, 0);
	g_clear_error(&error);

	g_assert_false(result);
}

static void
test_purple_credential_manager_no_provider_write_password_async(void) {
	PurpleAccount *account = NULL;
	PurpleCredentialManager *manager = NULL;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);

	account = purple_account_new("test", "test");
	purple_account_set_remember_password(account, TRUE);

	purple_credential_manager_write_password_async(manager, account, NULL,
	                                               NULL,
	                                               test_purple_credential_manager_no_provider_write_password_cb,
	                                               NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_finalize_object(manager);
	g_clear_object(&account);
}

static void
test_purple_credential_manager_no_provider_clear_password_cb(GObject *obj,
                                                             GAsyncResult *res,
                                                             G_GNUC_UNUSED gpointer data)
{
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	GError *error = NULL;
	gboolean result = FALSE;

	result = purple_credential_manager_clear_password_finish(manager, res,
	                                                         &error);
	g_assert_error(error, PURPLE_CREDENTIAL_MANAGER_DOMAIN, 0);
	g_clear_error(&error);

	g_assert_false(result);
}

static void
test_purple_credential_manager_no_provider_clear_password_async(void) {
	PurpleAccount *account = NULL;
	PurpleCredentialManager *manager = NULL;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);

	account = purple_account_new("test", "test");
	purple_account_set_remember_password(account, TRUE);

	purple_credential_manager_clear_password_async(manager, account, NULL,
	                                               test_purple_credential_manager_no_provider_clear_password_cb,
	                                               NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_finalize_object(manager);
	g_clear_object(&account);
}

/******************************************************************************
 * Provider Tests
 *****************************************************************************/
static void
test_purple_credential_manager_read_password_cb(GObject *obj,
                                                GAsyncResult *res,
                                                G_GNUC_UNUSED gpointer data)
{
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	GError *error = NULL;
	char *password = NULL;

	password = purple_credential_manager_read_password_finish(manager, res,
	                                                          &error);
	g_assert_no_error(error);
	g_assert_cmpstr(password, ==, "password");
	g_free(password);
}

static void
test_purple_credential_manager_read_password_async(void) {
	PurpleAccount *account = NULL;
	PurpleCredentialManager *manager = NULL;
	PurpleCredentialProvider *provider = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);
	provider = test_purple_credential_provider_new();

	result = purple_credential_manager_register(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	result = purple_credential_manager_set_active(manager, "test-provider",
	                                              &error);
	g_assert_no_error(error);
	g_assert_true(result);

	account = purple_account_new("test", "test");
	purple_account_set_remember_password(account, TRUE);

	purple_credential_manager_read_password_async(manager, account, NULL,
	                                              test_purple_credential_manager_read_password_cb,
	                                              NULL);

	g_main_context_iteration(NULL, FALSE);

	result = purple_credential_manager_set_active(manager, NULL, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	result = purple_credential_manager_unregister(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	g_assert_finalize_object(provider);
	g_assert_finalize_object(manager);
	g_clear_object(&account);
}

static void
test_purple_credential_manager_write_password_cb(GObject *obj,
                                                 GAsyncResult *res,
                                                 G_GNUC_UNUSED gpointer data)
{
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	GError *error = NULL;
	gboolean result = FALSE;

	result = purple_credential_manager_write_password_finish(manager, res,
	                                                         &error);

	g_assert_no_error(error);
	g_assert_true(result);
}

static void
test_purple_credential_manager_write_password_async(void) {
	PurpleAccount *account = NULL;
	PurpleCredentialManager *manager = NULL;
	PurpleCredentialProvider *provider = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);
	provider = test_purple_credential_provider_new();

	result = purple_credential_manager_register(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	result = purple_credential_manager_set_active(manager, "test-provider",
	                                              &error);
	g_assert_no_error(error);
	g_assert_true(result);

	account = purple_account_new("test", "test");
	purple_account_set_remember_password(account, TRUE);

	purple_credential_manager_write_password_async(manager, account, NULL,
	                                               NULL,
	                                               test_purple_credential_manager_write_password_cb,
	                                               NULL);

	g_main_context_iteration(NULL, FALSE);

	result = purple_credential_manager_set_active(manager, NULL, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	result = purple_credential_manager_unregister(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	g_assert_finalize_object(provider);
	g_assert_finalize_object(manager);
	g_clear_object(&account);
}

static void
test_purple_credential_manager_clear_password_cb(GObject *obj,
                                                 GAsyncResult *res,
                                                 G_GNUC_UNUSED gpointer data)
{
	PurpleCredentialManager *manager = PURPLE_CREDENTIAL_MANAGER(obj);
	GError *error = NULL;
	gboolean result = FALSE;

	result = purple_credential_manager_clear_password_finish(manager, res,
	                                                         &error);
	g_assert_no_error(error);
	g_assert_true(result);
}

static void
test_purple_credential_manager_clear_password_async(void) {
	PurpleAccount *account = NULL;
	PurpleCredentialManager *manager = NULL;
	PurpleCredentialProvider *provider = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	manager = g_object_new(PURPLE_TYPE_CREDENTIAL_MANAGER, NULL);
	provider = test_purple_credential_provider_new();

	result = purple_credential_manager_register(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	result = purple_credential_manager_set_active(manager, "test-provider",
	                                              &error);
	g_assert_no_error(error);
	g_assert_true(result);

	account = purple_account_new("test", "test");
	purple_account_set_remember_password(account, TRUE);

	purple_credential_manager_clear_password_async(manager, account, NULL,
	                                               test_purple_credential_manager_clear_password_cb,
	                                               NULL);

	g_main_context_iteration(NULL, FALSE);

	result = purple_credential_manager_set_active(manager, NULL, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	result = purple_credential_manager_unregister(manager, provider, &error);
	g_assert_no_error(error);
	g_assert_true(result);

	g_assert_finalize_object(provider);
	g_assert_finalize_object(manager);
	g_clear_object(&account);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/credential-manager/registration",
	                test_purple_credential_manager_registration);
	g_test_add_func("/credential-manager/set-active/null",
	                test_purple_credential_manager_set_active_null);
	g_test_add_func("/credential-manager/set-active/non-existent",
	                test_purple_credential_manager_set_active_non_existent);
	g_test_add_func("/credential-manager/set-active/normal",
	                test_purple_credential_manager_set_active_normal);

	g_test_add_func("/credential-manager/no-provider/read-password-async",
	                test_purple_credential_manager_no_provider_read_password_async);
	g_test_add_func("/credential-manager/no-provider/write-password-async",
	                test_purple_credential_manager_no_provider_write_password_async);
	g_test_add_func("/credential-manager/no-provider/clear-password-async",
	                test_purple_credential_manager_no_provider_clear_password_async);

	g_test_add_func("/credential-manager/provider/read-password-async",
	                test_purple_credential_manager_read_password_async);
	g_test_add_func("/credential-manager/provider/write-password-async",
	                test_purple_credential_manager_write_password_async);
	g_test_add_func("/credential-manager/provider/clear-password-async",
	                test_purple_credential_manager_clear_password_async);

	return g_test_run();
}
