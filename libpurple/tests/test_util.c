/*
 * purple
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */
#include <glib.h>

#include <purple.h>

/******************************************************************************
 * filename escape tests
 *****************************************************************************/
static void
test_util_filename_escape(void) {
	g_assert_cmpstr("foo", ==, purple_escape_filename("foo"));
	g_assert_cmpstr("@oo", ==, purple_escape_filename("@oo"));
	g_assert_cmpstr("#oo", ==, purple_escape_filename("#oo"));
	g_assert_cmpstr("-oo", ==, purple_escape_filename("-oo"));
	g_assert_cmpstr("_oo", ==, purple_escape_filename("_oo"));
	g_assert_cmpstr(".oo", ==, purple_escape_filename(".oo"));
	g_assert_cmpstr("%25oo", ==, purple_escape_filename("%oo"));
	g_assert_cmpstr("%21oo", ==, purple_escape_filename("!oo"));
}

/******************************************************************************
 * email tests
 *****************************************************************************/
/*
 * Many of the valid and invalid email addresses lised below are from
 * http://fightingforalostcause.net/misc/2006/compare-email-regex.php
 */
const gchar *valid_emails[] = {
	"purple-devel@lists.sf.net",
	"l3tt3rsAndNumb3rs@domain.com",
	"has-dash@domain.com",
	"hasApostrophe.o'leary@domain.org",
	"uncommonTLD@domain.museum",
	"uncommonTLD@domain.travel",
	"uncommonTLD@domain.mobi",
	"countryCodeTLD@domain.uk",
	"countryCodeTLD@domain.rw",
	"lettersInDomain@911.com",
	"underscore_inLocal@domain.net",
	"IPInsteadOfDomain@127.0.0.1",
	/* "IPAndPort@127.0.0.1:25", */
	"subdomain@sub.domain.com",
	"local@dash-inDomain.com",
	"dot.inLocal@foo.com",
	"a@singleLetterLocal.org",
	"singleLetterDomain@x.org",
	"&*=?^+{}'~@validCharsInLocal.net",
	"foor@bar.newTLD",
	"HenryTheGreatWhiteCricket@live.ca",
	"HenryThe__WhiteCricket@hotmail.com"
};

static void
test_util_email_is_valid(void) {
	size_t i;

	for (i = 0; i < G_N_ELEMENTS(valid_emails); i++)
		g_assert_true(purple_email_is_valid(valid_emails[i]));
}

const gchar *invalid_emails[] = {
	"purple-devel@@lists.sf.net",
	"purple@devel@lists.sf.net",
	"purple-devel@list..sf.net",
	"purple-devel",
	"purple-devel@",
	"@lists.sf.net",
	"totally bogus",
	"missingDomain@.com",
	"@missingLocal.org",
	"missingatSign.net",
	"missingDot@com",
	"two@@signs.com",
	"colonButNoPort@127.0.0.1:",
	"",
	/* "someone-else@127.0.0.1.26", */
	".localStartsWithDot@domain.com",
	/* "localEndsWithDot.@domain.com", */ /* I don't think this is invalid -- Stu */
	/* "two..consecutiveDots@domain.com", */ /* I don't think this is invalid -- Stu */
	"domainStartsWithDash@-domain.com",
	"domainEndsWithDash@domain-.com",
	/* "numbersInTLD@domain.c0m", */
	/* "missingTLD@domain.", */ /* This certainly isn't invalid -- Stu */
	"! \"#$%(),/;<>[]`|@invalidCharsInLocal.org",
	"invalidCharsInDomain@! \"#$%(),/;<>_[]`|.org",
	/* "local@SecondLevelDomainNamesAreInvalidIfTheyAreLongerThan64Charactersss.org" */
};

static void
test_util_email_is_invalid(void) {
	size_t i;

	for (i = 0; i < G_N_ELEMENTS(invalid_emails); i++)
		g_assert_false(purple_email_is_valid(invalid_emails[i]));
}

/******************************************************************************
 * MAIN
 *****************************************************************************/
gint
main(gint argc, gchar **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/util/filename/escape",
	                test_util_filename_escape);

	g_test_add_func("/util/email/is valid",
	                test_util_email_is_valid);
	g_test_add_func("/util/email/is invalid",
	                test_util_email_is_invalid);

	return g_test_run();
}
