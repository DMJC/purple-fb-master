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

#include "purpleconversationmember.h"

#include "purpleenums.h"
#include "util.h"

struct _PurpleConversationMember {
	GObject parent;

	PurpleBadges *badges;
	PurpleContactInfo *contact_info;
	PurpleTags *tags;

	guint typing_timeout;
	PurpleTypingState typing_state;
	char *nickname;
};

enum {
	PROP_0,
	PROP_BADGES,
	PROP_CONTACT_INFO,
	PROP_TAGS,
	PROP_TYPING_STATE,
	PROP_NICKNAME,
	PROP_NAME_FOR_DISPLAY,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_FINAL_TYPE(PurpleConversationMember, purple_conversation_member,
                    G_TYPE_OBJECT)

static void
purple_conversation_member_info_changed_cb(GObject *self, GParamSpec *pspec,
                                           gpointer data);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_conversation_member_set_contact_info(PurpleConversationMember *member,
                                            PurpleContactInfo *contact_info)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member));
	g_return_if_fail(PURPLE_IS_CONTACT_INFO(contact_info));

	if(PURPLE_IS_CONTACT_INFO(member->contact_info)) {
		g_signal_handlers_disconnect_by_func(member->contact_info,
		                                     purple_conversation_member_info_changed_cb,
		                                     member);
	}

	if(g_set_object(&member->contact_info, contact_info)) {
		GObject *obj = G_OBJECT(member);

		if(PURPLE_IS_CONTACT_INFO(member->contact_info)) {
			g_signal_connect_object(member->contact_info,
			                        "notify::name-for-display",
			                        G_CALLBACK(purple_conversation_member_info_changed_cb),
			                        member, G_CONNECT_DEFAULT);
		}

		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_CONTACT_INFO]);
		g_object_notify_by_pspec(obj, properties[PROP_NAME_FOR_DISPLAY]);
		g_object_thaw_notify(obj);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
purple_conversation_member_reset_typing_state(gpointer data) {
	PurpleConversationMember *member = data;

	purple_conversation_member_set_typing_state(member,
	                                            PURPLE_TYPING_STATE_NONE,
	                                            0);

	return G_SOURCE_REMOVE;
}

static void
purple_conversation_member_info_changed_cb(G_GNUC_UNUSED GObject *self,
                                           G_GNUC_UNUSED GParamSpec *pspec,
                                           gpointer data)
{
	g_object_notify_by_pspec(G_OBJECT(data),
	                         properties[PROP_NAME_FOR_DISPLAY]);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_conversation_member_get_property(GObject *obj, guint param_id,
                                        GValue *value, GParamSpec *pspec)
{
	PurpleConversationMember *member = PURPLE_CONVERSATION_MEMBER(obj);

	switch(param_id) {
	case PROP_BADGES:
		g_value_set_object(value,
		                   purple_conversation_member_get_badges(member));
		break;
	case PROP_CONTACT_INFO:
		g_value_set_object(value,
		                   purple_conversation_member_get_contact_info(member));
		break;
	case PROP_TAGS:
		g_value_set_object(value,
		                   purple_conversation_member_get_tags(member));
		break;
	case PROP_TYPING_STATE:
		g_value_set_enum(value,
		                 purple_conversation_member_get_typing_state(member));
		break;
	case PROP_NICKNAME:
		g_value_set_string(value,
		                   purple_conversation_member_get_nickname(member));
		break;
	case PROP_NAME_FOR_DISPLAY:
		g_value_set_string(value,
		                   purple_conversation_member_get_name_for_display(member));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_conversation_member_set_property(GObject *obj, guint param_id,
                                        const GValue *value, GParamSpec *pspec)
{
	PurpleConversationMember *member = PURPLE_CONVERSATION_MEMBER(obj);

	switch(param_id) {
	case PROP_CONTACT_INFO:
		purple_conversation_member_set_contact_info(member,
		                                            g_value_get_object(value));
		break;
	case PROP_TYPING_STATE:
		purple_conversation_member_set_typing_state(member,
		                                            g_value_get_enum(value),
		                                            0);
		break;
	case PROP_NICKNAME:
		purple_conversation_member_set_nickname(member,
		                                        g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_conversation_member_dispose(GObject *obj) {
	PurpleConversationMember *member = PURPLE_CONVERSATION_MEMBER(obj);

	g_clear_object(&member->contact_info);

	g_clear_handle_id(&member->typing_timeout, g_source_remove);

	G_OBJECT_CLASS(purple_conversation_member_parent_class)->dispose(obj);
}

static void
purple_conversation_member_finalize(GObject *obj) {
	PurpleConversationMember *member = PURPLE_CONVERSATION_MEMBER(obj);

	g_clear_object(&member->badges);
	g_clear_object(&member->tags);
	g_clear_pointer(&member->nickname, g_free);

	G_OBJECT_CLASS(purple_conversation_member_parent_class)->finalize(obj);
}

static void
purple_conversation_member_init(PurpleConversationMember *member) {
	member->badges = purple_badges_new();
	member->tags = purple_tags_new();
}

static void
purple_conversation_member_class_init(PurpleConversationMemberClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->dispose = purple_conversation_member_dispose;
	obj_class->finalize = purple_conversation_member_finalize;
	obj_class->get_property = purple_conversation_member_get_property;
	obj_class->set_property = purple_conversation_member_set_property;

	/**
	 * PurpleConversationMember:badges:
	 *
	 * The badges for the member.
	 *
	 * Since: 3.0
	 */
	properties[PROP_BADGES] = g_param_spec_object(
		"badges", NULL, NULL,
		PURPLE_TYPE_BADGES,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversationMember:contact-info:
	 *
	 * The contact info that this member is for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONTACT_INFO] = g_param_spec_object(
		"contact-info", NULL, NULL,
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversationMember:tags:
	 *
	 * The [class@Purple.Tags] instance for this member.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TAGS] = g_param_spec_object(
		"tags", NULL, NULL,
		PURPLE_TYPE_TAGS,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversationMember:typing-state:
	 *
	 * The [enum@Purple.TypingState] for this member.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TYPING_STATE] = g_param_spec_enum(
		"typing-state", NULL, NULL,
		PURPLE_TYPE_TYPING_STATE,
		PURPLE_TYPING_STATE_NONE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversationMember:nickname:
	 *
	 * A custom nick name that the remote user has chosen for themselves in the
	 * chat.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NICKNAME] = g_param_spec_string(
		"nickname", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversationMember:name-for-display:
	 *
	 * The nick name that should be display for the conversation member.
	 *
	 * If [property@ConversationMember:nickname] is set, that will be returned.
	 *
	 * Otherwise [property@ContactInfo:name-for-display] will be returned.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NAME_FOR_DISPLAY] = g_param_spec_string(
		"name-for-display", NULL, NULL,
		NULL,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleConversationMember *
purple_conversation_member_new(PurpleContactInfo *info) {
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(info), NULL);

	return g_object_new(
		PURPLE_TYPE_CONVERSATION_MEMBER,
		"contact-info", info,
		NULL);
}

PurpleBadges *
purple_conversation_member_get_badges(PurpleConversationMember *member) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member), NULL);

	return member->badges;
}

PurpleContactInfo *
purple_conversation_member_get_contact_info(PurpleConversationMember *member) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member), NULL);

	return member->contact_info;
}

PurpleTags *
purple_conversation_member_get_tags(PurpleConversationMember *member) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member), NULL);

	return member->tags;
}

PurpleTypingState
purple_conversation_member_get_typing_state(PurpleConversationMember *member)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member),
	                     PURPLE_TYPING_STATE_NONE);

	return member->typing_state;
}

void
purple_conversation_member_set_typing_state(PurpleConversationMember *member,
                                            PurpleTypingState state,
                                            guint seconds)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member));

	/* Remove an existing timeout if necessary. */
	g_clear_handle_id(&member->typing_timeout, g_source_remove);

	/* If the state has changed, notify. */
	if(state != member->typing_state) {
		member->typing_state = state;

		g_object_notify_by_pspec(G_OBJECT(member),
		                         properties[PROP_TYPING_STATE]);
	}

	/* If we got a timeout, add it. */
	if(seconds > 0) {
		guint source = 0;

		source = g_timeout_add_seconds(seconds,
		                               purple_conversation_member_reset_typing_state,
		                               member);

		member->typing_timeout = source;
	}
}

const char *
purple_conversation_member_get_nickname(PurpleConversationMember *member) {
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member), NULL);

	return member->nickname;
}

void
purple_conversation_member_set_nickname(PurpleConversationMember *member,
                                        const char *nickname)
{
	g_return_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member));

	if(g_set_str(&member->nickname, nickname)) {
		GObject *obj = G_OBJECT(member);

		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_NICKNAME]);
		g_object_notify_by_pspec(obj, properties[PROP_NAME_FOR_DISPLAY]);
		g_object_thaw_notify(obj);
	}
}

const char *
purple_conversation_member_get_name_for_display(PurpleConversationMember *member)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBER(member), NULL);

	if(!purple_strempty(member->nickname)) {
		return member->nickname;
	}

	return purple_contact_info_get_name_for_display(member->contact_info);
}
