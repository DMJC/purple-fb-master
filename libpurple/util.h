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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_UTIL_H
#define PURPLE_UTIL_H

#include <stdio.h>

#include "purpleaccount.h"
#include "purpleprotocol.h"
#include "purpleversion.h"
#include "xmlnode.h"

G_BEGIN_DECLS

/**************************************************************************/
/* Utility Subsystem                                                      */
/**************************************************************************/

/**
 * purple_util_init:
 *
 * Initializes the utility subsystem.
 *
 * Since: 2.3.0
 */
PURPLE_AVAILABLE_IN_2_3
void purple_util_init(void);

/**
 * purple_util_uninit:
 *
 * Uninitializes the util subsystem.
 *
 * Since: 2.3.0
 */
PURPLE_AVAILABLE_IN_2_3
void purple_util_uninit(void);

/**************************************************************************/
/* Path/Filename Functions                                                */
/**************************************************************************/

/**
 * purple_util_write_data_to_cache_file:
 * @filename: The basename of the file to write in the purple_cache_dir.
 * @data:     A string of data to write.
 * @size:     The size of the data to save.  If data is
 *                 null-terminated you can pass in -1.
 *
 * Write a string of data to a file of the given name in the Purple
 * cache directory ($HOME/.cache/purple by default).
 * 
 *  See purple_util_write_data_to_file()
 *
 * Returns: TRUE if the file was written successfully.  FALSE otherwise.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean
purple_util_write_data_to_cache_file(const char *filename, const char *data, gssize size);

/**
 * purple_util_write_data_to_config_file:
 * @filename: The basename of the file to write in the purple_config_dir.
 * @data:     A string of data to write.
 * @size:     The size of the data to save.  If data is
 *                 null-terminated you can pass in -1.
 *
 * Write a string of data to a file of the given name in the Purple
 * config directory ($HOME/.config/purple by default).
 *
 *  See purple_util_write_data_to_file()
 *
 * Returns: TRUE if the file was written successfully.  FALSE otherwise.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean
purple_util_write_data_to_config_file(const char *filename, const char *data, gssize size);

/**
 * purple_util_write_data_to_data_file:
 * @filename: The basename of the file to write in the purple_data_dir.
 * @data:     A string of data to write.
 * @size:     The size of the data to save.  If data is
 *                 null-terminated you can pass in -1.
 *
 * Write a string of data to a file of the given name in the Purple
 * data directory ($HOME/.local/share/purple by default).
 *
 *  See purple_util_write_data_to_file()
 *
 * Returns: TRUE if the file was written successfully.  FALSE otherwise.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean
purple_util_write_data_to_data_file(const char *filename, const char *data, gssize size);

/**
 * purple_util_read_xml_from_cache_file:
 * @filename:    The basename of the file to open in the purple_cache_dir.
 * @description: A very short description of the contents of this
 *                    file.  This is used in error messages shown to the
 *                    user when the file can not be opened.  For example,
 *                    "preferences," or "buddy pounces."
 *
 * Read the contents of a given file and parse the results into an
 * PurpleXmlNode tree structure.  This is intended to be used to read
 * Purple's cache xml files (xmpp-caps.xml, etc.)
 *
 * Returns: An PurpleXmlNode tree of the contents of the given file.  Or NULL, if
 *         the file does not exist or there was an error reading the file.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleXmlNode *
purple_util_read_xml_from_cache_file(const char *filename, const char *description);

/**
 * purple_util_read_xml_from_config_file:
 * @filename:    The basename of the file to open in the purple_config_dir.
 * @description: A very short description of the contents of this
 *                    file.  This is used in error messages shown to the
 *                    user when the file can not be opened.  For example,
 *                    "preferences," or "buddy pounces."
 *
 * Read the contents of a given file and parse the results into an
 * PurpleXmlNode tree structure.  This is intended to be used to read
 * Purple's config xml files (prefs.xml, pounces.xml, etc.)
 *
 * Returns: An PurpleXmlNode tree of the contents of the given file.  Or NULL, if
 *         the file does not exist or there was an error reading the file.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleXmlNode *
purple_util_read_xml_from_config_file(const char *filename, const char *description);

/**
 * purple_util_read_xml_from_data_file:
 * @filename:    The basename of the file to open in the purple_data_dir.
 * @description: A very short description of the contents of this
 *                    file.  This is used in error messages shown to the
 *                    user when the file can not be opened.  For example,
 *                    "preferences," or "buddy pounces."
 *
 * Read the contents of a given file and parse the results into an
 * PurpleXmlNode tree structure.  This is intended to be used to read
 * Purple's cache xml files (accounts.xml, etc.)
 *
 * Returns: An PurpleXmlNode tree of the contents of the given file.  Or NULL, if
 *         the file does not exist or there was an error reading the file.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleXmlNode *
purple_util_read_xml_from_data_file(const char *filename, const char *description);

/**************************************************************************/
/* Environment Detection Functions                                        */
/**************************************************************************/

/**
 * purple_running_gnome:
 *
 * Check if running GNOME.
 *
 * Returns: TRUE if running GNOME, FALSE otherwise.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_running_gnome(void);

/**
 * purple_running_kde:
 *
 * Check if running KDE.
 *
 * Returns: TRUE if running KDE, FALSE otherwise.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_running_kde(void);

/**************************************************************************/
/* String Functions                                                       */
/**************************************************************************/

/**
 * purple_strequal:
 * @left:	A string
 * @right: A string to compare with left
 *
 * Tests two strings for equality.
 *
 * Unlike strcmp(), this function will not crash if one or both of the
 * strings are %NULL.
 *
 * Returns: %TRUE if the strings are the same, else %FALSE.
 *
 * Since: 2.6.0
 */
PURPLE_AVAILABLE_STATIC_INLINE_IN_2_6
static inline gboolean
purple_strequal(const gchar *left, const gchar *right)
{
	return (g_strcmp0(left, right) == 0);
}

/**
 * purple_strempty:
 * @str: A string to check if it is empty.
 *
 * Determines if @str is empty. That is, if it is %NULL or an empty string.
 *
 * Returns: %TRUE if the @str is %NULL or an empty string.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_STATIC_INLINE_IN_3_0
static inline gboolean
purple_strempty(const char *str) {
	return (str == NULL || str[0] == '\0');
}

/**
 * purple_normalize:
 * @account:  The account the string belongs to, or NULL if you do
 *                 not know the account.  If you use NULL, the string
 *                 will still be normalized, but if the protocol uses a
 *                 custom normalization function then the string may
 *                 not be normalized correctly.
 * @str:      The string to normalize.
 *
 * Normalizes a string, so that it is suitable for comparison.
 *
 * The returned string will point to a static buffer, so if the
 * string is intended to be kept long-term, you <emphasis>must</emphasis>
 * g_strdup() it. Also, calling normalize() twice in the same line
 * will lead to problems.
 *
 * Returns: A pointer to the normalized version stored in a static buffer.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_normalize(PurpleAccount *account, const char *str);

/**
 * purple_validate:
 * @protocol: The protocol the string belongs to.
 * @str:      The string to validate.
 *
 * Checks, if a string is valid.
 *
 * Returns: TRUE, if string is valid, otherwise FALSE.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_validate(PurpleProtocol *protocol, const char *str);

/**
 * purple_strdup_withhtml:
 * @src: The source string.
 *
 * Duplicates a string and replaces all newline characters from the
 * source string with HTML linebreaks.
 *
 * Returns: The new string.  Must be g_free'd by the caller.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gchar *purple_strdup_withhtml(const gchar *src);

/**
 * purple_str_strip_char:
 * @str:     The string to strip characters from.
 * @thechar: The character to strip from the given string.
 *
 * Strips all instances of the given character from the
 * given string.  The string is modified in place.  This
 * is useful for stripping new line characters, for example.
 *
 * Example usage:
 * purple_str_strip_char(my_dumb_string, '\n');
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_str_strip_char(char *str, char thechar);

/**
 * purple_util_chrreplace:
 * @string: The string from which to replace stuff.
 * @delimiter: The character you want replaced.
 * @replacement: The character you want inserted in place
 *        of the delimiting character.
 *
 * Given a string, this replaces all instances of one character
 * with another.  This happens inline (the original string IS
 * modified).
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_util_chrreplace(char *string, char delimiter,
						  char replacement);

/**
 * purple_strreplace:
 * @string: The string from which to replace stuff.
 * @delimiter: The substring you want replaced.
 * @replacement: The substring you want inserted in place
 *        of the delimiting substring.
 *
 * Given a string, this replaces one substring with another
 * and returns a newly allocated string.
 *
 * Returns: A new string, after performing the substitution.
 *         free this with g_free().
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gchar *purple_strreplace(const char *string, const char *delimiter,
					   const char *replacement);

/**
 * purple_str_seconds_to_string:
 * @sec: The seconds.
 *
 * Converts seconds into a human-readable form.
 *
 * Returns: A human-readable form, containing days, hours, minutes, and
 *         seconds.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
char *purple_str_seconds_to_string(guint sec);

/**
 * purple_str_wipe:
 * @str: A NUL-terminated string to free, or a NULL-pointer.
 *
 * Fills a NUL-terminated string with zeros and frees it.
 *
 * It should be used to free sensitive data, like passwords.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_str_wipe(gchar *str);

/**
 * purple_strmatches:
 * @pattern: The pattern to search for.
 * @str: The string to check.
 *
 * Checks if @pattern occurs in sequential order in @str in a caseless fashion,
 * ignoring characters in between.
 *
 * For example, if @pattern was `Pg` and @str was `Pidgin`, this will return
 * %TRUE.
 *
 * Returns: %TRUE if @pattern occurs in sequential order in @str, %FALSE
 *          otherwise.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_strmatches(const char *pattern, const char *str);

/**************************************************************************/
/* URI/URL Functions                                                      */
/**************************************************************************/

PURPLE_AVAILABLE_IN_ALL
void purple_got_protocol_handler_uri(const char *uri);

/**
 * purple_url_encode:
 * @str: The string to translate.
 *
 * Encodes a URL into an escaped string.
 *
 * This will change non-alphanumeric characters to hex codes.
 *
 * Returns: The resulting string.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_url_encode(const char *str);

/**
 * purple_email_is_valid:
 * @address: The email address to validate.
 *
 * Checks if the given email address is syntactically valid.
 *
 * Returns: True if the email address is syntactically correct.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_email_is_valid(const char *address);

/**************************************************************************
 * UTF8 String Functions
 **************************************************************************/

/**
 * purple_utf8_try_convert:
 * @str: The source string.
 *
 * Attempts to convert a string to UTF-8 from an unknown encoding.
 *
 * This function checks the locale and tries sane defaults.
 *
 * Returns: The UTF-8 string, or %NULL if it could not be converted.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gchar *purple_utf8_try_convert(const char *str);

/**
 * purple_utf8_strip_unprintables:
 * @str: A valid UTF-8 string.
 *
 * Removes unprintable characters from a UTF-8 string. These characters
 * (in particular low-ASCII characters) are invalid in XML 1.0 and thus
 * are not allowed in XMPP and are rejected by libxml2 by default.
 *
 * The returned string must be freed by the caller.
 *
 * Returns: A newly allocated UTF-8 string without the unprintable characters.
 *
 * Since: 2.6.0
 */
PURPLE_AVAILABLE_IN_2_6
gchar *purple_utf8_strip_unprintables(const gchar *str);

/**
 * purple_utf8_strcasecmp:
 * @a: The first string.
 * @b: The second string.
 *
 * Compares two UTF-8 strings case-insensitively.  This comparison is
 * more expensive than a simple g_utf8_collate() comparison because
 * it calls g_utf8_casefold() on each string, which allocates new
 * strings.
 *
 * Returns: -1 if @a is less than @b.
 *           0 if @a is equal to @b.
 *           1 if @a is greater than @b.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
int purple_utf8_strcasecmp(const char *a, const char *b);

/**
 * purple_utf8_has_word:
 * @haystack: The string to search in.
 * @needle:   The substring to find.
 *
 * Case insensitive search for a word in a string. The needle string
 * must be contained in the haystack string and not be immediately
 * preceded or immediately followed by another alphanumeric character.
 *
 * Returns: TRUE if haystack has the word, otherwise FALSE
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_utf8_has_word(const char *haystack, const char *needle);

/**
 * purple_text_strip_mnemonic:
 * @in:  The string to strip
 *
 * Removes the underscore characters from a string used identify the mnemonic
 * character.
 *
 * Returns: The stripped string
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
char *purple_text_strip_mnemonic(const char *in);

/**
 * purple_escape_filename:
 * @str: The string to translate.
 *
 * Escapes filesystem-unfriendly characters from a filename
 *
 * Returns: The resulting string.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_escape_filename(const char *str);

/**
 * purple_value_new:
 * @type:  The type of data to be held by the GValue
 *
 * Creates a new GValue of the specified type.
 *
 * Returns:  The created GValue
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
GValue *purple_value_new(GType type);

/**
 * purple_value_dup:
 * @value:  The GValue to duplicate
 *
 * Duplicates a GValue.
 *
 * Returns:  The duplicated GValue
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
GValue *purple_value_dup(GValue *value);

/**
 * purple_value_free:
 * @value:  The GValue to free.
 *
 * Frees a GValue.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_value_free(GValue *value);

G_END_DECLS

#endif /* PURPLE_UTIL_H */
