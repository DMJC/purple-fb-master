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

#include "auth.h"
#include "auth_scram.h"

static const JabberScramHash hashes[] = {
	{ "-SHA-1", G_CHECKSUM_SHA1 },
};

static const JabberScramHash *mech_to_hash(const char *mech)
{
	gsize i;

	g_return_val_if_fail(mech != NULL && *mech != '\0', NULL);

	for (i = 0; i < G_N_ELEMENTS(hashes); ++i) {
		if (strstr(mech, hashes[i].mech_substr))
			return &(hashes[i]);
	}

	purple_debug_error("jabber", "Unknown SCRAM mechanism %s\n", mech);
	g_return_val_if_reached(NULL);
}

guchar *jabber_scram_hi(const JabberScramHash *hash, const GString *str,
                        GString *salt, guint iterations)
{
	GHmac *hmac;
	gsize digest_len;
	guchar *result;
	guint i;
	guchar *prev, *tmp;

	g_return_val_if_fail(hash != NULL, NULL);
	g_return_val_if_fail(str != NULL && str->len > 0, NULL);
	g_return_val_if_fail(salt != NULL && salt->len > 0, NULL);
	g_return_val_if_fail(iterations > 0, NULL);

	digest_len = g_checksum_type_get_length(hash->type);
	prev   = g_new0(guchar, digest_len);
	tmp    = g_new0(guchar, digest_len);
	result = g_new0(guchar, digest_len);

	hmac = g_hmac_new(hash->type, (guchar *)str->str, str->len);

	/* Append INT(1), a four-octet encoding of the integer 1, most significant
	 * octet first. */
	g_string_append_len(salt, "\0\0\0\1", 4);

	/* Compute U0 */
	g_hmac_update(hmac, (guchar *)salt->str, salt->len);
	g_hmac_get_digest(hmac, result, &digest_len);
	g_hmac_unref(hmac);

	memcpy(prev, result, digest_len);

	/* Compute U1...Ui */
	for (i = 1; i < iterations; ++i) {
		guint j;
		hmac = g_hmac_new(hash->type, (guchar *)str->str, str->len);
		g_hmac_update(hmac, prev, digest_len);
		g_hmac_get_digest(hmac, tmp, &digest_len);
		g_hmac_unref(hmac);

		for (j = 0; j < digest_len; ++j)
			result[j] ^= tmp[j];

		memcpy(prev, tmp, digest_len);
	}

	g_free(tmp);
	g_free(prev);
	return result;
}

/*
 * Helper functions for doing the SCRAM calculations. The first argument
 * is the hash algorithm.  All buffers must be of the appropriate size
 * according to the JabberScramHash.
 *
 * "str" is a NULL-terminated string for jabber_scram_hmac().
 *
 * Needless to say, these are fragile.
 */
static void
jabber_scram_hmac(const JabberScramHash *hash, guchar *out, const guchar *key, const gchar *str)
{
	GHmac *hmac;
	gsize digest_len = g_checksum_type_get_length(hash->type);

	hmac = g_hmac_new(hash->type, key, digest_len);
	g_hmac_update(hmac, (guchar *)str, -1);
	g_hmac_get_digest(hmac, out, &digest_len);
	g_hmac_unref(hmac);
}

static void
jabber_scram_hash(const JabberScramHash *hash, guchar *out, const guchar *data)
{
	GChecksum *checksum;
	gsize digest_len = g_checksum_type_get_length(hash->type);

	checksum = g_checksum_new(hash->type);
	g_checksum_update(checksum, data, digest_len);
	g_checksum_get_digest(checksum, out, &digest_len);
	g_checksum_free(checksum);
}

gboolean
jabber_scram_calc_proofs(JabberScramData *data, GString *salt, guint iterations)
{
	guint hash_len = g_checksum_type_get_length(data->hash->type);
	guint i;

	GString *pass = g_string_new(data->password);

	guchar *salted_password;
	guchar *client_key, *stored_key, *client_signature, *server_key;

	data->client_proof = g_string_sized_new(hash_len);
	data->client_proof->len = hash_len;
	data->server_signature = g_string_sized_new(hash_len);
	data->server_signature->len = hash_len;

	salted_password = jabber_scram_hi(data->hash, pass, salt, iterations);

	memset(pass->str, 0, pass->allocated_len);
	g_string_free(pass, TRUE);

	if (!salted_password)
		return FALSE;

	client_key = g_new0(guchar, hash_len);
	stored_key = g_new0(guchar, hash_len);
	client_signature = g_new0(guchar, hash_len);
	server_key = g_new0(guchar, hash_len);

	/* client_key = HMAC(salted_password, "Client Key") */
	jabber_scram_hmac(data->hash, client_key, salted_password, "Client Key");
	/* server_key = HMAC(salted_password, "Server Key") */
	jabber_scram_hmac(data->hash, server_key, salted_password, "Server Key");
	g_free(salted_password);

	/* stored_key = HASH(client_key) */
	jabber_scram_hash(data->hash, stored_key, client_key);

	/* client_signature = HMAC(stored_key, auth_message) */
	jabber_scram_hmac(data->hash, client_signature, stored_key, data->auth_message->str);
	/* server_signature = HMAC(server_key, auth_message) */
	jabber_scram_hmac(data->hash, (guchar *)data->server_signature->str, server_key, data->auth_message->str);

	/* client_proof = client_key XOR client_signature */
	for (i = 0; i < hash_len; ++i)
		data->client_proof->str[i] = client_key[i] ^ client_signature[i];

	g_free(server_key);
	g_free(client_signature);
	g_free(stored_key);
	g_free(client_key);

	return TRUE;
}

static gboolean
parse_server_step1(JabberScramData *data, const char *challenge,
                   gchar **out_nonce, GString **out_salt, guint *out_iterations)
{
	char **tokens;
	char *token, *decoded, *tmp;
	gsize len;
	char *nonce = NULL;
	GString *salt = NULL;
	guint iterations;

	tokens = g_strsplit(challenge, ",", -1);
	if (tokens == NULL)
		return FALSE;

	token = tokens[0];
	if (token[0] != 'r' || token[1] != '=')
		goto err;

	/* Ensure that the first cnonce_len bytes of the nonce are the original
	 * cnonce we sent to the server.
	 */
	if (0 != strncmp(data->cnonce, token + 2, strlen(data->cnonce)))
		goto err;

	nonce = g_strdup(token + 2);

	/* The Salt, base64-encoded */
	token = tokens[1];
	if (token[0] != 's' || token[1] != '=')
		goto err;

	decoded = (gchar *)g_base64_decode(token + 2, &len);
	if (!decoded || *decoded == '\0') {
		g_free(decoded);
		goto err;
	}
	salt = g_string_new_len(decoded, len);
	g_free(decoded);

	/* The iteration count */
	token = tokens[2];
	if (token[0] != 'i' || token[1] != '=' || token[2] == '\0')
		goto err;

	/* Validate the string */
	for (tmp = token + 2; *tmp; ++tmp)
		if (!g_ascii_isdigit(*tmp))
			goto err;

	iterations = strtoul(token + 2, NULL, 10);

	g_strfreev(tokens);
	*out_nonce = nonce;
	*out_salt = salt;
	*out_iterations = iterations;
	return TRUE;

err:
	g_free(nonce);
	if (salt)
		g_string_free(salt, TRUE);
	g_strfreev(tokens);
	return FALSE;
}

static gboolean
parse_server_step2(JabberScramData *data, const char *challenge, gchar **out_verifier)
{
	char **tokens;
	char *token;

	tokens = g_strsplit(challenge, ",", -1);
	if (tokens == NULL)
		return FALSE;

	token = tokens[0];
	if (token[0] != 'v' || token[1] != '=' || token[2] == '\0') {
		g_strfreev(tokens);
		return FALSE;
	}

	*out_verifier = g_strdup(token + 2);
	g_strfreev(tokens);
	return TRUE;
}

gboolean
jabber_scram_feed_parser(JabberScramData *data, gchar *in, gchar **out)
{
	gboolean ret;

	g_return_val_if_fail(data != NULL, FALSE);

	g_string_append_c(data->auth_message, ',');
	g_string_append(data->auth_message, in);

	if (data->step == 1) {
		gchar *nonce, *proof;
		GString *salt;
		guint iterations;

		ret = parse_server_step1(data, in, &nonce, &salt, &iterations);
		if (!ret)
			return FALSE;

		g_string_append_c(data->auth_message, ',');

		/* "biws" is the base64 encoding of "n,,". I promise. */
		g_string_append_printf(data->auth_message, "c=%s,r=%s", "biws", nonce);
#ifdef CHANNEL_BINDING
#error fix this
#endif

		ret = jabber_scram_calc_proofs(data, salt, iterations);

		g_string_free(salt, TRUE);
		salt = NULL;
		if (!ret) {
			g_free(nonce);
			return FALSE;
		}

		proof = g_base64_encode((guchar *)data->client_proof->str, data->client_proof->len);
		*out = g_strdup_printf("c=%s,r=%s,p=%s", "biws", nonce, proof);
		g_free(nonce);
		g_free(proof);
	} else if (data->step == 2) {
		gchar *server_sig, *enc_server_sig;
		gsize len;

		ret = parse_server_step2(data, in, &enc_server_sig);
		if (!ret)
			return FALSE;

		server_sig = (gchar *)g_base64_decode(enc_server_sig, &len);
		g_free(enc_server_sig);

		if (server_sig == NULL || len != data->server_signature->len) {
			g_free(server_sig);
			return FALSE;
		}

		if (0 != memcmp(server_sig, data->server_signature->str, len)) {
			g_free(server_sig);
			return FALSE;
		}
		g_free(server_sig);

		*out = NULL;
	} else {
		purple_debug_error("jabber", "SCRAM: There is no step %d\n", data->step);
		return FALSE;
	}

	return TRUE;
}

static gchar *escape_username(const gchar *in)
{
	gchar *tmp, *tmp2;

	tmp = purple_strreplace(in, "=", "=3D");
	tmp2 = purple_strreplace(tmp, ",", "=2C");
	g_free(tmp);
	return tmp2;
}

static JabberSaslState
scram_start(JabberStream *js, PurpleXmlNode *mechanisms, PurpleXmlNode **out, char **error)
{
	PurpleXmlNode *reply;
	JabberScramData *data;
	guint64 cnonce;
#ifdef CHANNEL_BINDING
	gboolean binding_supported = TRUE;
#endif
	gchar *dec_out, *enc_out;
	gchar *prepped_node, *tmp;
	gchar *prepped_pass;

	prepped_node = jabber_saslprep(js->user->node);
	if (!prepped_node) {
		*error = g_strdup(_("Unable to canonicalize username"));
		return JABBER_SASL_STATE_FAIL;
	}

	tmp = escape_username(prepped_node);
	g_free(prepped_node);
	prepped_node = tmp;

	prepped_pass = jabber_saslprep(purple_connection_get_password(js->gc));
	if (!prepped_pass) {
		g_free(prepped_node);
		*error = g_strdup(_("Unable to canonicalize password"));
		return JABBER_SASL_STATE_FAIL;
	}

	data = js->auth_mech_data = g_new0(JabberScramData, 1);
	data->hash = mech_to_hash(js->auth_mech->name);
	data->password = prepped_pass;

#ifdef CHANNEL_BINDING
	if (strstr(js->auth_mech_name, "-PLUS"))
		data->channel_binding = TRUE;
#endif
	cnonce = ((guint64)g_random_int() << 32) | g_random_int();
	data->cnonce = g_base64_encode((guchar *)&cnonce, sizeof(cnonce));

	data->auth_message = g_string_new(NULL);
	g_string_printf(data->auth_message, "n=%s,r=%s",
			prepped_node, data->cnonce);
	g_free(prepped_node);

	data->step = 1;

	reply = purple_xmlnode_new("auth");
	purple_xmlnode_set_namespace(reply, NS_XMPP_SASL);
	purple_xmlnode_set_attrib(reply, "mechanism", js->auth_mech->name);

	/* TODO: Channel binding */
	dec_out = g_strdup_printf("%c,,%s", 'n', data->auth_message->str);
	enc_out = g_base64_encode((guchar *)dec_out, strlen(dec_out));
	purple_debug_misc("jabber", "initial SCRAM message '%s'\n", dec_out);

	purple_xmlnode_insert_data(reply, enc_out, -1);

	g_free(enc_out);
	g_free(dec_out);

	*out = reply;
	return JABBER_SASL_STATE_CONTINUE;
}

static JabberSaslState
scram_handle_challenge(JabberStream *js, PurpleXmlNode *challenge, PurpleXmlNode **out, char **error)
{
	JabberScramData *data = js->auth_mech_data;
	PurpleXmlNode *reply;
	gchar *enc_in, *dec_in = NULL;
	gchar *enc_out = NULL, *dec_out = NULL;
	gsize len;
	JabberSaslState state = JABBER_SASL_STATE_FAIL;

	enc_in = purple_xmlnode_get_data(challenge);
	if (!enc_in || *enc_in == '\0') {
		reply = purple_xmlnode_new("abort");
		purple_xmlnode_set_namespace(reply, NS_XMPP_SASL);
		data->step = -1;
		*error = g_strdup(_("Invalid challenge from server"));
		goto out;
	}

	dec_in = (gchar *)g_base64_decode(enc_in, &len);
	if (!dec_in || len != strlen(dec_in)) {
		/* Danger afoot; SCRAM shouldn't contain NUL bytes */
		reply = purple_xmlnode_new("abort");
		purple_xmlnode_set_namespace(reply, NS_XMPP_SASL);
		data->step = -1;
		*error = g_strdup(_("Malicious challenge from server"));
		goto out;
	}

	purple_debug_misc("jabber", "decoded challenge: %s\n", dec_in);

	if (!jabber_scram_feed_parser(data, dec_in, &dec_out)) {
		reply = purple_xmlnode_new("abort");
		purple_xmlnode_set_namespace(reply, NS_XMPP_SASL);
		data->step = -1;
		*error = g_strdup(_("Invalid challenge from server"));
		goto out;
	}

	data->step += 1;

	reply = purple_xmlnode_new("response");
	purple_xmlnode_set_namespace(reply, NS_XMPP_SASL);

	purple_debug_misc("jabber", "decoded response: %s\n", dec_out ? dec_out : "(null)");
	if (dec_out) {
		enc_out = g_base64_encode((guchar *)dec_out, strlen(dec_out));
		purple_xmlnode_insert_data(reply, enc_out, -1);
	}

	state = JABBER_SASL_STATE_CONTINUE;

out:
	g_free(enc_in);
	g_free(dec_in);
	g_free(enc_out);
	g_free(dec_out);

	*out = reply;
	return state;
}

static JabberSaslState
scram_handle_success(JabberStream *js, PurpleXmlNode *packet, char **error)
{
	JabberScramData *data = js->auth_mech_data;
	char *enc_in, *dec_in;
	char *dec_out = NULL;
	gsize len;

	enc_in = purple_xmlnode_get_data(packet);
	if (data->step != 3 && (!enc_in || *enc_in == '\0')) {
		*error = g_strdup(_("Invalid challenge from server"));
		g_free(enc_in);
		return JABBER_SASL_STATE_FAIL;
	}

	if (data->step == 3) {
		/*
		 * If the server took the slow approach (sending the verifier
		 * as a challenge/response pair), we get here.
		 */
		g_free(enc_in);
		return JABBER_SASL_STATE_OK;
	}

	if (data->step != 2) {
		*error = g_strdup(_("Unexpected response from server"));
		g_free(enc_in);
		return JABBER_SASL_STATE_FAIL;
	}

	dec_in = (gchar *)g_base64_decode(enc_in, &len);
	g_free(enc_in);
	if (!dec_in || len != strlen(dec_in)) {
		/* Danger afoot; SCRAM shouldn't contain NUL bytes */
		g_free(dec_in);
		*error = g_strdup(_("Malicious challenge from server"));
		return JABBER_SASL_STATE_FAIL;
	}

	purple_debug_misc("jabber", "decoded success: %s\n", dec_in);

	if (!jabber_scram_feed_parser(data, dec_in, &dec_out) || dec_out != NULL) {
		g_free(dec_in);
		g_free(dec_out);
		*error = g_strdup(_("Invalid challenge from server"));
		return JABBER_SASL_STATE_FAIL;
	}

	g_free(dec_in);
	/* Hooray */
	return JABBER_SASL_STATE_OK;
}

void jabber_scram_data_destroy(JabberScramData *data)
{
	g_free(data->cnonce);
	if (data->auth_message)
		g_string_free(data->auth_message, TRUE);
	if (data->client_proof)
		g_string_free(data->client_proof, TRUE);
	if (data->server_signature)
		g_string_free(data->server_signature, TRUE);
	if (data->password) {
		memset(data->password, 0, strlen(data->password));
		g_free(data->password);
	}

	g_free(data);
}

static void scram_dispose(JabberStream *js)
{
	if (js->auth_mech_data) {
		jabber_scram_data_destroy(js->auth_mech_data);
		js->auth_mech_data = NULL;
	}
}

static JabberSaslMech scram_sha1_mech = {
	50, /* priority */
	"SCRAM-SHA-1", /* name */
	scram_start,
	scram_handle_challenge,
	scram_handle_success,
	NULL, /* handle_failure */
	scram_dispose
};

#ifdef CHANNEL_BINDING
/* With channel binding */
static JabberSaslMech scram_sha1_plus_mech = {
	scram_sha1_mech.priority + 1, /* priority */
	"SCRAM-SHA-1-PLUS", /* name */
	scram_start,
	scram_handle_challenge,
	scram_handle_success,
	NULL, /* handle_failure */
	scram_dispose
};
#endif

JabberSaslMech **jabber_auth_get_scram_mechs(gint *count)
{
	static JabberSaslMech *mechs[] = {
		&scram_sha1_mech,
#ifdef CHANNEL_BINDING
		&scram_sha1_plus_mech,
#endif
	};

	*count = G_N_ELEMENTS(mechs);
	return mechs;
}
