/* pidgin
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gplugin.h>

#define BUF_LONG (4096)

#ifdef HAVE_X11
# include <X11/Xlib.h>
#endif

#include <gdk/gdkkeysyms.h>

#include <talkatu.h>

#include <purple.h>

#include <math.h>

#include "gtkblist.h"
#include "gtkconv.h"
#include "gtkdialogs.h"
#include "gtkprivacy.h"
#include "gtkutils.h"
#include "pidginavatar.h"
#include "pidginclosebutton.h"
#include "pidgincolor.h"
#include "pidginconversationwindow.h"
#include "pidgincore.h"
#include "pidgingdkpixbuf.h"
#include "pidgininfopane.h"
#include "pidgininvitedialog.h"
#include "pidginmessage.h"
#include "pidginpresenceicon.h"
#include "pidginstylecontext.h"

#define ADD_MESSAGE_HISTORY_AT_ONCE 100

/*
 * A GTK Instant Message pane.
 */
struct _PidginImPane
{
	GtkWidget *block;
	GtkWidget *send_file;
	GtkWidget *sep1;
	GtkWidget *sep2;
	GtkWidget *check;
	GtkWidget *progress;
	guint32 typing_timer;

	GtkWidget *avatar;
};

/*
 * GTK Chat panes.
 */
struct _PidginChatPane
{
	GtkWidget *count;
	GtkWidget *list;
	GtkWidget *topic_text;
};

typedef enum
{
	PIDGIN_CONV_SET_TITLE			= 1 << 0,
	PIDGIN_CONV_BUDDY_ICON			= 1 << 1,
	PIDGIN_CONV_MENU			= 1 << 2,
	PIDGIN_CONV_TAB_ICON			= 1 << 3,
	PIDGIN_CONV_TOPIC			= 1 << 4,
	PIDGIN_CONV_COLORIZE_TITLE		= 1 << 6,
}PidginConvFields;

#define	PIDGIN_CONV_ALL	((1 << 7) - 1)

#define BUDDYICON_SIZE_MIN    32
#define BUDDYICON_SIZE_MAX    96

static GtkWidget *invite_dialog = NULL;

/* Prototypes. <-- because Paco-Paco hates this comment. */
static void got_typing_keypress(PidginConversation *gtkconv, gboolean first);
static void gray_stuff_out(PidginConversation *gtkconv);
static void add_chat_user_common(PurpleChatConversation *chat, PurpleChatUser *cb, const char *old_name);
static void pidgin_conv_updated(PurpleConversation *conv, PurpleConversationUpdateType type);
static void conv_set_unseen(PurpleConversation *gtkconv, PidginUnseenState state);
static void gtkconv_set_unseen(PidginConversation *gtkconv, PidginUnseenState state);
static void update_typing_icon(PidginConversation *gtkconv);
gboolean pidgin_conv_has_focus(PurpleConversation *conv);
static void pidgin_conv_update_fields(PurpleConversation *conv, PidginConvFields fields);

static void pidgin_conv_placement_place(PidginConversation *conv);

/**************************************************************************
 * Callbacks
 **************************************************************************/

static gboolean
close_this_sucker(gpointer data)
{
	PidginConversation *gtkconv = data;
	GList *list = g_list_copy(gtkconv->convs);
	purple_debug_misc("gtkconv", "closing %s", purple_conversation_get_name(list->data));
	g_list_free_full(list, g_object_unref);
	return FALSE;
}

static gboolean
close_conv_cb(GtkButton *button, PidginConversation *gtkconv)
{
	/* We are going to destroy the conversations immediately only if the 'close immediately'
	 * preference is selected. Otherwise, close the conversation after a reasonable timeout
	 * (I am going to consider 10 minutes as a 'reasonable timeout' here.
	 * For chats, close immediately if the chat is not in the buddylist, or if the chat is
	 * not marked 'Persistent' */
	PurpleConversation *conv = gtkconv->active_conv;

	if(PURPLE_IS_IM_CONVERSATION(conv) || PURPLE_IS_CHAT_CONVERSATION(conv)) {
		close_this_sucker(gtkconv);
	}

	return TRUE;
}

static gboolean
lbox_size_allocate_cb(GtkWidget *w, GtkAllocation *allocation, gpointer data)
{
	purple_prefs_set_int(PIDGIN_PREFS_ROOT "/conversations/chat/userlist_width", allocation->width == 1 ? 0 : allocation->width);

	return FALSE;
}

static const char *
pidgin_get_cmd_prefix(void)
{
	return "/";
}


static void clear_conversation_scrollback_cb(PurpleConversation *conv,
                                             void *data)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

	if (PIDGIN_CONVERSATION(conv)) {
		gtkconv->last_flags = 0;
	}
}

static void
send_history_add(PidginConversation *gtkconv, const char *message)
{
	GList *first;

	first = g_list_first(gtkconv->send_history);
	g_free(first->data);
	first->data = g_strdup(message);
	gtkconv->send_history = g_list_prepend(first, NULL);
}

static gboolean
check_for_and_do_command(PurpleConversation *conv)
{
	PidginConversation *gtkconv;
	GtkWidget *input = NULL;
	GtkTextBuffer *buffer = NULL;
	gchar *cmd;
	const gchar *prefix;
	gboolean retval = FALSE;

	gtkconv = PIDGIN_CONVERSATION(conv);
	prefix = pidgin_get_cmd_prefix();

	input = talkatu_editor_get_input(TALKATU_EDITOR(gtkconv->editor));
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input));

	cmd = talkatu_buffer_get_plain_text(TALKATU_BUFFER(buffer));

	if (cmd && g_str_has_prefix(cmd, prefix)) {
		PurpleCmdStatus status;
		char *error, *cmdline, *markup, *send_history;

		send_history = talkatu_markup_get_html(buffer, NULL);
		send_history_add(gtkconv, send_history);
		g_free(send_history);

		cmdline = cmd + strlen(prefix);

		if (purple_strequal(cmdline, "xyzzy")) {
			purple_conversation_write_system_message(conv,
				"Nothing happens", PURPLE_MESSAGE_NO_LOG);
			g_free(cmd);
			return TRUE;
		}

		/* Docs are unclear on whether or not prefix should be removed from
		 * the markup so, ignoring for now.  Notably if the markup is
		 * `<b>/foo arg1</b>` we now have to move the bold tag around?
		 * - gk 20190709 */
		markup = talkatu_markup_get_html(buffer, NULL);
		status = purple_cmd_do_command(conv, cmdline, markup, &error);
		g_free(markup);

		switch (status) {
			case PURPLE_CMD_STATUS_OK:
				retval = TRUE;
				break;
			case PURPLE_CMD_STATUS_NOT_FOUND:
				{
					PurpleProtocol *protocol = NULL;
					PurpleConnection *gc;

					if ((gc = purple_conversation_get_connection(conv)))
						protocol = purple_connection_get_protocol(gc);

					if ((protocol != NULL) && (purple_protocol_get_options(protocol) & OPT_PROTO_SLASH_COMMANDS_NATIVE)) {
						char *spaceslash;

						/* If the first word in the entered text has a '/' in it, then the user
						 * probably didn't mean it as a command. So send the text as message. */
						spaceslash = cmdline;
						while (*spaceslash && *spaceslash != ' ' && *spaceslash != '/')
							spaceslash++;

						if (*spaceslash != '/') {
							purple_conversation_write_system_message(conv,
								_("Unknown command."), PURPLE_MESSAGE_NO_LOG);
							retval = TRUE;
						}
					}
					break;
				}
			case PURPLE_CMD_STATUS_WRONG_ARGS:
				purple_conversation_write_system_message(conv,
					_("Syntax Error:  You typed the wrong "
					"number of arguments to that command."),
					PURPLE_MESSAGE_NO_LOG);
				retval = TRUE;
				break;
			case PURPLE_CMD_STATUS_FAILED:
				purple_conversation_write_system_message(conv,
					error ? error : _("Your command failed for an unknown reason."),
					PURPLE_MESSAGE_NO_LOG);
				g_free(error);
				retval = TRUE;
				break;
			case PURPLE_CMD_STATUS_WRONG_TYPE:
				if(PURPLE_IS_IM_CONVERSATION(conv))
					purple_conversation_write_system_message(conv,
						_("That command only works in chats, not IMs."),
						PURPLE_MESSAGE_NO_LOG);
				else
					purple_conversation_write_system_message(conv,
						_("That command only works in IMs, not chats."),
						PURPLE_MESSAGE_NO_LOG);
				retval = TRUE;
				break;
			case PURPLE_CMD_STATUS_WRONG_PROTOCOL:
				purple_conversation_write_system_message(conv,
					_("That command doesn't work on this protocol."),
					PURPLE_MESSAGE_NO_LOG);
				retval = TRUE;
				break;
		}
	}

	g_free(cmd);

	return retval;
}

static void
send_cb(GtkWidget *widget, PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleAccount *account;
	PurpleMessageFlags flags = 0;
	GtkTextBuffer *buffer = NULL;
	gchar *content;

	account = purple_conversation_get_account(conv);

	buffer = talkatu_editor_get_buffer(TALKATU_EDITOR(gtkconv->editor));

	if (check_for_and_do_command(conv)) {
		talkatu_buffer_clear(TALKATU_BUFFER(buffer));
		return;
	}

	if (PURPLE_IS_CHAT_CONVERSATION(conv) &&
		purple_chat_conversation_has_left(PURPLE_CHAT_CONVERSATION(conv))) {
		return;
	}

	if (!purple_account_is_connected(account)) {
		return;
	}

	content = talkatu_markup_get_html(buffer, NULL);
	if (purple_strequal(content, "")) {
		g_free(content);
		return;
	}

	purple_idle_touch();

	/* XXX: is there a better way to tell if the message has images? */
	// if (strstr(buf, "<img ") != NULL)
	// 	flags |= PURPLE_MESSAGE_IMAGES;

	purple_conversation_send_with_flags(conv, content, flags);

	g_free(content);

	talkatu_buffer_clear(TALKATU_BUFFER(buffer));
	gtkconv_set_unseen(gtkconv, PIDGIN_UNSEEN_NONE);
}

static void
add_remove_cb(GtkWidget *widget, PidginConversation *gtkconv)
{
	PurpleAccount *account;
	const char *name;
	PurpleConversation *conv = gtkconv->active_conv;

	account = purple_conversation_get_account(conv);
	name    = purple_conversation_get_name(conv);

	if (PURPLE_IS_IM_CONVERSATION(conv)) {
		PurpleBuddy *b;

		b = purple_blist_find_buddy(account, name);
		if (b != NULL)
			pidgin_dialogs_remove_buddy(b);
		else if (account != NULL && purple_account_is_connected(account))
			purple_blist_request_add_buddy(account, (char *)name, NULL, NULL);
	} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
		PurpleChat *c;

		c = purple_blist_find_chat(account, name);
		if (c != NULL)
			pidgin_dialogs_remove_chat(c);
		else if (account != NULL && purple_account_is_connected(account))
			purple_blist_request_add_chat(account, NULL, NULL, name);
	}
}

static void chat_do_info(PidginConversation *gtkconv, const char *who)
{
	PurpleChatConversation *chat = PURPLE_CHAT_CONVERSATION(gtkconv->active_conv);
	PurpleConnection *gc;

	if ((gc = purple_conversation_get_connection(gtkconv->active_conv))) {
		pidgin_retrieve_user_info_in_chat(gc, who, purple_chat_conversation_get_id(chat));
	}
}


static void
info_cb(GtkWidget *widget, PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;

	if (PURPLE_IS_IM_CONVERSATION(conv)) {
		pidgin_retrieve_user_info(purple_conversation_get_connection(conv),
					  purple_conversation_get_name(conv));
	} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
		/* Get info of the person currently selected in the GtkTreeView */
		PidginChatPane *gtkchat;
		GtkTreeIter iter;
		GtkTreeModel *model;
		GtkTreeSelection *sel;
		char *name;

		gtkchat = gtkconv->u.chat;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));
		sel   = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtkchat->list));

		if (gtk_tree_selection_get_selected(sel, NULL, &iter))
			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, CHAT_USERS_NAME_COLUMN, &name, -1);
		else
			return;

		chat_do_info(gtkconv, name);
		g_free(name);
	}
}

static void
block_cb(GtkWidget *widget, PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleAccount *account;

	account = purple_conversation_get_account(conv);

	if (account != NULL && purple_account_is_connected(account))
		pidgin_request_add_block(account, purple_conversation_get_name(conv));
}

static void
unblock_cb(GtkWidget *widget, PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleAccount *account;

	account = purple_conversation_get_account(conv);

	if (account != NULL && purple_account_is_connected(account))
		pidgin_request_add_permit(account, purple_conversation_get_name(conv));
}

static void
do_invite(GtkWidget *w, int resp, gpointer data)
{
	PidginInviteDialog *dialog = PIDGIN_INVITE_DIALOG(w);
	PurpleChatConversation *chat = pidgin_invite_dialog_get_conversation(dialog);
	const gchar *contact, *message;

	if (resp == GTK_RESPONSE_ACCEPT) {
		contact = pidgin_invite_dialog_get_contact(dialog);
		if (!g_ascii_strcasecmp(contact, ""))
			return;

		message = pidgin_invite_dialog_get_message(dialog);

		purple_serv_chat_invite(purple_conversation_get_connection(PURPLE_CONVERSATION(chat)),
						 purple_chat_conversation_get_id(chat),
						 message, contact);
	}

	g_clear_pointer(&invite_dialog, gtk_widget_destroy);
}

static void
invite_cb(GtkWidget *widget, PidginConversation *gtkconv)
{
	PurpleChatConversation *chat = PURPLE_CHAT_CONVERSATION(gtkconv->active_conv);

	if (invite_dialog == NULL) {
		invite_dialog = pidgin_invite_dialog_new(chat);

		/* Connect the signals. */
		g_signal_connect(G_OBJECT(invite_dialog), "response",
						 G_CALLBACK(do_invite), NULL);
	}

	gtk_widget_show_all(invite_dialog);
}

#ifdef USE_VV
static void
menu_initiate_media_call_cb(GtkAction *action, gpointer data)
{
#if 0
	PidginConvWindow *win = (PidginConvWindow *)data;
	PurpleConversation *conv = pidgin_conv_window_get_active_conversation(win);
	PurpleAccount *account = purple_conversation_get_account(conv);

	purple_protocol_initiate_media(account,
			purple_conversation_get_name(conv),
			action == win->menu->audio_call ? PURPLE_MEDIA_AUDIO :
			action == win->menu->video_call ? PURPLE_MEDIA_VIDEO :
			action == win->menu->audio_video_call ? PURPLE_MEDIA_AUDIO |
			PURPLE_MEDIA_VIDEO : PURPLE_MEDIA_NONE);
#endif
}
#endif

static void
menu_send_file_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;
	PurpleConversation *conv = pidgin_conversation_window_get_selected(win);

	if (PURPLE_IS_IM_CONVERSATION(conv)) {
		purple_serv_send_file(purple_conversation_get_connection(conv), purple_conversation_get_name(conv), NULL);
	}

}

static void
menu_alias_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;
	PurpleConversation *conv;
	PurpleAccount *account;
	const char *name;

	conv    = pidgin_conversation_window_get_selected(win);
	account = purple_conversation_get_account(conv);
	name    = purple_conversation_get_name(conv);

	if (PURPLE_IS_IM_CONVERSATION(conv)) {
		PurpleBuddy *b;

		b = purple_blist_find_buddy(account, name);
		if (b != NULL)
			pidgin_dialogs_alias_buddy(b);
	} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
		PurpleChat *c;

		c = purple_blist_find_chat(account, name);
		if (c != NULL)
			pidgin_dialogs_alias_chat(c);
	}
}

static void
menu_get_info_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;
	PurpleConversation *conv;

	conv = pidgin_conversation_window_get_selected(win);

	info_cb(NULL, PIDGIN_CONVERSATION(conv));
}

static void
menu_invite_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;
	PurpleConversation *conv;

	conv = pidgin_conversation_window_get_selected(win);

	invite_cb(NULL, PIDGIN_CONVERSATION(conv));
}

static void
menu_block_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;
	PurpleConversation *conv;

	conv = pidgin_conversation_window_get_selected(win);

	block_cb(NULL, PIDGIN_CONVERSATION(conv));
}

static void
menu_unblock_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;
	PurpleConversation *conv;

	conv = pidgin_conversation_window_get_selected(win);

	unblock_cb(NULL, PIDGIN_CONVERSATION(conv));
}

static void
menu_add_remove_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;
	PurpleConversation *conv;

	conv = pidgin_conversation_window_get_selected(win);

	add_remove_cb(NULL, PIDGIN_CONVERSATION(conv));
}

static void
menu_close_conv_cb(GtkAction *action, gpointer data)
{
	PidginConversationWindow *win = data;

	close_conv_cb(NULL, PIDGIN_CONVERSATION(pidgin_conversation_window_get_selected(win)));
}

static void
chat_do_im(PidginConversation *gtkconv, const char *who)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleAccount *account;
	PurpleConnection *gc;
	PurpleProtocol *protocol = NULL;
	gchar *real_who = NULL;

	account = purple_conversation_get_account(conv);
	g_return_if_fail(account != NULL);

	gc = purple_account_get_connection(account);
	g_return_if_fail(gc != NULL);

	protocol = purple_connection_get_protocol(gc);

	if(protocol) {
		real_who = purple_protocol_chat_get_user_real_name(PURPLE_PROTOCOL_CHAT(protocol), gc,
				purple_chat_conversation_get_id(PURPLE_CHAT_CONVERSATION(conv)), who);
	}

	if(!who && !real_who)
		return;

	pidgin_dialogs_im_with_user(account, real_who ? real_who : who);

	g_free(real_who);
}

static void pidgin_conv_chat_update_user(PurpleChatUser *chatuser);

static void
ignore_cb(GtkWidget *w, PidginConversation *gtkconv)
{
	PurpleChatConversation *chat = PURPLE_CHAT_CONVERSATION(gtkconv->active_conv);
	const char *name;

	name = g_object_get_data(G_OBJECT(w), "user_data");

	if (name == NULL)
		return;

	if (purple_chat_conversation_is_ignored_user(chat, name))
		purple_chat_conversation_unignore(chat, name);
	else
		purple_chat_conversation_ignore(chat, name);

	pidgin_conv_chat_update_user(purple_chat_conversation_find_user(chat, name));
}

static void
menu_chat_im_cb(GtkWidget *w, PidginConversation *gtkconv)
{
	const char *who = g_object_get_data(G_OBJECT(w), "user_data");

	chat_do_im(gtkconv, who);
}

static void
menu_chat_send_file_cb(GtkWidget *w, PidginConversation *gtkconv)
{
	PurpleProtocol *protocol;
	PurpleConversation *conv = gtkconv->active_conv;
	const char *who = g_object_get_data(G_OBJECT(w), "user_data");
	PurpleConnection *gc  = purple_conversation_get_connection(conv);
	gchar *real_who = NULL;

	g_return_if_fail(gc != NULL);

	protocol = purple_connection_get_protocol(gc);

	if(protocol) {
		real_who = purple_protocol_chat_get_user_real_name(PURPLE_PROTOCOL_CHAT(protocol), gc,
				purple_chat_conversation_get_id(PURPLE_CHAT_CONVERSATION(conv)), who);
	}

	purple_serv_send_file(gc, real_who ? real_who : who, NULL);
	g_free(real_who);
}

static void
menu_chat_info_cb(GtkWidget *w, PidginConversation *gtkconv)
{
	char *who;

	who = g_object_get_data(G_OBJECT(w), "user_data");

	chat_do_info(gtkconv, who);
}

static void
menu_chat_add_remove_cb(GtkWidget *w, PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleAccount *account;
	PurpleBuddy *b;
	char *name;

	account = purple_conversation_get_account(conv);
	name    = g_object_get_data(G_OBJECT(w), "user_data");
	b       = purple_blist_find_buddy(account, name);

	if (b != NULL)
		pidgin_dialogs_remove_buddy(b);
	else if (account != NULL && purple_account_is_connected(account))
		purple_blist_request_add_buddy(account, name, NULL, NULL);

	gtk_widget_grab_focus(PIDGIN_CONVERSATION(conv)->entry);
}

static GtkWidget *
create_chat_menu(PurpleChatConversation *chat, const char *who, PurpleConnection *gc)
{
	static GtkWidget *menu = NULL;
	PurpleProtocol *protocol = NULL;
	PurpleConversation *conv = PURPLE_CONVERSATION(chat);
	PurpleAccount *account = purple_conversation_get_account(conv);
	gboolean is_me = FALSE;
	GtkWidget *button;
	PurpleBuddy *buddy = NULL;

	if (gc != NULL)
		protocol = purple_connection_get_protocol(gc);

	/*
	 * If a menu already exists, destroy it before creating a new one,
	 * thus freeing-up the memory it occupied.
	 */
	if (menu)
		gtk_widget_destroy(menu);

	if (purple_strequal(purple_chat_conversation_get_nick(chat), purple_normalize(account, who)))
		is_me = TRUE;

	menu = gtk_menu_new();

	if (!is_me) {
                button = pidgin_new_menu_item(menu, _("IM"),
                                NULL,
                                G_CALLBACK(menu_chat_im_cb),
                                PIDGIN_CONVERSATION(conv));

		if (gc == NULL)
			gtk_widget_set_sensitive(button, FALSE);
		else
			g_object_set_data_full(G_OBJECT(button), "user_data", g_strdup(who), g_free);


		if (protocol && PURPLE_IS_PROTOCOL_XFER(protocol))
		{
			gboolean can_receive_file = TRUE;

			button = pidgin_new_menu_item(menu, _("Send File"),
				NULL, G_CALLBACK(menu_chat_send_file_cb),
				PIDGIN_CONVERSATION(conv));

			if (gc == NULL) {
				can_receive_file = FALSE;
			} else {
				gchar *real_who = NULL;
				real_who = purple_protocol_chat_get_user_real_name(PURPLE_PROTOCOL_CHAT(protocol), gc,
					purple_chat_conversation_get_id(chat), who);

				if (!purple_protocol_xfer_can_receive(
						PURPLE_PROTOCOL_XFER(protocol),
						gc, real_who ? real_who : who)) {
					can_receive_file = FALSE;
				}

				g_free(real_who);
			}

			if (!can_receive_file)
				gtk_widget_set_sensitive(button, FALSE);
			else
				g_object_set_data_full(G_OBJECT(button), "user_data", g_strdup(who), g_free);
		}


		if (purple_chat_conversation_is_ignored_user(chat, who))
			button = pidgin_new_menu_item(menu, _("Un-Ignore"),
                                        NULL, G_CALLBACK(ignore_cb),
                                        PIDGIN_CONVERSATION(conv));
		else
			button = pidgin_new_menu_item(menu, _("Ignore"),
                                        NULL, G_CALLBACK(ignore_cb),
                                        PIDGIN_CONVERSATION(conv));

		if (gc == NULL)
			gtk_widget_set_sensitive(button, FALSE);
		else
			g_object_set_data_full(G_OBJECT(button), "user_data", g_strdup(who), g_free);
	}

	if (protocol && PURPLE_PROTOCOL_IMPLEMENTS(protocol, SERVER, get_info)) {
		button = pidgin_new_menu_item(menu, _("Info"),
                                NULL,
                                G_CALLBACK(menu_chat_info_cb),
                                PIDGIN_CONVERSATION(conv));

		if (gc == NULL)
			gtk_widget_set_sensitive(button, FALSE);
		else
			g_object_set_data_full(G_OBJECT(button), "user_data", g_strdup(who), g_free);
	}

	if (!is_me && protocol && !(purple_protocol_get_options(protocol) & OPT_PROTO_UNIQUE_CHATNAME) && PURPLE_PROTOCOL_IMPLEMENTS(protocol, SERVER, add_buddy)) {
		if ((buddy = purple_blist_find_buddy(account, who)) != NULL)
			button = pidgin_new_menu_item(menu, _("Remove"),
                                        NULL,
                                        G_CALLBACK(menu_chat_add_remove_cb),
                                        PIDGIN_CONVERSATION(conv));
		else
			button = pidgin_new_menu_item(menu, _("Add"),
                                        NULL,
                                        G_CALLBACK(menu_chat_add_remove_cb),
                                        PIDGIN_CONVERSATION(conv));

		if (gc == NULL)
			gtk_widget_set_sensitive(button, FALSE);
		else
			g_object_set_data_full(G_OBJECT(button), "user_data", g_strdup(who), g_free);
	}

	if (buddy != NULL)
	{
		if (purple_account_is_connected(account))
			pidgin_append_blist_node_proto_menu(menu, purple_account_get_connection(account),
												  (PurpleBlistNode *)buddy);
		pidgin_append_blist_node_extended_menu(menu, (PurpleBlistNode *)buddy);
		gtk_widget_show_all(menu);
	}

	return menu;
}


static gint
gtkconv_chat_popup_menu_cb(GtkWidget *widget, PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PidginChatPane *gtkchat;
	PurpleConnection *gc;
	PurpleAccount *account;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *menu;
	gchar *who;

	gtkconv = PIDGIN_CONVERSATION(conv);
	gtkchat = gtkconv->u.chat;
	account = purple_conversation_get_account(conv);
	gc      = purple_account_get_connection(account);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtkchat->list));
	if(!gtk_tree_selection_get_selected(sel, NULL, &iter))
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, CHAT_USERS_NAME_COLUMN, &who, -1);
	menu = create_chat_menu (PURPLE_CHAT_CONVERSATION(conv), who, gc);
	pidgin_menu_popup_at_treeview_selection(menu, widget);
	g_free(who);

	return TRUE;
}


static gint
right_click_chat_cb(GtkWidget *widget, GdkEventButton *event,
					PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PidginChatPane *gtkchat;
	PurpleConnection *gc;
	PurpleAccount *account;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	gchar *who;
	int x, y;

	gtkchat = gtkconv->u.chat;
	account = purple_conversation_get_account(conv);
	gc      = purple_account_get_connection(account);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));

	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(gtkchat->list),
								  event->x, event->y, &path, &column, &x, &y);

	if (path == NULL)
		return FALSE;

	gtk_tree_selection_select_path(GTK_TREE_SELECTION(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(gtkchat->list))), path);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtkchat->list),
							 path, NULL, FALSE);
	gtk_widget_grab_focus(GTK_WIDGET(gtkchat->list));

	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, CHAT_USERS_NAME_COLUMN, &who, -1);

	/* emit chat-nick-clicked signal */
	if (event->type == GDK_BUTTON_PRESS) {
		gint plugin_return = GPOINTER_TO_INT(purple_signal_emit_return_1(
					pidgin_conversations_get_handle(), "chat-nick-clicked",
					conv, who, event->button));
		if (plugin_return)
			goto handled;
	}

	if (event->button == GDK_BUTTON_PRIMARY && event->type == GDK_2BUTTON_PRESS) {
		chat_do_im(gtkconv, who);
	} else if (gdk_event_triggers_context_menu((GdkEvent *)event)) {
		GtkWidget *menu = create_chat_menu (PURPLE_CHAT_CONVERSATION(conv), who, gc);
		gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
	}

handled:
	g_free(who);
	gtk_tree_path_free(path);

	return TRUE;
}

static void
activate_list_cb(GtkTreeView *list, GtkTreePath *path, GtkTreeViewColumn *column, PidginConversation *gtkconv)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *who;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));

	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, CHAT_USERS_NAME_COLUMN, &who, -1);
	chat_do_im(gtkconv, who);

	g_free(who);
}

static gboolean
gtkconv_cycle_focus(PidginConversation *gtkconv, GtkDirectionType dir)
{
	PurpleConversation *conv = gtkconv->active_conv;
	gboolean chat = PURPLE_IS_CHAT_CONVERSATION(conv);
	GtkWidget *next = NULL;
	struct {
		GtkWidget *from;
		GtkWidget *to;
	} transitions[] = {
		{gtkconv->entry, gtkconv->history},
		{gtkconv->history, chat ? gtkconv->u.chat->list : gtkconv->entry},
		{chat ? gtkconv->u.chat->list : NULL, gtkconv->entry},
		{NULL, NULL}
	}, *ptr;

	for (ptr = transitions; !next && ptr->from; ptr++) {
		GtkWidget *from, *to;
		if (dir == GTK_DIR_TAB_FORWARD) {
			from = ptr->from;
			to = ptr->to;
		} else {
			from = ptr->to;
			to = ptr->from;
		}
		if (gtk_widget_is_focus(from))
			next = to;
	}

	if (next)
		gtk_widget_grab_focus(next);
	return !!next;
}

static void
update_typing_inserting(PidginConversation *gtkconv)
{
	GtkTextBuffer *buffer = NULL;
	gboolean is_empty = FALSE;

	g_return_if_fail(gtkconv != NULL);

	buffer = talkatu_editor_get_buffer(TALKATU_EDITOR(gtkconv->editor));
	is_empty = talkatu_buffer_get_is_empty(TALKATU_BUFFER(buffer));

	got_typing_keypress(gtkconv, is_empty);
}

static gboolean
update_typing_deleting_cb(PidginConversation *gtkconv)
{
	PurpleIMConversation *im = PURPLE_IM_CONVERSATION(gtkconv->active_conv);
	GtkTextBuffer *buffer = NULL;

	buffer = talkatu_editor_get_buffer(TALKATU_EDITOR(gtkconv->editor));

	if (!talkatu_buffer_get_is_empty(TALKATU_BUFFER(buffer))) {
		/* We deleted all the text, so turn off typing. */
		purple_im_conversation_stop_send_typed_timeout(im);

		purple_serv_send_typing(purple_conversation_get_connection(gtkconv->active_conv),
						 purple_conversation_get_name(gtkconv->active_conv),
						 PURPLE_IM_NOT_TYPING);
	} else {
		/* We're deleting, but not all of it, so it counts as typing. */
		got_typing_keypress(gtkconv, FALSE);
	}

	return FALSE;
}

static void
update_typing_deleting(PidginConversation *gtkconv)
{
	GtkTextBuffer *buffer = NULL;

	g_return_if_fail(gtkconv != NULL);

	buffer = talkatu_editor_get_buffer(TALKATU_EDITOR(gtkconv->editor));

	if (!talkatu_buffer_get_is_empty(TALKATU_BUFFER(buffer))) {
		g_timeout_add(0, (GSourceFunc)update_typing_deleting_cb, gtkconv);
	}
}

static gboolean
conv_keypress_common(PidginConversation *gtkconv, GdkEventKey *event)
{
	/* If CTRL was held down... */
	if (event->state & GDK_CONTROL_MASK) {
		switch (event->keyval) {
			case GDK_KEY_F6:
				if (gtkconv_cycle_focus(gtkconv, event->state & GDK_SHIFT_MASK ? GTK_DIR_TAB_BACKWARD : GTK_DIR_TAB_FORWARD))
					return TRUE;
				break;
		} /* End of switch */
	}

	/* If ALT (or whatever) was held down... */
	else if (event->state & GDK_MOD1_MASK)
	{
	}

	/* If neither CTRL nor ALT were held down... */
	else
	{
		switch (event->keyval) {
		case GDK_KEY_F6:
			if (gtkconv_cycle_focus(gtkconv, event->state & GDK_SHIFT_MASK ? GTK_DIR_TAB_BACKWARD : GTK_DIR_TAB_FORWARD))
				return TRUE;
			break;
		}
	}
	return FALSE;
}

static gboolean
entry_key_press_cb(GtkWidget *entry, GdkEventKey *event, gpointer data)
{
	PurpleConversation *conv;
	PidginConversation *gtkconv;

	gtkconv  = (PidginConversation *)data;
	conv     = gtkconv->active_conv;

	if (conv_keypress_common(gtkconv, event))
		return TRUE;

	/* If CTRL was held down... */
	if (event->state & GDK_CONTROL_MASK) {
	}
	/* If ALT (or whatever) was held down... */
	else if (event->state & GDK_MOD1_MASK) 	{
	}

	/* If neither CTRL nor ALT were held down... */
	else {
		switch (event->keyval) {
		case GDK_KEY_Tab:
		case GDK_KEY_KP_Tab:
		case GDK_KEY_ISO_Left_Tab:
			if (gtkconv->entry != entry)
				break;
			{
				gint plugin_return;
				plugin_return = GPOINTER_TO_INT(purple_signal_emit_return_1(
							pidgin_conversations_get_handle(), "chat-nick-autocomplete",
							conv, event->state & GDK_SHIFT_MASK));
				return plugin_return;
			}
			break;

		case GDK_KEY_Page_Up:
		case GDK_KEY_KP_Page_Up:
			talkatu_scrolled_window_page_up(TALKATU_SCROLLED_WINDOW(gtkconv->history_sw));
			return TRUE;
			break;

		case GDK_KEY_Page_Down:
		case GDK_KEY_KP_Page_Down:
			talkatu_scrolled_window_page_down(TALKATU_SCROLLED_WINDOW(gtkconv->history_sw));
			return TRUE;
			break;

		case GDK_KEY_KP_Enter:
		case GDK_KEY_Return:
			send_cb(entry, gtkconv);
			return TRUE;
			break;

		}
	}

	if (PURPLE_IS_IM_CONVERSATION(conv) &&
			purple_prefs_get_bool("/purple/conversations/im/send_typing")) {

		switch (event->keyval) {
		case GDK_KEY_BackSpace:
		case GDK_KEY_Delete:
		case GDK_KEY_KP_Delete:
			update_typing_deleting(gtkconv);
			break;
		default:
			update_typing_inserting(gtkconv);
		}
	}

	return FALSE;
}

/*
 * If someone tries to type into the conversation backlog of a
 * conversation window then we yank focus from the conversation backlog
 * and give it to the text entry box so that people can type
 * all the live long day and it will get entered into the entry box.
 */
static gboolean
refocus_entry_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GtkWidget *input = NULL;
	PidginConversation *gtkconv = data;

	/* If we have a valid key for the conversation display, then exit */
	if ((event->state & GDK_CONTROL_MASK) ||
		(event->keyval == GDK_KEY_F6) ||
		(event->keyval == GDK_KEY_F10) ||
		(event->keyval == GDK_KEY_Menu) ||
		(event->keyval == GDK_KEY_Shift_L) ||
		(event->keyval == GDK_KEY_Shift_R) ||
		(event->keyval == GDK_KEY_Control_L) ||
		(event->keyval == GDK_KEY_Control_R) ||
		(event->keyval == GDK_KEY_Escape) ||
		(event->keyval == GDK_KEY_Up) ||
		(event->keyval == GDK_KEY_Down) ||
		(event->keyval == GDK_KEY_Left) ||
		(event->keyval == GDK_KEY_Right) ||
		(event->keyval == GDK_KEY_Page_Up) ||
		(event->keyval == GDK_KEY_KP_Page_Up) ||
		(event->keyval == GDK_KEY_Page_Down) ||
		(event->keyval == GDK_KEY_KP_Page_Down) ||
		(event->keyval == GDK_KEY_Home) ||
		(event->keyval == GDK_KEY_End) ||
		(event->keyval == GDK_KEY_Tab) ||
		(event->keyval == GDK_KEY_KP_Tab) ||
		(event->keyval == GDK_KEY_ISO_Left_Tab))
	{
		if (event->type == GDK_KEY_PRESS)
			return conv_keypress_common(gtkconv, event);
		return FALSE;
	}

	input = talkatu_editor_get_input(TALKATU_EDITOR(gtkconv->editor));
	gtk_widget_grab_focus(input);
	gtk_widget_event(input, (GdkEvent *)event);

	return TRUE;
}

void
pidgin_conv_switch_active_conversation(PurpleConversation *conv)
{
	PidginConversation *gtkconv;
	PurpleConversation *old_conv;

	g_return_if_fail(conv != NULL);

	gtkconv = PIDGIN_CONVERSATION(conv);
	old_conv = gtkconv->active_conv;

	purple_debug_info("gtkconv", "setting active conversation on toolbar %p\n",
		conv);

	if (old_conv == conv)
		return;

	gtkconv->active_conv = conv;

	purple_signal_emit(pidgin_conversations_get_handle(), "conversation-switched", conv);

	gray_stuff_out(gtkconv);
	update_typing_icon(gtkconv);
	g_object_set_data(G_OBJECT(gtkconv->entry), "transient_buddy", NULL);
}

void
pidgin_conv_present_conversation(PurpleConversation *conv)
{
#if 0
	PidginConversation *gtkconv;
	GdkModifierType state;

	pidgin_conv_attach_to_conversation(conv);
	gtkconv = PIDGIN_CONVERSATION(conv);

	pidgin_conv_switch_active_conversation(conv);
	/* Switch the tab only if the user initiated the event by pressing
	 * a button or hitting a key. */
	if (gtk_get_current_event_state(&state))
		pidgin_conv_window_switch_gtkconv(gtkconv->win, gtkconv);
	gtk_window_present(GTK_WINDOW(gtkconv->win->window));
#endif
}

static GList *
pidgin_conversations_get_unseen(GList *l,
									PidginUnseenState min_state,
									guint max_count)
{
	GList *r = NULL;
	guint c = 0;

	for (; l != NULL && (max_count == 0 || c < max_count); l = l->next) {
		PurpleConversation *conv = (PurpleConversation*)l->data;
		PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

		if(gtkconv == NULL || gtkconv->active_conv != conv) {
			continue;
		}

		if (gtkconv->unseen_state >= min_state) {
			r = g_list_prepend(r, conv);
			c++;
		}
	}

	return r;
}

GList *
pidgin_conversations_get_unseen_all(PidginUnseenState min_state,
                                    guint max_count)
{
	PurpleConversationManager *manager;
	GList *list, *ret = NULL;

	manager = purple_conversation_manager_get_default();
	list = purple_conversation_manager_get_all(manager);

	ret = pidgin_conversations_get_unseen(list, min_state, max_count);
	g_list_free(list);

	return ret;
}

static GtkActionEntry menu_entries[] =
/* TODO: fill out tooltips... */
{
	/* Conversation menu */
	{ "ConversationMenu", NULL, N_("_Conversation"), NULL, NULL, NULL },

#ifdef USE_VV
	{ "MediaMenu", NULL, N_("M_edia"), NULL, NULL, NULL },
	{ "AudioCall", NULL, N_("_Audio Call"), NULL, NULL, G_CALLBACK(menu_initiate_media_call_cb) },
	{ "VideoCall", NULL, N_("_Video Call"), NULL, NULL, G_CALLBACK(menu_initiate_media_call_cb) },
	{ "AudioVideoCall", NULL, N_("Audio/Video _Call"), NULL, NULL, G_CALLBACK(menu_initiate_media_call_cb) },
#endif

	{ "SendFile", NULL, N_("Se_nd File..."), NULL, NULL, G_CALLBACK(menu_send_file_cb) },
	{ "GetInfo", NULL, N_("_Get Info"), "<control>O", NULL, G_CALLBACK(menu_get_info_cb) },
	{ "Invite", NULL, N_("In_vite..."), NULL, NULL, G_CALLBACK(menu_invite_cb) },
	{ "MoreMenu", NULL, N_("M_ore"), NULL, NULL, NULL },
	{ "Alias", NULL, N_("Al_ias..."), NULL, NULL, G_CALLBACK(menu_alias_cb) },
	{ "Block", NULL, N_("_Block..."), NULL, NULL, G_CALLBACK(menu_block_cb) },
	{ "Unblock", NULL, N_("_Unblock..."), NULL, NULL, G_CALLBACK(menu_unblock_cb) },
	{ "Add", NULL, N_("_Add..."), NULL, NULL, G_CALLBACK(menu_add_remove_cb) },
	{ "Remove", NULL, N_("_Remove..."), NULL, NULL, G_CALLBACK(menu_add_remove_cb) },
	{ "InsertLink", NULL, N_("Insert Lin_k..."), NULL, NULL, NULL },
	{ "InsertImage", NULL, N_("Insert Imag_e..."), NULL, NULL, NULL },
	{ "Close", NULL, N_("_Close"), "<control>W", NULL, G_CALLBACK(menu_close_conv_cb) },
};

/**************************************************************************
 * Utility functions
 **************************************************************************/

static void
got_typing_keypress(PidginConversation *gtkconv, gboolean first)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleIMConversation *im;

	/*
	 * We know we got something, so we at least have to make sure we don't
	 * send PURPLE_IM_TYPED any time soon.
	 */

	im = PURPLE_IM_CONVERSATION(conv);

	purple_im_conversation_stop_send_typed_timeout(im);
	purple_im_conversation_start_send_typed_timeout(im);

	/* Check if we need to send another PURPLE_IM_TYPING message */
	if (first || (purple_im_conversation_get_type_again(im) != 0 &&
				  time(NULL) > purple_im_conversation_get_type_again(im)))
	{
		unsigned int timeout;
		timeout = purple_serv_send_typing(purple_conversation_get_connection(conv),
								   purple_conversation_get_name(conv),
								   PURPLE_IM_TYPING);
		purple_im_conversation_set_type_again(im, timeout);
	}
}

static void
update_typing_icon(PidginConversation *gtkconv)
{
	PurpleIMConversation *im;
	char *message = NULL;

	if (!PURPLE_IS_IM_CONVERSATION(gtkconv->active_conv))
		return;

	im = PURPLE_IM_CONVERSATION(gtkconv->active_conv);

	if (purple_im_conversation_get_typing_state(im) == PURPLE_IM_NOT_TYPING) {
		return;
	}

	if (purple_im_conversation_get_typing_state(im) == PURPLE_IM_TYPING) {
		message = g_strdup_printf(_("\n%s is typing..."), purple_conversation_get_title(PURPLE_CONVERSATION(im)));
	} else {
		message = g_strdup_printf(_("\n%s has stopped typing"), purple_conversation_get_title(PURPLE_CONVERSATION(im)));
	}

	g_free(message);
}

static const char *
get_chat_user_status_icon(PurpleChatConversation *chat, const char *name, PurpleChatUserFlags flags)
{
	const gchar *icon_name = NULL;

	if (flags & PURPLE_CHAT_USER_FOUNDER) {
		icon_name = "pidgin-status-founder";
	} else if (flags & PURPLE_CHAT_USER_OP) {
		icon_name = "pidgin-status-operator";
	} else if (flags & PURPLE_CHAT_USER_HALFOP) {
		icon_name = "pidgin-status-halfop";
	} else if (flags & PURPLE_CHAT_USER_VOICE) {
		icon_name = "pidgin-status-voice";
	} else if ((!flags) && purple_chat_conversation_is_ignored_user(chat, name)) {
		icon_name = "pidgin-status-ignored";
	}
	return icon_name;
}

static void
add_chat_user_common(PurpleChatConversation *chat, PurpleChatUser *cb, const char *old_name)
{
	PidginConversation *gtkconv;
	PurpleConversation *conv;
	PidginChatPane *gtkchat;
	PurpleConnection *gc;
	GtkTreeModel *tm;
	GtkListStore *ls;
	GtkTreePath *newpath;
	const gchar *icon_name;
	GtkTreeIter iter;
	gboolean is_buddy;
	const gchar *name, *alias;
	gchar *tmp, *alias_key;
	PurpleChatUserFlags flags;
	GdkRGBA color;

	alias = purple_chat_user_get_alias(cb);
	name  = purple_chat_user_get_name(cb);
	flags = purple_chat_user_get_flags(cb);

	conv    = PURPLE_CONVERSATION(chat);
	gtkconv = PIDGIN_CONVERSATION(conv);
	gtkchat = gtkconv->u.chat;
	gc      = purple_conversation_get_connection(conv);

	if (!gc || !purple_connection_get_protocol(gc))
		return;

	tm = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));
	ls = GTK_LIST_STORE(tm);

	icon_name = get_chat_user_status_icon(chat, name, flags);

	is_buddy = purple_chat_user_is_buddy(cb);

	tmp = g_utf8_casefold(alias, -1);
	alias_key = g_utf8_collate_key(tmp, -1);
	g_free(tmp);

	pidgin_color_calculate_for_text(name, &color);

	gtk_list_store_insert_with_values(ls, &iter,
/*
* The GTK docs are mute about the effects of the "row" value for performance.
* X-Chat hardcodes their value to 0 (prepend) and -1 (append), so we will too.
* It *might* be faster to search the gtk_list_store and set row accurately,
* but no one in #gtk+ seems to know anything about it either.
* Inserting in the "wrong" location has no visible ill effects. - F.P.
*/
			-1, /* "row" */
			CHAT_USERS_ICON_NAME_COLUMN, icon_name,
			CHAT_USERS_ALIAS_COLUMN, alias,
			CHAT_USERS_ALIAS_KEY_COLUMN, alias_key,
			CHAT_USERS_NAME_COLUMN,  name,
			CHAT_USERS_FLAGS_COLUMN, flags,
			CHAT_USERS_COLOR_COLUMN, &color,
			CHAT_USERS_WEIGHT_COLUMN, is_buddy ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
			-1);

	newpath = gtk_tree_model_get_path(tm, &iter);
	g_object_set_data_full(G_OBJECT(cb), "pidgin-tree-row",
	                       gtk_tree_row_reference_new(tm, newpath),
	                       (GDestroyNotify)gtk_tree_row_reference_free);
	gtk_tree_path_free(newpath);

	g_free(alias_key);
}

static void topic_callback(GtkWidget *w, PidginConversation *gtkconv)
{
	PurpleProtocol *protocol = NULL;
	PurpleConnection *gc;
	PurpleConversation *conv = gtkconv->active_conv;
	PidginChatPane *gtkchat;
	char *new_topic;
	const char *current_topic;

	gc      = purple_conversation_get_connection(conv);

	if(!gc || !(protocol = purple_connection_get_protocol(gc)))
		return;

	if(!PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, set_topic))
		return;

	gtkconv = PIDGIN_CONVERSATION(conv);
	gtkchat = gtkconv->u.chat;
	new_topic = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtkchat->topic_text)));
	current_topic = purple_chat_conversation_get_topic(PURPLE_CHAT_CONVERSATION(conv));

	if(current_topic && !g_utf8_collate(new_topic, current_topic)){
		g_free(new_topic);
		return;
	}

	if (current_topic)
		gtk_entry_set_text(GTK_ENTRY(gtkchat->topic_text), current_topic);
	else
		gtk_entry_set_text(GTK_ENTRY(gtkchat->topic_text), "");

	purple_protocol_chat_set_topic(PURPLE_PROTOCOL_CHAT(protocol), gc, purple_chat_conversation_get_id(PURPLE_CHAT_CONVERSATION(conv)),
			new_topic);

	g_free(new_topic);
}

static gint
sort_chat_users(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	PurpleChatUserFlags f1 = 0, f2 = 0;
	char *user1 = NULL, *user2 = NULL;
	gboolean buddy1 = FALSE, buddy2 = FALSE;
	gint ret = 0;

	gtk_tree_model_get(model, a,
	                   CHAT_USERS_ALIAS_KEY_COLUMN, &user1,
	                   CHAT_USERS_FLAGS_COLUMN, &f1,
	                   CHAT_USERS_WEIGHT_COLUMN, &buddy1,
	                   -1);
	gtk_tree_model_get(model, b,
	                   CHAT_USERS_ALIAS_KEY_COLUMN, &user2,
	                   CHAT_USERS_FLAGS_COLUMN, &f2,
	                   CHAT_USERS_WEIGHT_COLUMN, &buddy2,
	                   -1);

	/* Only sort by membership levels */
	f1 &= PURPLE_CHAT_USER_VOICE | PURPLE_CHAT_USER_HALFOP | PURPLE_CHAT_USER_OP |
			PURPLE_CHAT_USER_FOUNDER;
	f2 &= PURPLE_CHAT_USER_VOICE | PURPLE_CHAT_USER_HALFOP | PURPLE_CHAT_USER_OP |
			PURPLE_CHAT_USER_FOUNDER;

	ret = g_strcmp0(user1, user2);

	if (user1 != NULL && user2 != NULL) {
		if (f1 != f2) {
			/* sort more important users first */
			ret = (f1 > f2) ? -1 : 1;
		} else if (buddy1 != buddy2) {
			ret = (buddy1 > buddy2) ? -1 : 1;
		}
	}

	g_free(user1);
	g_free(user2);

	return ret;
}

static void
update_chat_alias(PurpleBuddy *buddy, PurpleChatConversation *chat, PurpleConnection *gc, PurpleProtocol *protocol)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(PURPLE_CONVERSATION(chat));
	PurpleAccount *account = purple_conversation_get_account(PURPLE_CONVERSATION(chat));
	GtkTreeModel *model;
	char *normalized_name;
	GtkTreeIter iter;
	int f;

	g_return_if_fail(buddy != NULL);
	g_return_if_fail(chat != NULL);

	/* This is safe because this callback is only used in chats, not IMs. */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkconv->u.chat->list));

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
		return;

	normalized_name = g_strdup(purple_normalize(account, purple_buddy_get_name(buddy)));

	do {
		char *name;

		gtk_tree_model_get(model, &iter, CHAT_USERS_NAME_COLUMN, &name, -1);

		if (purple_strequal(normalized_name, purple_normalize(account, name))) {
			const char *alias = name;
			char *tmp;
			char *alias_key = NULL;
			PurpleBuddy *buddy2;

			if (!purple_strequal(purple_chat_conversation_get_nick(chat), purple_normalize(account, name))) {
				/* This user is not me, so look into updating the alias. */

				if ((buddy2 = purple_blist_find_buddy(account, name)) != NULL) {
					alias = purple_buddy_get_contact_alias(buddy2);
				}

				tmp = g_utf8_casefold(alias, -1);
				alias_key = g_utf8_collate_key(tmp, -1);
				g_free(tmp);

				gtk_list_store_set(GTK_LIST_STORE(model), &iter,
								CHAT_USERS_ALIAS_COLUMN, alias,
								CHAT_USERS_ALIAS_KEY_COLUMN, alias_key,
								-1);
				g_free(alias_key);
			}
			g_free(name);
			break;
		}

		f = gtk_tree_model_iter_next(model, &iter);

		g_free(name);
	} while (f != 0);

	g_free(normalized_name);
}

static void
blist_node_aliased_cb(PurpleBlistNode *node, const char *old_alias, PurpleChatConversation *chat)
{
	PurpleConnection *gc;
	PurpleProtocol *protocol;
	PurpleConversation *conv = PURPLE_CONVERSATION(chat);

	g_return_if_fail(node != NULL);
	g_return_if_fail(conv != NULL);

	gc = purple_conversation_get_connection(conv);
	g_return_if_fail(gc != NULL);
	g_return_if_fail(purple_connection_get_protocol(gc) != NULL);
	protocol = purple_connection_get_protocol(gc);

	if (purple_protocol_get_options(protocol) & OPT_PROTO_UNIQUE_CHATNAME)
		return;

	if (PURPLE_IS_CONTACT(node))
	{
		PurpleBlistNode *bnode;

		for(bnode = node->child; bnode; bnode = bnode->next) {

			if(!PURPLE_IS_BUDDY(bnode))
				continue;

			update_chat_alias((PurpleBuddy *)bnode, chat, gc, protocol);
		}
	}
	else if (PURPLE_IS_BUDDY(node))
		update_chat_alias((PurpleBuddy *)node, chat, gc, protocol);
	else if (PURPLE_IS_CHAT(node) &&
			purple_conversation_get_account(conv) == purple_chat_get_account((PurpleChat*)node))
	{
		if (old_alias == NULL || g_utf8_collate(old_alias, purple_conversation_get_title(conv)) == 0)
			pidgin_conv_update_fields(conv, PIDGIN_CONV_SET_TITLE);
	}
}

static void
buddy_cb_common(PurpleBuddy *buddy, PurpleChatConversation *chat, gboolean is_buddy)
{
	GtkTreeModel *model;
	char *normalized_name;
	GtkTreeIter iter;
	PurpleConversation *conv = PURPLE_CONVERSATION(chat);
	int f;

	g_return_if_fail(buddy != NULL);
	g_return_if_fail(conv != NULL);

	/* Do nothing if the buddy does not belong to the conv's account */
	if (purple_buddy_get_account(buddy) != purple_conversation_get_account(conv))
		return;

	/* This is safe because this callback is only used in chats, not IMs. */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(PIDGIN_CONVERSATION(conv)->u.chat->list));

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
		return;

	normalized_name = g_strdup(purple_normalize(purple_conversation_get_account(conv), purple_buddy_get_name(buddy)));

	do {
		char *name;

		gtk_tree_model_get(model, &iter, CHAT_USERS_NAME_COLUMN, &name, -1);

		if (purple_strequal(normalized_name, purple_normalize(purple_conversation_get_account(conv), name))) {
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			                   CHAT_USERS_WEIGHT_COLUMN, is_buddy ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL, -1);
			g_free(name);
			break;
		}

		f = gtk_tree_model_iter_next(model, &iter);

		g_free(name);
	} while (f != 0);

	g_free(normalized_name);

	blist_node_aliased_cb((PurpleBlistNode *)buddy, NULL, chat);
}

static void
buddy_added_cb(PurpleBlistNode *node, PurpleChatConversation *chat)
{
	if (!PURPLE_IS_BUDDY(node))
		return;

	buddy_cb_common(PURPLE_BUDDY(node), chat, TRUE);
}

static void
buddy_removed_cb(PurpleBlistNode *node, PurpleChatConversation *chat)
{
	if (!PURPLE_IS_BUDDY(node))
		return;

	/* If there's another buddy for the same "dude" on the list, do nothing. */
	if (purple_blist_find_buddy(purple_buddy_get_account(PURPLE_BUDDY(node)),
		                  purple_buddy_get_name(PURPLE_BUDDY(node))) != NULL)
		return;

	buddy_cb_common(PURPLE_BUDDY(node), chat, FALSE);
}

static void
setup_chat_topic(PidginConversation *gtkconv, GtkWidget *vbox)
{
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleConnection *gc = purple_conversation_get_connection(conv);
	PurpleProtocol *protocol = purple_connection_get_protocol(gc);
	if (purple_protocol_get_options(protocol) & OPT_PROTO_CHAT_TOPIC)
	{
		GtkWidget *hbox, *label;
		PidginChatPane *gtkchat = gtkconv->u.chat;

		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

		label = gtk_label_new(_("Topic:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

		gtkchat->topic_text = gtk_entry_new();
		gtk_widget_set_size_request(gtkchat->topic_text, -1, BUDDYICON_SIZE_MIN);

		if(!PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, set_topic)) {
			gtk_editable_set_editable(GTK_EDITABLE(gtkchat->topic_text), FALSE);
		} else {
			g_signal_connect(G_OBJECT(gtkchat->topic_text), "activate",
					G_CALLBACK(topic_callback), gtkconv);
		}

		gtk_box_pack_start(GTK_BOX(hbox), gtkchat->topic_text, TRUE, TRUE, 0);
		g_signal_connect(G_OBJECT(gtkchat->topic_text), "key_press_event",
			             G_CALLBACK(entry_key_press_cb), gtkconv);
	}
}

static gboolean
pidgin_conv_userlist_query_tooltip(GtkWidget *widget, int x, int y,
                                   gboolean keyboard_mode, GtkTooltip *tooltip,
                                   gpointer userdata)
{
	PidginConversation *gtkconv = userdata;
	PurpleConversation *conv = NULL;
	PurpleAccount *account = NULL;
	PurpleConnection *connection = NULL;
	GtkTreePath *path = NULL;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	PurpleBlistNode *node = NULL;
	PurpleProtocol *protocol = NULL;
	char *who = NULL;

	conv = gtkconv->active_conv;
	account = purple_conversation_get_account(conv);
	connection = purple_account_get_connection(account);
	if (!PURPLE_IS_CONNECTION(connection)) {
		return FALSE;
	}
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkconv->u.chat->list));

	if (keyboard_mode) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
		if (!gtk_tree_selection_get_selected(selection, NULL, &iter)) {
			return FALSE;
		}
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
	} else {
		gint bx, by;

		gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(widget),
		                                                  x, y, &bx, &by);
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bx, by, &path,
		                              NULL, NULL, NULL);
		if (path == NULL) {
			return FALSE;
		}
		if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(model),
		                             &iter, path))
		{
			gtk_tree_path_free(path);
			return FALSE;
		}
	}

	gtk_tree_model_get(model, &iter, CHAT_USERS_NAME_COLUMN, &who, -1);

	protocol = purple_connection_get_protocol(connection);
	node = (PurpleBlistNode*)purple_blist_find_buddy(account, who);
	g_free(who);

	if (node && protocol && (purple_protocol_get_options(protocol) & OPT_PROTO_UNIQUE_CHATNAME)) {
		if (pidgin_blist_query_tooltip_for_node(node, tooltip)) {
			gtk_tree_view_set_tooltip_row(GTK_TREE_VIEW(widget), tooltip, path);
			gtk_tree_path_free(path);
			return TRUE;
		}
	}

	gtk_tree_path_free(path);
	return FALSE;
}

static void
setup_chat_userlist(PidginConversation *gtkconv, GtkWidget *hpaned)
{
	PidginChatPane *gtkchat = gtkconv->u.chat;
	GtkWidget *lbox, *list;
	GtkListStore *ls;
	GtkCellRenderer *rend;
	GtkTreeViewColumn *col;
	int ul_width;
	void *blist_handle = purple_blist_get_handle();
	PurpleConversation *conv = gtkconv->active_conv;

	/* Build the right pane. */
	lbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_paned_pack2(GTK_PANED(hpaned), lbox, FALSE, TRUE);
	gtk_widget_show(lbox);

	/* Setup the label telling how many people are in the room. */
	gtkchat->count = gtk_label_new(_("0 people in room"));
	gtk_label_set_ellipsize(GTK_LABEL(gtkchat->count), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start(GTK_BOX(lbox), gtkchat->count, FALSE, FALSE, 0);
	gtk_widget_show(gtkchat->count);

	/* Setup the list of users. */

	ls = gtk_list_store_new(CHAT_USERS_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
							G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
							GDK_TYPE_RGBA, G_TYPE_INT, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ls), CHAT_USERS_ALIAS_KEY_COLUMN,
									sort_chat_users, NULL, NULL);

	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ls));

	/* Allow a user to specify gtkrc settings for the chat userlist only */
	gtk_widget_set_name(list, "pidgin_conv_userlist");

	rend = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes(NULL, rend,
			"icon-name", CHAT_USERS_ICON_NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);
	ul_width = purple_prefs_get_int(PIDGIN_PREFS_ROOT "/conversations/chat/userlist_width");
	gtk_widget_set_size_request(lbox, ul_width, -1);

	/* Hack to prevent completely collapsed userlist coming back with a 1 pixel width.
	 * I would have liked to use the GtkPaned "max-position", but for some reason that didn't work */
	if (ul_width == 0)
		gtk_paned_set_position(GTK_PANED(hpaned), 999999);

	g_signal_connect(G_OBJECT(list), "button_press_event",
					 G_CALLBACK(right_click_chat_cb), gtkconv);
	g_signal_connect(G_OBJECT(list), "row-activated",
					 G_CALLBACK(activate_list_cb), gtkconv);
	g_signal_connect(G_OBJECT(list), "popup-menu",
			 G_CALLBACK(gtkconv_chat_popup_menu_cb), gtkconv);
	g_signal_connect(G_OBJECT(lbox), "size-allocate", G_CALLBACK(lbox_size_allocate_cb), gtkconv);

	gtk_widget_set_has_tooltip(list, TRUE);
	g_signal_connect(list, "query-tooltip",
	                 G_CALLBACK(pidgin_conv_userlist_query_tooltip),
	                 gtkconv);

	rend = gtk_cell_renderer_text_new();
	g_object_set(rend,
				 "foreground-set", TRUE,
				 "weight-set", TRUE,
				 NULL);
	g_object_set(G_OBJECT(rend), "editable", TRUE, NULL);

	col = gtk_tree_view_column_new_with_attributes(NULL, rend,
	                                               "text", CHAT_USERS_ALIAS_COLUMN,
	                                               "foreground-rgba", CHAT_USERS_COLOR_COLUMN,
	                                               "weight", CHAT_USERS_WEIGHT_COLUMN,
	                                               NULL);

	purple_signal_connect(blist_handle, "blist-node-added",
						gtkchat, PURPLE_CALLBACK(buddy_added_cb), conv);
	purple_signal_connect(blist_handle, "blist-node-removed",
						gtkchat, PURPLE_CALLBACK(buddy_removed_cb), conv);
	purple_signal_connect(blist_handle, "blist-node-aliased",
						gtkchat, PURPLE_CALLBACK(blist_node_aliased_cb), conv);

	gtk_tree_view_column_set_expand(col, TRUE);
	g_object_set(rend, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
	gtk_widget_show(list);

	gtkchat->list = list;

	gtk_box_pack_start(GTK_BOX(lbox),
		pidgin_make_scrollable(list, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC, GTK_SHADOW_IN, -1, -1),
		TRUE, TRUE, 0);
}

static GtkWidget *
setup_common_pane(PidginConversation *gtkconv)
{
	GtkWidget *vbox, *input;
	PurpleConversation *conv = gtkconv->active_conv;
	gboolean chat = PURPLE_IS_CHAT_CONVERSATION(conv);

	/* Setup the top part of the pane */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);

	/* Setup the info pane */
	gtkconv->infopane = pidgin_info_pane_new(conv);
	gtk_box_pack_start(GTK_BOX(vbox), gtkconv->infopane, FALSE, FALSE, 0);
	gtk_widget_show(gtkconv->infopane);

	/* Setup the history widget */
	gtkconv->history_sw = talkatu_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(
		GTK_SCROLLED_WINDOW(gtkconv->history_sw),
		GTK_SHADOW_IN
	);
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(gtkconv->history_sw),
		GTK_POLICY_NEVER,
		GTK_POLICY_ALWAYS
	);

	gtkconv->history = talkatu_history_new();
	gtk_container_add(GTK_CONTAINER(gtkconv->history_sw), gtkconv->history);

	if (chat) {
		GtkWidget *hpaned;

		/* Add the topic */
		setup_chat_topic(gtkconv, vbox);

		/* Add the talkatu history */
		hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
		gtk_widget_show(hpaned);
		gtk_paned_pack1(GTK_PANED(hpaned), gtkconv->history_sw, TRUE, TRUE);

		/* Now add the userlist */
		setup_chat_userlist(gtkconv, hpaned);
	} else {
		gtk_box_pack_start(GTK_BOX(vbox), gtkconv->history_sw, TRUE, TRUE, 0);
	}
	gtk_widget_show_all(gtkconv->history_sw);

	g_object_set_data(G_OBJECT(gtkconv->history), "gtkconv", gtkconv);

	g_signal_connect(G_OBJECT(gtkconv->history), "key_press_event",
	                 G_CALLBACK(refocus_entry_cb), gtkconv);
	g_signal_connect(G_OBJECT(gtkconv->history), "key_release_event",
	                 G_CALLBACK(refocus_entry_cb), gtkconv);

	/* Setup the entry widget and all signals */
	gtkconv->editor = talkatu_editor_new();
	talkatu_editor_set_buffer(TALKATU_EDITOR(gtkconv->editor), talkatu_html_buffer_new());
	gtk_box_pack_start(GTK_BOX(vbox), gtkconv->editor, FALSE, FALSE, 0);

	input = talkatu_editor_get_input(TALKATU_EDITOR(gtkconv->editor));
	gtk_widget_set_name(input, "pidgin_conv_entry");
	talkatu_input_set_send_binding(TALKATU_INPUT(input), TALKATU_INPUT_SEND_BINDING_RETURN | TALKATU_INPUT_SEND_BINDING_KP_ENTER);
	g_signal_connect(
		G_OBJECT(input),
		"send-message",
		G_CALLBACK(send_cb),
		gtkconv
	);

	return vbox;
}

static PidginConversation *
pidgin_conv_find_gtkconv(PurpleConversation * conv)
{
	PurpleBuddy *bud = purple_blist_find_buddy(purple_conversation_get_account(conv), purple_conversation_get_name(conv));
	PurpleContact *c;
	PurpleBlistNode *cn, *bn;

	if (!bud)
		return NULL;

	if (!(c = purple_buddy_get_contact(bud)))
		return NULL;

	cn = PURPLE_BLIST_NODE(c);
	for (bn = purple_blist_node_get_first_child(cn); bn; bn = purple_blist_node_get_sibling_next(bn)) {
		PurpleBuddy *b = PURPLE_BUDDY(bn);
		PurpleConversation *im;
		PurpleConversationManager *manager;

		manager = purple_conversation_manager_get_default();
		im = purple_conversation_manager_find_im(manager,
		                                         purple_buddy_get_account(b),
		                                         purple_buddy_get_name(b));

		if(PIDGIN_CONVERSATION(im)) {
			return PIDGIN_CONVERSATION(im);
		}
	}

	return NULL;
}

static gboolean
ignore_middle_click(GtkWidget *widget, GdkEventButton *e, gpointer null)
{
	/* A click on the pane is propagated to the notebook containing the pane.
	 * So if Stu accidentally aims high and middle clicks on the pane-handle,
	 * it causes a conversation tab to close. Let's stop that from happening.
	 */
	if (e->button == GDK_BUTTON_MIDDLE && e->type == GDK_BUTTON_PRESS)
		return TRUE;
	return FALSE;
}

/**************************************************************************
 * Conversation UI operations
 **************************************************************************/
static void
private_gtkconv_new(PurpleConversation *conv, gboolean hidden)
{
	PidginConversation *gtkconv;
	GtkWidget *tab_cont, *pane;

	if (PURPLE_IS_IM_CONVERSATION(conv) && (gtkconv = pidgin_conv_find_gtkconv(conv))) {
		purple_debug_misc("gtkconv", "found existing gtkconv %p", gtkconv);
		g_object_set_data(G_OBJECT(conv), "pidgin", gtkconv);
		if (!g_list_find(gtkconv->convs, conv))
			gtkconv->convs = g_list_prepend(gtkconv->convs, conv);
		pidgin_conv_switch_active_conversation(conv);
		return;
	}

	purple_debug_misc("gtkconv", "creating new gtkconv for %p", conv);
	gtkconv = g_new0(PidginConversation, 1);
	g_object_set_data(G_OBJECT(conv), "pidgin", gtkconv);
	gtkconv->active_conv = conv;
	gtkconv->convs = g_list_prepend(gtkconv->convs, conv);
	gtkconv->send_history = g_list_append(NULL, NULL);

	/* Setup some initial variables. */
	gtkconv->unseen_state = PIDGIN_UNSEEN_NONE;
	gtkconv->unseen_count = 0;
	gtkconv->last_flags = 0;

	if (PURPLE_IS_IM_CONVERSATION(conv)) {
		gtkconv->u.im = g_malloc0(sizeof(PidginImPane));
	} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
		gtkconv->u.chat = g_malloc0(sizeof(PidginChatPane));
	}
	pane = setup_common_pane(gtkconv);

	if (pane == NULL) {
		if (PURPLE_IS_CHAT_CONVERSATION(conv))
			g_free(gtkconv->u.chat);
		else if (PURPLE_IS_IM_CONVERSATION(conv))
			g_free(gtkconv->u.im);

		g_free(gtkconv);
		g_object_set_data(G_OBJECT(conv), "pidgin", NULL);
		return;
	}

	g_signal_connect(G_OBJECT(pane), "button_press_event",
	                 G_CALLBACK(ignore_middle_click), NULL);

	/* Setup the container for the tab. */
	gtkconv->tab_cont = tab_cont = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	g_object_set_data(G_OBJECT(tab_cont), "PidginConversation", gtkconv);
	gtk_container_set_border_width(GTK_CONTAINER(tab_cont), 6);
	gtk_box_pack_start(GTK_BOX(tab_cont), pane, TRUE, TRUE, 0);
	gtk_widget_show(pane);

	talkatu_editor_set_toolbar_visible(
		TALKATU_EDITOR(gtkconv->editor),
		purple_prefs_get_bool(PIDGIN_PREFS_ROOT "/conversations/show_formatting_toolbar")
	);

	g_signal_connect_swapped(G_OBJECT(pane), "focus",
	                         G_CALLBACK(gtk_widget_grab_focus),
	                         gtkconv->editor);

	pidgin_conv_placement_place(gtkconv);
}

void
pidgin_conv_new(PurpleConversation *conv)
{
	private_gtkconv_new(conv, FALSE);
	if (PIDGIN_IS_PIDGIN_CONVERSATION(conv))
		purple_signal_emit(pidgin_conversations_get_handle(),
				"conversation-displayed", PIDGIN_CONVERSATION(conv));
}

void
pidgin_conversation_detach(PurpleConversation *conv) {
	if(PIDGIN_IS_PIDGIN_CONVERSATION(conv)) {
		PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

		close_conv_cb(NULL, gtkconv);

		if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
			g_free(gtkconv->u.chat);
		} else if (PURPLE_IS_IM_CONVERSATION(conv)) {
			g_free(gtkconv->u.im);
		}

		g_free(gtkconv);

		g_object_set_data(G_OBJECT(conv), "pidgin", NULL);
	}
}

static void
received_im_msg_cb(PurpleAccount *account, char *sender, char *message,
				   PurpleConversation *conv, PurpleMessageFlags flags)
{
	guint timer;

	/* Somebody wants to keep this conversation around, so don't time it out */
	if (conv) {
		timer = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(conv), "close-timer"));
		if (timer) {
			g_source_remove(timer);
			g_object_set_data(G_OBJECT(conv), "close-timer", GINT_TO_POINTER(0));
		}
	}
}

static void
pidgin_conv_destroy(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	GtkWidget *win;

	gtkconv->convs = g_list_remove(gtkconv->convs, conv);
	/* Don't destroy ourselves until all our convos are gone */
	if (gtkconv->convs) {
		/* Make sure the destroyed conversation is not the active one */
		if (gtkconv->active_conv == conv) {
			gtkconv->active_conv = gtkconv->convs->data;
			purple_conversation_update(gtkconv->active_conv, PURPLE_CONVERSATION_UPDATE_FEATURES);
		}
		return;
	}

	win = gtk_widget_get_toplevel(gtkconv->tab_cont);
	pidgin_conversation_window_remove(PIDGIN_CONVERSATION_WINDOW(win), conv);

	/* If the "Save Conversation" or "Save Icon" dialogs are open then close them */
	purple_request_close_with_handle(gtkconv);
	purple_notify_close_with_handle(gtkconv);

	gtk_widget_destroy(gtkconv->tab_cont);

	if (PURPLE_IS_IM_CONVERSATION(conv)) {
		if (gtkconv->u.im->typing_timer != 0)
			g_source_remove(gtkconv->u.im->typing_timer);

		g_free(gtkconv->u.im);
	} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
		purple_signals_disconnect_by_handle(gtkconv->u.chat);
		g_free(gtkconv->u.chat);
	}

	gtkconv->send_history = g_list_first(gtkconv->send_history);
	g_list_free_full(gtkconv->send_history, g_free);

	if (gtkconv->attach_timer) {
		g_source_remove(gtkconv->attach_timer);
	}

	g_free(gtkconv);
}

static gboolean
writing_msg(PurpleConversation *conv, PurpleMessage *msg, gpointer _unused)
{
	PidginConversation *gtkconv;

	g_return_val_if_fail(msg != NULL, FALSE);

	if (!(purple_message_get_flags(msg) & PURPLE_MESSAGE_ACTIVE_ONLY))
		return FALSE;

	g_return_val_if_fail(conv != NULL, FALSE);
	gtkconv = PIDGIN_CONVERSATION(conv);
	g_return_val_if_fail(gtkconv != NULL, FALSE);

	if (conv == gtkconv->active_conv)
		return FALSE;

	purple_debug_info("gtkconv",
		"Suppressing message for an inactive conversation");

	return TRUE;
}

static void
pidgin_conv_write_conv(PurpleConversation *conv, PurpleMessage *pmsg)
{
	PidginMessage *pidgin_msg = NULL;
	PurpleMessageFlags flags;
	PidginConversation *gtkconv;
	PurpleConnection *gc;
	PurpleAccount *account;
	gboolean plugin_return;

	g_return_if_fail(conv != NULL);
	gtkconv = PIDGIN_CONVERSATION(conv);
	g_return_if_fail(gtkconv != NULL);
	flags = purple_message_get_flags(pmsg);

	account = purple_conversation_get_account(conv);
	g_return_if_fail(account != NULL);
	gc = purple_account_get_connection(account);
	g_return_if_fail(gc != NULL || !(flags & (PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_RECV)));

	plugin_return = GPOINTER_TO_INT(purple_signal_emit_return_1(
		pidgin_conversations_get_handle(),
		(PURPLE_IS_IM_CONVERSATION(conv) ? "displaying-im-msg" : "displaying-chat-msg"),
		conv, pmsg));
	if (plugin_return)
	{
		return;
	}

	pidgin_msg = pidgin_message_new(pmsg);
	talkatu_history_write_message(
		TALKATU_HISTORY(gtkconv->history),
		TALKATU_MESSAGE(pidgin_msg)
	);

	/* Tab highlighting stuff */
	if (!(flags & PURPLE_MESSAGE_SEND) && !pidgin_conv_has_focus(conv))
	{
		PidginUnseenState unseen = PIDGIN_UNSEEN_NONE;

		if ((flags & PURPLE_MESSAGE_NICK) == PURPLE_MESSAGE_NICK)
			unseen = PIDGIN_UNSEEN_NICK;
		else if (((flags & PURPLE_MESSAGE_SYSTEM) == PURPLE_MESSAGE_SYSTEM) ||
			  ((flags & PURPLE_MESSAGE_ERROR) == PURPLE_MESSAGE_ERROR))
			unseen = PIDGIN_UNSEEN_EVENT;
		else if ((flags & PURPLE_MESSAGE_NO_LOG) == PURPLE_MESSAGE_NO_LOG)
			unseen = PIDGIN_UNSEEN_NO_LOG;
		else
			unseen = PIDGIN_UNSEEN_TEXT;

		gtkconv_set_unseen(gtkconv, unseen);
	}

	purple_signal_emit(pidgin_conversations_get_handle(),
		(PURPLE_IS_IM_CONVERSATION(conv) ? "displayed-im-msg" : "displayed-chat-msg"),
		conv, pmsg);
}

static gboolean get_iter_from_chatuser(PurpleChatUser *cb, GtkTreeIter *iter)
{
	GtkTreeRowReference *ref;
	GtkTreePath *path;
	GtkTreeModel *model;

	g_return_val_if_fail(cb != NULL, FALSE);

	ref = g_object_get_data(G_OBJECT(cb), "pidgin-tree-row");
	if (!ref)
		return FALSE;

	if ((path = gtk_tree_row_reference_get_path(ref)) == NULL)
		return FALSE;

	model = gtk_tree_row_reference_get_model(ref);
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(model), iter, path)) {
		gtk_tree_path_free(path);
		return FALSE;
	}

	gtk_tree_path_free(path);
	return TRUE;
}

static void
pidgin_conv_chat_add_users(PurpleChatConversation *chat, GList *cbuddies, gboolean new_arrivals)
{
	PidginConversation *gtkconv;
	PidginChatPane *gtkchat;
	GtkListStore *ls;
	GList *l;

	char tmp[BUF_LONG];
	int num_users;

	gtkconv = PIDGIN_CONVERSATION(PURPLE_CONVERSATION(chat));
	gtkchat = gtkconv->u.chat;

	num_users = purple_chat_conversation_get_users_count(chat);

	g_snprintf(tmp, sizeof(tmp),
			   ngettext("%d person in room", "%d people in room",
						num_users),
			   num_users);

	gtk_label_set_text(GTK_LABEL(gtkchat->count), tmp);

	ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list)));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ls),  GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
										 GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID);

	l = cbuddies;
	while (l != NULL) {
		add_chat_user_common(chat, (PurpleChatUser *)l->data, NULL);
		l = l->next;
	}

	/* Currently GTK maintains our sorted list after it's in the tree.
	 * This may change if it turns out we can manage it faster ourselves.
	 */
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ls),  CHAT_USERS_ALIAS_KEY_COLUMN,
										 GTK_SORT_ASCENDING);
}

static void
pidgin_conv_chat_rename_user(PurpleChatConversation *chat, const char *old_name,
			      const char *new_name, const char *new_alias)
{
	PidginConversation *gtkconv;
	PidginChatPane *gtkchat;
	PurpleChatUser *old_chatuser, *new_chatuser;
	GtkTreeIter iter;
	GtkTreeModel *model;

	gtkconv = PIDGIN_CONVERSATION(PURPLE_CONVERSATION(chat));
	gtkchat = gtkconv->u.chat;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
		return;

	old_chatuser = purple_chat_conversation_find_user(chat, old_name);
	if (!old_chatuser)
		return;

	if (get_iter_from_chatuser(old_chatuser, &iter)) {
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		g_object_set_data(G_OBJECT(old_chatuser), "pidgin-tree-row", NULL);
	}

	g_return_if_fail(new_alias != NULL);

	new_chatuser = purple_chat_conversation_find_user(chat, new_name);

	add_chat_user_common(chat, new_chatuser, old_name);
}

static void
pidgin_conv_chat_remove_users(PurpleChatConversation *chat, GList *users)
{
	PidginConversation *gtkconv;
	PidginChatPane *gtkchat;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList *l;
	char tmp[BUF_LONG];
	int num_users;
	gboolean f;

	gtkconv = PIDGIN_CONVERSATION(PURPLE_CONVERSATION(chat));
	gtkchat = gtkconv->u.chat;

	num_users = purple_chat_conversation_get_users_count(chat);

	for (l = users; l != NULL; l = l->next) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));

		if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
			/* XXX: Break? */
			continue;

		do {
			char *val;

			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
							   CHAT_USERS_NAME_COLUMN, &val, -1);

			if (!purple_utf8_strcasecmp((char *)l->data, val)) {
				f = gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			}
			else
				f = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);

			g_free(val);
		} while (f);
	}

	g_snprintf(tmp, sizeof(tmp),
			   ngettext("%d person in room", "%d people in room",
						num_users), num_users);

	gtk_label_set_text(GTK_LABEL(gtkchat->count), tmp);
}

static void
pidgin_conv_chat_update_user(PurpleChatUser *chatuser)
{
	PurpleChatConversation *chat;
	PidginConversation *gtkconv;
	PidginChatPane *gtkchat;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (!chatuser)
		return;

	chat = purple_chat_user_get_chat(chatuser);
	gtkconv = PIDGIN_CONVERSATION(PURPLE_CONVERSATION(chat));
	gtkchat = gtkconv->u.chat;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
		return;

	if (get_iter_from_chatuser(chatuser, &iter)) {
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		g_object_set_data(G_OBJECT(chatuser), "pidgin-tree-row", NULL);
	}

	add_chat_user_common(chat, chatuser, NULL);
}

gboolean
pidgin_conv_has_focus(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	GtkWidget *win;

	win = gtk_widget_get_toplevel(gtkconv->tab_cont);
	if(gtk_window_has_toplevel_focus(GTK_WINDOW(win))) {
		PidginConversationWindow *convwin = PIDGIN_CONVERSATION_WINDOW(win);

		if(pidgin_conversation_window_conversation_is_selected(convwin, conv)) {
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Makes sure all the menu items and all the buttons are hidden/shown and
 * sensitive/insensitive.  This is called after changing tabs and when an
 * account signs on or off.
 */
static void
gray_stuff_out(PidginConversation *gtkconv)
{
/* This will be replaced by managing an action group in the new conversation
 * window.
 */
#if 0
	PidginConvWindow *win;
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleConnection *gc;
	PurpleProtocol *protocol = NULL;
	PurpleAccount *account;

	win     = pidgin_conv_get_window(gtkconv);
	gc      = purple_conversation_get_connection(conv);
	account = purple_conversation_get_account(conv);

	if (gc != NULL)
		protocol = purple_connection_get_protocol(gc);

	/*
	 * Handle hiding and showing stuff based on what type of conv this is.
	 * Stuff that Purple IMs support in general should be shown for IM
	 * conversations.  Stuff that Purple chats support in general should be
	 * shown for chat conversations.  It doesn't matter whether the protocol
	 * supports it or not--that only affects if the button or menu item
	 * is sensitive or not.
	 */
	if (PURPLE_IS_IM_CONVERSATION(conv)) {
		/* Show stuff that applies to IMs, hide stuff that applies to chats */

		/* Deal with menu items */
		gtk_action_set_visible(win->menu->view_log, TRUE);
		gtk_action_set_visible(win->menu->send_file, TRUE);
		gtk_action_set_visible(win->menu->get_info, TRUE);
		gtk_action_set_visible(win->menu->invite, FALSE);
		gtk_action_set_visible(win->menu->alias, TRUE);
		if (purple_account_privacy_check(account, purple_conversation_get_name(conv))) {
			gtk_action_set_visible(win->menu->unblock, FALSE);
			gtk_action_set_visible(win->menu->block, TRUE);
		} else {
			gtk_action_set_visible(win->menu->block, FALSE);
			gtk_action_set_visible(win->menu->unblock, TRUE);
		}

		if (purple_blist_find_buddy(account, purple_conversation_get_name(conv)) == NULL) {
			gtk_action_set_visible(win->menu->add, TRUE);
			gtk_action_set_visible(win->menu->remove, FALSE);
		} else {
			gtk_action_set_visible(win->menu->remove, TRUE);
			gtk_action_set_visible(win->menu->add, FALSE);
		}

		gtk_action_set_visible(win->menu->insert_link, TRUE);
		gtk_action_set_visible(win->menu->insert_image, TRUE);
	} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
		/* Show stuff that applies to Chats, hide stuff that applies to IMs */

		/* Deal with menu items */
		gtk_action_set_visible(win->menu->view_log, TRUE);
		gtk_action_set_visible(win->menu->send_file, FALSE);
		gtk_action_set_visible(win->menu->get_info, FALSE);
		gtk_action_set_visible(win->menu->invite, TRUE);
		gtk_action_set_visible(win->menu->alias, TRUE);
		gtk_action_set_visible(win->menu->block, FALSE);
		gtk_action_set_visible(win->menu->unblock, FALSE);

		if ((account == NULL) || purple_blist_find_chat(account, purple_conversation_get_name(conv)) == NULL) {
			/* If the chat is NOT in the buddy list */
			gtk_action_set_visible(win->menu->add, TRUE);
			gtk_action_set_visible(win->menu->remove, FALSE);
		} else {
			/* If the chat IS in the buddy list */
			gtk_action_set_visible(win->menu->add, FALSE);
			gtk_action_set_visible(win->menu->remove, TRUE);
		}

		gtk_action_set_visible(win->menu->insert_link, TRUE);
		gtk_action_set_visible(win->menu->insert_image, TRUE);
	}

	/*
	 * Handle graying stuff out based on whether an account is connected
	 * and what features that account supports.
	 */
	if ((gc != NULL) &&
		(!PURPLE_IS_CHAT_CONVERSATION(conv) ||
		 !purple_chat_conversation_has_left(PURPLE_CHAT_CONVERSATION(conv)) ))
	{
		PurpleConnectionFlags features = purple_conversation_get_features(conv);
		/* Account is online */

		/* Deal with menu items */
		gtk_action_set_sensitive(win->menu->view_log, TRUE);
		gtk_action_set_sensitive(win->menu->get_info, (PURPLE_PROTOCOL_IMPLEMENTS(protocol, SERVER, get_info)));
		gtk_action_set_sensitive(win->menu->invite, (PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, invite)));
		gtk_action_set_sensitive(win->menu->insert_link, (features & PURPLE_CONNECTION_FLAG_HTML));
		gtk_action_set_sensitive(win->menu->insert_image, !(features & PURPLE_CONNECTION_FLAG_NO_IMAGES));

		if (PURPLE_IS_IM_CONVERSATION(conv))
		{
			gboolean can_send_file = FALSE;
			const gchar *name = purple_conversation_get_name(conv);

			if (PURPLE_IS_PROTOCOL_XFER(protocol) &&
			    purple_protocol_xfer_can_receive(PURPLE_PROTOCOL_XFER(protocol), gc, name)
			) {
				can_send_file = TRUE;
			}

			gtk_action_set_sensitive(win->menu->add, (PURPLE_PROTOCOL_IMPLEMENTS(protocol, SERVER, add_buddy)));
			gtk_action_set_sensitive(win->menu->remove, (PURPLE_PROTOCOL_IMPLEMENTS(protocol, SERVER, remove_buddy)));
			gtk_action_set_sensitive(win->menu->send_file, can_send_file);
			gtk_action_set_sensitive(win->menu->alias,
									 (account != NULL) &&
									 (purple_blist_find_buddy(account, purple_conversation_get_name(conv)) != NULL));
		}
		else if (PURPLE_IS_CHAT_CONVERSATION(conv))
		{
			gtk_action_set_sensitive(win->menu->add, (PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, join)));
			gtk_action_set_sensitive(win->menu->remove, (PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, join)));
			gtk_action_set_sensitive(win->menu->alias,
									 (account != NULL) &&
									 (purple_blist_find_chat(account, purple_conversation_get_name(conv)) != NULL));
		}

	} else {
		/* Account is offline */
		/* Or it's a chat that we've left. */

		/* Then deal with menu items */
		gtk_action_set_sensitive(win->menu->view_log, TRUE);
		gtk_action_set_sensitive(win->menu->send_file, FALSE);
		gtk_action_set_sensitive(win->menu->get_info, FALSE);
		gtk_action_set_sensitive(win->menu->invite, FALSE);
		gtk_action_set_sensitive(win->menu->alias, FALSE);
		gtk_action_set_sensitive(win->menu->add, FALSE);
		gtk_action_set_sensitive(win->menu->remove, FALSE);
		gtk_action_set_sensitive(win->menu->insert_link, TRUE);
		gtk_action_set_sensitive(win->menu->insert_image, FALSE);
	}
#endif
}

static void
pidgin_conv_update_fields(PurpleConversation *conv, PidginConvFields fields)
{
	PidginConversation *gtkconv;
	PidginConversationWindow *convwin;
	GtkWidget *win;

	gtkconv = PIDGIN_CONVERSATION(conv);
	if (!gtkconv)
		return;

	win = gtk_widget_get_toplevel(gtkconv->tab_cont);
	convwin = PIDGIN_CONVERSATION_WINDOW(win);

	if (fields & PIDGIN_CONV_SET_TITLE)
	{
		purple_conversation_autoset_title(conv);
	}

	if (fields & PIDGIN_CONV_MENU)
	{
		gray_stuff_out(PIDGIN_CONVERSATION(conv));
	}

	if ((fields & PIDGIN_CONV_TOPIC) &&
				PURPLE_IS_CHAT_CONVERSATION(conv))
	{
		const char *topic;
		PidginChatPane *gtkchat = gtkconv->u.chat;

		if (gtkchat->topic_text != NULL)
		{
			topic = purple_chat_conversation_get_topic(PURPLE_CHAT_CONVERSATION(conv));

			gtk_entry_set_text(GTK_ENTRY(gtkchat->topic_text), topic ? topic : "");
			gtk_widget_set_tooltip_text(gtkchat->topic_text,
			                            topic ? topic : "");
		}
	}

	if ((fields & PIDGIN_CONV_COLORIZE_TITLE) ||
			(fields & PIDGIN_CONV_SET_TITLE) ||
			(fields & PIDGIN_CONV_TOPIC))
	{
		char *title;
		PurpleAccount *account = purple_conversation_get_account(conv);
		PurpleBuddy *buddy = NULL;
		char *markup = NULL;

		if ((account == NULL) ||
			!purple_account_is_connected(account) ||
			(PURPLE_IS_CHAT_CONVERSATION(conv)
				&& purple_chat_conversation_has_left(PURPLE_CHAT_CONVERSATION(conv))))
			title = g_strdup_printf("(%s)", purple_conversation_get_title(conv));
		else
			title = g_strdup(purple_conversation_get_title(conv));

		if (PURPLE_IS_IM_CONVERSATION(conv)) {
			buddy = purple_blist_find_buddy(account, purple_conversation_get_name(conv));
			if (buddy) {
				markup = pidgin_blist_get_name_markup(buddy, FALSE, FALSE);
			} else {
				markup = title;
			}
		} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
			const char *topic = gtkconv->u.chat->topic_text
				? gtk_entry_get_text(GTK_ENTRY(gtkconv->u.chat->topic_text))
				: NULL;
			const char *title = purple_conversation_get_title(conv);
			const char *name = purple_conversation_get_name(conv);

			char *topic_esc, *unaliased, *unaliased_esc, *title_esc;

			topic_esc = topic ? g_markup_escape_text(topic, -1) : NULL;
			unaliased = g_utf8_collate(title, name) ? g_strdup_printf("(%s)", name) : NULL;
			unaliased_esc = unaliased ? g_markup_escape_text(unaliased, -1) : NULL;
			title_esc = g_markup_escape_text(title, -1);

			markup = g_strdup_printf("%s%s<span size='smaller'>%s</span>%s<span size='smaller'>%s</span>",
						title_esc,
						unaliased_esc ? " " : "",
						unaliased_esc ? unaliased_esc : "",
						topic_esc  && *topic_esc ? "\n" : "",
						topic_esc ? topic_esc : "");

			g_free(title_esc);
			g_free(topic_esc);
			g_free(unaliased);
			g_free(unaliased_esc);
		}

		if (title != markup)
			g_free(markup);

#if 0
		if (gtkconv->unseen_state == PIDGIN_UNSEEN_TEXT ||
				gtkconv->unseen_state == PIDGIN_UNSEEN_NICK ||
				gtkconv->unseen_state == PIDGIN_UNSEEN_EVENT) {
			PangoAttrList *list = pango_attr_list_new();
			PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
			attr->start_index = 0;
			attr->end_index = -1;
			pango_attr_list_insert(list, attr);
			gtk_label_set_attributes(GTK_LABEL(gtkconv->tab_label), list);
			pango_attr_list_unref(list);
		} else
			gtk_label_set_attributes(GTK_LABEL(gtkconv->tab_label), NULL);
#endif

		if (pidgin_conversation_window_conversation_is_selected(convwin, conv)) {
			const char* current_title = gtk_window_get_title(GTK_WINDOW(win));
			if (current_title == NULL || !purple_strequal(current_title, title)) {
				gtk_window_set_title(GTK_WINDOW(win), title);
			}

			update_typing_icon(gtkconv);
		}

		g_free(title);
	}
}

static void
pidgin_conv_updated(PurpleConversation *conv, PurpleConversationUpdateType type)
{
	PidginConvFields flags = 0;

	g_return_if_fail(conv != NULL);

	if (type == PURPLE_CONVERSATION_UPDATE_ACCOUNT)
	{
		flags = PIDGIN_CONV_ALL;
	}
	else if (type == PURPLE_CONVERSATION_UPDATE_TYPING ||
	         type == PURPLE_CONVERSATION_UPDATE_UNSEEN ||
	         type == PURPLE_CONVERSATION_UPDATE_TITLE)
	{
		flags = PIDGIN_CONV_COLORIZE_TITLE;
	}
	else if (type == PURPLE_CONVERSATION_UPDATE_TOPIC)
	{
		flags = PIDGIN_CONV_TOPIC;
	}
	else if (type == PURPLE_CONVERSATION_ACCOUNT_ONLINE ||
	         type == PURPLE_CONVERSATION_ACCOUNT_OFFLINE)
	{
		flags = PIDGIN_CONV_MENU | PIDGIN_CONV_TAB_ICON | PIDGIN_CONV_SET_TITLE;
	}
	else if (type == PURPLE_CONVERSATION_UPDATE_AWAY)
	{
		flags = PIDGIN_CONV_TAB_ICON;
	}
	else if (type == PURPLE_CONVERSATION_UPDATE_ADD ||
	         type == PURPLE_CONVERSATION_UPDATE_REMOVE ||
	         type == PURPLE_CONVERSATION_UPDATE_CHATLEFT)
	{
		flags = PIDGIN_CONV_SET_TITLE | PIDGIN_CONV_MENU;
	}
	else if (type == PURPLE_CONVERSATION_UPDATE_ICON)
	{
		flags = PIDGIN_CONV_BUDDY_ICON;
	}
	else if (type == PURPLE_CONVERSATION_UPDATE_FEATURES)
	{
		flags = PIDGIN_CONV_MENU;
	}

	pidgin_conv_update_fields(conv, flags);
}

static void
wrote_msg_update_unseen_cb(PurpleConversation *conv, PurpleMessage *msg,
	gpointer _unused)
{
	PurpleMessageFlags flags;
	if (conv == NULL)
		return;
	flags = purple_message_get_flags(msg);
	if (flags & (PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_RECV)) {
		PidginUnseenState unseen = PIDGIN_UNSEEN_NONE;

		if ((flags & PURPLE_MESSAGE_NICK) == PURPLE_MESSAGE_NICK)
			unseen = PIDGIN_UNSEEN_NICK;
		else if (((flags & PURPLE_MESSAGE_SYSTEM) == PURPLE_MESSAGE_SYSTEM) ||
			  ((flags & PURPLE_MESSAGE_ERROR) == PURPLE_MESSAGE_ERROR))
			unseen = PIDGIN_UNSEEN_EVENT;
		else if ((flags & PURPLE_MESSAGE_NO_LOG) == PURPLE_MESSAGE_NO_LOG)
			unseen = PIDGIN_UNSEEN_NO_LOG;
		else
			unseen = PIDGIN_UNSEEN_TEXT;

		conv_set_unseen(conv, unseen);
	}
}

static PurpleConversationUiOps conversation_ui_ops =
{
	pidgin_conv_new,
	pidgin_conv_destroy,              /* destroy_conversation */
	NULL,                              /* write_chat           */
	NULL,                             /* write_im             */
	pidgin_conv_write_conv,           /* write_conv           */
	pidgin_conv_chat_add_users,       /* chat_add_users       */
	pidgin_conv_chat_rename_user,     /* chat_rename_user     */
	pidgin_conv_chat_remove_users,    /* chat_remove_users    */
	pidgin_conv_chat_update_user,     /* chat_update_user     */
	pidgin_conv_present_conversation, /* present              */
	pidgin_conv_has_focus,            /* has_focus            */
	NULL,                             /* send_confirm         */
	NULL,
	NULL,
	NULL,
	NULL
};

PurpleConversationUiOps *
pidgin_conversations_get_conv_ui_ops(void)
{
	return &conversation_ui_ops;
}

/**************************************************************************
 * Public conversation utility functions
 **************************************************************************/
void
pidgin_conv_update_buttons_by_protocol(PurpleConversation *conv)
{
	PidginConversation *gtkconv;
	GtkWidget *win;

	if (!PIDGIN_IS_PIDGIN_CONVERSATION(conv))
		return;

	gtkconv = PIDGIN_CONVERSATION(conv);
	win = gtk_widget_get_toplevel(gtkconv->tab_cont);

	if(PIDGIN_IS_CONVERSATION_WINDOW(win)) {
		PidginConversationWindow *convwin = PIDGIN_CONVERSATION_WINDOW(win);

		if(pidgin_conversation_window_conversation_is_selected(convwin, conv)) {
			gray_stuff_out(PIDGIN_CONVERSATION(conv));
		}
	}
}

static void
show_formatting_toolbar_pref_cb(const char *name, PurplePrefType type,
								gconstpointer value, gpointer data)
{
	GList *list;
	PurpleConversation *conv;
	PurpleConversationManager *manager;
	PidginConversation *gtkconv;
	gboolean visible = (gboolean)GPOINTER_TO_INT(value);

	manager = purple_conversation_manager_get_default();
	list = purple_conversation_manager_get_all(manager);
	while(list != NULL) {
		conv = PURPLE_CONVERSATION(list->data);

		if (!PIDGIN_IS_PIDGIN_CONVERSATION(conv)) {
			list = g_list_delete_link(list, list);

			continue;
		}

		gtkconv = PIDGIN_CONVERSATION(conv);

#if 0
		gtk_toggle_action_set_active(
		        GTK_TOGGLE_ACTION(win->menu->show_formatting_toolbar),
		        visible
		);
#endif

		talkatu_editor_set_toolbar_visible(TALKATU_EDITOR(gtkconv->editor), visible);

		list = g_list_delete_link(list, list);
	}
}

static PidginConversation *
get_gtkconv_with_contact(PurpleContact *contact)
{
	PurpleBlistNode *node;

	node = ((PurpleBlistNode*)contact)->child;

	for (; node; node = node->next)
	{
		PurpleBuddy *buddy = (PurpleBuddy*)node;
		PurpleConversation *im;
		PurpleConversationManager *manager;

		manager = purple_conversation_manager_get_default();
		im = purple_conversation_manager_find_im(manager,
		                                         purple_buddy_get_account(buddy),
		                                         purple_buddy_get_name(buddy));
		if(PURPLE_IS_IM_CONVERSATION(im)) {
			return PIDGIN_CONVERSATION(im);
		}
	}
	return NULL;
}

static void
account_signed_off_cb(PurpleConnection *gc, gpointer event)
{
	PurpleConversationManager *manager;
	GList *list;

	manager = purple_conversation_manager_get_default();
	list = purple_conversation_manager_get_all(manager);

	while(list != NULL) {
		PurpleConversation *conv = PURPLE_CONVERSATION(list->data);

		/* This seems fine in theory, but we also need to cover the
		 * case of this account matching one of the other buddies in
		 * one of the contacts containing the buddy corresponding to
		 * a conversation.  It's easier to just update them all. */
		/* if (purple_conversation_get_account(conv) == account) */
			pidgin_conv_update_fields(conv, PIDGIN_CONV_TAB_ICON |
							PIDGIN_CONV_MENU | PIDGIN_CONV_COLORIZE_TITLE);

		if (PURPLE_CONNECTION_IS_CONNECTED(gc) &&
				PURPLE_IS_CHAT_CONVERSATION(conv) &&
				purple_conversation_get_account(conv) == purple_connection_get_account(gc) &&
				g_object_get_data(G_OBJECT(conv), "want-to-rejoin")) {
			GHashTable *comps = NULL;
			PurpleChat *chat = purple_blist_find_chat(purple_conversation_get_account(conv), purple_conversation_get_name(conv));
			if (chat == NULL) {
				PurpleProtocol *protocol = purple_connection_get_protocol(gc);
				comps = purple_protocol_chat_info_defaults(PURPLE_PROTOCOL_CHAT(protocol), gc, purple_conversation_get_name(conv));
			} else {
				comps = purple_chat_get_components(chat);
			}
			purple_serv_join_chat(gc, comps);
			if (chat == NULL && comps != NULL)
				g_hash_table_destroy(comps);
		}

		list = g_list_delete_link(list, list);
	}
}

static void
account_signing_off(PurpleConnection *gc)
{
	PurpleConversationManager *manager;
	GList *list;
	PurpleAccount *account = purple_connection_get_account(gc);

	manager = purple_conversation_manager_get_default();
	list = purple_conversation_manager_get_all(manager);

	/* We are about to sign off. See which chats we are currently in, and mark
	 * them for rejoin on reconnect. */
	while(list != NULL) {
		if(PURPLE_IS_CHAT_CONVERSATION(list->data)) {
			PurpleConversation *conv;
			PurpleChatConversation *chat;
			gboolean left;

			conv = PURPLE_CONVERSATION(list->data);
			chat = PURPLE_CHAT_CONVERSATION(conv);
			left = purple_chat_conversation_has_left(chat);

			if(!left && purple_conversation_get_account(conv) == account) {
				g_object_set_data(G_OBJECT(conv), "want-to-rejoin",
				                  GINT_TO_POINTER(TRUE));

				purple_conversation_write_system_message(
					conv,
					_("The account has disconnected and you are no longer in "
					  "this chat. You will automatically rejoin the chat when "
					  "the account reconnects."),
					PURPLE_MESSAGE_NO_LOG);
			}
		}

		list = g_list_delete_link(list, list);
	}
}

static void
update_buddy_status_changed(PurpleBuddy *buddy, PurpleStatus *old, PurpleStatus *newstatus)
{
	PidginConversation *gtkconv;
	PurpleConversation *conv;

	gtkconv = get_gtkconv_with_contact(purple_buddy_get_contact(buddy));
	if (gtkconv)
	{
		conv = gtkconv->active_conv;
		pidgin_conv_update_fields(conv, PIDGIN_CONV_TAB_ICON
		                              | PIDGIN_CONV_COLORIZE_TITLE
		                              | PIDGIN_CONV_BUDDY_ICON);
		if ((purple_status_is_online(old) ^ purple_status_is_online(newstatus)) != 0)
			pidgin_conv_update_fields(conv, PIDGIN_CONV_MENU);
	}
}

static void
update_buddy_privacy_changed(PurpleBuddy *buddy)
{
	PidginConversation *gtkconv;
	PurpleConversation *conv;

	gtkconv = get_gtkconv_with_contact(purple_buddy_get_contact(buddy));
	if (gtkconv) {
		conv = gtkconv->active_conv;
		pidgin_conv_update_fields(conv, PIDGIN_CONV_TAB_ICON | PIDGIN_CONV_MENU);
	}
}

static void
update_buddy_idle_changed(PurpleBuddy *buddy, gboolean old, gboolean newidle)
{
	PurpleConversation *im;
	PurpleConversationManager *manager;

	manager = purple_conversation_manager_get_default();
	im = purple_conversation_manager_find_im(manager,
	                                         purple_buddy_get_account(buddy),
	                                         purple_buddy_get_name(buddy));
	if(PURPLE_IS_IM_CONVERSATION(im)) {
		pidgin_conv_update_fields(im, PIDGIN_CONV_TAB_ICON);
	}
}

static void
update_buddy_icon(PurpleBuddy *buddy)
{
	PurpleConversation *im;
	PurpleConversationManager *manager;

	manager = purple_conversation_manager_get_default();
	im = purple_conversation_manager_find_im(manager,
	                                         purple_buddy_get_account(buddy),
	                                         purple_buddy_get_name(buddy));

	if(PURPLE_IS_IM_CONVERSATION(im)) {
		pidgin_conv_update_fields(im, PIDGIN_CONV_BUDDY_ICON);
	}
}

static void
update_buddy_sign(PurpleBuddy *buddy, const char *which)
{
	PurplePresence *presence;
	PurpleStatus *on, *off;

	presence = purple_buddy_get_presence(buddy);
	if (!presence)
		return;
	off = purple_presence_get_status(presence, "offline");
	on = purple_presence_get_status(presence, "available");

	if (*(which+1) == 'f')
		update_buddy_status_changed(buddy, on, off);
	else
		update_buddy_status_changed(buddy, off, on);
}

static void
update_conversation_switched(PurpleConversation *conv)
{
	pidgin_conv_update_fields(conv, PIDGIN_CONV_TAB_ICON |
		PIDGIN_CONV_SET_TITLE | PIDGIN_CONV_MENU |
		PIDGIN_CONV_BUDDY_ICON);
}

static void
update_buddy_typing(PurpleAccount *account, const char *who)
{
	PurpleConversation *conv;
	PurpleConversationManager *manager;
	PidginConversation *gtkconv;

	manager = purple_conversation_manager_get_default();
	conv = purple_conversation_manager_find_im(manager, account, who);
	if(!PURPLE_IS_CONVERSATION(conv)) {
		return;
	}

	gtkconv = PIDGIN_CONVERSATION(conv);
	if(gtkconv && gtkconv->active_conv == conv) {
		pidgin_conv_update_fields(conv, PIDGIN_CONV_COLORIZE_TITLE);
	}
}

static void
update_chat(PurpleChatConversation *chat)
{
	pidgin_conv_update_fields(PURPLE_CONVERSATION(chat), PIDGIN_CONV_TOPIC |
					PIDGIN_CONV_MENU | PIDGIN_CONV_SET_TITLE);
}

static void
update_chat_topic(PurpleChatConversation *chat, const char *old, const char *new)
{
	pidgin_conv_update_fields(PURPLE_CONVERSATION(chat), PIDGIN_CONV_TOPIC);
}

/* Message history stuff */

/* Compare two PurpleMessages, according to time in ascending order. */
static int
message_compare(PurpleMessage *m1, PurpleMessage *m2)
{
	GDateTime *dt1 = purple_message_get_timestamp(m1);
	GDateTime *dt2 = purple_message_get_timestamp(m2);

	return g_date_time_compare(dt1, dt2);
}

/* Adds some message history to the gtkconv. This happens in a idle-callback. */
static gboolean
add_message_history_to_gtkconv(gpointer data)
{
	PidginConversation *gtkconv = data;
	int count = 0;
	int timer = gtkconv->attach_timer;
	GDateTime *when = (GDateTime *)g_object_get_data(G_OBJECT(gtkconv->editor), "attach-start-time");
	gboolean im = (PURPLE_IS_IM_CONVERSATION(gtkconv->active_conv));

	gtkconv->attach_timer = 0;
	while (gtkconv->attach_current && count < ADD_MESSAGE_HISTORY_AT_ONCE) {
		PurpleMessage *msg = gtkconv->attach_current->data;
		GDateTime *dt = purple_message_get_timestamp(msg);
		if (!im && when && g_date_time_difference(dt, when) >= 0) {
			g_object_set_data(G_OBJECT(gtkconv->editor), "attach-start-time", NULL);
		}
		/* XXX: should it be gtkconv->active_conv? */
		pidgin_conv_write_conv(gtkconv->active_conv, msg);
		if (im) {
			gtkconv->attach_current = g_list_delete_link(gtkconv->attach_current, gtkconv->attach_current);
		} else {
			gtkconv->attach_current = gtkconv->attach_current->prev;
		}
		count++;
	}
	gtkconv->attach_timer = timer;
	if (gtkconv->attach_current)
		return TRUE;

	g_source_remove(gtkconv->attach_timer);
	gtkconv->attach_timer = 0;
	if (im) {
		/* Print any message that was sent while the old history was being added back. */
		GList *msgs = NULL;
		GList *iter = gtkconv->convs;
		for (; iter; iter = iter->next) {
			PurpleConversation *conv = iter->data;
			GList *history = purple_conversation_get_message_history(conv);
			for (; history; history = history->next) {
				PurpleMessage *msg = history->data;
				GDateTime *dt = purple_message_get_timestamp(msg);
				if(g_date_time_difference(dt, when) > 0) {
					msgs = g_list_prepend(msgs, msg);
				}
			}
		}
		msgs = g_list_sort(msgs, (GCompareFunc)message_compare);
		for (; msgs; msgs = g_list_delete_link(msgs, msgs)) {
			PurpleMessage *msg = msgs->data;
			/* XXX: see above - should it be active_conv? */
			pidgin_conv_write_conv(gtkconv->active_conv, msg);
		}
		g_object_set_data(G_OBJECT(gtkconv->editor), "attach-start-time", NULL);
	}

	g_object_set_data(G_OBJECT(gtkconv->editor), "attach-start-time", NULL);
	purple_signal_emit(pidgin_conversations_get_handle(),
			"conversation-displayed", gtkconv);
	return FALSE;
}

static void
pidgin_conv_attach(PurpleConversation *conv)
{
	int timer;
	g_object_set_data(G_OBJECT(conv), "unseen-count", NULL);
	g_object_set_data(G_OBJECT(conv), "unseen-state", NULL);
	purple_conversation_set_ui_ops(conv, pidgin_conversations_get_conv_ui_ops());
	if (!PIDGIN_CONVERSATION(conv))
		private_gtkconv_new(conv, FALSE);
	timer = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(conv), "close-timer"));
	if (timer) {
		g_source_remove(timer);
		g_object_set_data(G_OBJECT(conv), "close-timer", NULL);
	}
}

gboolean pidgin_conv_attach_to_conversation(PurpleConversation *conv)
{
	GList *list;
	PidginConversation *gtkconv;

	pidgin_conv_attach(conv);
	gtkconv = PIDGIN_CONVERSATION(conv);

	list = purple_conversation_get_message_history(conv);
	if (list) {
		GDateTime *dt = NULL;

		if (PURPLE_IS_IM_CONVERSATION(conv)) {
			PurpleConversationManager *manager;
			GList *convs;

			list = g_list_copy(list);
			manager = purple_conversation_manager_get_default();
			convs = purple_conversation_manager_get_all(manager);

			while(convs != NULL) {
				if(!PURPLE_IS_IM_CONVERSATION(convs->data)) {
					convs = g_list_delete_link(convs, convs);

					continue;
				}
				if (convs->data != conv &&
						pidgin_conv_find_gtkconv(convs->data) == gtkconv) {
					pidgin_conv_attach(convs->data);
					list = g_list_concat(list, g_list_copy(purple_conversation_get_message_history(convs->data)));
				}

				convs = g_list_delete_link(convs, convs);
			}
			list = g_list_sort(list, (GCompareFunc)message_compare);
			gtkconv->attach_current = list;
			list = g_list_last(list);
		} else if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
			gtkconv->attach_current = g_list_last(list);
		}

		dt = purple_message_get_timestamp(PURPLE_MESSAGE(list->data));
		g_object_set_data_full(G_OBJECT(gtkconv->editor), "attach-start-time",
		                       g_date_time_ref(dt), (GDestroyNotify)g_date_time_unref);
		gtkconv->attach_timer = g_idle_add(add_message_history_to_gtkconv, gtkconv);
	} else {
		purple_signal_emit(pidgin_conversations_get_handle(),
				"conversation-displayed", gtkconv);
	}

	if (PURPLE_IS_CHAT_CONVERSATION(conv)) {
		GList *users;
		PurpleChatConversation *chat = PURPLE_CHAT_CONVERSATION(conv);
		pidgin_conv_update_fields(conv, PIDGIN_CONV_TOPIC);
		users = purple_chat_conversation_get_users(chat);
		pidgin_conv_chat_add_users(chat, users, TRUE);
		g_list_free(users);
	}

	return TRUE;
}

void *
pidgin_conversations_get_handle(void)
{
	static int handle;

	return &handle;
}

void
pidgin_conversations_init(void)
{
	void *handle = pidgin_conversations_get_handle();
	void *blist_handle = purple_blist_get_handle();

	/* Conversations */
	purple_prefs_add_none(PIDGIN_PREFS_ROOT "/conversations");
	purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/conversations/use_smooth_scrolling", TRUE);
	purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/conversations/send_bold", FALSE);
	purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/conversations/send_italic", FALSE);
	purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/conversations/send_underline", FALSE);
	purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/conversations/send_strike", FALSE);
	purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/conversations/show_incoming_formatting", TRUE);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/minimum_entry_lines", 2);

	purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/conversations/show_formatting_toolbar", TRUE);

	purple_prefs_add_string(PIDGIN_PREFS_ROOT "/conversations/bgcolor", "");
	purple_prefs_add_string(PIDGIN_PREFS_ROOT "/conversations/fgcolor", "");
	purple_prefs_add_string(PIDGIN_PREFS_ROOT "/conversations/font_face", "");
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/font_size", 3);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/scrollback_lines", 4000);

	/* Conversations -> Chat */
	purple_prefs_add_none(PIDGIN_PREFS_ROOT "/conversations/chat");
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/chat/entry_height", 54);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/chat/userlist_width", 80);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/chat/x", 0);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/chat/y", 0);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/chat/width", 340);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/chat/height", 390);

	/* Conversations -> IM */
	purple_prefs_add_none(PIDGIN_PREFS_ROOT "/conversations/im");
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/im/x", 0);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/im/y", 0);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/im/width", 340);
	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/im/height", 390);

	purple_prefs_add_int(PIDGIN_PREFS_ROOT "/conversations/im/entry_height", 54);

	/* Connect callbacks. */
	purple_prefs_connect_callback(handle, PIDGIN_PREFS_ROOT "/conversations/show_formatting_toolbar",
								show_formatting_toolbar_pref_cb, NULL);

	/**********************************************************************
	 * Register signals
	 **********************************************************************/
	purple_signal_register(handle, "conversation-dragging",
	                     purple_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2,
	                     G_TYPE_POINTER, /* pointer to a (PidginConvWindow *) */
	                     G_TYPE_POINTER); /* pointer to a (PidginConvWindow *) */

	purple_signal_register(handle, "displaying-im-msg",
		purple_marshal_BOOLEAN__POINTER_POINTER,
		G_TYPE_BOOLEAN, 2, PURPLE_TYPE_CONVERSATION, PURPLE_TYPE_MESSAGE);

	purple_signal_register(handle, "displayed-im-msg",
		purple_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2,
		PURPLE_TYPE_CONVERSATION, PURPLE_TYPE_MESSAGE);

	purple_signal_register(handle, "displaying-chat-msg",
		purple_marshal_BOOLEAN__POINTER_POINTER,
		G_TYPE_BOOLEAN, 2, PURPLE_TYPE_CONVERSATION, PURPLE_TYPE_MESSAGE);

	purple_signal_register(handle, "displayed-chat-msg",
		purple_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2,
		PURPLE_TYPE_CONVERSATION, PURPLE_TYPE_MESSAGE);

	purple_signal_register(handle, "conversation-switched",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 PURPLE_TYPE_CONVERSATION);

	purple_signal_register(handle, "conversation-displayed",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 G_TYPE_POINTER); /* (PidginConversation *) */

	purple_signal_register(handle, "chat-nick-autocomplete",
						 purple_marshal_BOOLEAN__POINTER_BOOLEAN,
						 G_TYPE_BOOLEAN, 1, PURPLE_TYPE_CONVERSATION);

	purple_signal_register(handle, "chat-nick-clicked",
						 purple_marshal_BOOLEAN__POINTER_POINTER_UINT,
						 G_TYPE_BOOLEAN, 3, PURPLE_TYPE_CONVERSATION,
						 G_TYPE_STRING, G_TYPE_UINT);

	purple_signal_register(handle, "conversation-window-created",
		purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER); /* (PidginConvWindow *) */


	/**********************************************************************
	 * UI operations
	 **********************************************************************/

	purple_signal_connect(purple_connections_get_handle(), "signed-on", handle,
						G_CALLBACK(account_signed_off_cb),
						GINT_TO_POINTER(PURPLE_CONVERSATION_ACCOUNT_ONLINE));
	purple_signal_connect(purple_connections_get_handle(), "signed-off", handle,
						G_CALLBACK(account_signed_off_cb),
						GINT_TO_POINTER(PURPLE_CONVERSATION_ACCOUNT_OFFLINE));
	purple_signal_connect(purple_connections_get_handle(), "signing-off", handle,
						G_CALLBACK(account_signing_off), NULL);

	purple_signal_connect(purple_conversations_get_handle(), "writing-im-msg",
		handle, G_CALLBACK(writing_msg), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "writing-chat-msg",
		handle, G_CALLBACK(writing_msg), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "received-im-msg",
						handle, G_CALLBACK(received_im_msg_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "cleared-message-history",
	                      handle, G_CALLBACK(clear_conversation_scrollback_cb), NULL);

	purple_conversations_set_ui_ops(&conversation_ui_ops);

	/* Callbacks to update a conversation */
	purple_signal_connect(blist_handle, "buddy-signed-on",
						handle, PURPLE_CALLBACK(update_buddy_sign), "on");
	purple_signal_connect(blist_handle, "buddy-signed-off",
						handle, PURPLE_CALLBACK(update_buddy_sign), "off");
	purple_signal_connect(blist_handle, "buddy-status-changed",
						handle, PURPLE_CALLBACK(update_buddy_status_changed), NULL);
	purple_signal_connect(blist_handle, "buddy-privacy-changed",
						handle, PURPLE_CALLBACK(update_buddy_privacy_changed), NULL);
	purple_signal_connect(blist_handle, "buddy-idle-changed",
						handle, PURPLE_CALLBACK(update_buddy_idle_changed), NULL);
	purple_signal_connect(blist_handle, "buddy-icon-changed",
						handle, PURPLE_CALLBACK(update_buddy_icon), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "buddy-typing",
						handle, PURPLE_CALLBACK(update_buddy_typing), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped",
						handle, PURPLE_CALLBACK(update_buddy_typing), NULL);
	purple_signal_connect(pidgin_conversations_get_handle(), "conversation-switched",
						handle, PURPLE_CALLBACK(update_conversation_switched), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "chat-left", handle,
						PURPLE_CALLBACK(update_chat), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "chat-joined", handle,
						PURPLE_CALLBACK(update_chat), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed", handle,
						PURPLE_CALLBACK(update_chat_topic), NULL);
	purple_signal_connect_priority(purple_conversations_get_handle(), "conversation-updated", handle,
						PURPLE_CALLBACK(pidgin_conv_updated), NULL,
						PURPLE_SIGNAL_PRIORITY_LOWEST);
	purple_signal_connect(purple_conversations_get_handle(), "wrote-im-msg", handle,
			PURPLE_CALLBACK(wrote_msg_update_unseen_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "wrote-chat-msg", handle,
			PURPLE_CALLBACK(wrote_msg_update_unseen_cb), NULL);
}

void
pidgin_conversations_uninit(void)
{
	purple_prefs_disconnect_by_handle(pidgin_conversations_get_handle());
	purple_signals_disconnect_by_handle(pidgin_conversations_get_handle());
	purple_signals_unregister_by_instance(pidgin_conversations_get_handle());
}














/* down here is where gtkconvwin.c ought to start. except they share like every freaking function,
 * and touch each others' private members all day long */

/* pidgin
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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

#include <gdk/gdkkeysyms.h>

#include <purple.h>

#include "gtkblist.h"
#include "gtkconv.h"
#include "gtkdialogs.h"
#include "gtkprivacy.h"
#include "gtkutils.h"

/**************************************************************************
 * Callbacks
 **************************************************************************/

static void
conv_set_unseen(PurpleConversation *conv, PidginUnseenState state)
{
	int unseen_count = 0;
	PidginUnseenState unseen_state = PIDGIN_UNSEEN_NONE;

	if(g_object_get_data(G_OBJECT(conv), "unseen-count"))
		unseen_count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(conv), "unseen-count"));

	if(g_object_get_data(G_OBJECT(conv), "unseen-state"))
		unseen_state = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(conv), "unseen-state"));

	if (state == PIDGIN_UNSEEN_NONE)
	{
		unseen_count = 0;
		unseen_state = PIDGIN_UNSEEN_NONE;
	}
	else
	{
		if (state >= PIDGIN_UNSEEN_TEXT)
			unseen_count++;

		if (state > unseen_state)
			unseen_state = state;
	}

	g_object_set_data(G_OBJECT(conv), "unseen-count", GINT_TO_POINTER(unseen_count));
	g_object_set_data(G_OBJECT(conv), "unseen-state", GINT_TO_POINTER(unseen_state));

	purple_conversation_update(conv, PURPLE_CONVERSATION_UPDATE_UNSEEN);
}

static void
gtkconv_set_unseen(PidginConversation *gtkconv, PidginUnseenState state)
{
	if (state == PIDGIN_UNSEEN_NONE)
	{
		gtkconv->unseen_count = 0;
		gtkconv->unseen_state = PIDGIN_UNSEEN_NONE;
	}
	else
	{
		if (state >= PIDGIN_UNSEEN_TEXT)
			gtkconv->unseen_count++;

		if (state > gtkconv->unseen_state)
			gtkconv->unseen_state = state;
	}

	g_object_set_data(G_OBJECT(gtkconv->active_conv), "unseen-count", GINT_TO_POINTER(gtkconv->unseen_count));
	g_object_set_data(G_OBJECT(gtkconv->active_conv), "unseen-state", GINT_TO_POINTER(gtkconv->unseen_state));

	purple_conversation_update(gtkconv->active_conv, PURPLE_CONVERSATION_UPDATE_UNSEEN);
}

/**************************************************************************
 * GTK window ops
 **************************************************************************/
static void
pidgin_conv_placement_place(PidginConversation *conv) {
	GtkWidget *window = NULL;
	PidginConversationWindow *conv_window = NULL;

	window = pidgin_conversation_window_get_default();
	conv_window = PIDGIN_CONVERSATION_WINDOW(window);

	pidgin_conversation_window_add(conv_window, conv->active_conv);
}
