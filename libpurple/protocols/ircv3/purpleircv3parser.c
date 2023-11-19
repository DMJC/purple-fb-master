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

#include "purpleircv3parser.h"

#include "purpleircv3capabilities.h"
#include "purpleircv3constants.h"
#include "purpleircv3core.h"
#include "purpleircv3message.h"
#include "purpleircv3messagehandlers.h"
#include "purpleircv3sasl.h"

struct _PurpleIRCv3Parser {
	GObject parent;

	GRegex *regex_message;
	GRegex *regex_tags;

	PurpleIRCv3MessageHandler fallback_handler;
	GHashTable *handlers;
};

G_DEFINE_TYPE(PurpleIRCv3Parser, purple_ircv3_parser, G_TYPE_OBJECT)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static char *
purple_ircv3_parser_unescape_tag_value(const char *value) {
	GString *unescaped = g_string_new("");
	gboolean escaping = FALSE;

	/* Walk the string and replace escaped values according to
	 * https://ircv3.net/specs/extensions/message-tags.html#escaping-values
	 */
	for(int i = 0; value[i] != '\0'; i++) {
		if(escaping) {
			/* Set the replacement to the current character which will fall
			 * through for everything, including '\\'
			 */
			char replacement = value[i];

			if(value[i] == ':') {
				replacement = ';';
			} else if(value[i] == 's') {
				replacement = ' ';
			} else if(value[i] == 'r') {
				replacement = '\r';
			} else if(value[i] == 'n') {
				replacement = '\n';
			}

			g_string_append_c(unescaped, replacement);
			escaping = FALSE;
		} else {
			if(value[i] == '\\') {
				escaping = TRUE;
			} else {
				g_string_append_c(unescaped, value[i]);
			}
		}
	}

	return g_string_free(unescaped, FALSE);
}

static GHashTable *
purple_ircv3_parser_parse_tags(PurpleIRCv3Parser *parser,
                               const gchar *tags_string, GError **error)
{
	GError *local_error = NULL;
	GHashTable *tags = NULL;
	GMatchInfo *info = NULL;
	gboolean matches = FALSE;

	tags = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	/* tags_string can never be NULL, because g_match_info_fetch_named always
	 * returns a string. So if we were passed an empty string, just return the
	 * empty hash table.
	 */
	if(*tags_string == '\0') {
		return tags;
	}

	matches = g_regex_match_full(parser->regex_tags, tags_string, -1, 0, 0,
	                             &info, &local_error);

	if(local_error != NULL) {
		g_propagate_error(error, local_error);

		g_match_info_unref(info);

		return tags;
	}

	if(!matches) {
		g_set_error_literal(error, PURPLE_IRCV3_DOMAIN, 0,
		                    "failed to parse tags: unknown error");

		g_match_info_unref(info);

		return tags;
	}

	while(g_match_info_matches(info)) {
		char *key = NULL;
		char *value = NULL;
		char *unescaped = NULL;

		key = g_match_info_fetch_named(info, "key");
		value = g_match_info_fetch_named(info, "value");

		unescaped = purple_ircv3_parser_unescape_tag_value(value);
		g_free(value);

		/* the hash table is created with destroy notifies for both key and
		 * value, so there's no need to free the allocated memory right now.
		 */
		g_hash_table_insert(tags, key, unescaped);
		g_match_info_next(info, &local_error);

		if(local_error != NULL) {
			g_propagate_error(error, local_error);

			break;
		}
	}

	g_match_info_unref(info);

	return tags;
}

static guint
purple_ircv3_parser_extract_params(G_GNUC_UNUSED PurpleIRCv3Parser *parser,
                                   GStrvBuilder *builder, const gchar *str)
{
	gchar *ptr = NULL;
	guint count = 0;

	/* Loop through str finding each space separated string. */
	while(str != NULL && *str != '\0') {
		/* Look for a space. */
		ptr = strchr(str, ' ');

		/* If we found one, set it to null terminator and add the string to our
		 * builder.
		 */
		if(ptr != NULL) {
			*ptr = '\0';
			g_strv_builder_add(builder, str);

			/* Move str to the next character as we know there's another
			 * character which might be another null terminator.
			 */
			str = ptr + 1;

			/* And don't forget to increment the count... ah ah ah! */
			count++;
		} else {
			/* Add the remaining string. */
			g_strv_builder_add(builder, str);

			/* Give the count another one, ah ah ah! */
			count++;

			/* Finally break out of the loop. */
			break;
		}
	}

	return count;
}

static GStrv
purple_ircv3_parser_build_params(PurpleIRCv3Parser *parser,
                                 const gchar *middle, const gchar *coda,
                                 const gchar *trailing, guint *n_params)
{
	GStrvBuilder *builder = g_strv_builder_new();
	GStrv result = NULL;

	purple_ircv3_parser_extract_params(parser, builder, middle);

	if(*coda != '\0') {
		g_strv_builder_add(builder, trailing);
	}

	result = g_strv_builder_end(builder);

	g_strv_builder_unref(builder);

	if(result != NULL && n_params != NULL) {
		*n_params = g_strv_length(result);
	}

	return result;
}

/******************************************************************************
 * Handlers
 *****************************************************************************/
static gboolean
purple_ircv3_fallback_handler(PurpleIRCv3Message *message,
                              GError **error,
                              G_GNUC_UNUSED gpointer data)
{
	g_set_error(error, PURPLE_IRCV3_DOMAIN, 0, "no handler for command %s",
	            purple_ircv3_message_get_command(message));

	return FALSE;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_ircv3_parser_finalize(GObject *obj) {
	PurpleIRCv3Parser *parser = PURPLE_IRCV3_PARSER(obj);

	g_clear_pointer(&parser->regex_message, g_regex_unref);
	g_clear_pointer(&parser->regex_tags, g_regex_unref);

	g_hash_table_destroy(parser->handlers);

	G_OBJECT_CLASS(purple_ircv3_parser_parent_class)->finalize(obj);
}

static void
purple_ircv3_parser_init(PurpleIRCv3Parser *parser) {
	parser->regex_message =  g_regex_new("(?:@(?<tags>[^ ]+) )?"
	                                     "(?::(?<source>[^ ]+) +)?"
	                                     "(?<command>[^ :]+)"
	                                     "(?: +(?<middle>(?:[^ :]+(?: +[^ :]+)*)))*"
	                                     "(?<coda> +:(?<trailing>.*)?)?",
	                                     0, 0, NULL);
	g_assert(parser->regex_message != NULL);

	parser->regex_tags = g_regex_new("(?<key>(?<client_prefix>\\+?)"
	                                        "(?:(?<vendor>[A-Za-z0-9-\\.]+)\\/)?"
	                                        "(?<key_name>[A-Za-z0-9-]+)"
	                                 ")"
	                                 "(?:=(?<value>[^\r\n;]*))?(?:;|$)",
	                                 0, 0, NULL);
	g_assert(parser->regex_tags != NULL);

	parser->fallback_handler = purple_ircv3_fallback_handler;
	parser->handlers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
	                                         NULL);
}

static void
purple_ircv3_parser_class_init(PurpleIRCv3ParserClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_ircv3_parser_finalize;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleIRCv3Parser *
purple_ircv3_parser_new(void) {
	return g_object_new(PURPLE_IRCV3_TYPE_PARSER, NULL);
}

void
purple_ircv3_parser_set_fallback_handler(PurpleIRCv3Parser *parser,
                                         PurpleIRCv3MessageHandler handler)
{
	g_return_if_fail(PURPLE_IRCV3_IS_PARSER(parser));

	parser->fallback_handler = handler;
}

gboolean
purple_ircv3_parser_parse(PurpleIRCv3Parser *parser, const gchar *buffer,
                          GError **error, gpointer data)
{
	PurpleIRCv3Message *message = NULL;
	PurpleIRCv3MessageHandler handler = NULL;
	GError *local_error = NULL;
	GHashTable *tags = NULL;
	GMatchInfo *info = NULL;
	GStrv params = NULL;
	gchar *coda = NULL;
	gchar *command = NULL;
	gchar *middle = NULL;
	gchar *source = NULL;
	gchar *tags_string = NULL;
	gchar *trailing = NULL;
	gboolean matches = FALSE;
	gboolean result = FALSE;

	g_return_val_if_fail(PURPLE_IRCV3_IS_PARSER(parser), FALSE);
	g_return_val_if_fail(buffer != NULL, FALSE);

	/* Check if the buffer matches our regex for messages. */
	matches = g_regex_match(parser->regex_message, buffer, 0, &info);
	if(!matches) {
		g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
		            "failed to parser buffer '%s'", buffer);

		g_match_info_unref(info);

		return FALSE;
	}

	/* Extract the command from the buffer, so we can find the handler. */
	command = g_match_info_fetch_named(info, "command");
	handler = g_hash_table_lookup(parser->handlers, command);
	if(handler == NULL) {
		if(parser->fallback_handler == NULL) {
			g_set_error(error, PURPLE_IRCV3_DOMAIN, 0,
			            "no handler found for command %s and no default "
			            "handler set.", command);

			g_free(command);
			g_match_info_unref(info);

			return FALSE;
		}

		handler = parser->fallback_handler;
	}

	/* If we made it this far, we have our handler, so lets get the rest of the
	 * parameters and call the handler.
	 */
	message = purple_ircv3_message_new(command);

	tags_string = g_match_info_fetch_named(info, "tags");
	tags = purple_ircv3_parser_parse_tags(parser, tags_string, &local_error);
	g_free(tags_string);
	if(local_error != NULL) {
		g_propagate_error(error, local_error);

		g_free(command);
		g_clear_object(&message);
		g_hash_table_destroy(tags);
		g_match_info_unref(info);

		return FALSE;
	}
	if(tags != NULL) {
		purple_ircv3_message_set_tags(message, tags);
		g_hash_table_unref(tags);
	}

	source = g_match_info_fetch_named(info, "source");
	if(!purple_strempty(source)) {
		purple_ircv3_message_set_source(message, source);
	}
	g_free(source);

	middle = g_match_info_fetch_named(info, "middle");
	coda = g_match_info_fetch_named(info, "coda");
	trailing = g_match_info_fetch_named(info, "trailing");
	params = purple_ircv3_parser_build_params(parser, middle, coda, trailing,
	                                          NULL);
	if(params != NULL) {
		purple_ircv3_message_set_params(message, params);
	}

	g_free(command);
	g_free(middle);
	g_free(coda);
	g_free(trailing);
	g_strfreev(params);

	/* Call the handler. */
	result = handler(message, error, data);

	/* Cleanup the left overs. */
	g_match_info_unref(info);
	g_clear_object(&message);

	return result;
}

void
purple_ircv3_parser_add_handler(PurpleIRCv3Parser *parser,
                                const char *command,
                                PurpleIRCv3MessageHandler handler)
{
	g_return_if_fail(PURPLE_IRCV3_IS_PARSER(parser));
	g_return_if_fail(command != NULL);
	g_return_if_fail(handler != NULL);

	g_hash_table_insert(parser->handlers, g_strdup(command), handler);
}

void
purple_ircv3_parser_add_handlers(PurpleIRCv3Parser *parser,
                                 PurpleIRCv3MessageHandler handler,
                                 ...)
{
	va_list vargs;
	const char *command = NULL;

	g_return_if_fail(PURPLE_IRCV3_IS_PARSER(parser));
	g_return_if_fail(handler != NULL);

	va_start(vargs, handler);

	while((command = va_arg(vargs, const char *)) != NULL) {
		purple_ircv3_parser_add_handler(parser, command, handler);
	}

	va_end(vargs);
}

void
purple_ircv3_parser_add_default_handlers(PurpleIRCv3Parser *parser) {
	g_return_if_fail(PURPLE_IRCV3_IS_PARSER(parser));

	purple_ircv3_parser_set_fallback_handler(parser,
	                                         purple_ircv3_message_handler_fallback);

	/* Core functionality. */
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_MSG_CAP,
	                                purple_ircv3_capabilities_message_handler);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_MSG_NOTICE,
	                                purple_ircv3_message_handler_privmsg);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_MSG_PING,
	                                purple_ircv3_message_handler_ping);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_MSG_PRIVMSG,
	                                purple_ircv3_message_handler_privmsg);

	/* Topic stuff. */
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_MSG_TOPIC,
	                                purple_ircv3_message_handler_topic);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_RPL_NOTOPIC,
	                                purple_ircv3_message_handler_topic);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_RPL_TOPIC,
	                                purple_ircv3_message_handler_topic);

	/* Post Registration Greetings */
	purple_ircv3_parser_add_handlers(parser,
	                                 purple_ircv3_message_handler_status_ignore_param0,
	                                 PURPLE_IRCV3_RPL_WELCOME,
	                                 PURPLE_IRCV3_RPL_YOURHOST,
	                                 PURPLE_IRCV3_RPL_CREATED,
	                                 PURPLE_IRCV3_RPL_MYINFO,
	                                 NULL);

	/* Luser's */
	purple_ircv3_parser_add_handlers(parser,
	                                 purple_ircv3_message_handler_status_ignore_param0,
	                                 PURPLE_IRCV3_RPL_LUSERCLIENT,
	                                 PURPLE_IRCV3_RPL_LUSEROP,
	                                 PURPLE_IRCV3_RPL_LUSERUNKNOWN,
	                                 PURPLE_IRCV3_RPL_LUSERCHANNELS,
	                                 PURPLE_IRCV3_RPL_LUSERME,
	                                 NULL);

	/* MOTD */
	purple_ircv3_parser_add_handlers(parser,
	                                 purple_ircv3_message_handler_status_ignore_param0,
	                                 PURPLE_IRCV3_RPL_MOTD,
	                                 PURPLE_IRCV3_RPL_MOTDSTART,
	                                 PURPLE_IRCV3_RPL_ENDOFMOTD,
	                                 NULL);

	/* SASL stuff. */
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_RPL_LOGGEDIN,
	                                purple_ircv3_sasl_logged_in);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_RPL_LOGGEDOUT,
	                                purple_ircv3_sasl_logged_out);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_ERR_NICKLOCKED,
	                                purple_ircv3_sasl_nick_locked);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_RPL_SASLSUCCESS,
	                                purple_ircv3_sasl_success);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_ERR_SASLFAIL,
	                                purple_ircv3_sasl_failed);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_ERR_SASLTOOLONG,
	                                purple_ircv3_sasl_message_too_long);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_ERR_SASLABORTED,
	                                purple_ircv3_sasl_aborted);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_ERR_SASLALREADY,
	                                purple_ircv3_sasl_already_authed);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_RPL_SASLMECHS,
	                                purple_ircv3_sasl_mechanisms);
	purple_ircv3_parser_add_handler(parser, PURPLE_IRCV3_MSG_AUTHENTICATE,
	                                purple_ircv3_sasl_authenticate);

}
