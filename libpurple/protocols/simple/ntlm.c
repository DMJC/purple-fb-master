/*
 * purple
 *
 * Copyright (C) 2005 Thomas Butter <butter@uni-mannheim.de>
 *
 * hashing done according to description of NTLM on
 * http://www.innovation.ch/java/ntlm.html
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

#include "ntlm.h"

#ifdef HAVE_NETTLE
#include <nettle/des.h>
#include <nettle/md4.h>
#include <nettle/yarrow.h>
#endif

#include <string.h>

#define NTLM_NEGOTIATE_NTLM2_KEY 0x00080000

struct type2_message {
	guint8  protocol[8];     /* 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'*/
	guint32 type;            /* 0x00000002 */

	guint32 zero;
	guint16 msg_len1;        /* target name length */
	guint16 msg_len2;        /* target name length */

	guint32 flags;           /* 0x00008201 */

	guint8  nonce[8];        /* nonce */
	guint8  context[8];
};

struct type3_message {
	guint8  protocol[8];     /* 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'*/
	guint32 type;            /* 0x00000003 */

	guint16 lm_resp_len1;    /* LanManager response length (always 0x18)*/
	guint16 lm_resp_len2;    /* LanManager response length (always 0x18)*/
	guint32 lm_resp_off;     /* LanManager response offset */

	guint16 nt_resp_len1;    /* NT response length (always 0x18) */
	guint16 nt_resp_len2;    /* NT response length (always 0x18) */
	guint32 nt_resp_off;     /* NT response offset */

	guint16 dom_len1;        /* domain string length */
	guint16 dom_len2;        /* domain string length */
	guint32 dom_off;         /* domain string offset (always 0x00000040) */

	guint16 user_len1;       /* username string length */
	guint16 user_len2;       /* username string length */
	guint32 user_off;        /* username string offset */

	guint16 host_len1;       /* host string length */
	guint16 host_len2;       /* host string length */
	guint32 host_off;        /* host string offset */

	guint16 sess_len1;
	guint16 sess_len2;
	guint32 sess_off;         /* message length */

	guint32 flags;            /* 0x00008201 */
	/* guint32 flags2; */     /* unknown, used in windows messenger */
	/* guint32 flags3; */
};

guint8 *
purple_ntlm_parse_type2(const gchar *type2, guint32 *flags)
{
	gsize retlen;
	guchar *buff;
	struct type2_message tmsg;
	static guint8 nonce[8];

	buff = g_base64_decode(type2, &retlen);

	if (buff != NULL && retlen >= (sizeof(struct type2_message) - 1)) {
		memcpy(&tmsg, buff, MIN(retlen, sizeof(tmsg)));
		memcpy(nonce, tmsg.nonce, 8);
		if (flags != NULL)
			*flags = GUINT16_FROM_LE(tmsg.flags);
	} else {
		purple_debug_error("ntlm", "Unable to parse type2 message - returning empty nonce.\n");
		memset(nonce, 0, 8);
	}
	g_free(buff);

	return nonce;
}

#ifdef HAVE_NETTLE
/*
 * Create a 64bit DES key by taking a 56bit key and adding
 * a parity bit after every 7th bit.
 */
static void
setup_des_key(const guint8 key_56[], guint8 *key)
{
	key[0] = key_56[0];
	key[1] = ((key_56[0] << 7) & 0xFF) | (key_56[1] >> 1);
	key[2] = ((key_56[1] << 6) & 0xFF) | (key_56[2] >> 2);
	key[3] = ((key_56[2] << 5) & 0xFF) | (key_56[3] >> 3);
	key[4] = ((key_56[3] << 4) & 0xFF) | (key_56[4] >> 4);
	key[5] = ((key_56[4] << 3) & 0xFF) | (key_56[5] >> 5);
	key[6] = ((key_56[5] << 2) & 0xFF) | (key_56[6] >> 6);
	key[7] =  (key_56[6] << 1) & 0xFF;
}

/*
 * helper function for des encryption
 */
static void
des_ecb_encrypt(const guint8 *plaintext, guint8 *result, const guint8 *key)
{
	struct des_ctx ctx;

	des_set_key(&ctx, key);
	des_encrypt(&ctx, DES_BLOCK_SIZE, result, plaintext);
}

/*
 * takes a 21 byte array and treats it as 3 56-bit DES keys. The
 * 8 byte plaintext is encrypted with each key and the resulting 24
 * bytes are stored in the results array.
 */
static void
calc_resp(guint8 *keys, const guint8 *plaintext, unsigned char *results)
{
	guint8 key[8];
	setup_des_key(keys, key);
	des_ecb_encrypt(plaintext, results, key);

	setup_des_key(keys + 7, key);
	des_ecb_encrypt(plaintext, results + 8, key);

	setup_des_key(keys + 14, key);
	des_ecb_encrypt(plaintext, results + 16, key);
}

static void
gensesskey(char *buffer)
{
	int fd;
	int i;
	ssize_t bytes_seeded = 0;
	uint8_t seed[YARROW256_SEED_FILE_SIZE];
	struct yarrow256_ctx ctx;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0) {
		bytes_seeded = read(fd, seed, YARROW256_SEED_FILE_SIZE);
		if (bytes_seeded < 0) {
			bytes_seeded = 0;
		}
		close(fd);
	}

	for (i = bytes_seeded; i < YARROW256_SEED_FILE_SIZE; i++) {
		seed[i] = (char)(g_random_int() & 0xff);
	}

	yarrow256_init(&ctx, 0, NULL);
	yarrow256_seed(&ctx, YARROW256_SEED_FILE_SIZE, seed);
	yarrow256_random(&ctx, YARROW256_SEED_FILE_SIZE, seed);
	yarrow256_random(&ctx, 16, (uint8_t *)buffer);
}
#endif /* HAVE_NETTLE */

gchar *
purple_ntlm_gen_type3(const gchar *username, const gchar *passw, const gchar *hostname, const gchar *domain, const guint8 *nonce, guint32 *flags)
{
#ifdef HAVE_NETTLE
	char lm_pw[14];
	unsigned char lm_hpw[21];
	char sesskey[16];
	guint8 key[8];
	int domainlen;
	int usernamelen;
	int hostnamelen;
	int msglen;
	struct type3_message *tmsg;
	int passwlen, lennt;
	unsigned char lm_resp[24], nt_resp[24];
	unsigned char magic[] = { 0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25 };
	unsigned char nt_hpw[21];
	char nt_pw[128];
	struct md4_ctx ctx;
	char *tmp;
	int idx;
	gchar *ucs2le;

	domainlen = strlen(domain) * 2;
	usernamelen = strlen(username) * 2;
	hostnamelen = strlen(hostname) * 2;
	msglen = sizeof(struct type3_message) + domainlen +
		usernamelen + hostnamelen + 0x18 + 0x18 + ((flags) ? 0x10 : 0);
	tmsg = g_malloc0(msglen);
	passwlen = strlen(passw);

	/* type3 message initialization */
	tmsg->protocol[0] = 'N';
	tmsg->protocol[1] = 'T';
	tmsg->protocol[2] = 'L';
	tmsg->protocol[3] = 'M';
	tmsg->protocol[4] = 'S';
	tmsg->protocol[5] = 'S';
	tmsg->protocol[6] = 'P';
	tmsg->type = GUINT32_TO_LE(0x00000003);
	tmsg->lm_resp_len1 = tmsg->lm_resp_len2 = GUINT16_TO_LE(0x18);
	tmsg->lm_resp_off = GUINT32_TO_LE(sizeof(struct type3_message) + domainlen + usernamelen + hostnamelen);
	tmsg->nt_resp_len1 = tmsg->nt_resp_len2 = GUINT16_TO_LE(0x18);
	tmsg->nt_resp_off = GUINT32_TO_LE(sizeof(struct type3_message) + domainlen + usernamelen + hostnamelen + 0x18);

	tmsg->dom_len1 = tmsg->dom_len2 = GUINT16_TO_LE(domainlen);
	tmsg->dom_off = GUINT32_TO_LE(sizeof(struct type3_message));

	tmsg->user_len1 = tmsg->user_len2 = GUINT16_TO_LE(usernamelen);
	tmsg->user_off = GUINT32_TO_LE(sizeof(struct type3_message) + domainlen);

	tmsg->host_len1 = tmsg->host_len2 = GUINT16_TO_LE(hostnamelen);
	tmsg->host_off = GUINT32_TO_LE(sizeof(struct type3_message) + domainlen + usernamelen);

	if(flags) {
		tmsg->sess_off = GUINT32_TO_LE(sizeof(struct type3_message) + domainlen + usernamelen + hostnamelen + 0x18 + 0x18);
		tmsg->sess_len1 = tmsg->sess_len2 = GUINT16_TO_LE(0x0010);
	}

	tmsg->flags = GUINT32_TO_LE(0x00008201);

	tmp = (char *)tmsg + sizeof(struct type3_message);

	ucs2le = g_convert(domain, -1, "UTF-16LE", "UTF-8", NULL, NULL, NULL);
	if (ucs2le != NULL) {
		memcpy(tmp, ucs2le, domainlen);
		g_free(ucs2le);
		tmp += domainlen;
	} else {
		purple_debug_info("ntlm", "Unable to encode domain in UTF-16LE.\n");
	}

	ucs2le = g_convert(username, -1, "UTF-16LE", "UTF-8", NULL, NULL, NULL);
	if (ucs2le != NULL) {
		memcpy(tmp, ucs2le, usernamelen);
		g_free(ucs2le);
		tmp += usernamelen;
	} else {
		purple_debug_info("ntlm", "Unable to encode username in UTF-16LE.\n");
	}

	ucs2le = g_convert(hostname, -1, "UTF-16LE", "UTF-8", NULL, NULL, NULL);
	if (ucs2le != NULL) {
		memcpy(tmp, ucs2le, hostnamelen);
		g_free(ucs2le);
		tmp += hostnamelen;
	} else {
		purple_debug_info("ntlm", "Unable to encode hostname in UTF-16LE.\n");
	}

	/* LM */
	if (passwlen > 14)
		passwlen = 14;

	for (idx = 0; idx < passwlen; idx++)
		lm_pw[idx] = g_ascii_toupper(passw[idx]);
	for (; idx < 14; idx++)
		lm_pw[idx] = 0;

	setup_des_key((unsigned char*)lm_pw, key);
	des_ecb_encrypt(magic, lm_hpw, key);

	setup_des_key((unsigned char*)(lm_pw + 7), key);
	des_ecb_encrypt(magic, lm_hpw + 8, key);

	memset(lm_hpw + 16, 0, 5);
	calc_resp(lm_hpw, nonce, lm_resp);
	memcpy(tmp, lm_resp, 0x18);
	tmp += 0x18;

	/* NTLM */
	/* Convert the password to UTF-16LE */
	lennt = strlen(passw);
	for (idx = 0; idx < lennt; idx++)
	{
		nt_pw[2 * idx]   = passw[idx];
		nt_pw[2 * idx + 1] = 0;
	}

	md4_init(&ctx);
	md4_update(&ctx, 2 * lennt, (uint8_t *)nt_pw);
	md4_digest(&ctx, MD4_DIGEST_SIZE, nt_hpw);

	memset(nt_hpw + 16, 0, 5);
	calc_resp(nt_hpw, nonce, nt_resp);
	memcpy(tmp, nt_resp, 0x18);
	tmp += 0x18;

	/* LCS Stuff */
	if (flags) {
		tmsg->flags = GUINT32_TO_LE(0x409082d4);
		gensesskey(sesskey);
		memcpy(tmp, sesskey, 0x10);
	}

	/*tmsg->flags2 = 0x0a280105;
	tmsg->flags3 = 0x0f000000;*/

	tmp = g_base64_encode((guchar *)tmsg, msglen);
	g_free(tmsg);

	return tmp;
#else
	/* Used without support enabled */
	g_return_val_if_reached(NULL);
#endif /* HAVE_NETTLE */
}
