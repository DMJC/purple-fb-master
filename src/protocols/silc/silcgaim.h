/*

  silcgaim.h

  Author: Pekka Riikonen <priikone@silcnet.org>

  Copyright (C) 2004 Pekka Riikonen

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

*/

#ifndef SILCGAIM_H
#define SILCGAIM_H

/* Gaim includes */
#include "internal.h"
#include "account.h"
#include "accountopt.h"
#include "debug.h"
#include "multi.h"
#include "notify.h"
#include "prpl.h"
#include "request.h"
#include "server.h"
#include "util.h"
#include "roomlist.h"
#include "ft.h"
#include "cmds.h"

/* Default public and private key file names */
#define SILCGAIM_PUBLIC_KEY_NAME "public_key.pub"
#define SILCGAIM_PRIVATE_KEY_NAME "private_key.prv"

/* Default settings for creating key pair */
#define SILCGAIM_DEF_PKCS "rsa"
#define SILCGAIM_DEF_PKCS_LEN 2048

#define SILCGAIM_PRVGRP 0x001fffff

typedef struct {
	unsigned long id;
	const char *channel;
	unsigned long chid;
	const char *parentch;
	SilcChannelPrivateKey key;
} *SilcGaimPrvgrp;

/* The SILC Gaim plugin context */
typedef struct SilcGaimStruct {
	SilcClient client;
	SilcClientConnection conn;

	guint scheduler;
	GaimConnection *gc;
	GaimAccount *account;
	unsigned long channel_ids;
	GList *grps;

	char *motd;
	GaimRoomlist *roomlist;

	unsigned int detaching            : 1;
	unsigned int resuming             : 1;
	unsigned int roomlist_canceled    : 1;
	unsigned int chpk                 : 1;
} *SilcGaim;


gboolean silcgaim_check_silc_dir(GaimConnection *gc);
void silcgaim_chat_join_done(SilcClient client,
			     SilcClientConnection conn,
			     SilcClientEntry *clients,
			     SilcUInt32 clients_count,
			     void *context);
const char *silcgaim_silcdir(void);
const char *silcgaim_session_file(const char *account);
void silcgaim_verify_public_key(SilcClient client, SilcClientConnection conn,
				const char *name, SilcSocketType conn_type,
				unsigned char *pk, SilcUInt32 pk_len,
				SilcSKEPKType pk_type,
				SilcVerifyPublicKey completion, void *context);
GList *silcgaim_buddy_menu(GaimBuddy *buddy);
void silcgaim_add_buddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group);
void silcgaim_remove_buddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group);
void silcgaim_buddy_keyagr_request(SilcClient client,
				   SilcClientConnection conn,
				   SilcClientEntry client_entry,
				   const char *hostname, SilcUInt16 port);
void silcgaim_idle_set(GaimConnection *gc, int idle);
char *silcgaim_tooltip_text(GaimBuddy *b);
char *silcgaim_status_text(GaimBuddy *b);
gboolean silcgaim_ip_is_private(const char *ip);
void silcgaim_ftp_send_file(GaimConnection *gc, const char *name);
void silcgaim_ftp_request(SilcClient client, SilcClientConnection conn,
			  SilcClientEntry client_entry, SilcUInt32 session_id,
			  const char *hostname, SilcUInt16 port);
void silcgaim_show_public_key(SilcGaim sg,
			      const char *name, SilcPublicKey public_key,
			      GCallback callback, void *context);
void silcgaim_get_info(GaimConnection *gc, const char *who);
SilcAttributePayload
silcgaim_get_attr(SilcDList attrs, SilcAttribute attribute);
void silcgaim_get_umode_string(SilcUInt32 mode, char *buf,
			       SilcUInt32 buf_size);
void silcgaim_get_chmode_string(SilcUInt32 mode, char *buf,
				SilcUInt32 buf_size);
void silcgaim_get_chumode_string(SilcUInt32 mode, char *buf,
				 SilcUInt32 buf_size);
GList *silcgaim_chat_info(GaimConnection *gc);
GList *silcgaim_chat_menu(GaimChat *);
void silcgaim_chat_join(GaimConnection *gc, GHashTable *data);
void silcgaim_chat_invite(GaimConnection *gc, int id, const char *msg,
			  const char *name);
void silcgaim_chat_leave(GaimConnection *gc, int id);
int silcgaim_chat_send(GaimConnection *gc, int id, const char *msg);
void silcgaim_chat_set_topic(GaimConnection *gc, int id, const char *topic);
GaimRoomlist *silcgaim_roomlist_get_list(GaimConnection *gc);
void silcgaim_roomlist_cancel(GaimRoomlist *list);
void silcgaim_chat_chauth_show(SilcGaim sg, SilcChannelEntry channel,
			       SilcBuffer channel_pubkeys);

#ifdef _WIN32
typedef int uid_t;

struct passwd {
	char	*pw_name;	/* user name */
	char	*pw_passwd;	/* user password */
	int		pw_uid;		/* user id */
	int		pw_gid;		/* group id */
	char	*pw_gecos;	/* real name */
	char	*pw_dir;	/* home directory */
	char	*pw_shell;	/* shell program */
};

struct passwd *getpwuid(int uid);
int getuid();
int geteuid();
#endif

#endif /* SILCGAIM_H */
