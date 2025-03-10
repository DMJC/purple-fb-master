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
 * base 16 tests
 *****************************************************************************/
static void
test_util_base_16_encode(void) {
	gchar *in = purple_base16_encode((const guchar *)"hello, world!", 14);
	g_assert_cmpstr("68656c6c6f2c20776f726c642100", ==, in);
}

static void
test_util_base_16_decode(void) {
	gsize sz = 0;
	guchar *out = purple_base16_decode("21646c726f77202c6f6c6c656800", &sz);

	g_assert_cmpint(sz, ==, 14);
	g_assert_cmpstr("!dlrow ,olleh", ==, (const gchar *)out);
}

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

static void
test_util_filename_unescape(void) {
	g_assert_cmpstr("bar", ==, purple_unescape_filename("bar"));
	g_assert_cmpstr("@ar", ==, purple_unescape_filename("@ar"));
	g_assert_cmpstr("!ar", ==, purple_unescape_filename("!ar"));
	g_assert_cmpstr("!ar", ==, purple_unescape_filename("%21ar"));
	g_assert_cmpstr("%ar", ==, purple_unescape_filename("%25ar"));
}

/******************************************************************************
 * text_strip tests
 *****************************************************************************/
static void
test_util_text_strip_mnemonic(void) {
	g_assert_cmpstr("", ==, purple_text_strip_mnemonic(""));
	g_assert_cmpstr("foo", ==, purple_text_strip_mnemonic("foo"));
	g_assert_cmpstr("foo", ==, purple_text_strip_mnemonic("_foo"));

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
 * str_to_time tests
 *****************************************************************************/
static void
test_util_str_to_time(void) {
	struct tm tm;
	glong tz_off;
	const gchar *rest;
	time_t timestamp;

	g_assert_cmpint(377182200, ==, purple_str_to_time("19811214T12:50:00", TRUE, NULL, NULL, NULL));
	g_assert_cmpint(1175919261, ==, purple_str_to_time("20070407T04:14:21", TRUE, NULL, NULL, NULL));
	g_assert_cmpint(1282941722, ==, purple_str_to_time("2010-08-27.204202", TRUE, NULL, NULL, NULL));

	timestamp = purple_str_to_time("2010-08-27.134202-0700PDT", FALSE, &tm, &tz_off, &rest);
	g_assert_cmpint(1282941722, ==, timestamp);
	g_assert_cmpint((-7 * 60 * 60), ==, tz_off);
	g_assert_cmpstr("PDT", ==, rest);
}

/******************************************************************************
 * str_to_date_time tests
 *****************************************************************************/
static void
test_util_str_to_date_time(void)
{
	GDateTime *dt;

	dt = purple_str_to_date_time("19811214T12:50:00", TRUE);
	g_assert_cmpint(377182200, ==, g_date_time_to_unix(dt));
	g_assert_cmpint(0, ==, g_date_time_get_utc_offset(dt));
	g_date_time_unref(dt);

	dt = purple_str_to_date_time("20070407T04:14:21.1234", TRUE);
	g_assert_cmpint(1175919261, ==, g_date_time_to_unix(dt));
	g_assert_cmpint(0, ==, g_date_time_get_utc_offset(dt));
	g_assert_cmpint(123400, ==, g_date_time_get_microsecond(dt));
	g_date_time_unref(dt);

	dt = purple_str_to_date_time("2010-08-27.204202", TRUE);
	g_assert_cmpint(1282941722, ==, g_date_time_to_unix(dt));
	g_assert_cmpint(0, ==, g_date_time_get_utc_offset(dt));
	g_date_time_unref(dt);

	dt = purple_str_to_date_time("2010-08-27.204202.123456", TRUE);
	g_assert_cmpint(1282941722, ==, g_date_time_to_unix(dt));
	g_assert_cmpint(0, ==, g_date_time_get_utc_offset(dt));
	g_assert_cmpint(123456, ==, g_date_time_get_microsecond(dt));
	g_date_time_unref(dt);

	dt = purple_str_to_date_time("2010-08-27.134202-0700PDT", FALSE);
	g_assert_cmpint(1282941722, ==, g_date_time_to_unix(dt));
	g_assert_cmpint((-7LL * 60 * 60 * G_USEC_PER_SEC), ==, g_date_time_get_utc_offset(dt));

	dt = purple_str_to_date_time("2010-08-27.134202.1234-0700PDT", FALSE);
	g_assert_cmpint(1282941722, ==, g_date_time_to_unix(dt));
	g_assert_cmpint(123400, ==, g_date_time_get_microsecond(dt));
	g_assert_cmpint((-7LL * 60 * 60 * G_USEC_PER_SEC), ==, g_date_time_get_utc_offset(dt));
}

/******************************************************************************
 * Markup tests
 *****************************************************************************/
typedef struct {
	gchar *markup;
	gchar *xhtml;
	gchar *plaintext;
} MarkupTestData;

static void
test_util_markup_html_to_xhtml(void) {
	gint i;
	MarkupTestData data[] = {
		{
			"<a>",
			"<a href=\"\"></a>",
			"",
		}, {
			"<A href='URL'>ABOUT</a>",
			"<a href=\"URL\">ABOUT</a>",
			"ABOUT <URL>",
		}, {
			"<a href='URL'>URL</a>",
			"<a href=\"URL\">URL</a>",
			"URL",
		}, {
			"<a href='mailto:mail'>mail</a>",
			"<a href=\"mailto:mail\">mail</a>",
			"mail",
		}, {
			"<A href='\"U&apos;R&L'>ABOUT</a>",
			"<a href=\"&quot;U&apos;R&amp;L\">ABOUT</a>",
			"ABOUT <\"U'R&L>",
		}, {
			"<img src='SRC' alt='ALT'/>",
			"<img src='SRC' alt='ALT' />",
			"ALT",
		}, {
			"<img src=\"'S&apos;R&C\" alt=\"'A&apos;L&T\"/>",
			"<img src='&apos;S&apos;R&amp;C' alt='&apos;A&apos;L&amp;T' />",
			"'A'L&T",
		}, {
			"<unknown>",
			"&lt;unknown>",
			"<unknown>",
		}, {
			"&eacute;&amp;",
			"&eacute;&amp;",
			"&eacute;&",
		}, {
			"<h1>A<h2>B</h2>C</h1>",
			"<h1>A<h2>B</h2>C</h1>",
			"ABC",
		}, {
			"<h1><h2><h3><h4>",
			"<h1><h2><h3><h4></h4></h3></h2></h1>",
			"",
		}, {
			"<italic/>",
			"<em/>",
			"",
		}, {
			"</",
			"&lt;/",
			"</",
		}, {
			"</div>",
			"",
			"",
		}, {
			"<hr/>",
			"<br/>",
			"\n",
		}, {
			"<hr>",
			"<br/>",
			"\n",
		}, {
			"<br />",
			"<br/>",
			"\n",
		}, {
			"<br>INSIDE</br>",
			"<br/>INSIDE",
			"\nINSIDE",
		}, {
			"<div></div>",
			"<div></div>",
			"",
		}, {
			"<div/>",
			"<div/>",
			"",
		}, {
			"<div attr='\"&<>'/>",
			"<div attr='&quot;&amp;&lt;&gt;'/>",
			"",
		}, {
			"<div attr=\"'\"/>",
			"<div attr=\"&apos;\"/>",
			"",
		}, {
			"<div/> < <div/>",
			"<div/> &lt; <div/>",
			" < ",
		}, {
			"<div>x</div>",
			"<div>x</div>",
			"x",
		}, {
			"<b>x</b>",
			"<span style='font-weight: bold;'>x</span>",
			"x",
		}, {
			"<bold>x</bold>",
			"<span style='font-weight: bold;'>x</span>",
			"x",
		}, {
			"<strong>x</strong>",
			"<span style='font-weight: bold;'>x</span>",
			"x",
		}, {
			"<u>x</u>",
			"<span style='text-decoration: underline;'>x</span>",
			"x",
		}, {
			"<underline>x</underline>",
			"<span style='text-decoration: underline;'>x</span>",
			"x",
		}, {
			"<s>x</s>",
			"<span style='text-decoration: line-through;'>x</span>",
			"x",
		}, {
			"<strike>x</strike>",
			"<span style='text-decoration: line-through;'>x</span>",
			"x",
		}, {
			"<sub>x</sub>",
			"<span style='vertical-align:sub;'>x</span>",
			"x",
		}, {
			"<sup>x</sup>",
			"<span style='vertical-align:super;'>x</span>",
			"x",
		}, {
			"<FONT>x</FONT>",
			"x",
			"x",
		}, {
			"<font face=\"'Times&gt;New & Roman'\">x</font>",
			"<span style='font-family: \"Times&gt;New &amp; Roman\";'>x</span>",
			"x",
		}, {
			"<font back=\"'color&gt;blue&red'\">x</font>",
			"<span style='background: \"color&gt;blue&amp;red\";'>x</span>",
			"x",
		}, {
			"<font color=\"'color&gt;blue&red'\">x</font>",
			"<span style='color: \"color&gt;blue&amp;red\";'>x</span>",
			"x",
		}, {
			"<font size=1>x</font>",
			"<span style='font-size: xx-small;'>x</span>",
			"x",
		}, {
			"<font size=432>x</font>",
			"<span style='font-size: medium;'>x</span>",
			"x",
		}, {
			"<!--COMMENT-->",
			"<!--COMMENT-->",
			"COMMENT-->",
		}, {
			"<br  />",
			"&lt;br  />",
			"<br  />",
		}, {
			"<hr  />",
			"&lt;hr  />",
			"<hr  />"
		}, {
			NULL, NULL, NULL,
		}
	};

	for(i = 0; data[i].markup; i++) {
		gchar *xhtml = NULL, *plaintext = NULL;

		purple_markup_html_to_xhtml(data[i].markup, &xhtml, &plaintext);

		g_assert_cmpstr(data[i].xhtml, ==, xhtml);
		g_free(xhtml);

		g_assert_cmpstr(data[i].plaintext, ==, plaintext);
		g_free(plaintext);
	}
}

/******************************************************************************
 * UTF8 tests
 *****************************************************************************/
typedef struct {
	gchar *input;
	gchar *output;
} UTF8TestData;

static void
test_util_utf8_strip_unprintables(void) {
	gint i;
	UTF8TestData data[] = {
		{
			/* \t, \n, \r, space */
			"ab \tcd\nef\r   ",
			"ab \tcd\nef\r   ",
		}, {
			/* ASCII control characters (stripped) */
			"\x01\x02\x03\x04\x05\x06\x07\x08\x0B\x0C\x0E\x0F\x10 aaaa "
			"\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F",
			" aaaa ",
		}, {
			/* Basic ASCII */
			"Foobar",
			"Foobar",
		}, {
			/* 0xE000 - 0xFFFD (UTF-8 encoded) */
			/* U+F1F7 */
			"aaaa\xef\x87\xb7",
			"aaaa\xef\x87\xb7",
		}, {
			/* U+FEFF (should not be stripped) */
			"aaaa\xef\xbb\xbf",
			"aaaa\xef\xbb\xbf",
		}, {
			/* U+FFFE (should be stripped) */
			"aaaa\xef\xbf\xbe",
			"aaaa",
		}, {
			NULL,
			NULL,
		}
	};

	for(i = 0; ; i++) {
		gchar *result = purple_utf8_strip_unprintables(data[i].input);

		g_assert_cmpstr(data[i].output, ==, result);

		g_free(result);

		/* NULL as input is a valid test, but it's the last test, so we break
		 * after it.
		 */
		if(data[i].input == NULL)
			break;
	}
}

/******************************************************************************
 * strdup_withhtml tests
 *****************************************************************************/
static void
test_util_strdup_withhtml(void) {
	gchar *result = purple_strdup_withhtml("hi\r\nthere\n");

	g_assert_cmpstr("hi<BR>there<BR>", ==, result);

	g_free(result);
}

/******************************************************************************
 * URI Escaping
 *****************************************************************************/
static void
test_uri_escape_for_open(void) {
	/* make sure shell stuff is escaped... */
	gchar *result = purple_uri_escape_for_open("https://$(xterm)");
	g_assert_cmpstr("https://%24%28xterm%29", ==, result);
	g_free(result);

	result = purple_uri_escape_for_open("https://`xterm`");
	g_assert_cmpstr("https://%60xterm%60", ==, result);
	g_free(result);

	result = purple_uri_escape_for_open("https://$((25 + 13))");
	g_assert_cmpstr("https://%24%28%2825%20+%2013%29%29", ==, result);
	g_free(result);

	/* ...but keep brackets so that ipv6 links can be opened. */
	result = purple_uri_escape_for_open("https://[123:4567:89a::::]");
	g_assert_cmpstr("https://[123:4567:89a::::]", ==, result);
	g_free(result);
}

/******************************************************************************
 * MANE
 *****************************************************************************/
gint
main(gint argc, gchar **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/util/base/16/encode",
	                test_util_base_16_encode);
	g_test_add_func("/util/base/16/decode",
	                test_util_base_16_decode);

	g_test_add_func("/util/filename/escape",
	                test_util_filename_escape);
	g_test_add_func("/util/filename/unescape",
	                test_util_filename_unescape);

	g_test_add_func("/util/mnemonic/strip",
	                test_util_text_strip_mnemonic);

	g_test_add_func("/util/email/is valid",
	                test_util_email_is_valid);
	g_test_add_func("/util/email/is invalid",
	                test_util_email_is_invalid);

	g_test_add_func("/util/str to time",
	                test_util_str_to_time);

	g_test_add_func("/util/str to date time",
	                test_util_str_to_date_time);

	g_test_add_func("/util/markup/html to xhtml",
	                test_util_markup_html_to_xhtml);

	g_test_add_func("/util/utf8/strip unprintables",
	                test_util_utf8_strip_unprintables);

	g_test_add_func("/util/test_strdup_withhtml",
	                test_util_strdup_withhtml);

	g_test_add_func("/util/test_uri_escape_for_open",
	                test_uri_escape_for_open);

	return g_test_run();
}
