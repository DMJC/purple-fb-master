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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */

#include "internal.h"
#include <purple.h>

#include <libsoup/soup.h>

#include "useravatar.h"
#include "pep.h"

#define MAX_HTTP_BUDDYICON_BYTES (200 * 1024)

static void update_buddy_metadata(JabberStream *js, const char *from, PurpleXmlNode *items);

void jabber_avatar_init(void)
{
	jabber_add_feature(NS_AVATAR_1_1_METADATA,
	                   jabber_pep_namespace_only_when_pep_enabled_cb);
	jabber_add_feature(NS_AVATAR_1_1_DATA,
	                   jabber_pep_namespace_only_when_pep_enabled_cb);

	jabber_pep_register_handler(NS_AVATAR_1_1_METADATA,
	                            update_buddy_metadata);
}

static void
remove_avatar_0_12_nodes(JabberStream *js)
{
	/*
	 * This causes ejabberd 2.0.0 to kill the connection unceremoniously.
	 * See https://support.process-one.net/browse/EJAB-623. When adiumx.com
	 * was upgraded, the issue went away.
	 *
	 * I think it makes a lot of sense to not have an avatar at the old
	 * node instead of having something interpreted as "no avatar". When
	 * a contact with an older client logs in, in the latter situation,
	 * there's a race between interpreting the <presence/> vcard-temp:x:update
	 * avatar (non-empty) and the XEP-0084 v0.12 avatar (empty, so show no
	 * avatar for the buddy) which leads to unhappy and confused users.
	 *
	 * A deluge of frustrating "Read error" bug reports may change my mind
	 * about this.
	 * --darkrain42
	 */
	jabber_pep_delete_node(js, NS_AVATAR_0_12_METADATA);
	jabber_pep_delete_node(js, NS_AVATAR_0_12_DATA);
}

void jabber_avatar_set(JabberStream *js, PurpleImage *img)
{
	PurpleXmlNode *publish, *metadata, *item;

	if (!js->pep)
		return;

	/* Hmmm, not sure if this is worth the traffic, but meh */
	remove_avatar_0_12_nodes(js);

	if (!img) {
		publish = purple_xmlnode_new("publish");
		purple_xmlnode_set_attrib(publish, "node", NS_AVATAR_1_1_METADATA);

		item = purple_xmlnode_new_child(publish, "item");
		metadata = purple_xmlnode_new_child(item, "metadata");
		purple_xmlnode_set_namespace(metadata, NS_AVATAR_1_1_METADATA);

		/* publish */
		jabber_pep_publish(js, publish);
	} else {
		/*
		 * TODO: This is pretty gross.  The Jabber protocol really shouldn't
		 *       do voodoo to try to determine the image type, height
		 *       and width.
		 */
		/* A PNG header, including the IHDR, but nothing else */
		/* ATTN: this is in network byte order! */
		const struct {
			guchar signature[8]; /* must be hex 89 50 4E 47 0D 0A 1A 0A */
			struct {
				guint32 length; /* must be 0x0d */
				guchar type[4]; /* must be 'I' 'H' 'D' 'R' */
				guint32 width;
				guint32 height;
				guchar bitdepth;
				guchar colortype;
				guchar compression;
				guchar filter;
				guchar interlace;
			} ihdr;
		} *png = NULL;

		if (purple_image_get_data_size(img) > sizeof(*png))
			png = purple_image_get_data(img);

		/* check if the data is a valid png file (well, at least to some extent) */
		if(png && png->signature[0] == 0x89 &&
		   png->signature[1] == 0x50 &&
		   png->signature[2] == 0x4e &&
		   png->signature[3] == 0x47 &&
		   png->signature[4] == 0x0d &&
		   png->signature[5] == 0x0a &&
		   png->signature[6] == 0x1a &&
		   png->signature[7] == 0x0a &&
		   ntohl(png->ihdr.length) == 0x0d &&
		   png->ihdr.type[0] == 'I' &&
		   png->ihdr.type[1] == 'H' &&
		   png->ihdr.type[2] == 'D' &&
		   png->ihdr.type[3] == 'R') {
			/* parse PNG header to get the size of the image (yes, this is required) */
			guint32 width = ntohl(png->ihdr.width);
			guint32 height = ntohl(png->ihdr.height);
			PurpleXmlNode *data, *info;
			char *lengthstring, *widthstring, *heightstring;

			/* compute the sha1 hash */
			char *hash = g_compute_checksum_for_data(
				G_CHECKSUM_SHA1,
				purple_image_get_data(img),
				purple_image_get_data_size(img));
			char *base64avatar = g_base64_encode(
				purple_image_get_data(img),
				purple_image_get_data_size(img));

			publish = purple_xmlnode_new("publish");
			purple_xmlnode_set_attrib(publish, "node", NS_AVATAR_1_1_DATA);

			item = purple_xmlnode_new_child(publish, "item");
			purple_xmlnode_set_attrib(item, "id", hash);

			data = purple_xmlnode_new_child(item, "data");
			purple_xmlnode_set_namespace(data, NS_AVATAR_1_1_DATA);

			purple_xmlnode_insert_data(data, base64avatar, -1);
			/* publish the avatar itself */
			jabber_pep_publish(js, publish);

			g_free(base64avatar);

			lengthstring = g_strdup_printf("%" G_GSIZE_FORMAT,
				purple_image_get_data_size(img));
			widthstring = g_strdup_printf("%u", width);
			heightstring = g_strdup_printf("%u", height);

			/* publish the metadata */
			publish = purple_xmlnode_new("publish");
			purple_xmlnode_set_attrib(publish, "node", NS_AVATAR_1_1_METADATA);

			item = purple_xmlnode_new_child(publish, "item");
			purple_xmlnode_set_attrib(item, "id", hash);

			metadata = purple_xmlnode_new_child(item, "metadata");
			purple_xmlnode_set_namespace(metadata, NS_AVATAR_1_1_METADATA);

			info = purple_xmlnode_new_child(metadata, "info");
			purple_xmlnode_set_attrib(info, "id", hash);
			purple_xmlnode_set_attrib(info, "type", "image/png");
			purple_xmlnode_set_attrib(info, "bytes", lengthstring);
			purple_xmlnode_set_attrib(info, "width", widthstring);
			purple_xmlnode_set_attrib(info, "height", heightstring);

			jabber_pep_publish(js, publish);

			g_free(lengthstring);
			g_free(widthstring);
			g_free(heightstring);
			g_free(hash);
		} else {
			purple_debug_error("jabber", "Cannot set PEP avatar to non-PNG data\n");
		}
	}
}

static void
do_got_own_avatar_0_12_cb(JabberStream *js, const char *from, PurpleXmlNode *items)
{
	if (items)
		/* It wasn't an error (i.e. 'item-not-found') */
		remove_avatar_0_12_nodes(js);
}

static void
do_got_own_avatar_cb(JabberStream *js, const char *from, PurpleXmlNode *items)
{
	PurpleXmlNode *item = NULL, *metadata = NULL, *info = NULL;
	PurpleAccount *account = purple_connection_get_account(js->gc);
	const char *server_hash = NULL;

	if (items && (item = purple_xmlnode_get_child(items, "item")) &&
			(metadata = purple_xmlnode_get_child(item, "metadata")) &&
			(info = purple_xmlnode_get_child(metadata, "info"))) {
		server_hash = purple_xmlnode_get_attrib(info, "id");
	}

	/*
	 * If we have an avatar and the server returned an error/malformed data,
	 * push our avatar. If the server avatar doesn't match the local one, push
	 * our avatar.
	 */
	if ((!items || !metadata) ||
			!purple_strequal(server_hash, js->initial_avatar_hash)) {
		PurpleImage *img = purple_buddy_icons_find_account_icon(account);
		jabber_avatar_set(js, img);
		if (img)
			g_object_unref(img);
	}
}

void jabber_avatar_fetch_mine(JabberStream *js)
{
	if (js->initial_avatar_hash) {
		jabber_pep_request_item(js, NULL, NS_AVATAR_0_12_METADATA, NULL,
		                        do_got_own_avatar_0_12_cb);
		jabber_pep_request_item(js, NULL, NS_AVATAR_1_1_METADATA, NULL,
		                        do_got_own_avatar_cb);
	}
}

typedef struct {
	JabberStream *js;
	char *from;
	char *id;
} JabberBuddyAvatarUpdateURLInfo;

static void
do_buddy_avatar_update_fromurl(G_GNUC_UNUSED SoupSession *session,
                               SoupMessage *msg, gpointer data)
{
	JabberBuddyAvatarUpdateURLInfo *info = data;
	gpointer icon_data;

	if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		purple_debug_error("jabber",
		                   "do_buddy_avatar_update_fromurl got error \"%s\"",
		                   msg->reason_phrase);
		goto out;
	}

	icon_data = g_memdup(msg->response_body->data, msg->response_body->length);
	purple_buddy_icons_set_for_user(purple_connection_get_account(info->js->gc),
	                                info->from, icon_data,
	                                msg->response_body->length, info->id);

out:
	g_free(info->from);
	g_free(info->id);
	g_free(info);
}

static void
do_buddy_avatar_update_data(JabberStream *js, const char *from, PurpleXmlNode *items)
{
	PurpleXmlNode *item, *data;
	const char *checksum;
	char *b64data;
	void *img;
	size_t size;
	if(!items)
		return;

	item = purple_xmlnode_get_child(items, "item");
	if(!item)
		return;

	data = purple_xmlnode_get_child(item, "data");
	if(!data)
		return;

	checksum = purple_xmlnode_get_attrib(item,"id");
	if(!checksum)
		return;

	b64data = purple_xmlnode_get_data(data);
	if(!b64data)
		return;

	img = g_base64_decode(b64data, &size);
	if(!img) {
		g_free(b64data);
		return;
	}

	purple_buddy_icons_set_for_user(purple_connection_get_account(js->gc), from, img, size, checksum);
	g_free(b64data);
}

static void
update_buddy_metadata(JabberStream *js, const char *from, PurpleXmlNode *items)
{
	PurpleBuddy *buddy = purple_blist_find_buddy(purple_connection_get_account(js->gc), from);
	const char *checksum;
	PurpleXmlNode *item, *metadata;
	if(!buddy)
		return;

	if (!items)
		return;

	item = purple_xmlnode_get_child(items,"item");
	if (!item)
		return;

	metadata = purple_xmlnode_get_child(item, "metadata");
	if(!metadata)
		return;

	checksum = purple_buddy_icons_get_checksum_for_user(buddy);

	/* <stop/> was the pre-v1.1 method of publishing an empty avatar */
	if(purple_xmlnode_get_child(metadata, "stop")) {
		purple_buddy_icons_set_for_user(purple_connection_get_account(js->gc), from, NULL, 0, NULL);
	} else {
		PurpleXmlNode *info, *goodinfo = NULL;
		gboolean has_children = FALSE;

		/* iterate over all info nodes to get one we can use */
		for(info = metadata->child; info; info = info->next) {
			if(info->type == PURPLE_XMLNODE_TYPE_TAG)
				has_children = TRUE;
			if(info->type == PURPLE_XMLNODE_TYPE_TAG && purple_strequal(info->name,"info")) {
				const char *type = purple_xmlnode_get_attrib(info,"type");
				const char *id = purple_xmlnode_get_attrib(info,"id");

				if(checksum && id && purple_strequal(id, checksum)) {
					/* we already have that avatar, so we don't have to do anything */
					goodinfo = NULL;
					break;
				}
				/* We'll only pick the png one for now. It's a very nice image format anyways. */
				if(id && !goodinfo && purple_strequal(type, "image/png"))
					goodinfo = info;
			}
		}
		if(has_children == FALSE) {
			purple_buddy_icons_set_for_user(purple_connection_get_account(js->gc), from, NULL, 0, NULL);
		} else if(goodinfo) {
			const char *url = purple_xmlnode_get_attrib(goodinfo, "url");
			const char *id = purple_xmlnode_get_attrib(goodinfo,"id");

			/* the avatar might either be stored in a pep node, or on a HTTP(S) URL */
			if(!url) {
				jabber_pep_request_item(js, from, NS_AVATAR_1_1_DATA, id,
				                        do_buddy_avatar_update_data);
			} else {
				SoupMessage *msg;
				JabberBuddyAvatarUpdateURLInfo *info = g_new0(JabberBuddyAvatarUpdateURLInfo, 1);
				info->js = js;
				info->from = g_strdup(from);
				info->id = g_strdup(id);

				msg = soup_message_new("GET", url);
				soup_session_queue_message(js->http_conns, msg,
				                           do_buddy_avatar_update_fromurl,
				                           info);
			}
		}
	}
}
