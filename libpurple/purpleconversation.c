/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpleconversation.h"

#include "debug.h"
#include "purpleconversationmanager.h"
#include "purpleconversationmember.h"
#include "purpleenums.h"
#include "purplehistorymanager.h"
#include "purplemarkup.h"
#include "purpleprivate.h"
#include "purpleprotocolconversation.h"
#include "purpletags.h"
#include "signals.h"
#include "util.h"

struct _PurpleConversation {
	GObject parent;

	char *id;
	PurpleConversationType type;
	PurpleAccount *account;

	PurpleAvatar *avatar;
	char *name;
	char *alias;
	char *title;
	gboolean title_generated;

	gboolean age_restricted;
	char *description;
	char *topic;
	PurpleContactInfo *topic_author;
	GDateTime *topic_updated;
	char *user_nickname;
	gboolean favorite;
	GDateTime *created_on;
	PurpleContactInfo *creator;
	gboolean online;
	gboolean federated;
	PurpleTags *tags;

	PurpleConversationMembers *members;

	GListStore *messages;
	gboolean needs_attention;

	PurpleTypingState typing_state;
	guint typing_state_source;
	GDateTime *last_typing;
};

enum {
	PROP_0,
	PROP_ID,
	PROP_GLOBAL_ID,
	PROP_TYPE,
	PROP_ACCOUNT,
	PROP_AVATAR,
	PROP_NAME,
	PROP_ALIAS,
	PROP_TITLE,
	PROP_TITLE_FOR_DISPLAY,
	PROP_TITLE_GENERATED,
	PROP_AGE_RESTRICTED,
	PROP_DESCRIPTION,
	PROP_TOPIC,
	PROP_TOPIC_AUTHOR,
	PROP_TOPIC_UPDATED,
	PROP_USER_NICKNAME,
	PROP_FAVORITE,
	PROP_CREATED_ON,
	PROP_CREATOR,
	PROP_ONLINE,
	PROP_FEDERATED,
	PROP_TAGS,
	PROP_MEMBERS,
	PROP_MESSAGES,
	PROP_NEEDS_ATTENTION,
	PROP_TYPING_STATE,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_PRESENT,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

G_DEFINE_FINAL_TYPE(PurpleConversation, purple_conversation, G_TYPE_OBJECT)

static void purple_conversation_account_connected_cb(GObject *obj,
                                                     GParamSpec *pspec,
                                                     gpointer data);

/**************************************************************************
 * Helpers
 **************************************************************************/
static void
purple_conversation_set_title_generated(PurpleConversation *conversation,
                                        gboolean title_generated)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	/* If conversation isn't a dm or group dm, and title_generated is being set
	 * to %TRUE exit immediately because generating the title is only allowed
	 * on DMs and GroupDMs.
	 */
	if(conversation->type != PURPLE_CONVERSATION_TYPE_DM &&
	   conversation->type != PURPLE_CONVERSATION_TYPE_GROUP_DM &&
	   title_generated)
	{
		return;
	}

	if(conversation->title_generated != title_generated) {
		GObject *obj = G_OBJECT(conversation);

		conversation->title_generated = title_generated;

		g_object_freeze_notify(obj);

		if(conversation->title_generated) {
			purple_conversation_generate_title(conversation);
		}

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_TITLE_GENERATED]);

		g_object_thaw_notify(obj);
	}
}

static void
purple_conversation_set_id(PurpleConversation *conversation, const char *id) {
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_str(&conversation->id, id)) {
		GObject *obj = G_OBJECT(conversation);

		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_ID]);
		g_object_notify_by_pspec(obj, properties[PROP_GLOBAL_ID]);
		g_object_thaw_notify(obj);
	}
}

static void
purple_conversation_set_account(PurpleConversation *conversation,
                                PurpleAccount *account)
{
	PurpleConversationMember *member = NULL;

	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	/* Remove the account from the conversation if it's a member. */
	if(PURPLE_IS_ACCOUNT(conversation->account)) {
		if(PURPLE_IS_CONVERSATION_MEMBER(member)) {
			PurpleContactInfo *info = NULL;

			info = purple_account_get_contact_info(conversation->account);
			purple_conversation_members_remove_member(conversation->members,
			                                          info, FALSE, NULL);
		}
	}

	if(g_set_object(&conversation->account, account)) {
		GObject *obj = NULL;

		if(PURPLE_IS_ACCOUNT(conversation->account)) {
			PurpleContactInfo *info = NULL;
			PurpleConversationMember *member = NULL;

			info = purple_account_get_contact_info(account);
			member = purple_conversation_members_add_member(conversation->members,
			                                                info, FALSE, NULL);

			g_object_bind_property(conversation, "typing-state",
			                       member, "typing-state",
			                       G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

			g_signal_connect_object(account, "notify::connected",
			                        G_CALLBACK(purple_conversation_account_connected_cb),
			                        conversation, 0);
		}

		obj = G_OBJECT(conversation);
		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_ACCOUNT]);
		g_object_notify_by_pspec(obj, properties[PROP_GLOBAL_ID]);
		g_object_thaw_notify(obj);
	}
}

static void
purple_conversation_set_conversation_type(PurpleConversation *conversation,
                                          PurpleConversationType type)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(type != conversation->type) {
		conversation->type = type;

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_TYPE]);
	}
}

static void
purple_conversation_set_federated(PurpleConversation *conversation,
                                  gboolean federated)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(conversation->federated != federated) {
		conversation->federated = federated;

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_FEDERATED]);
	}
}

static void
purple_conversation_send_message_async_cb(GObject *source,
                                          GAsyncResult *result,
                                          gpointer data)
{
	PurpleMessage *message = NULL;
	PurpleProtocolConversation *protocol = NULL;
	GError *error = NULL;
	GTask *task = data;
	gboolean success = FALSE;

	/* task and result share a cancellable, so we just need to clear task to
	 * make sure its callback gets called.
	 */
	if(g_task_return_error_if_cancelled(G_TASK(task))) {
		g_clear_object(&task);

		return;
	}

	protocol = PURPLE_PROTOCOL_CONVERSATION(source);
	message = g_task_get_task_data(G_TASK(task));

	success = purple_protocol_conversation_send_message_finish(protocol,
	                                                           result, &error);

	if(!success) {
		if(error == NULL) {
			error = g_error_new(PURPLE_CONVERSATION_DOMAIN, 0,
			                    "unknown error");
		}

		purple_message_set_error(message, error);
		g_task_return_error(task, error);

		g_clear_error(&error);
	} else {
		/* If the protocol didn't set delivered, set it now. */
		if(!purple_message_get_delivered(message)) {
			purple_message_set_delivered(message, TRUE);
		}

		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

/**************************************************************************
 * Callbacks
 **************************************************************************/
static void
purple_conversation_members_item_changed_cb(G_GNUC_UNUSED GListModel *model,
                                            G_GNUC_UNUSED guint position,
                                            G_GNUC_UNUSED guint removed,
                                            G_GNUC_UNUSED guint added,
                                            gpointer data)
{
	PurpleConversation *conversation = data;

	if(purple_conversation_get_title_generated(conversation)) {
		purple_conversation_generate_title(conversation);
	}
}

static void
purple_conversation_account_connected_cb(GObject *obj,
                                         G_GNUC_UNUSED GParamSpec *pspec,
                                         gpointer data)
{
	PurpleConversation *conversation = data;
	gboolean connected = purple_account_is_connected(PURPLE_ACCOUNT(obj));

	if(conversation->federated) {
		/* If the account changed to connected and the conversation is
		 * federated we do nothing. But if the account went offline, we can
		 * safely set the conversation to offline.
		 */
		if(!connected) {
			purple_conversation_set_online(conversation, FALSE);
		}
	} else {
		purple_conversation_set_online(conversation, connected);
	}
}

/*
 * purple_conversation_typing_state_typing_cb: (skip)
 * @data: The conversation instance.
 *
 * If this callback manages to get called, it means the user has stopped typing
 * and we need to change the typing state of the conversation to paused.
 *
 * There's some specific ordering we have to worry about because
 * purple_conversation_set_typing_state will attempt to remove the source that
 * called us even though we're going to exit cleanly after we call that
 * function.
 *
 * To avoid this, we just set the typing_state_source to 0 which will make
 * purple_conversation_set_typing_state not try to cancel the source.
 */
static gboolean
purple_conversation_typing_state_typing_cb(gpointer data) {
	PurpleConversation *conversation = data;

	conversation->typing_state_source = 0;

	purple_conversation_set_typing_state(conversation,
	                                     PURPLE_TYPING_STATE_PAUSED);

	return G_SOURCE_REMOVE;
}

/*
 * purple_conversation_typing_state_paused_cb: (skip)
 * @data: The conversation instance.
 *
 * If this callback manages to get called, it means the user has stopped typing
 * some time ago, and we need to set the state to NONE.
 *
 * There's some specific ordering we have to worry about because
 * purple_conversation_set_typing_state will attempt to remove the source that
 * called us even though we're going to exit cleanly after we call that
 * function.
 *
 * To avoid this, we just set the typing_state_source to 0 which will make
 * purple_conversation_set_typing_state not try to cancel the source.
 */
static gboolean
purple_conversation_typing_state_paused_cb(gpointer data) {
	PurpleConversation *conversation = data;

	conversation->typing_state_source = 0;

	purple_conversation_set_typing_state(conversation,
	                                     PURPLE_TYPING_STATE_NONE);

	return G_SOURCE_REMOVE;
}

/**************************************************************************
 * GObject Implementation
 **************************************************************************/
static void
purple_conversation_set_property(GObject *obj, guint param_id,
                                 const GValue *value, GParamSpec *pspec)
{
	PurpleConversation *conversation = PURPLE_CONVERSATION(obj);

	switch (param_id) {
	case PROP_ID:
		purple_conversation_set_id(conversation, g_value_get_string(value));
		break;
	case PROP_TYPE:
		purple_conversation_set_conversation_type(conversation,
		                                          g_value_get_enum(value));
		break;
	case PROP_ACCOUNT:
		purple_conversation_set_account(conversation,
		                                g_value_get_object(value));
		break;
	case PROP_AVATAR:
		purple_conversation_set_avatar(conversation,
		                               g_value_get_object(value));
		break;
	case PROP_NAME:
		purple_conversation_set_name(conversation, g_value_get_string(value));
		break;
	case PROP_ALIAS:
		purple_conversation_set_alias(conversation, g_value_get_string(value));
		break;
	case PROP_TITLE:
		purple_conversation_set_title(conversation, g_value_get_string(value));
		break;
	case PROP_AGE_RESTRICTED:
		purple_conversation_set_age_restricted(conversation,
		                                       g_value_get_boolean(value));
		break;
	case PROP_DESCRIPTION:
		purple_conversation_set_description(conversation,
		                                    g_value_get_string(value));
		break;
	case PROP_TOPIC:
		purple_conversation_set_topic(conversation, g_value_get_string(value));
		break;
	case PROP_TOPIC_AUTHOR:
		purple_conversation_set_topic_author(conversation,
		                                     g_value_get_object(value));
		break;
	case PROP_TOPIC_UPDATED:
		purple_conversation_set_topic_updated(conversation,
		                                      g_value_get_boxed(value));
		break;
	case PROP_USER_NICKNAME:
		purple_conversation_set_user_nickname(conversation,
		                                      g_value_get_string(value));
		break;
	case PROP_FAVORITE:
		purple_conversation_set_favorite(conversation,
		                                 g_value_get_boolean(value));
		break;
	case PROP_CREATED_ON:
		purple_conversation_set_created_on(conversation,
		                                   g_value_get_boxed(value));
		break;
	case PROP_CREATOR:
		purple_conversation_set_creator(conversation,
		                                g_value_get_object(value));
		break;
	case PROP_ONLINE:
		purple_conversation_set_online(conversation,
		                               g_value_get_boolean(value));
		break;
	case PROP_FEDERATED:
		purple_conversation_set_federated(conversation,
		                                  g_value_get_boolean(value));
		break;
	case PROP_NEEDS_ATTENTION:
		purple_conversation_set_needs_attention(conversation,
		                                        g_value_get_boolean(value));
		break;
	case PROP_TYPING_STATE:
		purple_conversation_set_typing_state(conversation,
		                                     g_value_get_enum(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_conversation_get_property(GObject *obj, guint param_id, GValue *value,
                                 GParamSpec *pspec)
{
	PurpleConversation *conversation = PURPLE_CONVERSATION(obj);

	switch(param_id) {
	case PROP_ID:
		g_value_set_string(value, purple_conversation_get_id(conversation));
		break;
	case PROP_GLOBAL_ID:
		g_value_take_string(value,
		                    purple_conversation_get_global_id(conversation));
		break;
	case PROP_TYPE:
		g_value_set_enum(value,
		                 purple_conversation_get_conversation_type(conversation));
		break;
	case PROP_ACCOUNT:
		g_value_set_object(value,
		                   purple_conversation_get_account(conversation));
		break;
	case PROP_AVATAR:
		g_value_set_object(value,
		                   purple_conversation_get_avatar(conversation));
		break;
	case PROP_NAME:
		g_value_set_string(value, purple_conversation_get_name(conversation));
		break;
	case PROP_ALIAS:
		g_value_set_string(value, purple_conversation_get_alias(conversation));
		break;
	case PROP_TITLE:
		g_value_set_string(value, purple_conversation_get_title(conversation));
		break;
	case PROP_TITLE_FOR_DISPLAY:
		g_value_set_string(value,
		                   purple_conversation_get_title_for_display(conversation));
		break;
	case PROP_TITLE_GENERATED:
		g_value_set_boolean(value,
		                    purple_conversation_get_title_generated(conversation));
		break;
	case PROP_AGE_RESTRICTED:
		g_value_set_boolean(value,
		                    purple_conversation_get_age_restricted(conversation));
		break;
	case PROP_DESCRIPTION:
		g_value_set_string(value,
		                   purple_conversation_get_description(conversation));
		break;
	case PROP_TOPIC:
		g_value_set_string(value, purple_conversation_get_topic(conversation));
		break;
	case PROP_TOPIC_AUTHOR:
		g_value_set_object(value,
		                   purple_conversation_get_topic_author(conversation));
		break;
	case PROP_TOPIC_UPDATED:
		g_value_set_boxed(value,
		                  purple_conversation_get_topic_updated(conversation));
		break;
	case PROP_USER_NICKNAME:
		g_value_set_string(value,
		                   purple_conversation_get_user_nickname(conversation));
		break;
	case PROP_FAVORITE:
		g_value_set_boolean(value,
		                    purple_conversation_get_favorite(conversation));
		break;
	case PROP_CREATED_ON:
		g_value_set_boxed(value,
		                  purple_conversation_get_created_on(conversation));
		break;
	case PROP_CREATOR:
		g_value_set_object(value,
		                   purple_conversation_get_creator(conversation));
		break;
	case PROP_ONLINE:
		g_value_set_boolean(value,
		                    purple_conversation_get_online(conversation));
		break;
	case PROP_FEDERATED:
		g_value_set_boolean(value,
		                    purple_conversation_get_federated(conversation));
		break;
	case PROP_TAGS:
		g_value_set_object(value, purple_conversation_get_tags(conversation));
		break;
	case PROP_MEMBERS:
		g_value_set_object(value,
		                   purple_conversation_get_members(conversation));
		break;
	case PROP_MESSAGES:
		g_value_set_object(value,
		                   purple_conversation_get_messages(conversation));
		break;
	case PROP_NEEDS_ATTENTION:
		g_value_set_boolean(value,
		                    purple_conversation_get_needs_attention(conversation));
		break;
	case PROP_TYPING_STATE:
		g_value_set_enum(value,
		                 purple_conversation_get_typing_state(conversation));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_conversation_init(PurpleConversation *conversation) {
	conversation->tags = purple_tags_new();
	conversation->messages = g_list_store_new(PURPLE_TYPE_MESSAGE);

	conversation->members = purple_conversation_members_new();
	g_signal_connect_object(conversation->members, "items-changed",
	                        G_CALLBACK(purple_conversation_members_item_changed_cb),
	                        conversation, G_CONNECT_DEFAULT);
}

static void
purple_conversation_constructed(GObject *object) {
	PurpleConversation *conversation = PURPLE_CONVERSATION(object);

	G_OBJECT_CLASS(purple_conversation_parent_class)->constructed(object);

	if(purple_strempty(conversation->title)) {
		if(conversation->type == PURPLE_CONVERSATION_TYPE_DM ||
		   conversation->type == PURPLE_CONVERSATION_TYPE_GROUP_DM)
		{
			/* There's no way to add members during construction, so just call
			 * set_title_generated.
			 */
			purple_conversation_set_title_generated(conversation, TRUE);
		}
	}
}

static void
purple_conversation_dispose(GObject *obj) {
	g_object_set_data(obj, "is-finalizing", GINT_TO_POINTER(TRUE));
}

static void
purple_conversation_finalize(GObject *object) {
	PurpleConversation *conversation = PURPLE_CONVERSATION(object);

	purple_request_close_with_handle(conversation);

	g_clear_object(&conversation->account);
	g_clear_pointer(&conversation->id, g_free);
	g_clear_object(&conversation->avatar);
	g_clear_pointer(&conversation->name, g_free);
	g_clear_pointer(&conversation->alias, g_free);
	g_clear_pointer(&conversation->title, g_free);

	g_clear_pointer(&conversation->description, g_free);
	g_clear_pointer(&conversation->topic, g_free);
	g_clear_object(&conversation->topic_author);
	g_clear_pointer(&conversation->topic_updated, g_date_time_unref);
	g_clear_pointer(&conversation->user_nickname, g_free);
	g_clear_pointer(&conversation->created_on, g_date_time_unref);
	g_clear_object(&conversation->creator);
	g_clear_object(&conversation->tags);
	g_clear_object(&conversation->members);
	g_clear_object(&conversation->messages);

	g_clear_handle_id(&conversation->typing_state_source, g_source_remove);
	g_clear_pointer(&conversation->last_typing, g_date_time_unref);

	G_OBJECT_CLASS(purple_conversation_parent_class)->finalize(object);
}

static void
purple_conversation_class_init(PurpleConversationClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->constructed = purple_conversation_constructed;
	obj_class->dispose = purple_conversation_dispose;
	obj_class->finalize = purple_conversation_finalize;
	obj_class->get_property = purple_conversation_get_property;
	obj_class->set_property = purple_conversation_set_property;

	/**
	 * PurpleConversation:id:
	 *
	 * An opaque identifier for this conversation. Generally speaking this is
	 * protocol dependent and should only be used as a unique identifier.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", "id",
		"The identifier for the conversation.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:global-id:
	 *
	 * A libpurple global id for the conversation.
	 *
	 * This is an opaque value but it ties the conversation to the libpurple
	 * account it belongs to, which makes it globally unique inside of
	 * libpurple.
	 *
	 * Since: 3.0
	 */
	properties[PROP_GLOBAL_ID] = g_param_spec_string(
		"global-id", NULL, NULL,
		NULL,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:type:
	 *
	 * A type hint for the conversation. This may be useful for protocols, but
	 * libpurple treats all conversations the same.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TYPE] = g_param_spec_enum(
		"type", "type",
		"The type of the conversation.",
		PURPLE_TYPE_CONVERSATION_TYPE,
		PURPLE_CONVERSATION_TYPE_UNSET,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:account:
	 *
	 * The account this conversation belongs to.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", "Account",
		"The account for the conversation.",
		PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:avatar:
	 *
	 * The [class@Avatar] for the conversation.
	 *
	 * Not all protocols support this and most user interfaces will use the
	 * avatar of the remote contact for direct messages.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AVATAR] = g_param_spec_object(
		"avatar", "avatar",
		"The avatar for this conversation.",
		PURPLE_TYPE_AVATAR,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:name:
	 *
	 * The name of the conversation.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NAME] = g_param_spec_string(
		"name", "Name",
		"The name of the conversation.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:alias:
	 *
	 * An alias for the conversation that is local to the libpurple user.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ALIAS] = g_param_spec_string(
		"alias", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:title:
	 *
	 * The title of the conversation.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TITLE] = g_param_spec_string(
		"title", "Title",
		"The title of the conversation.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:title-for-display:
	 *
	 * The title that should be displayed for the conversation based on which
	 * properties are set.
	 *
	 * If [property@Conversation:alias] is set, that will be returned.
	 *
	 * If alias is not set but [property@Conversation:title] is set, then value
	 * of title will be returned.
	 *
	 * As a fallback, [property@Conversation:id] will be returned if nothing
	 * else is set.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TITLE_FOR_DISPLAY] = g_param_spec_string(
		"title-for-display", NULL, NULL,
		NULL,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:title-generated:
	 *
	 * Whether or not the title of the conversation was generated by
	 * [method@Conversation.generate_title].
	 *
	 * Note: This only works on DMs and GroupDMs.
	 *
	 * If this is %TRUE, [method@Conversation.generate_title] will
	 * automatically be called whenever a member is added or removed, or when
	 * their display name changes.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TITLE_GENERATED] = g_param_spec_boolean(
		"title-generated", "title-generated",
		"Whether or not the current title was generated.",
		FALSE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:age-restricted:
	 *
	 * Whether or not the conversation is age restricted.
	 *
	 * This is typically set only by a protocol plugin.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AGE_RESTRICTED] = g_param_spec_boolean(
		"age-restricted", "age-restricted",
		"Whether or not the conversation is age restricted.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:description:
	 *
	 * Sets the description of the conversation. This field is typically used
	 * to give more information about a conversation than that which would fit
	 * in [property@Conversation:topic].
	 *
	 * This is typically set only by a protocol plugin.
	 *
	 * Since: 3.0
	 */
	properties[PROP_DESCRIPTION] = g_param_spec_string(
		"description", "description",
		"The description for the conversation.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:topic:
	 *
	 * The topic of the conversation.
	 *
	 * This is normally controlled by the protocol plugin and often times
	 * requires permission for the user to set.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TOPIC] = g_param_spec_string(
		"topic", "topic",
		"The topic for the conversation.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:topic-author:
	 *
	 * Sets the author of the topic for the conversation.
	 *
	 * This should typically only be set by a protocol plugin.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TOPIC_AUTHOR] = g_param_spec_object(
		"topic-author", "topic-author",
		"The author of the topic for the conversation.",
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:topic-updated:
	 *
	 * Set to the time that the topic was last updated.
	 *
	 * This should typically only be set by a protocol plugin.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TOPIC_UPDATED] = g_param_spec_boxed(
		"topic-updated", "topic-updated",
		"The time when the topic was last updated for the conversation.",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:user-nickname:
	 *
	 * The user's nickname in this conversation.
	 *
	 * Some protocols allow the user to use a nickname rather than their normal
	 * contact information when joining a conversation. This field holds that
	 * value.
	 *
	 * Since: 3.0
	 */
	properties[PROP_USER_NICKNAME] = g_param_spec_string(
		"user-nickname", "user-nickname",
		"The nickname for the user in the conversation.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:favorite:
	 *
	 * Whether or not the conversation has been marked as favorite by the user.
	 *
	 * Since: 3.0
	 */
	properties[PROP_FAVORITE] = g_param_spec_boolean(
		"favorite", "favorite",
		"Whether or not the conversation is a favorite.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:created-on:
	 *
	 * The [struct@GLib.DateTime] when this conversation was created. This can
	 * be %NULL if the value is not known or supported.
	 *
	 * This should typically only be set by a protocol plugin.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CREATED_ON] = g_param_spec_boxed(
		"created-on", "created-on",
		"When the conversation was created.",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:creator:
	 *
	 * The [class@ContactInfo] that created the conversation.
	 *
	 * This should typically only be set by a protocol plugin.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CREATOR] = g_param_spec_object(
		"creator", "creator",
		"The contact info of who created the conversation.",
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:online:
	 *
	 * Whether or not the conversation is able to send and receive messages.
	 *
	 * This is typically tied to whether or not the account that this
	 * conversation belongs is online or not.
	 *
	 * However, if a protocol supports federated conversation, it is possible
	 * for a conversation to be offline if the server it is on is currently
	 * unreachable.
	 *
	 * See also [property@Conversation:federated].
	 *
	 * Since: 3.0
	 */
	properties[PROP_ONLINE] = g_param_spec_boolean(
		"online", "online",
		"Whether or not the conversation can send and receive messages.",
		TRUE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:federated:
	 *
	 * Whether or this conversation is federated.
	 *
	 * This should only be set by protocols that support federated
	 * conversations.
	 *
	 * When this is %TRUE the [property@Conversation:online] property will not
	 * be automatically set to match the [property@Account:connected] property
	 * of the account that this conversation belongs to. It is the
	 * responsibility of the protocol to manage the online property in this
	 * case.
	 *
	 * Since: 3.0
	 */
	properties[PROP_FEDERATED] = g_param_spec_boolean(
		"federated", "federated",
		"Whether or not this conversation is federated.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:tags:
	 *
	 * [class@Tags] for the conversation.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TAGS] = g_param_spec_object(
		"tags", "tags",
		"The tags for the conversation.",
		PURPLE_TYPE_TAGS,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:members:
	 *
	 * The members that are currently in this conversation.
	 *
	 * Since: 3.0
	 */
	properties[PROP_MEMBERS] = g_param_spec_object(
		"members", "members",
		"The members that are currently in this conversation",
		PURPLE_TYPE_CONVERSATION_MEMBERS,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:messages:
	 *
	 * A [iface.Gio.ListModel] of all the messages in this conversation.
	 *
	 * Since: 3.0
	 */
	properties[PROP_MESSAGES] = g_param_spec_object(
		"messages", "messages",
		"All of the messages in this conversation's history.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:needs-attention:
	 *
	 * Whether or not the conversation needs attention.
	 *
	 * This could be because there are new messages or the user has been
	 * kicked from the room, or something else.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NEEDS_ATTENTION] = g_param_spec_boolean(
		"needs-attention", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversation:typing-state:
	 *
	 * The [enum@TypingState] of the libpurple user in this conversation.
	 *
	 * When the property changes to `typing`, a timeout will be setup to change
	 * the property to `paused` if the property hasn't been set to `typing`
	 * again before the timeout expires.
	 *
	 * If the above timeout fires, the state will be changed to `paused`, and a
	 * new timeout will be added that will reset the state to `none` if it
	 * expires.
	 *
	 * This means that user interfaces should only ever need to set the state
	 * to typing and should do so whenever the user types anything that could
	 * be part of a message. Things like keyboard navigation and %commands
	 * should not result in this property being changed.
	 *
	 * If the [class@Protocol] that this conversation belongs to implements
	 * [iface@ProtocolConversation] and
	 * [vfunc@ProtocolConversation.send_typing],
	 * [vfunc@ProtocolConversation.send_typing] will be called when this
	 * property is set even if the state hasn't changed.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TYPING_STATE] = g_param_spec_enum(
		"typing-state", NULL, NULL,
		PURPLE_TYPE_TYPING_STATE,
		PURPLE_TYPING_STATE_NONE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleConversation::present:
	 * @conversation: The instance.
	 *
	 * Emitted by [method@Conversation.present] when something wants the
	 * conversation presented to the user.
	 *
	 * Since: 3.0
	 */
	signals[SIG_PRESENT] = g_signal_new_class_handler(
		"present",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
gboolean
purple_conversation_is_dm(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->type == PURPLE_CONVERSATION_TYPE_DM;
}

gboolean
purple_conversation_is_group_dm(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->type == PURPLE_CONVERSATION_TYPE_GROUP_DM;
}

gboolean
purple_conversation_is_channel(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->type == PURPLE_CONVERSATION_TYPE_CHANNEL;
}

gboolean
purple_conversation_is_thread(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->type == PURPLE_CONVERSATION_TYPE_THREAD;
}

void
purple_conversation_present(PurpleConversation *conversation) {
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	g_signal_emit(conversation, signals[SIG_PRESENT], 0);
}

const char *
purple_conversation_get_id(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->id;
}

char *
purple_conversation_get_global_id(PurpleConversation *conversation) {
	const char *account_id = NULL;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	account_id = purple_account_get_id(conversation->account);

	return g_strdup_printf("%s-%s", account_id, conversation->id);
}

PurpleConversationType
purple_conversation_get_conversation_type(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation),
	                     PURPLE_CONVERSATION_TYPE_UNSET);

	return conversation->type;
}

PurpleAccount *
purple_conversation_get_account(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->account;
}

PurpleConnection *
purple_conversation_get_connection(PurpleConversation *conversation) {
	PurpleAccount *account;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	account = purple_conversation_get_account(conversation);

	if(account == NULL) {
		return NULL;
	}

	return purple_account_get_connection(account);
}

void
purple_conversation_set_title(PurpleConversation *conversation,
                              const char *title)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_str(&conversation->title, title)) {
		GObject *obj = G_OBJECT(conversation);

		/* We have to g_object_freeze_notify here because we're modifying more
		 * than one property. However, purple_conversation_generate_title will
		 * also have called g_object_freeze_notify before calling us because it
		 * needs to set the title-generated property to TRUE even though we set
		 * it to FALSE here. We do this, because we didn't want to write
		 * additional API that skips that part.
		 */
		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_TITLE]);
		g_object_notify_by_pspec(obj, properties[PROP_TITLE_FOR_DISPLAY]);
		purple_conversation_set_title_generated(conversation, FALSE);
		g_object_thaw_notify(obj);
	}
}

const char *
purple_conversation_get_title(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->title;
}

void
purple_conversation_generate_title(PurpleConversation *conversation) {
	PurpleAccount *account = NULL;
	PurpleContactInfo *account_info = NULL;
	GString *str = NULL;
	guint n_members = 0;
	gboolean first = TRUE;

	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(conversation->type != PURPLE_CONVERSATION_TYPE_DM &&
	   conversation->type != PURPLE_CONVERSATION_TYPE_GROUP_DM)
	{
		g_warning("purple_conversation_generate_title called for non DM/Group "
		          "DM conversation");

		return;
	}

	account = purple_conversation_get_account(conversation);
	account_info = purple_account_get_contact_info(account);

	str = g_string_new("");

	n_members = g_list_model_get_n_items(G_LIST_MODEL(conversation->members));
	for(guint i = 0; i < n_members; i++) {
		PurpleContactInfo *info = NULL;
		PurpleConversationMember *member = NULL;
		const char *name = NULL;

		member = g_list_model_get_item(G_LIST_MODEL(conversation->members), i);
		info = purple_conversation_member_get_contact_info(member);
		if(purple_contact_info_compare(info, account_info) == 0) {
			g_clear_object(&member);

			continue;
		}

		name = purple_contact_info_get_name_for_display(info);
		if(purple_strempty(name)) {
			g_warning("contact %p has no displayable name", info);

			g_clear_object(&member);

			continue;
		}

		if(!first) {
			g_string_append_printf(str, ", %s", name);
		} else {
			g_string_append(str, name);
			first = FALSE;
		}

		g_clear_object(&member);
	}

	/* If we found at least 1 user to add, then we set the title. */
	if(!first) {
		GObject *obj = G_OBJECT(conversation);

		g_object_freeze_notify(obj);
		purple_conversation_set_title(conversation, str->str);
		purple_conversation_set_title_generated(conversation, TRUE);
		g_object_thaw_notify(obj);
	}

	g_string_free(str, TRUE);
}

gboolean
purple_conversation_get_title_generated(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->title_generated;
}

void
purple_conversation_set_name(PurpleConversation *conversation,
                             const char *name)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_str(&conversation->name, name)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_NAME]);
	}
}

const char *
purple_conversation_get_name(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->name;
}

void
purple_conversation_write_message(PurpleConversation *conversation,
                                  PurpleMessage *message)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));
	g_return_if_fail(message != NULL);

	if(!(purple_message_get_flags(message) & PURPLE_MESSAGE_NO_LOG)) {
		GError *error = NULL;
		PurpleHistoryManager *manager = NULL;
		gboolean success = FALSE;

		manager = purple_history_manager_get_default();
		/* We should probably handle this error somehow, but I don't think that
		 * spamming purple_debug_warning is necessarily the right call.
		 */
		success = purple_history_manager_write(manager, conversation, message,
		                                     &error);
		if(!success){
			purple_debug_info("conversation",
			                  "history manager write returned error: %s",
			                  error->message);

			g_clear_error(&error);
		}
	}

	g_list_store_append(conversation->messages, message);
}

void
purple_conversation_send_message_async(PurpleConversation *conversation,
                                       PurpleMessage *message,
                                       GCancellable *cancellable,
                                       GAsyncReadyCallback callback,
                                       gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleProtocol *protocol = NULL;
	GTask *task = NULL;

	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	purple_conversation_set_typing_state(conversation,
	                                     PURPLE_TYPING_STATE_NONE);

	task = g_task_new(conversation, cancellable, callback, data);
	g_task_set_source_tag(task, purple_conversation_send_message_async);
	g_task_set_task_data(task, g_object_ref(message), g_object_unref);

	account = purple_conversation_get_account(conversation);
	protocol = purple_account_get_protocol(account);

	if(!PURPLE_IS_PROTOCOL_CONVERSATION(protocol)) {
		g_task_return_new_error(task, PURPLE_CONVERSATION_DOMAIN, 0,
		                        "protocol does not implement "
		                        "PurpleProtocolConversation");

		g_clear_object(&task);

		return;
	}

	purple_protocol_conversation_send_message_async(PURPLE_PROTOCOL_CONVERSATION(protocol),
	                                                conversation,
	                                                message,
	                                                cancellable,
	                                                purple_conversation_send_message_async_cb,
	                                                task);
}

gboolean
purple_conversation_send_message_finish(PurpleConversation *conversation,
                                        GAsyncResult *result,
                                        GError **error)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	g_return_val_if_fail(g_task_get_source_tag(G_TASK(result)) !=
	                     purple_conversation_send_message_async, FALSE);

	return g_task_propagate_boolean(G_TASK(result), error);
}

gboolean
purple_conversation_has_focus(PurpleConversation *conversation) {
	gboolean ret = FALSE;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return ret;
}

gboolean
purple_conversation_get_age_restricted(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->age_restricted;
}

void
purple_conversation_set_age_restricted(PurpleConversation *conversation,
                                       gboolean age_restricted)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(conversation->age_restricted != age_restricted) {
		conversation->age_restricted = age_restricted;

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_AGE_RESTRICTED]);
	}
}

const char *
purple_conversation_get_description(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->description;
}

void
purple_conversation_set_description(PurpleConversation *conversation,
                                    const char *description)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_str(&conversation->description, description)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_DESCRIPTION]);
	}
}

const char *
purple_conversation_get_topic(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->topic;
}

void
purple_conversation_set_topic(PurpleConversation *conversation,
                              const char *topic)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_str(&conversation->topic, topic)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_TOPIC]);
	}
}

void
purple_conversation_set_topic_full(PurpleConversation *conversation,
                                   const char *topic,
                                   PurpleContactInfo *author,
                                   GDateTime *updated)
{
	GObject *obj = NULL;

	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	obj = G_OBJECT(conversation);
	g_object_freeze_notify(obj);

	purple_conversation_set_topic(conversation, topic);
	purple_conversation_set_topic_author(conversation, author);
	purple_conversation_set_topic_updated(conversation, updated);

	g_object_thaw_notify(obj);
}


PurpleContactInfo *
purple_conversation_get_topic_author(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->topic_author;
}

void
purple_conversation_set_topic_author(PurpleConversation *conversation,
                                     PurpleContactInfo *author)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_object(&conversation->topic_author, author)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_TOPIC_AUTHOR]);
	}
}

GDateTime *
purple_conversation_get_topic_updated(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->topic_updated;
}

void
purple_conversation_set_topic_updated(PurpleConversation *conversation,
                                      GDateTime *updated)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(birb_date_time_set(&conversation->topic_updated, updated)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_TOPIC_UPDATED]);
	}
}

const char *
purple_conversation_get_user_nickname(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->user_nickname;
}

void
purple_conversation_set_user_nickname(PurpleConversation *conversation,
                                      const char *nickname)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_str(&conversation->user_nickname, nickname)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_USER_NICKNAME]);
	}
}

gboolean
purple_conversation_get_favorite(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->favorite;
}

void
purple_conversation_set_favorite(PurpleConversation *conversation,
                                 gboolean favorite)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(conversation->favorite != favorite) {
		conversation->favorite = favorite;

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_FAVORITE]);
	}
}

GDateTime *
purple_conversation_get_created_on(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->created_on;
}

void
purple_conversation_set_created_on(PurpleConversation *conversation,
                                   GDateTime *created_on)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(birb_date_time_set(&conversation->created_on, created_on)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_CREATED_ON]);
	}
}

PurpleContactInfo *
purple_conversation_get_creator(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->creator;
}

void
purple_conversation_set_creator(PurpleConversation *conversation,
                                PurpleContactInfo *creator)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_object(&conversation->creator, creator)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_CREATOR]);
	}
}

gboolean
purple_conversation_get_online(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->online;
}

void
purple_conversation_set_online(PurpleConversation *conversation,
                               gboolean online)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(conversation->online != online) {
		conversation->online = online;

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_ONLINE]);
	}
}

gboolean
purple_conversation_get_federated(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->federated;
}

PurpleTags *
purple_conversation_get_tags(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->tags;
}

PurpleConversationMembers *
purple_conversation_get_members(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->members;
}

GListModel *
purple_conversation_get_messages(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	if(G_IS_LIST_MODEL(conversation->messages)) {
		return G_LIST_MODEL(conversation->messages);
	}

	return NULL;
}

PurpleAvatar *
purple_conversation_get_avatar(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->avatar;
}

void
purple_conversation_set_avatar(PurpleConversation *conversation,
                               PurpleAvatar *avatar)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_object(&conversation->avatar, avatar)) {
		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_AVATAR]);
	}
}

const char *
purple_conversation_get_title_for_display(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	if(!purple_strempty(conversation->alias)) {
		return conversation->alias;
	}

	if(!purple_strempty(conversation->title)) {
		return conversation->title;
	}

	return conversation->id;
}

const char *
purple_conversation_get_alias(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), NULL);

	return conversation->alias;
}

void
purple_conversation_set_alias(PurpleConversation *conversation,
                              const char *alias)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(g_set_str(&conversation->alias, alias)) {
		GObject *obj = G_OBJECT(conversation);

		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_ALIAS]);
		g_object_notify_by_pspec(obj, properties[PROP_TITLE_FOR_DISPLAY]);
		g_object_thaw_notify(obj);
	}
}

gboolean
purple_conversation_get_needs_attention(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	return conversation->needs_attention;
}

void
purple_conversation_set_needs_attention(PurpleConversation *conversation,
                                        gboolean needs_attention)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	if(conversation->needs_attention != needs_attention) {
		conversation->needs_attention = needs_attention;

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_NEEDS_ATTENTION]);
	}
}

PurpleTypingState
purple_conversation_get_typing_state(PurpleConversation *conversation) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation),
	                     PURPLE_TYPING_STATE_NONE);

	return conversation->typing_state;
}

void
purple_conversation_set_typing_state(PurpleConversation *conversation,
                                     PurpleTypingState typing_state)
{
	gboolean send = FALSE;

	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	/* Remove the old timeout because we have new activity. */
	g_clear_handle_id(&conversation->typing_state_source, g_source_remove);

	/* We set some default timeouts based on the state. If the new state is
	 * TYPING, we use a 6 second timeout that will change the state to PAUSED.
	 * When the state changes to PAUSED we will set a 30 second time that will
	 * change the state to NONE.
	 *
	 * This allows the user interface to just tell libpurple when the user is
	 * typing, and the rest happens automatically.
	 */
	if(typing_state == PURPLE_TYPING_STATE_TYPING) {
		GDateTime *now = NULL;

		conversation->typing_state_source =
			g_timeout_add_seconds(6,
			                      purple_conversation_typing_state_typing_cb,
			                      conversation);

		/* Use local time because this is local to the user and we might want
		 * to output this during debug or something, and a local time stamp
		 * will make a lot more sense then.
		 */
		now = g_date_time_new_now_local();
		if(conversation->last_typing != NULL) {
			GTimeSpan difference = 0;

			difference = g_date_time_difference(now, conversation->last_typing);

			if(difference > 3 * G_TIME_SPAN_SECOND) {
				send = TRUE;
			}
		}

		conversation->last_typing = now;
	} else if(typing_state == PURPLE_TYPING_STATE_PAUSED) {
		conversation->typing_state_source =
			g_timeout_add_seconds(30,
			                      purple_conversation_typing_state_paused_cb,
			                      conversation);
	}

	if(conversation->typing_state != typing_state) {
		conversation->typing_state = typing_state;

		g_object_notify_by_pspec(G_OBJECT(conversation),
		                         properties[PROP_TYPING_STATE]);

		/* The state changed so we need to send it. */
		send = TRUE;
	}

	/* Check if we have a protocol that implements
	 * ProtocolConversation.send_typing and call it if it does.
	 *
	 * We do this after the notify above to make sure the user interface will
	 * not be possibly blocked by the protocol.
	 */
	if(send && PURPLE_IS_ACCOUNT(conversation->account)) {
		PurpleProtocol *protocol = NULL;

		protocol = purple_account_get_protocol(conversation->account);
		if(PURPLE_IS_PROTOCOL_CONVERSATION(protocol)) {
			PurpleProtocolConversation *protocol_conversation = NULL;

			protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(protocol);

			if(purple_protocol_conversation_implements_send_typing(protocol_conversation))
			{
				purple_protocol_conversation_send_typing(protocol_conversation,
				                                         conversation,
				                                         conversation->typing_state);
			}
		}
	}
}
