/**
 * purple
 *
 * Copyright (C) 2005 Thomas Butter <butter@uni-mannheim.de>
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
 */

#include "internal.h"
#include <purple.h>

#include "simple.h"
#include "sipmsg.h"

#define MAX_CONTENT_LENGTH 30000000

struct sipmsg *sipmsg_parse_msg(const gchar *msg) {
	const char *tmp = strstr(msg, "\r\n\r\n");
	char *line;
	struct sipmsg *smsg;

	if(!tmp) return NULL;

	line = g_strndup(msg, tmp - msg);

	smsg = sipmsg_parse_header(line);
	if(smsg != NULL)
		smsg->body = g_strdup(tmp + 4);
	else
		purple_debug_error("SIMPLE", "No header parsed from line: %s\n", line);

	g_free(line);
	return smsg;
}

struct sipmsg *sipmsg_parse_header(const gchar *header) {
	struct sipmsg *msg;
	gchar **parts, **lines;
	gchar *dummy, *dummy2, *tmp;
	const gchar *tmp2;
	int i = 1;

	lines = g_strsplit(header,"\r\n",0);
	if(!lines[0]) {
		g_strfreev(lines);
		return NULL;
	}

	parts = g_strsplit(lines[0], " ", 3);
	if(!parts[0] || !parts[1] || !parts[2]) {
		g_strfreev(parts);
		g_strfreev(lines);
		return NULL;
	}

	msg = g_new0(struct sipmsg,1);
	if(strstr(parts[0],"SIP")) { /* numeric response */
		msg->method = g_strdup(parts[2]);
		msg->response = strtol(parts[1],NULL,10);
	} else { /* request */
		msg->method = g_strdup(parts[0]);
		msg->target = g_strdup(parts[1]);
		msg->response = 0;
	}
	g_strfreev(parts);

	for(i=1; lines[i] && strlen(lines[i])>2; i++) {
		parts = g_strsplit(lines[i], ":", 2);
		if(!parts[0] || !parts[1]) {
			g_strfreev(parts);
			g_strfreev(lines);
			sipmsg_free(msg);
			return NULL;
		}
		dummy = parts[1];
		dummy2 = NULL;
		while(*dummy==' ' || *dummy=='\t') dummy++;
		dummy2 = g_strdup(dummy);
		while(lines[i+1] && (lines[i+1][0]==' ' || lines[i+1][0]=='\t')) {
			i++;
			dummy = lines[i];
			while(*dummy==' ' || *dummy=='\t') dummy++;
			tmp = g_strdup_printf("%s %s",dummy2, dummy);
			g_free(dummy2);
			dummy2 = tmp;
		}
		sipmsg_add_header(msg, parts[0], dummy2);
		g_free(dummy2);
		g_strfreev(parts);
	}
	g_strfreev(lines);

	tmp2 = sipmsg_find_header(msg, "Content-Length");
	if (tmp2 != NULL)
		msg->bodylen = strtol(tmp2, NULL, 10);
	if (msg->bodylen < 0) {
		purple_debug_warning("simple", "Invalid body length: %d",
			msg->bodylen);
		msg->bodylen = 0;
	} else if (msg->bodylen > MAX_CONTENT_LENGTH) {
		purple_debug_warning("simple", "Got Content-Length of %d bytes on "
				"incoming message (max is %u bytes). Ignoring message body.\n",
				msg->bodylen, MAX_CONTENT_LENGTH);
		msg->bodylen = 0;
	}

	if(msg->response) {
		tmp2 = sipmsg_find_header(msg, "CSeq");
		g_free(msg->method);
		if(!tmp2) {
			/* SHOULD NOT HAPPEN */
			msg->method = NULL;
		} else {
			parts = g_strsplit(tmp2, " ", 2);
			msg->method = g_strdup(parts[1]);
			g_strfreev(parts);
		}
	}

	return msg;
}

void sipmsg_print(const struct sipmsg *msg) {
	GSList *cur;
	PurpleKeyValuePair *elem;
	purple_debug_misc("simple", "SIP MSG");
	purple_debug_misc("simple", "response: %d\nmethod: %s\nbodylen: %d",
	                  msg->response, msg->method, msg->bodylen);
	if (msg->target) {
		purple_debug_misc("simple", "target: %s", msg->target);
	}
	cur = msg->headers;
	while(cur) {
		elem = cur->data;
		purple_debug_misc("simple", "name: %s value: %s", elem->key,
		                  (gchar *)elem->value);
		cur = g_slist_next(cur);
	}
}

gchar *
sipmsg_to_string(const struct sipmsg *msg, const gchar *status_text)
{
	GSList *cur;
	GString *outstr = g_string_new("");
	PurpleKeyValuePair *elem;

	if (msg->response) {
		g_string_append_printf(outstr, "SIP/2.0 %d %s\r\n", msg->response,
		                       status_text ? status_text : "Unknown");
	} else {
		g_string_append_printf(outstr, "%s %s SIP/2.0\r\n",
			msg->method, msg->target);
	}

	cur = msg->headers;
	while(cur) {
		elem = cur->data;
		g_string_append_printf(outstr, "%s: %s\r\n", elem->key,
		                       (gchar *)elem->value);
		cur = g_slist_next(cur);
	}

	g_string_append_printf(outstr, "\r\n%s", msg->bodylen ? msg->body : "");

	return g_string_free(outstr, FALSE);
}

void
sipmsg_add_header(struct sipmsg *msg, const gchar *name, const gchar *value)
{
	PurpleKeyValuePair *element =
	        purple_key_value_pair_new_full(name, g_strdup(value), g_free);
	msg->headers = g_slist_append(msg->headers, element);
}

void sipmsg_free(struct sipmsg *msg) {
	g_slist_free_full(msg->headers, (GDestroyNotify)purple_key_value_pair_free);
	g_free(msg->method);
	g_free(msg->target);
	g_free(msg->body);
	g_free(msg);
}

static gint
compare_header_names(gconstpointer a, gconstpointer b)
{
	const PurpleKeyValuePair *kvpa = a;
	const gchar *name = b;
	return g_ascii_strcasecmp(kvpa->key, name);
}

void
sipmsg_remove_header(struct sipmsg *msg, const gchar *name)
{
	GSList *tmp = g_slist_find_custom(msg->headers, name, compare_header_names);
	if (tmp) {
		PurpleKeyValuePair *elem = tmp->data;
		msg->headers = g_slist_delete_link(msg->headers, tmp);
		purple_key_value_pair_free(elem);
	}
}

const gchar *
sipmsg_find_header(struct sipmsg *msg, const gchar *name)
{
	GSList *tmp = g_slist_find_custom(msg->headers, name, compare_header_names);
	if (tmp) {
		PurpleKeyValuePair *elem = tmp->data;
		return elem->value;
	}
	return NULL;
}
