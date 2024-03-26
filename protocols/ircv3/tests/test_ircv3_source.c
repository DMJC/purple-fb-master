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

#include "../purpleircv3source.h"

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_ircv3_source_parse_return_address_required(void) {
	if(g_test_subprocess()) {
		purple_ircv3_source_parse("pidgy", NULL, NULL, NULL);
	}

	g_test_trap_subprocess(NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
	g_test_trap_assert_stderr("*nick != NULL || user != NULL || host != NULL' failed*");
}

static void
test_ircv3_source_parse_nick(void) {
	char *nick = NULL;
	char *user = NULL;
	char *host = NULL;

	/* Once to make sure that user and host are null. */
	purple_ircv3_source_parse("pidgy", &nick, &user, &host);
	g_assert_cmpstr(nick, ==, "pidgy");
	g_clear_pointer(&nick, g_free);

	g_assert_null(user);
	g_assert_null(host);

	/* Again to make sure it works without user and host. */
	purple_ircv3_source_parse("pidgy", &nick, NULL, NULL);
	g_assert_cmpstr(nick, ==, "pidgy");
	g_clear_pointer(&nick, g_free);
}

static void
test_ircv3_source_parse_user(void) {
	char *nick = NULL;
	char *user = NULL;
	char *host = NULL;

	/* Once to make sure host is null. */
	purple_ircv3_source_parse("pidgy!~u", &nick, &user, &host);
	g_assert_cmpstr(nick, ==, "pidgy");
	g_clear_pointer(&nick, g_free);

	g_assert_cmpstr(user, ==, "~u");
	g_clear_pointer(&user, g_free);

	g_assert_null(host);

	/* Again to make sure nick and host aren't required. */
	purple_ircv3_source_parse("pidgy!~u", NULL, &user, NULL);
	g_assert_cmpstr(user, ==, "~u");
	g_clear_pointer(&user, g_free);
}

static void
test_ircv3_source_parse_host(void) {
	char *nick = NULL;
	char *user = NULL;
	char *host = NULL;

	/* Once to make sure everything works. */
	purple_ircv3_source_parse("pidgy!~u@53unc8n42i868.irc", &nick, &user,
	                          &host);
	g_assert_cmpstr(nick, ==, "pidgy");
	g_clear_pointer(&nick, g_free);

	g_assert_cmpstr(user, ==, "~u");
	g_clear_pointer(&user, g_free);

	g_assert_cmpstr(host, ==, "53unc8n42i868.irc");
	g_clear_pointer(&host, g_free);

	/* Again to make sure nick and host aren't required. */
	purple_ircv3_source_parse("pidgy!~u@53unc8n42i868.irc", NULL, NULL, &host);
	g_assert_cmpstr(host, ==, "53unc8n42i868.irc");
	g_clear_pointer(&host, g_free);
}

static void
test_ircv3_source_parse_baddies(void) {
	char *nick = NULL;
	char *user = NULL;
	char *server = NULL;

	purple_ircv3_source_parse("\n", &nick, NULL, NULL);
	g_assert_null(nick);

	purple_ircv3_source_parse("a\nb", &nick, NULL, NULL);
	g_assert_null(nick);

	purple_ircv3_source_parse("\r", &nick, NULL, NULL);
	g_assert_null(nick);

	purple_ircv3_source_parse("c\rd", &nick, NULL, NULL);
	g_assert_null(nick);

	purple_ircv3_source_parse(" ", &nick, NULL, NULL);
	g_assert_null(nick);

	purple_ircv3_source_parse("e f", &nick, NULL, NULL);
	g_assert_null(nick);

	purple_ircv3_source_parse("nick@foo!user@server", &nick, &user, &server);
	g_assert_cmpstr(nick, ==, "nick@foo");
	g_clear_pointer(&nick, g_free);
	g_assert_cmpstr(user, ==, "user");
	g_clear_pointer(&user, g_free);
	g_assert_cmpstr(server, ==, "server");
	g_clear_pointer(&server, g_free);

	purple_ircv3_source_parse("nick!user@server!foo", &nick, &user, &server);
	g_assert_cmpstr(nick, ==, "nick!user@server!foo");
	g_assert_null(user);
	g_assert_null(server);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/ircv3/source/parse/return-address-required",
	                test_ircv3_source_parse_return_address_required);
	g_test_add_func("/ircv3/source/parse/nick", test_ircv3_source_parse_nick);
	g_test_add_func("/ircv3/source/parse/user", test_ircv3_source_parse_user);
	g_test_add_func("/ircv3/source/parse/host", test_ircv3_source_parse_host);
	g_test_add_func("/ircv3/source/parse/baddies",
	                test_ircv3_source_parse_baddies);

	return g_test_run();
}