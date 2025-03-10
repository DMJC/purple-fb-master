/*
 * purple - Jabber Protocol Plugin
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
#include "internal.h"
#include <purple.h>

#include "chat.h"
#include "presence.h"
#include "jutil.h"

#include <idna.h>
#include <stringprep.h>
static char idn_buffer[1024];

static gboolean jabber_nodeprep(char *str, size_t buflen)
{
	return stringprep_xmpp_nodeprep(str, buflen) == STRINGPREP_OK;
}

static gboolean jabber_resourceprep(char *str, size_t buflen)
{
	return stringprep_xmpp_resourceprep(str, buflen) == STRINGPREP_OK;
}

static JabberID*
jabber_idn_validate(const char *str, const char *at, const char *slash,
                    const char *null)
{
	const char *node = NULL;
	const char *domain = NULL;
	const char *resource = NULL;
	int node_len = 0;
	int domain_len = 0;
	int resource_len = 0;
	char *out;
	JabberID *jid;

	/* Ensure no parts are > 1023 bytes */
	if (at) {
		node = str;
		node_len = at - str;

		domain = at + 1;
		if (slash) {
			domain_len = slash - (at + 1);
			resource = slash + 1;
			resource_len = null - (slash + 1);
		} else {
			domain_len = null - (at + 1);
		}
	} else {
		domain = str;

		if (slash) {
			domain_len = slash - str;
			resource = slash + 1;
			resource_len = null - (slash + 1);
		} else {
			domain_len = null - str;
		}
	}

	if (node && node_len > 1023)
		return NULL;
	if (domain_len > 1023)
		return NULL;
	if (resource && resource_len > 1023)
		return NULL;

	jid = g_new0(JabberID, 1);

	if (node) {
		strncpy(idn_buffer, node, node_len);
		idn_buffer[node_len] = '\0';

		if (!jabber_nodeprep(idn_buffer, sizeof(idn_buffer))) {
			jabber_id_free(jid);
			jid = NULL;
			goto out;
		}

		jid->node = g_strdup(idn_buffer);
	}

	/* domain *must* be here */
	strncpy(idn_buffer, domain, domain_len);
	idn_buffer[domain_len] = '\0';
	if (domain[0] == '[') { /* IPv6 address */
		gboolean valid = FALSE;

		if (domain_len > 2 && idn_buffer[domain_len - 1] == ']') {
			GInetAddress *addr;
			idn_buffer[domain_len - 1] = '\0';
			addr = g_inet_address_new_from_string(idn_buffer + 1);
			if (addr != NULL) {
				valid = (g_inet_address_get_family(addr) ==
				         G_SOCKET_FAMILY_IPV6);
				g_object_unref(addr);
			}
		}

		if (!valid) {
			jabber_id_free(jid);
			jid = NULL;
			goto out;
		}

		jid->domain = g_strndup(domain, domain_len);
	} else {
		/* Apply nameprep */
		if (stringprep_nameprep(idn_buffer, sizeof(idn_buffer)) != STRINGPREP_OK) {
			jabber_id_free(jid);
			jid = NULL;
			goto out;
		}

		/* And now ToASCII */
		if (idna_to_ascii_8z(idn_buffer, &out, IDNA_USE_STD3_ASCII_RULES) != IDNA_SUCCESS) {
			jabber_id_free(jid);
			jid = NULL;
			goto out;
		}

		/* This *MUST* be freed using 'free', not 'g_free' */
		free(out);
		jid->domain = g_strdup(idn_buffer);
	}

	if (resource) {
		strncpy(idn_buffer, resource, resource_len);
		idn_buffer[resource_len] = '\0';

		if (!jabber_resourceprep(idn_buffer, sizeof(idn_buffer))) {
			jabber_id_free(jid);
			jid = NULL;
			goto out;
		} else
			jid->resource = g_strdup(idn_buffer);
	}

out:
	return jid;
}

gboolean jabber_nodeprep_validate(const char *str)
{
	gboolean result;

	if(!str)
		return TRUE;

	if(strlen(str) > 1023)
		return FALSE;

	strncpy(idn_buffer, str, sizeof(idn_buffer) - 1);
	idn_buffer[sizeof(idn_buffer) - 1] = '\0';
	result = jabber_nodeprep(idn_buffer, sizeof(idn_buffer));
	return result;
}

gboolean jabber_domain_validate(const char *str)
{
	const char *c;
	size_t len;

	if(!str)
		return TRUE;

	len = strlen(str);
	if (len > 1023)
		return FALSE;

	c = str;

	if (*c == '[') {
		/* Check if str is a valid IPv6 identifier */
		GInetAddress *addr;
		gboolean valid = FALSE;

		if (len <= 2 || *(c + len - 1) != ']') {
			return FALSE;
		}

		/* Ugly, but in-place */
		*(gchar *)(c + len - 1) = '\0';
		addr = g_inet_address_new_from_string(c + 1);
		if (addr != NULL) {
			valid = (g_inet_address_get_family(addr) == G_SOCKET_FAMILY_IPV6);
			g_object_unref(addr);
		}
		*(gchar *)(c + len - 1) = ']';

		return valid;
	}

	while(c && *c) {
		gunichar ch = g_utf8_get_char(c);
		/* The list of characters allowed in domain names is pretty small */
		if ((ch <= 0x7F && !( (ch >= 'a' && ch <= 'z')
				|| (ch >= '0' && ch <= '9')
				|| (ch >= 'A' && ch <= 'Z')
				|| ch == '.'
				|| ch == '-' )) || (ch >= 0x80 && !g_unichar_isgraph(ch)))
			return FALSE;

		c = g_utf8_next_char(c);
	}

	return TRUE;
}

gboolean jabber_resourceprep_validate(const char *str)
{
	gboolean result;

	if(!str)
		return TRUE;

	if(strlen(str) > 1023)
		return FALSE;

	strncpy(idn_buffer, str, sizeof(idn_buffer) - 1);
	idn_buffer[sizeof(idn_buffer) - 1] = '\0';
	result = jabber_resourceprep(idn_buffer, sizeof(idn_buffer));
	return result;
}

char *jabber_saslprep(const char *in)
{
	char *out;

	g_return_val_if_fail(in != NULL, NULL);
	g_return_val_if_fail(strlen(in) <= sizeof(idn_buffer) - 1, NULL);

	strncpy(idn_buffer, in, sizeof(idn_buffer) - 1);
	idn_buffer[sizeof(idn_buffer) - 1] = '\0';

	if (STRINGPREP_OK != stringprep(idn_buffer, sizeof(idn_buffer), 0,
	                                stringprep_saslprep)) {
		memset(idn_buffer, 0, sizeof(idn_buffer));
		return NULL;
	}

	out = g_strdup(idn_buffer);
	memset(idn_buffer, 0, sizeof(idn_buffer));
	return out;
}

static JabberID*
jabber_id_new_internal(const char *str, gboolean allow_terminating_slash)
{
	const char *at = NULL;
	const char *slash = NULL;
	const char *c;
	gboolean needs_validation = FALSE;
	JabberID *jid;

	if (!str)
		return NULL;

	for (c = str; *c != '\0'; c++)
	{
		switch (*c) {
			case '@':
				if (!slash) {
					if (at) {
						/* Multiple @'s in the node/domain portion, not a valid JID! */
						return NULL;
					}
					if (c == str) {
						/* JIDs cannot start with @ */
						return NULL;
					}
					if (c[1] == '\0') {
						/* JIDs cannot end with @ */
						return NULL;
					}
					at = c;
				}
				break;

			case '/':
				if (!slash) {
					if (c == str) {
						/* JIDs cannot start with / */
						return NULL;
					}
					if (c[1] == '\0' && !allow_terminating_slash) {
						/* JIDs cannot end with / */
						return NULL;
					}
					slash = c;
				}
				break;

			default:
				/* characters allowed everywhere */
				if ((*c >= 'a' && *c <= 'z')
						|| (*c >= '0' && *c <= '9')
						|| (*c >= 'A' && *c <= 'Z')
						|| *c == '.' || *c == '-')
					/* We're good */
					break;

				/*
				 * Hmm, this character is a bit more exotic.  Better fall
				 * back to using the more expensive UTF-8 compliant
				 * stringprep functions.
				 */
				needs_validation = TRUE;
				break;
		}
	}

	if (!needs_validation) {
		/* JID is made of only ASCII characters--just lowercase and return */
		jid = g_new0(JabberID, 1);

		if (at) {
			jid->node = g_ascii_strdown(str, at - str);
			if (slash) {
				jid->domain = g_ascii_strdown(at + 1, slash - (at + 1));
				if (*(slash + 1))
					jid->resource = g_strdup(slash + 1);
			} else {
				jid->domain = g_ascii_strdown(at + 1, -1);
			}
		} else {
			if (slash) {
				jid->domain = g_ascii_strdown(str, slash - str);
				if (*(slash + 1))
					jid->resource = g_strdup(slash + 1);
			} else {
				jid->domain = g_ascii_strdown(str, -1);
			}
		}
		return jid;
	}

	/*
	 * If we get here, there are some non-ASCII chars in the string, so
	 * we'll need to validate it, normalize, and finally do a full jabber
	 * nodeprep on the jid.
	 */

	if (!g_utf8_validate(str, -1, NULL))
		return NULL;

	return jabber_idn_validate(str, at, slash, c /* points to the null */);
}

void
jabber_id_free(JabberID *jid)
{
	if(jid) {
		g_free(jid->node);
		g_free(jid->domain);
		g_free(jid->resource);
		g_free(jid);
	}
}


gboolean
jabber_id_equal(const JabberID *jid1, const JabberID *jid2)
{
	if (!jid1 && !jid2) {
		/* Both are null therefore equal */
		return TRUE;
	}

	if (!jid1 || !jid2) {
		/* One is null, other is non-null, therefore not equal */
		return FALSE;
	}

	return purple_strequal(jid1->node, jid2->node) &&
			purple_strequal(jid1->domain, jid2->domain) &&
			purple_strequal(jid1->resource, jid2->resource);
}

char *jabber_get_domain(const char *in)
{
	JabberID *jid = jabber_id_new(in);
	char *out;

	if (!jid)
		return NULL;

	out = g_strdup(jid->domain);
	jabber_id_free(jid);

	return out;
}

char *jabber_get_resource(const char *in)
{
	JabberID *jid = jabber_id_new(in);
	char *out;

	if(!jid)
		return NULL;

	out = g_strdup(jid->resource);
	jabber_id_free(jid);

	return out;
}

JabberID *
jabber_id_to_bare_jid(const JabberID *jid)
{
	JabberID *result = g_new0(JabberID, 1);

	result->node = g_strdup(jid->node);
	result->domain = g_strdup(jid->domain);

	return result;
}

char *
jabber_get_bare_jid(const char *in)
{
	JabberID *jid = jabber_id_new(in);
	char *out;

	if (!jid)
		return NULL;
	out = jabber_id_get_bare_jid(jid);
	jabber_id_free(jid);

	return out;
}

char *
jabber_id_get_bare_jid(const JabberID *jid)
{
	g_return_val_if_fail(jid != NULL, NULL);

	return g_strconcat(jid->node ? jid->node : "",
	                   jid->node ? "@" : "",
	                   jid->domain,
	                   NULL);
}

char *
jabber_id_get_full_jid(const JabberID *jid)
{
	g_return_val_if_fail(jid != NULL, NULL);

	return g_strconcat(jid->node ? jid->node : "",
	                   jid->node ? "@" : "",
	                   jid->domain,
	                   jid->resource ? "/" : "",
	                   jid->resource ? jid->resource : "",
	                   NULL);
}

gboolean
jabber_jid_is_domain(const char *jid)
{
	const char *c;

	for (c = jid; *c; ++c) {
		if (*c == '@' || *c == '/')
			return FALSE;
	}

	return TRUE;
}


JabberID *
jabber_id_new(const char *str)
{
	return jabber_id_new_internal(str, FALSE);
}

const char *jabber_normalize(const PurpleAccount *account, const char *in)
{
	PurpleConnection *gc = NULL;
	JabberStream *js = NULL;
	static char buf[3072]; /* maximum legal length of a jabber jid */
	JabberID *jid;

	if (account) {
		gc = purple_account_get_connection((PurpleAccount *)account);
	}
	if (gc)
		js = purple_connection_get_protocol_data(gc);

	jid = jabber_id_new_internal(in, TRUE);
	if(!jid)
		return NULL;

	if(js && jid->node && jid->resource &&
			jabber_chat_find(js, jid->node, jid->domain))
		g_snprintf(buf, sizeof(buf), "%s@%s/%s", jid->node, jid->domain,
				jid->resource);
	else
		g_snprintf(buf, sizeof(buf), "%s%s%s", jid->node ? jid->node : "",
				jid->node ? "@" : "", jid->domain);

	jabber_id_free(jid);

	return buf;
}

gboolean
jabber_is_own_server(JabberStream *js, const char *str)
{
	JabberID *jid;
	gboolean equal;

	if (str == NULL)
		return FALSE;

	g_return_val_if_fail(*str != '\0', FALSE);

	jid = jabber_id_new(str);
	if (!jid)
		return FALSE;

	equal = (jid->node == NULL &&
	         purple_strequal(jid->domain, js->user->domain) &&
	         jid->resource == NULL);
	jabber_id_free(jid);
	return equal;
}

gboolean
jabber_is_own_account(JabberStream *js, const char *str)
{
	JabberID *jid;
	gboolean equal;

	if (str == NULL)
		return TRUE;

	g_return_val_if_fail(*str != '\0', FALSE);

	jid = jabber_id_new(str);
	if (!jid)
		return FALSE;

	equal = (purple_strequal(jid->node, js->user->node) &&
	         purple_strequal(jid->domain, js->user->domain) &&
	         (jid->resource == NULL ||
	             purple_strequal(jid->resource, js->user->resource)));
	jabber_id_free(jid);
	return equal;
}

static const struct {
		const char *status_id; /* link to core */
		const char *show; /* The show child's cdata in a presence stanza */
		const char *readable; /* readable representation */
		JabberBuddyState state;
} jabber_statuses[] = {
	{ "offline",       NULL,   N_("Offline"),        JABBER_BUDDY_STATE_UNAVAILABLE },
	{ "available",     NULL,   N_("Available"),      JABBER_BUDDY_STATE_ONLINE},
	{ "freeforchat",   "chat", N_("Chatty"),         JABBER_BUDDY_STATE_CHAT },
	{ "away",          "away", N_("Away"),           JABBER_BUDDY_STATE_AWAY },
	{ "extended_away", "xa",   N_("Extended Away"),  JABBER_BUDDY_STATE_XA },
	{ "dnd",           "dnd",  N_("Do Not Disturb"), JABBER_BUDDY_STATE_DND },
	{ "error",         NULL,   N_("Error"),          JABBER_BUDDY_STATE_ERROR }
};

const char *
jabber_buddy_state_get_name(const JabberBuddyState state)
{
	gsize i;
	for (i = 0; i < G_N_ELEMENTS(jabber_statuses); ++i)
		if (jabber_statuses[i].state == state)
			return _(jabber_statuses[i].readable);

	return _("Unknown");
}

JabberBuddyState
jabber_buddy_status_id_get_state(const char *id)
{
	gsize i;
	if (!id)
		return JABBER_BUDDY_STATE_UNKNOWN;

	for (i = 0; i < G_N_ELEMENTS(jabber_statuses); ++i)
		if (purple_strequal(id, jabber_statuses[i].status_id))
			return jabber_statuses[i].state;

	return JABBER_BUDDY_STATE_UNKNOWN;
}

JabberBuddyState jabber_buddy_show_get_state(const char *id)
{
	gsize i;

	g_return_val_if_fail(id != NULL, JABBER_BUDDY_STATE_UNKNOWN);

	for (i = 0; i < G_N_ELEMENTS(jabber_statuses); ++i)
		if (jabber_statuses[i].show && purple_strequal(id, jabber_statuses[i].show))
			return jabber_statuses[i].state;

	purple_debug_warning("jabber", "Invalid value of presence <show/> "
	                     "attribute: %s\n", id);
	return JABBER_BUDDY_STATE_UNKNOWN;
}

const char *
jabber_buddy_state_get_show(JabberBuddyState state)
{
	gsize i;
	for (i = 0; i < G_N_ELEMENTS(jabber_statuses); ++i)
		if (state == jabber_statuses[i].state)
			return jabber_statuses[i].show;

	return NULL;
}

const char *
jabber_buddy_state_get_status_id(JabberBuddyState state)
{
	gsize i;
	for (i = 0; i < G_N_ELEMENTS(jabber_statuses); ++i)
		if (state == jabber_statuses[i].state)
			return jabber_statuses[i].status_id;

	return NULL;
}

