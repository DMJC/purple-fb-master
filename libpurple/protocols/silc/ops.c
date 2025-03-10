/*

  silcpurple_ops.c

  Author: Pekka Riikonen <priikone@silcnet.org>

  Copyright (C) 2004 - 2007 Pekka Riikonen

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

*/

#include "internal.h"
PURPLE_BEGIN_IGNORE_CAST_ALIGN
#include "silc.h"
PURPLE_END_IGNORE_CAST_ALIGN
#include "silcclient.h"
#include "silcpurple.h"
#include "wb.h"

static void
silc_channel_message(SilcClient client, SilcClientConnection conn,
		     SilcClientEntry sender, SilcChannelEntry channel,
		     SilcMessagePayload payload,
		     SilcChannelPrivateKey key, SilcMessageFlags flags,
		     const unsigned char *message,
		     SilcUInt32 message_len);
static void
silc_private_message(SilcClient client, SilcClientConnection conn,
		     SilcClientEntry sender, SilcMessagePayload payload,
		     SilcMessageFlags flags, const unsigned char *message,
		     SilcUInt32 message_len);
static void
silc_ask_passphrase(SilcClient client, SilcClientConnection conn,
		    SilcAskPassphrase completion, void *context);

/* Message sent to the application by library. `conn' associates the
   message to a specific connection.  `conn', however, may be NULL.
   The `type' indicates the type of the message sent by the library.
   The application can for example filter the message according the
   type. */

void silc_say(SilcClient client, SilcClientConnection conn,
	      SilcClientMessageType type, char *msg, ...)
{
	char tmp[256];
	va_list va;
	PurpleConnection *gc = NULL;
	PurpleConnectionError reason = PURPLE_CONNECTION_ERROR_NETWORK_ERROR;

	va_start(va, msg);
	silc_vsnprintf(tmp, sizeof(tmp), msg, va);
	va_end(va);

	if (type != SILC_CLIENT_MESSAGE_ERROR) {
		purple_debug_misc("silc", "silc_say (%d) %s\n", type, tmp);
		return;
	}

	purple_debug_error("silc", "silc_say error: %s\n", tmp);

	if (purple_strequal(tmp, "Authentication failed"))
		reason = PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED;

	if (client != NULL)
		gc = client->application;

	if (gc != NULL)
		purple_connection_error(gc, reason, tmp);
	else
		purple_notify_error(NULL, _("Error"), _("Error occurred"), tmp, NULL);
}

/* Processes incoming MIME message.  Can be private message or channel
   message.  Returns TRUE if the message `mime' was displayed. */

static SilcBool
silcpurple_mime_message(SilcClient client, SilcClientConnection conn,
			SilcClientEntry sender, SilcChannelEntry channel,
			SilcMessagePayload payload, SilcChannelPrivateKey key,
			SilcMessageFlags flags, SilcMime mime,
			gboolean recursive)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	const char *type;
	const unsigned char *data;
	SilcUInt32 data_len;
	PurpleMessageFlags cflags = 0;
	PurpleChatConversation *chat = NULL;
	SilcBool ret = FALSE;

	if (!mime)
		return FALSE;

	/* Check for fragmented MIME message */
	if (silc_mime_is_partial(mime)) {
		if (!sg->mimeass)
			sg->mimeass = silc_mime_assembler_alloc();

		/* Defragment */
		mime = silc_mime_assemble(sg->mimeass, mime);
		if (!mime)
			/* More fragments to come */
			return FALSE;

		/* Process the complete message */
		return silcpurple_mime_message(client, conn, sender, channel,
					       payload, key, flags, mime,
					       FALSE);
	}

	/* Check for multipart message */
	if (silc_mime_is_multipart(mime)) {
		SilcMime p;
		const char *mtype;
		SilcDList parts = silc_mime_get_multiparts(mime, &mtype);

		if (purple_strequal(mtype, "mixed")) {
			/* Contains multiple messages */
			silc_dlist_start(parts);
			while ((p = silc_dlist_get(parts)) != SILC_LIST_END) {
			  /* Recursively process parts */
			  ret = silcpurple_mime_message(client, conn, sender, channel,
							payload, key, flags, p, TRUE);
			}
		}

		if (purple_strequal(mtype, "alternative")) {
			/* Same message in alternative formats.  Kopete sends
			   these.  Go in order from last to first. */
			silc_dlist_end(parts);
			while ((p = silc_dlist_get(parts)) != SILC_LIST_END) {
			  /* Go through the alternatives and display the first
			     one we support. */
			  if (silcpurple_mime_message(client, conn, sender, channel,
						      payload, key, flags, p, TRUE)) {
			    ret = TRUE;
			    break;
			  }
			}
		}

		goto out;
	}

	/* Get content type and MIME data */
	type = silc_mime_get_field(mime, "Content-Type");
	if (!type)
		goto out;
	data = silc_mime_get_data(mime, &data_len);
	if (!data)
		goto out;

	/* Process according to content type */

	/* Plain text */
	if (strstr(type, "text/plain")) {
		/* Default is UTF-8, don't check for other charsets */
		if (!strstr(type, "utf-8"))
			goto out;

		if (channel)
			silc_channel_message(client, conn, sender, channel,
					     payload, key,
					     SILC_MESSAGE_FLAG_UTF8, data,
					     data_len);
		else
			silc_private_message(client, conn, sender, payload,
					     SILC_MESSAGE_FLAG_UTF8, data,
					     data_len);
		ret = TRUE;
		goto out;
	}

	/* Image */
	if (g_str_has_prefix(type, "image/")) {
		char tmp[32];
		PurpleImage *img;
		guint img_id;

		/* Get channel chat (if message is for channel) */
		if (key && channel) {
			GList *l;
			SilcPurplePrvgrp prv;

			for (l = sg->grps; l; l = l->next)
				if (((SilcPurplePrvgrp)l->data)->key == key) {
					prv = l->data;
					chat = purple_conversations_find_chat_with_account(
							prv->channel, sg->account);
					break;
				}
		}
		if (channel && !chat)
			chat = purple_conversations_find_chat_with_account(
								      channel->channel_name, sg->account);
		if (channel && !chat)
			goto out;

		img = purple_image_new_from_data(g_memdup(data, data_len), data_len);
		if (!img)
			goto out;
		img_id = purple_image_store_add_temporary(img);
		if (!img_id)
			goto out;

		cflags |= PURPLE_MESSAGE_IMAGES | PURPLE_MESSAGE_RECV;
		g_snprintf(tmp, sizeof(tmp), "<img src=\""
			PURPLE_IMAGE_STORE_PROTOCOL "%u\">", img_id);

		if (channel) {
			purple_serv_got_chat_in(gc,
				purple_chat_conversation_get_id(chat),
				sender->nickname, cflags, tmp, time(NULL));
		} else {
			purple_serv_got_im(gc, sender->nickname,
				tmp, cflags, time(NULL));
		}

		goto out;
	}

	/* Whiteboard message */
	if (strstr(type, "application/x-wb") &&
	    !purple_account_get_bool(sg->account, "block-wb", FALSE)) {
		if (channel)
			silcpurple_wb_receive_ch(client, conn, sender, channel,
					       payload, flags, data, data_len);
		else
			silcpurple_wb_receive(client, conn, sender, payload,
					      flags, data, data_len);
		ret = TRUE;
		goto out;
	}

 out:
	if (!recursive)
		silc_mime_free(mime);
	return ret;
}

/* Message for a channel. The `sender' is the sender of the message
   The `channel' is the channel. The `message' is the message.  Note
   that `message' maybe NULL.  The `flags' indicates message flags
   and it is used to determine how the message can be interpreted
   (like it may tell the message is multimedia message). */

static void
silc_channel_message(SilcClient client, SilcClientConnection conn,
		     SilcClientEntry sender, SilcChannelEntry channel,
		     SilcMessagePayload payload,
		     SilcChannelPrivateKey key, SilcMessageFlags flags,
		     const unsigned char *message,
		     SilcUInt32 message_len)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	PurpleChatConversation *chat = NULL;
	char *msg, *tmp;

	if (!message)
		return;

	if (key) {
		GList *l;
		SilcPurplePrvgrp prv;

		for (l = sg->grps; l; l = l->next)
			if (((SilcPurplePrvgrp)l->data)->key == key) {
				prv = l->data;
				chat = purple_conversations_find_chat_with_account(
										prv->channel, sg->account);
				break;
			}
	}
	if (!chat)
		chat = purple_conversations_find_chat_with_account(
							      channel->channel_name, sg->account);
	if (!chat)
		return;

	if (flags & SILC_MESSAGE_FLAG_SIGNED &&
	    purple_account_get_bool(sg->account, "sign-verify", FALSE)) {
		/* XXX */
	}

	if (flags & SILC_MESSAGE_FLAG_DATA) {
		/* Process MIME message */
		SilcMime mime;
		mime = silc_mime_decode(NULL, message, message_len);
		silcpurple_mime_message(client, conn, sender, channel, payload,
					key, flags, mime, FALSE);
		return;
	}

	if (flags & SILC_MESSAGE_FLAG_ACTION) {
		msg = g_strdup_printf("/me %s",
				      (const char *)message);
		if (!msg)
			return;

		tmp = g_markup_escape_text(msg, -1);
		/* Send to Purple */
		purple_serv_got_chat_in(gc, purple_chat_conversation_get_id(chat),
			sender->nickname, PURPLE_MESSAGE_RECV, tmp, time(NULL));
		g_free(tmp);
		g_free(msg);
		return;
	}

	if (flags & SILC_MESSAGE_FLAG_NOTICE) {
		msg = g_strdup_printf("(notice) <I>%s</I> %s",
				      sender->nickname, (const char *)message);
		if (!msg)
			return;

		/* Send to Purple */
		purple_conversation_write_system_message(
			PURPLE_CONVERSATION(chat), msg, 0);
		g_free(msg);
		return;
	}

	if (flags & SILC_MESSAGE_FLAG_UTF8) {
		const char *msg = (const char *)message;
		char *salvaged = NULL;
		if (!g_utf8_validate((const char *)message, -1, NULL)) {
			salvaged = purple_utf8_salvage((const char *)message);
			msg = salvaged;
		}
		tmp = g_markup_escape_text(msg, -1);
		/* Send to Purple */
		purple_serv_got_chat_in(gc, purple_chat_conversation_get_id(chat),
			sender->nickname, PURPLE_MESSAGE_RECV, tmp, time(NULL));
		g_free(salvaged);
		g_free(tmp);
	}
}


/* Private message to the client. The `sender' is the sender of the
   message. The message is `message'and maybe NULL.  The `flags'
   indicates message flags  and it is used to determine how the message
   can be interpreted (like it may tell the message is multimedia
   message). */

static void
silc_private_message(SilcClient client, SilcClientConnection conn,
		     SilcClientEntry sender, SilcMessagePayload payload,
		     SilcMessageFlags flags, const unsigned char *message,
		     SilcUInt32 message_len)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	PurpleConversation *convo;
	char *msg, *tmp;

	if (!message)
		return;

	/* XXX - Should this be PURPLE_CONV_TYPE_IM? */
	convo = purple_conversations_find_with_account(
							      sender->nickname, sg->account);

	if (flags & SILC_MESSAGE_FLAG_SIGNED &&
	    purple_account_get_bool(sg->account, "sign-verify", FALSE)) {
		/* XXX */
	}

	if (flags & SILC_MESSAGE_FLAG_DATA) {
		/* Process MIME message */
		SilcMime mime;
		mime = silc_mime_decode(NULL, message, message_len);
		silcpurple_mime_message(client, conn, sender, NULL, payload,
				      NULL, flags, mime, FALSE);
		return;
	}

	if (flags & SILC_MESSAGE_FLAG_ACTION && convo) {
		msg = g_strdup_printf("/me %s",
				      (const char *)message);
		if (!msg)
			return;

		/* Send to Purple */
		tmp = g_markup_escape_text(msg, -1);
		purple_serv_got_im(gc, sender->nickname, tmp, 0, time(NULL));
		g_free(msg);
		g_free(tmp);
		return;
	}

	if (flags & SILC_MESSAGE_FLAG_NOTICE && convo) {
		msg = g_strdup_printf("(notice) <I>%s</I> %s",
				      sender->nickname, (const char *)message);
		if (!msg)
			return;

		/* Send to Purple */
		purple_conversation_write_system_message(convo, msg, 0);
		g_free(msg);
		return;
	}

	if (flags & SILC_MESSAGE_FLAG_UTF8) {
		const char *msg = (const char *)message;
		char *salvaged = NULL;
		if (!g_utf8_validate((const char *)message, -1, NULL)) {
			salvaged = purple_utf8_salvage((const char *)message);
			msg = salvaged;
		}
		tmp = g_markup_escape_text(msg, -1);
		/* Send to Purple */
		purple_serv_got_im(gc, sender->nickname, tmp, 0, time(NULL));
		g_free(salvaged);
		g_free(tmp);
	}
}


/* Notify message to the client. The notify arguments are sent in the
   same order as servers sends them. The arguments are same as received
   from the server except for ID's.  If ID is received application receives
   the corresponding entry to the ID. For example, if Client ID is received
   application receives SilcClientEntry.  Also, if the notify type is
   for channel the channel entry is sent to application (even if server
   does not send it because client library gets the channel entry from
   the Channel ID in the packet's header). */

static void
silc_notify(SilcClient client, SilcClientConnection conn,
	    SilcNotifyType type, ...)
{
	va_list va;
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	PurpleAccount *account = purple_connection_get_account(gc);
	PurpleChatConversation *chat;
	SilcClientEntry client_entry, client_entry2;
	SilcChannelEntry channel;
	SilcServerEntry server_entry;
	SilcIdType idtype;
	void *entry;
	SilcUInt32 mode;
	SilcHashTableList htl;
	SilcChannelUser chu;
	char buf[512], buf2[512], *tmp, *name;
	SilcNotifyType notify;
	PurpleBuddy *b;
	SilcDList list;

	va_start(va, type);
	memset(buf, 0, sizeof(buf));

	switch (type) {

	case SILC_NOTIFY_TYPE_NONE:
		break;

	case SILC_NOTIFY_TYPE_INVITE:
		{
			GHashTable *components;
			(void)va_arg(va, SilcChannelEntry);
			name = va_arg(va, char *);
			client_entry = va_arg(va, SilcClientEntry);

			components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
			g_hash_table_insert(components, g_strdup("channel"), g_strdup(name));
			purple_serv_got_chat_invite(gc, name, client_entry->nickname, NULL, components);
		}
		break;

	case SILC_NOTIFY_TYPE_JOIN:
		client_entry = va_arg(va, SilcClientEntry);
		channel = va_arg(va, SilcChannelEntry);

		/* If we joined channel, do nothing */
		if (client_entry == conn->local_entry)
			break;

		chat = purple_conversations_find_chat_with_account(
							      channel->channel_name, sg->account);
		if (!chat)
			break;

		/* Join user to channel */
		g_snprintf(buf, sizeof(buf), "%s@%s",
			   client_entry->username, client_entry->hostname);
		purple_chat_conversation_add_user(chat,
					  client_entry->nickname, buf, PURPLE_CHAT_USER_NONE, TRUE);

		break;

	case SILC_NOTIFY_TYPE_LEAVE:
		client_entry = va_arg(va, SilcClientEntry);
		channel = va_arg(va, SilcChannelEntry);

		chat = purple_conversations_find_chat_with_account(
							      channel->channel_name, sg->account);
		if (!chat)
			break;

		/* Remove user from channel */
		purple_chat_conversation_remove_user(chat,
					     client_entry->nickname, NULL);

		break;

	case SILC_NOTIFY_TYPE_SIGNOFF:
		client_entry = va_arg(va, SilcClientEntry);
		tmp = va_arg(va, char *);

		/* Remove from all channels */
		silc_hash_table_list(client_entry->channels, &htl);
		while (silc_hash_table_get(&htl, NULL, (void *)&chu)) {
			chat = purple_conversations_find_chat_with_account(
								      chu->channel->channel_name, sg->account);
			if (!chat)
				continue;
			purple_chat_conversation_remove_user(chat,
						     client_entry->nickname,
						     tmp);
		}
		silc_hash_table_list_reset(&htl);

		break;

	case SILC_NOTIFY_TYPE_TOPIC_SET:
		{
			char *esc, *tmp2;
			idtype = va_arg(va, int);
			entry = va_arg(va, void *);
			tmp = va_arg(va, char *);
			channel = va_arg(va, SilcChannelEntry);

			chat = purple_conversations_find_chat_with_account(
								      channel->channel_name, sg->account);
			if (!chat)
				break;

			if (!tmp)
				break;

			esc = g_markup_escape_text(tmp, -1);
			tmp2 = purple_markup_linkify(esc);
			g_free(esc);

			if (idtype == SILC_ID_CLIENT) {
				client_entry = (SilcClientEntry)entry;
				g_snprintf(buf, sizeof(buf),
						_("%s has changed the topic of <I>%s</I> to: %s"),
						client_entry->nickname, channel->channel_name, tmp2);
				purple_conversation_write_system_message(
					PURPLE_CONVERSATION(chat), buf, 0);
				purple_chat_conversation_set_topic(chat,
						client_entry->nickname, tmp);
			} else if (idtype == SILC_ID_SERVER) {
				server_entry = (SilcServerEntry)entry;
				g_snprintf(buf, sizeof(buf),
						_("%s has changed the topic of <I>%s</I> to: %s"),
						server_entry->server_name, channel->channel_name, tmp2);
				purple_conversation_write_system_message(
					PURPLE_CONVERSATION(chat), buf, 0);
				purple_chat_conversation_set_topic(chat,
						server_entry->server_name, tmp);
			} else if (idtype == SILC_ID_CHANNEL) {
				channel = (SilcChannelEntry)entry;
				g_snprintf(buf, sizeof(buf),
						_("%s has changed the topic of <I>%s</I> to: %s"),
						channel->channel_name, channel->channel_name, tmp2);
				purple_conversation_write_system_message(
					PURPLE_CONVERSATION(chat), buf, 0);
				purple_chat_conversation_set_topic(chat,
						channel->channel_name, tmp);
			} else {
				purple_chat_conversation_set_topic(chat, NULL, tmp);
			}

			g_free(tmp2);

			break;

		}
	case SILC_NOTIFY_TYPE_NICK_CHANGE:
		client_entry = va_arg(va, SilcClientEntry);
		tmp = va_arg(va, char *);      /* Old nick */
		name = va_arg(va, char *);     /* New nick */

		if (purple_strequal(tmp, name))
			break;

		/* Change nick on all channels */
		silc_hash_table_list(client_entry->channels, &htl);
		while (silc_hash_table_get(&htl, NULL, (void *)&chu)) {
			chat = purple_conversations_find_chat_with_account(
								      chu->channel->channel_name, sg->account);
			if (!chat)
				continue;
			if (purple_chat_conversation_has_user(chat, client_entry->nickname))
				purple_chat_conversation_rename_user(chat,
							     tmp, name);
		}
		silc_hash_table_list_reset(&htl);

		break;

	case SILC_NOTIFY_TYPE_CMODE_CHANGE:
		idtype = va_arg(va, int);
		entry = va_arg(va, void *);
		mode = va_arg(va, SilcUInt32);
		(void)va_arg(va, char *);
		(void)va_arg(va, char *);
		(void)va_arg(va, char *);
		(void)va_arg(va, SilcPublicKey);
		(void)va_arg(va, SilcDList);
		channel = va_arg(va, SilcChannelEntry);

		chat = purple_conversations_find_chat_with_account(
							      channel->channel_name, sg->account);
		if (!chat)
			break;

		if (idtype == SILC_ID_CLIENT)
			name = ((SilcClientEntry)entry)->nickname;
		else if (idtype == SILC_ID_SERVER)
			name = ((SilcServerEntry)entry)->server_name;
		else
			name = ((SilcChannelEntry)entry)->channel_name;
		if (!name)
			break;

		if (mode) {
			silcpurple_get_chmode_string(mode, buf2, sizeof(buf2));
			g_snprintf(buf, sizeof(buf),
				   _("<I>%s</I> set channel <I>%s</I> modes to: %s"), name,
				   channel->channel_name, buf2);
		} else {
			g_snprintf(buf, sizeof(buf),
				   _("<I>%s</I> removed all channel <I>%s</I> modes"), name,
				   channel->channel_name);
		}
		purple_conversation_write_system_message(PURPLE_CONVERSATION(chat), buf, 0);
		break;

	case SILC_NOTIFY_TYPE_CUMODE_CHANGE:
		{
			PurpleChatUserFlags flags = PURPLE_CHAT_USER_NONE;
			idtype = va_arg(va, int);
			entry = va_arg(va, void *);
			mode = va_arg(va, SilcUInt32);
			client_entry2 = va_arg(va, SilcClientEntry);
			channel = va_arg(va, SilcChannelEntry);

			chat = purple_conversations_find_chat_with_account(
								      channel->channel_name, sg->account);
			if (!chat)
				break;

			if (idtype == SILC_ID_CLIENT)
				name = ((SilcClientEntry)entry)->nickname;
			else if (idtype == SILC_ID_SERVER)
				name = ((SilcServerEntry)entry)->server_name;
			else
				name = ((SilcChannelEntry)entry)->channel_name;
			if (!name)
				break;

			if (mode) {
				silcpurple_get_chumode_string(mode, buf2, sizeof(buf2));
				g_snprintf(buf, sizeof(buf),
					   _("<I>%s</I> set <I>%s's</I> modes to: %s"), name,
					   client_entry2->nickname, buf2);
				if (mode & SILC_CHANNEL_UMODE_CHANFO)
					flags |= PURPLE_CHAT_USER_FOUNDER;
				if (mode & SILC_CHANNEL_UMODE_CHANOP)
					flags |= PURPLE_CHAT_USER_OP;
			} else {
				g_snprintf(buf, sizeof(buf),
					   _("<I>%s</I> removed all <I>%s's</I> modes"), name,
					   client_entry2->nickname);
			}
			purple_conversation_write_system_message(PURPLE_CONVERSATION(chat), buf, 0);
			purple_chat_user_set_flags(purple_chat_conversation_find_user(
					       chat, client_entry2->nickname), flags);
			break;
		}

	case SILC_NOTIFY_TYPE_MOTD:
		tmp = va_arg(va, char *);
		silc_free(sg->motd);
		sg->motd = silc_memdup(tmp, strlen(tmp));
		break;

	case SILC_NOTIFY_TYPE_KICKED:
		client_entry = va_arg(va, SilcClientEntry);
		tmp = va_arg(va, char *);
		client_entry2 = va_arg(va, SilcClientEntry);
		channel = va_arg(va, SilcChannelEntry);

		chat = purple_conversations_find_chat_with_account(
							      channel->channel_name, sg->account);
		if (!chat)
			break;

		if (client_entry == conn->local_entry) {
			/* Remove us from channel */
			g_snprintf(buf, sizeof(buf),
				   _("You have been kicked off <I>%s</I> by <I>%s</I> (%s)"),
				   channel->channel_name, client_entry2->nickname,
				   tmp ? tmp : "");
			purple_conversation_write_system_message(PURPLE_CONVERSATION(chat),
				buf, 0);
			purple_serv_got_chat_left(gc, purple_chat_conversation_get_id(chat));
		} else {
			/* Remove user from channel */
			g_snprintf(buf, sizeof(buf), _("Kicked by %s (%s)"),
				   client_entry2->nickname, tmp ? tmp : "");
			purple_chat_conversation_remove_user(chat,
						     client_entry->nickname,
						     buf);
		}

		break;

	case SILC_NOTIFY_TYPE_KILLED:
		client_entry = va_arg(va, SilcClientEntry);
		tmp = va_arg(va, char *);
		idtype = va_arg(va, int);
		entry = va_arg(va, SilcClientEntry);

		if (client_entry == conn->local_entry) {
			if (idtype == SILC_ID_CLIENT) {
				client_entry2 = (SilcClientEntry)entry;
				g_snprintf(buf, sizeof(buf),
					   _("You have been killed by %s (%s)"),
					   client_entry2->nickname, tmp ? tmp : "");
			} else if (idtype == SILC_ID_SERVER) {
				server_entry = (SilcServerEntry)entry;
				g_snprintf(buf, sizeof(buf),
					   _("You have been killed by %s (%s)"),
					   server_entry->server_name, tmp ? tmp : "");
			} else if (idtype == SILC_ID_CHANNEL) {
				channel = (SilcChannelEntry)entry;
				g_snprintf(buf, sizeof(buf),
					   _("You have been killed by %s (%s)"),
					   channel->channel_name, tmp ? tmp : "");
			}

			/* Remove us from all channels */
			silc_hash_table_list(client_entry->channels, &htl);
			while (silc_hash_table_get(&htl, NULL, (void *)&chu)) {
				chat = purple_conversations_find_chat_with_account(
										chu->channel->channel_name, sg->account);
				if (!chat)
					continue;
				purple_conversation_write_system_message(
					PURPLE_CONVERSATION(chat), buf, 0);
				purple_serv_got_chat_left(gc, purple_chat_conversation_get_id(chat));
			}
			silc_hash_table_list_reset(&htl);

		} else {
			if (idtype == SILC_ID_CLIENT) {
				client_entry2 = (SilcClientEntry)entry;
				g_snprintf(buf, sizeof(buf),
					   _("Killed by %s (%s)"),
					   client_entry2->nickname, tmp ? tmp : "");
			} else if (idtype == SILC_ID_SERVER) {
				server_entry = (SilcServerEntry)entry;
				g_snprintf(buf, sizeof(buf),
					   _("Killed by %s (%s)"),
					   server_entry->server_name, tmp ? tmp : "");
			} else if (idtype == SILC_ID_CHANNEL) {
				channel = (SilcChannelEntry)entry;
				g_snprintf(buf, sizeof(buf),
					   _("Killed by %s (%s)"),
					   channel->channel_name, tmp ? tmp : "");
			}

			/* Remove user from all channels */
			silc_hash_table_list(client_entry->channels, &htl);
			while (silc_hash_table_get(&htl, NULL, (void *)&chu)) {
				chat = purple_conversations_find_chat_with_account(
										chu->channel->channel_name, sg->account);
				if (!chat)
					continue;
				purple_chat_conversation_remove_user(chat,
							     client_entry->nickname, tmp);
			}
			silc_hash_table_list_reset(&htl);
		}

		break;

	case SILC_NOTIFY_TYPE_CHANNEL_CHANGE:
		break;

	case SILC_NOTIFY_TYPE_SERVER_SIGNOFF:
		(void)va_arg(va, void *);
		list = va_arg(va, SilcDList);

		silc_dlist_start(list);
		while ((client_entry = silc_dlist_get(list))) {
			/* Remove from all channels */
			silc_hash_table_list(client_entry->channels, &htl);
			while (silc_hash_table_get(&htl, NULL, (void *)&chu)) {
				chat = purple_conversations_find_chat_with_account(
									      chu->channel->channel_name, sg->account);
				if (!chat)
					continue;
				purple_chat_conversation_remove_user(chat,
							     client_entry->nickname,
							     _("Server signoff"));
			}
			silc_hash_table_list_reset(&htl);
		}
		break;

	case SILC_NOTIFY_TYPE_ERROR:
		{
			SilcStatus error = va_arg(va, int);
			purple_notify_error(gc, "Error Notify",
					    silc_get_status_message(error),
					    NULL, purple_request_cpar_from_connection(gc));
		}
		break;

	case SILC_NOTIFY_TYPE_WATCH:
		{
			SilcPublicKey public_key;
			unsigned char *pk;
			SilcUInt32 pk_len;
			char *fingerprint;

			client_entry = va_arg(va, SilcClientEntry);
			(void)va_arg(va, char *);
			mode = va_arg(va, SilcUInt32);
			notify = va_arg(va, int);
			public_key = va_arg(va, SilcPublicKey);

			b = NULL;
			if (public_key) {
				GSList *buddies;
				const char *f;
				gsize i;

				pk = silc_pkcs_public_key_encode(public_key, &pk_len);
				if (!pk)
					break;
				fingerprint = silc_hash_fingerprint(NULL, pk, pk_len);
				for (i = 0; i < strlen(fingerprint); i++)
					if (fingerprint[i] == ' ')
						fingerprint[i] = '_';
				g_snprintf(buf, sizeof(buf) - 1,
					   "%s" G_DIR_SEPARATOR_S "clientkeys"
					   G_DIR_SEPARATOR_S "clientkey_%s.pub",
					   silcpurple_silcdir(), fingerprint);
				silc_free(fingerprint);
				silc_free(pk);

				/* Find buddy by associated public key */
				for (buddies = purple_blist_find_buddies(account, NULL); buddies;
						buddies = g_slist_delete_link(buddies, buddies)) {
					b = buddies->data;
					f = purple_blist_node_get_string(PURPLE_BLIST_NODE(b), "public-key");
					if (purple_strequal(f, buf))
						goto cont;
					b = NULL;
				}
			}
		cont:
			if (!b) {
				/* Find buddy by nickname */
				b = purple_blist_find_buddy(sg->account, client_entry->nickname);
				if (!b) {
					purple_debug_warning("silc", "WATCH for %s, unknown buddy\n",
						client_entry->nickname);
					break;
				}
			}

			silc_free(purple_buddy_get_protocol_data(b));
			purple_buddy_set_protocol_data(b, silc_memdup(&client_entry->id,
						    sizeof(client_entry->id)));
			if (notify == SILC_NOTIFY_TYPE_NICK_CHANGE) {
				break;
			} else if (notify == SILC_NOTIFY_TYPE_UMODE_CHANGE) {
				/* See if client was away and is now present */
				if (!(mode & (SILC_UMODE_GONE | SILC_UMODE_INDISPOSED |
					      SILC_UMODE_BUSY | SILC_UMODE_PAGE |
					      SILC_UMODE_DETACHED)) &&
				    (client_entry->mode & SILC_UMODE_GONE ||
				     client_entry->mode & SILC_UMODE_INDISPOSED ||
				     client_entry->mode & SILC_UMODE_BUSY ||
				     client_entry->mode & SILC_UMODE_PAGE ||
				     client_entry->mode & SILC_UMODE_DETACHED)) {
					client_entry->mode = mode;
					purple_protocol_got_user_status(purple_buddy_get_account(b), purple_buddy_get_name(b), SILCPURPLE_STATUS_ID_AVAILABLE, NULL);
				}
				else if ((mode & SILC_UMODE_GONE) ||
					 (mode & SILC_UMODE_INDISPOSED) ||
					 (mode & SILC_UMODE_BUSY) ||
					 (mode & SILC_UMODE_PAGE) ||
					 (mode & SILC_UMODE_DETACHED)) {
					client_entry->mode = mode;
					purple_protocol_got_user_status(purple_buddy_get_account(b), purple_buddy_get_name(b), SILCPURPLE_STATUS_ID_OFFLINE, NULL);
				}
			} else if (notify == SILC_NOTIFY_TYPE_SIGNOFF ||
				   notify == SILC_NOTIFY_TYPE_SERVER_SIGNOFF ||
				   notify == SILC_NOTIFY_TYPE_KILLED) {
				client_entry->mode = mode;
				purple_protocol_got_user_status(purple_buddy_get_account(b), purple_buddy_get_name(b), SILCPURPLE_STATUS_ID_OFFLINE, NULL);
			} else if (notify == SILC_NOTIFY_TYPE_NONE) {
				client_entry->mode = mode;
				purple_protocol_got_user_status(purple_buddy_get_account(b), purple_buddy_get_name(b), SILCPURPLE_STATUS_ID_AVAILABLE, NULL);
			}
		}
		break;

	default:
		purple_debug_info("silc", "Unhandled notification: %d\n", type);
		break;
	}

	va_end(va);
}


/* Command handler. This function is called always after application has
   called a command.  It will be called to indicate that the command
   was processed.  It will also be called if error occurs while processing
   the command.  The `success' indicates whether the command was sent
   or if error occurred.  The `status' indicates the actual error.
   The `argc' and `argv' are the command line arguments sent to the
   command by application.  Note that, this is not reply to the command
   from server, this is merely and indication to application that the
   command was processed. */

static void
silc_command(SilcClient client, SilcClientConnection conn,
	     SilcBool success, SilcCommand command, SilcStatus status,
	     SilcUInt32 argc, unsigned char **argv)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);

	switch (command) {

	case SILC_COMMAND_CMODE:
		if (argc == 3 && purple_strequal((char *)argv[2], "+C"))
			sg->chpk = TRUE;
		else
			sg->chpk = FALSE;
		break;

	default:
		break;
	}
}


/* Command reply handler.  Delivers a reply to command that was sent
   earlier.  The `conn' is the associated client connection.  The `command'
   indicates the command reply type.  If the `status' other than
   SILC_STATUS_OK an error occurred.  In this case the `error' will indicate
   the error.  It is possible to receive list of command replies and list
   of errors.  In this case the `status' will indicate it is an list entry
   (the `status' is SILC_STATUS_LIST_START, SILC_STATUS_LIST_ITEM and/or
   SILC_STATUS_LIST_END).

   The arguments received in `ap' are command specific.  See a separate
   documentation in the Toolkit Reference Manual for the command reply
   arguments. */

static void
silc_command_reply(SilcClient client, SilcClientConnection conn,
		   SilcCommand command, SilcStatus status,
		   SilcStatus error, va_list ap)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	PurpleChatConversation *chat;

	switch (command) {
	case SILC_COMMAND_JOIN:
		{
			SilcChannelEntry channel;
			PurpleChatConversation *chat;
			SilcHashTableList *user_list;
			SilcChannelUser chu;
			GList *users = NULL, *flags = NULL;
			char tmp[256], *topic;

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Join Chat"), _("Cannot join channel"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				return;
			}

			(void)va_arg(ap, char *);
			channel = va_arg(ap, SilcChannelEntry);
			(void)va_arg(ap, SilcUInt32);
			user_list = va_arg(ap, SilcHashTableList *);
			topic = va_arg(ap, char *);

			/* Add channel to Purple */
			channel->context = SILC_32_TO_PTR(++sg->channel_ids);
			purple_serv_got_joined_chat(gc, sg->channel_ids, channel->channel_name);
			chat = purple_conversations_find_chat_with_account(
								      channel->channel_name, sg->account);
			if (!chat)
			  return;

			/* Add all users to channel */
			while (silc_hash_table_get(user_list, NULL, (void *)&chu)) {
			  PurpleChatUserFlags f = PURPLE_CHAT_USER_NONE;
			  chu->context = SILC_32_TO_PTR(sg->channel_ids);

			  if (chu->mode & SILC_CHANNEL_UMODE_CHANFO)
			    f |= PURPLE_CHAT_USER_FOUNDER;
			  if (chu->mode & SILC_CHANNEL_UMODE_CHANOP)
			    f |= PURPLE_CHAT_USER_OP;
			  users = g_list_append(users, chu->client->nickname);
			  flags = g_list_append(flags, GINT_TO_POINTER(f));

			  if (chu->mode & SILC_CHANNEL_UMODE_CHANFO) {
			    if (chu->client == conn->local_entry)
				g_snprintf(tmp, sizeof(tmp),
					   _("You are channel founder on <I>%s</I>"),
					   channel->channel_name);
			    else
				g_snprintf(tmp, sizeof(tmp),
					   _("Channel founder on <I>%s</I> is <I>%s</I>"),
					   channel->channel_name, chu->client->nickname);

			    purple_conversation_write_system_message(
				PURPLE_CONVERSATION(chat), tmp, 0);
			  }
			}

			purple_chat_conversation_add_users(chat, users, NULL, flags, FALSE);
			g_list_free(users);
			g_list_free(flags);

			/* Set topic */
			if (topic)
			  purple_chat_conversation_set_topic(chat, NULL, topic);

			/* Set nick */
			purple_chat_conversation_set_nick(chat, conn->local_entry->nickname);
		}
		break;

	case SILC_COMMAND_LEAVE:
		break;

	case SILC_COMMAND_USERS:
		break;

	case SILC_COMMAND_WHOIS:
		{
			SilcUInt32 *user_modes;
			SilcDList channels;
			SilcClientEntry client_entry;
			char tmp[1024];
			char *moodstr, *statusstr, *contactstr, *langstr, *devicestr, *tzstr, *geostr;
			PurpleNotifyUserInfo *user_info;

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("User Information"),
						_("Cannot get user information"),
						silc_get_status_message(error),
						purple_request_cpar_from_connection(gc));
				break;
			}

			client_entry = va_arg(ap, SilcClientEntry);
			(void)va_arg(ap, char *);
			(void)va_arg(ap, char *);
			(void)va_arg(ap, char *);
			channels = va_arg(ap, SilcDList);
			(void)va_arg(ap, SilcUInt32);
			(void)va_arg(ap, SilcUInt32); /* idle */
			(void)va_arg(ap, unsigned char *);
			user_modes = va_arg(ap, SilcUInt32 *);

			user_info = purple_notify_user_info_new();
			purple_notify_user_info_add_pair_plaintext(user_info, _("Nickname"), client_entry->nickname);
			if (client_entry->realname) {
				purple_notify_user_info_add_pair_plaintext(user_info, _("Real Name"), client_entry->realname);
			}
			if (*client_entry->hostname) {
				gchar *tmp2;
				tmp2 = g_strdup_printf("%s@%s", client_entry->username, client_entry->hostname);
				purple_notify_user_info_add_pair_plaintext(user_info, _("Username"), tmp2);
				g_free(tmp2);
			} else
				purple_notify_user_info_add_pair_plaintext(user_info, _("Username"), client_entry->username);

			if (client_entry->mode) {
				memset(tmp, 0, sizeof(tmp));
				silcpurple_get_umode_string(client_entry->mode,
							    tmp, sizeof(tmp) - strlen(tmp));
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("User Modes"), tmp);
			}

			silcpurple_parse_attrs(client_entry->attrs, &moodstr, &statusstr, &contactstr, &langstr, &devicestr, &tzstr, &geostr);
			if (moodstr) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Mood"), moodstr);
				g_free(moodstr);
			}

			if (statusstr) {
				purple_notify_user_info_add_pair_plaintext(user_info, _("Status Text"), statusstr);
				g_free(statusstr);
			}

			if (contactstr) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Preferred Contact"), contactstr);
				g_free(contactstr);
			}

			if (langstr) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Preferred Language"), langstr);
				g_free(langstr);
			}

			if (devicestr) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Device"), devicestr);
				g_free(devicestr);
			}

			if (tzstr) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Timezone"), tzstr);
				g_free(tzstr);
			}

			if (geostr) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Geolocation"), geostr);
				g_free(geostr);
			}

			if (*client_entry->server) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Server"), client_entry->server);
			}

			if (channels && user_modes) {
				SilcChannelPayload entry;
				int i = 0;

				memset(tmp, 0, sizeof(tmp));
				silc_dlist_start(channels);
				while ((entry = silc_dlist_get(channels))) {
					SilcUInt32 name_len;
					char *m = silc_client_chumode_char(user_modes[i++]);
					char *name = (char *)silc_channel_get_name(entry, &name_len);
					if (m)
						silc_strncat(tmp, sizeof(tmp) - 1, m, strlen(m));
					silc_strncat(tmp, sizeof(tmp) - 1, name, name_len);
					silc_strncat(tmp, sizeof(tmp) - 1, "  ", 1);
					silc_free(m);
				}
				purple_notify_user_info_add_pair_plaintext(user_info, _("Currently on"), tmp);
			}

			if (client_entry->public_key) {
				char *fingerprint, *babbleprint;
				unsigned char *pk;
				SilcUInt32 pk_len;
				pk = silc_pkcs_public_key_encode(client_entry->public_key, &pk_len);
				if (pk) {
					fingerprint = silc_hash_fingerprint(NULL, pk, pk_len);
					babbleprint = silc_hash_babbleprint(NULL, pk, pk_len);
					purple_notify_user_info_add_pair_plaintext(user_info, _("Public Key Fingerprint"), fingerprint);
					purple_notify_user_info_add_pair_plaintext(user_info, _("Public Key Babbleprint"), babbleprint);
					silc_free(fingerprint);
					silc_free(babbleprint);
					silc_free(pk);
				}
			}

			purple_notify_userinfo(gc, client_entry->nickname, user_info, NULL, NULL);
			purple_notify_user_info_destroy(user_info);
		}
		break;

	case SILC_COMMAND_WHOWAS:
		{
			SilcClientEntry client_entry;
			char *nickname, *realname, *username;
			PurpleNotifyUserInfo *user_info;

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("User Information"),
						  _("Cannot get user information"),
						  silc_get_status_message(error),
						  purple_request_cpar_from_connection(gc));
				break;
			}

			client_entry = va_arg(ap, SilcClientEntry);
			nickname = va_arg(ap, char *);
			username = va_arg(ap, char *);
			realname = va_arg(ap, char *);
			if (!nickname)
				break;

			user_info = purple_notify_user_info_new();
			purple_notify_user_info_add_pair_plaintext(user_info, _("Nickname"), nickname);
			if (realname)
				purple_notify_user_info_add_pair_plaintext(user_info, _("Real Name"), realname);
			if (username) {
				if (client_entry && *client_entry->hostname) {
					gchar *tmp;
					tmp = g_strdup_printf("%s@%s", username, client_entry->hostname);
					purple_notify_user_info_add_pair_plaintext(user_info, _("Username"), tmp);
					g_free(tmp);
				} else
					purple_notify_user_info_add_pair_plaintext(user_info, _("Username"), username);
			}
			if (client_entry && *client_entry->server) {
				/* TODO: Check whether it's correct to call add_pair_html,
				         or if we should be using add_pair_plaintext */
				purple_notify_user_info_add_pair_html(user_info, _("Server"), client_entry->server);
			}


			if (client_entry && client_entry->public_key) {
				char *fingerprint, *babbleprint;
				unsigned char *pk;
				SilcUInt32 pk_len;
				pk = silc_pkcs_public_key_encode(client_entry->public_key, &pk_len);
				if (pk) {
					fingerprint = silc_hash_fingerprint(NULL, pk, pk_len);
					babbleprint = silc_hash_babbleprint(NULL, pk, pk_len);
					purple_notify_user_info_add_pair_plaintext(user_info, _("Public Key Fingerprint"), fingerprint);
					purple_notify_user_info_add_pair_plaintext(user_info, _("Public Key Babbleprint"), babbleprint);
					silc_free(fingerprint);
					silc_free(babbleprint);
					silc_free(pk);
				}
			}

			purple_notify_userinfo(gc, nickname, user_info, NULL, NULL);
			purple_notify_user_info_destroy(user_info);
		}
		break;

	case SILC_COMMAND_DETACH:
		{
			const char *file;
			SilcBuffer detach_data;

			if (status != SILC_STATUS_OK) {
			  purple_notify_error(gc, _("Detach From Server"), _("Cannot detach"),
					      silc_get_status_message(error),
					      purple_request_cpar_from_connection(gc));
			  return;
			}

			detach_data = va_arg(ap, SilcBuffer);

			/* Save the detachment data to file. */
			file = silcpurple_session_file(purple_account_get_username(sg->account));
			g_unlink(file);
			silc_file_writefile(file, (const char *)silc_buffer_data(detach_data),
					    silc_buffer_len(detach_data));
		}
		break;

	case SILC_COMMAND_TOPIC:
		{
			SilcChannelEntry channel;

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Topic"), _("Cannot set topic"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				return;
			}

			channel = va_arg(ap, SilcChannelEntry);

			chat = purple_conversations_find_chat_with_account(
								      channel->channel_name, sg->account);
			if (!chat) {
				purple_debug_error("silc", "Got a topic for %s, which doesn't exist\n",
						   channel->channel_name);
				break;
			}

			/* Set topic */
			if (channel->topic)
				purple_chat_conversation_set_topic(chat, NULL, channel->topic);
		}
		break;

	case SILC_COMMAND_NICK:
		{
			SilcClientEntry local_entry;
			SilcHashTableList htl;
			SilcChannelUser chu;
			const char *oldnick, *newnick;

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Nick"), _("Failed to change nickname"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				return;
			}

			local_entry = va_arg(ap, SilcClientEntry);
			newnick = va_arg(ap, char *);

			/* Change nick on all channels */
			silc_hash_table_list(local_entry->channels, &htl);
			while (silc_hash_table_get(&htl, NULL, (void *)&chu)) {
				chat = purple_conversations_find_chat_with_account(
									      chu->channel->channel_name, sg->account);
				if (!chat)
					continue;
				oldnick = purple_chat_conversation_get_nick(chat);
				if (!purple_strequal(oldnick,
						purple_normalize(purple_conversation_get_account
						(PURPLE_CONVERSATION(chat)), newnick))) {

					purple_chat_conversation_rename_user(chat,
								     oldnick, newnick);
					purple_chat_conversation_set_nick(chat, newnick);
				}
			}
			silc_hash_table_list_reset(&htl);

			purple_connection_set_display_name(gc, newnick);
		}
		break;

	case SILC_COMMAND_LIST:
		{
			char *topic, *name;
			int usercount;
			PurpleRoomlistRoom *room;

			if (sg->roomlist_cancelled)
				break;

			if (error != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Error"), _("Error retrieving room list"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				purple_roomlist_set_in_progress(sg->roomlist, FALSE);
				g_object_unref(sg->roomlist);
				sg->roomlist = NULL;
				return;
			}

			(void)va_arg(ap, SilcChannelEntry);
			name = va_arg(ap, char *);
			if (!name) {
				purple_notify_error(gc, _("Roomlist"), _("Cannot get room list"),
						    _("Network is empty"),
						    purple_request_cpar_from_connection(gc));
				purple_roomlist_set_in_progress(sg->roomlist, FALSE);
				g_object_unref(sg->roomlist);
				sg->roomlist = NULL;
				return;
			}
			topic = va_arg(ap, char *);
			usercount = va_arg(ap, int);

			room = purple_roomlist_room_new(PURPLE_ROOMLIST_ROOMTYPE_ROOM, name, NULL);
			purple_roomlist_room_add_field(sg->roomlist, room, name);
			purple_roomlist_room_add_field(sg->roomlist, room,
						       SILC_32_TO_PTR(usercount));
			purple_roomlist_room_add_field(sg->roomlist, room,
						       topic ? topic : "");
			purple_roomlist_room_add(sg->roomlist, room);

			if (status == SILC_STATUS_LIST_END ||
			    status == SILC_STATUS_OK) {
				purple_roomlist_set_in_progress(sg->roomlist, FALSE);
				g_object_unref(sg->roomlist);
				sg->roomlist = NULL;
			}
		}
		break;

	case SILC_COMMAND_GETKEY:
		{
			SilcPublicKey public_key;

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Get Public Key"),
						    _("Cannot fetch the public key"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				return;
			}

			(void)va_arg(ap, SilcUInt32);
			(void)va_arg(ap, void *);
			public_key = va_arg(ap, SilcPublicKey);

			if (!public_key)
				purple_notify_error(gc, _("Get Public Key"),
						    _("Cannot fetch the public key"),
						    _("No public key was received"),
						    purple_request_cpar_from_connection(gc));
		}
		break;

	case SILC_COMMAND_INFO:
		{

			char *server_name;
			char *server_info;
			char tmp[256];

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Server Information"),
						    _("Cannot get server information"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				return;
			}

			(void)va_arg(ap, SilcServerEntry);
			server_name = va_arg(ap, char *);
			server_info = va_arg(ap, char *);

			if (server_name && server_info) {
				g_snprintf(tmp, sizeof(tmp), "Server: %s\n%s",
					   server_name, server_info);
				purple_notify_info(gc, NULL, _("Server Information"), tmp,
					purple_request_cpar_from_connection(gc));
			}
		}
		break;

	case SILC_COMMAND_STATS:
		{
			SilcClientStats *stats;
			char *msg;

			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Server Statistics"),
						    _("Cannot get server statistics"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				return;
			}

			stats = va_arg(ap, SilcClientStats *);

			msg = g_strdup_printf(_("Local server start time: %s\n"
						"Local server uptime: %s\n"
						"Local server clients: %d\n"
						"Local server channels: %d\n"
						"Local server operators: %d\n"
						"Local router operators: %d\n"
						"Local cell clients: %d\n"
						"Local cell channels: %d\n"
						"Local cell servers: %d\n"
						"Total clients: %d\n"
						"Total channels: %d\n"
						"Total servers: %d\n"
						"Total routers: %d\n"
						"Total server operators: %d\n"
						"Total router operators: %d\n"),
					      silc_time_string(stats->starttime),
					      purple_str_seconds_to_string((int)stats->uptime),
					      (int)stats->my_clients,
					      (int)stats->my_channels,
					      (int)stats->my_server_ops,
					      (int)stats->my_router_ops,
					      (int)stats->cell_clients,
					      (int)stats->cell_channels,
					      (int)stats->cell_servers,
					      (int)stats->clients,
					      (int)stats->channels,
					      (int)stats->servers,
					      (int)stats->routers,
					      (int)stats->server_ops,
					      (int)stats->router_ops);

			purple_notify_info(gc, NULL,
					   _("Network Statistics"), msg,
					   purple_request_cpar_from_connection(gc));
			g_free(msg);
		}
		break;

	case SILC_COMMAND_PING:
		{
			if (status != SILC_STATUS_OK) {
				purple_notify_error(gc, _("Ping"), _("Ping failed"),
						    silc_get_status_message(error),
						    purple_request_cpar_from_connection(gc));
				return;
			}

			purple_notify_info(gc, _("Ping"), _("Ping reply received from server"),
					   NULL, purple_request_cpar_from_connection(gc));
		}
		break;

	case SILC_COMMAND_KILL:
		if (status != SILC_STATUS_OK) {
			purple_notify_error(gc, _("Kill User"),
					    _("Could not kill user"),
					    silc_get_status_message(error),
					    purple_request_cpar_from_connection(gc));
			return;
		}
		break;

	case SILC_COMMAND_CMODE:
		{
			SilcChannelEntry channel_entry;
			SilcDList channel_pubkeys, list;
			SilcArgumentDecodedList e;

			if (status != SILC_STATUS_OK)
				return;

			channel_entry = va_arg(ap, SilcChannelEntry);
			(void)va_arg(ap, SilcUInt32);
			(void)va_arg(ap, SilcPublicKey);
			channel_pubkeys = va_arg(ap, SilcDList);

			if (!sg->chpk)
				break;

			list = silc_dlist_init();

			if (channel_pubkeys) {
			  silc_dlist_start(channel_pubkeys);
			  while ((e = silc_dlist_get(channel_pubkeys))) {
				if (e->arg_type == 0x00 ||
				    e->arg_type == 0x03)
				  silc_dlist_add(list, silc_pkcs_public_key_copy(e->argument));
			  }
			}
			silcpurple_chat_chauth_show(sg, channel_entry, list);
		}
		break;

	case SILC_COMMAND_WATCH:
		if (status != SILC_STATUS_OK) {
			purple_notify_error(gc, _("WATCH"), _("Cannot watch user"),
				silc_get_status_message(error),
				purple_request_cpar_from_connection(gc));
			return;
		}
		break;

	default:
		if (status == SILC_STATUS_OK)
			purple_debug_info("silc", "Unhandled command: %d (succeeded)\n", command);
		else
			purple_debug_info("silc", "Unhandled command: %d (failed: %s)\n", command,
					  silc_get_status_message(error));
		break;
	}
}

/* Generic command reply callback for silc_client_command_send.  Simply
   calls the default command_reply client operation callback */

SilcBool silcpurple_command_reply(SilcClient client, SilcClientConnection conn,
				  SilcCommand command, SilcStatus status,
				  SilcStatus error, void *context, va_list ap)
{
  silc_command_reply(client, conn, command, status, error, ap);
  return TRUE;
}


typedef struct {
        union {
	  SilcAskPassphrase ask_pass;
	  SilcGetAuthMeth get_auth;
	} u;
	void *context;
} *SilcPurpleAskPassphrase;

static void
silc_ask_auth_password_cb(const unsigned char *passphrase,
			  SilcUInt32 passphrase_len, void *context)
{
	SilcPurpleAskPassphrase internal = context;

	if (!passphrase || !(*passphrase))
	  internal->u.get_auth(SILC_AUTH_NONE, NULL, 0, internal->context);
	else
	  internal->u.get_auth(SILC_AUTH_PASSWORD,
			       (unsigned char *)passphrase,
			       passphrase_len, internal->context);
	silc_free(internal);
}

/* Find authentication method and authentication data by hostname and
   port. The hostname may be IP address as well. The `auth_method' is
   the authentication method the remote connection requires.  It is
   however possible that remote accepts also some other authentication
   method.  Application should use the method that may have been
   configured for this connection.  If none has been configured it should
   use the required `auth_method'.  If the `auth_method' is
   SILC_AUTH_NONE, server does not require any authentication or the
   required authentication method is not known.  The `completion'
   callback must be called to deliver the chosen authentication method
   and data. The `conn' may be NULL. */

static void
silc_get_auth_method(SilcClient client, SilcClientConnection conn,
		     char *hostname, SilcUInt16 port,
		     SilcAuthMethod auth_method,
		     SilcGetAuthMeth completion, void *context)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	SilcPurpleAskPassphrase internal;
	const char *password;

	/* Progress */
	if (sg->resuming)
		purple_connection_update_progress(gc, _("Resuming session"), 4, 5);
	else
		purple_connection_update_progress(gc, _("Authenticating connection"), 4, 5);

	/* Check configuration if we have this connection configured. */
	if (auth_method == SILC_AUTH_PUBLIC_KEY &&
	    purple_account_get_bool(sg->account, "pubkey-auth", FALSE)) {
		completion(SILC_AUTH_PUBLIC_KEY, NULL, 0, context);
		return;
	}
	if (auth_method == SILC_AUTH_PASSWORD) {
		password = purple_connection_get_password(gc);
		if (password && *password) {
		  completion(SILC_AUTH_PASSWORD, (unsigned char *)password, strlen(password), context);
		  return;
		}

		/* Ask password from user */
		internal = silc_calloc(1, sizeof(*internal));
		if (!internal)
		  return;
		internal->u.get_auth = completion;
		internal->context = context;
		silc_ask_passphrase(client, conn, silc_ask_auth_password_cb,
				    internal);
		return;
	}

	completion(SILC_AUTH_NONE, NULL, 0, context);
}


/* Called to verify received public key. The `conn_type' indicates which
   entity (server or client) has sent the public key. If user decides to
   trust the key the application may save the key as trusted public key for
   later use. The `completion' must be called after the public key has
   been verified. */

static void
silc_verify_public_key(SilcClient client, SilcClientConnection conn,
		       SilcConnectionType conn_type,
		       SilcPublicKey public_key,
		       SilcVerifyPublicKey completion, void *context)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);

	if (!sg->conn && (conn_type == SILC_CONN_SERVER ||
			  conn_type == SILC_CONN_ROUTER)) {
		/* Progress */
		if (sg->resuming)
			purple_connection_update_progress(gc, _("Resuming session"), 3, 5);
		else
			purple_connection_update_progress(gc, _("Verifying server public key"),
							  3, 5);
	}

	/* Verify public key */
	silcpurple_verify_public_key(client, conn, NULL, conn_type,
				     public_key, completion, context);
}

static void
silc_ask_passphrase_cb(SilcPurpleAskPassphrase internal, const char *passphrase)
{
	if (!passphrase || !(*passphrase))
		internal->u.ask_pass(NULL, 0, internal->context);
	else
		internal->u.ask_pass((unsigned char *)passphrase,
				     strlen(passphrase), internal->context);
	silc_free(internal);
}

/* Ask (interact, that is) a passphrase from user. The passphrase is
   returned to the library by calling the `completion' callback with
   the `context'. The returned passphrase SHOULD be in UTF-8 encoded,
   if not then the library will attempt to encode. */

static void
silc_ask_passphrase(SilcClient client, SilcClientConnection conn,
		    SilcAskPassphrase completion, void *context)
{
	PurpleConnection *gc = client->application;
	SilcPurpleAskPassphrase internal = silc_calloc(1, sizeof(*internal));

	if (!internal)
		return;
	internal->u.ask_pass = completion;
	internal->context = context;
	purple_request_input(gc, _("Passphrase"), NULL,
			     _("Passphrase required"), NULL, FALSE, TRUE, NULL,
			     _("OK"), G_CALLBACK(silc_ask_passphrase_cb),
			     _("Cancel"), G_CALLBACK(silc_ask_passphrase_cb),
			     purple_request_cpar_from_connection(gc), internal);
}


/* Called to indicate that incoming key agreement request has been
   received.  If the application wants to perform key agreement it may
   call silc_client_perform_key_agreement to initiate key agreement or
   silc_client_send_key_agreement to provide connection point to the
   remote client in case the `hostname' is NULL.  If key agreement is
   not desired this request can be ignored.  The `protocol' is either
   value 0 for TCP or value 1 for UDP. */

static void
silc_key_agreement(SilcClient client, SilcClientConnection conn,
		   SilcClientEntry client_entry,
		   const char *hostname, SilcUInt16 protocol,
		   SilcUInt16 port)
{
	silcpurple_buddy_keyagr_request(client, conn, client_entry,
					hostname, port, protocol);
}


/* Notifies application that file transfer protocol session is being
   requested by the remote client indicated by the `client_entry' from
   the `hostname' and `port'. The `session_id' is the file transfer
   session and it can be used to either accept or reject the file
   transfer request, by calling the silc_client_file_receive or
   silc_client_file_close, respectively. */

static void
silc_ftp(SilcClient client, SilcClientConnection conn,
	 SilcClientEntry client_entry, SilcUInt32 session_id,
	 const char *hostname, SilcUInt16 port)
{
	silcpurple_ftp_request(client, conn, client_entry, session_id,
			       hostname, port);
}

SilcClientOperations ops = {
	silc_say,
	silc_channel_message,
	silc_private_message,
	silc_notify,
	silc_command,
	silc_command_reply,
	silc_get_auth_method,
	silc_verify_public_key,
	silc_ask_passphrase,
	silc_key_agreement,
	silc_ftp
};
