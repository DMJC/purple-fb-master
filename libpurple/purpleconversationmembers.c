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

#include "purpleconversationmembers.h"

struct _PurpleConversationMembers {
	GObject parent;

	GPtrArray *members;

	GListStore *active_typers;
};

enum {
	PROP_0,
	PROP_ITEM_TYPE,
	PROP_N_ITEMS,
	PROP_ACTIVE_TYPERS,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_MEMBER_ADDED,
	SIG_MEMBER_REMOVED,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
purple_conversation_members_check_member_equal(gconstpointer a, gconstpointer b)
{
	PurpleConversationMember *member_a = (PurpleConversationMember *)a;
	PurpleConversationMember *member_b = (PurpleConversationMember *)b;
	PurpleContactInfo *info_a = NULL;
	PurpleContactInfo *info_b = NULL;

	info_a = purple_conversation_member_get_contact_info(member_a);
	info_b = purple_conversation_member_get_contact_info(member_b);

	return (purple_contact_info_compare(info_a, info_b) == 0);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_conversation_members_member_changed_cb(GObject *self,
                                              G_GNUC_UNUSED GParamSpec *pspec,
                                              gpointer data)
{
	PurpleConversationMember *member = PURPLE_CONVERSATION_MEMBER(self);
	PurpleConversationMembers *members = data;
	gboolean found = FALSE;
	guint position = 0;

	found = g_ptr_array_find_with_equal_func(members->members, member,
	                                         purple_conversation_members_check_member_equal,
	                                         &position);

	if(found) {
		g_list_model_items_changed(G_LIST_MODEL(members), position, 0, 0);
	}
}

static void
purple_conversation_members_typing_changed_cb(GObject *self,
                                              G_GNUC_UNUSED GParamSpec *pspec,
                                              gpointer data)
{
	PurpleConversationMember *member = PURPLE_CONVERSATION_MEMBER(self);
	PurpleConversationMembers *members = data;
	PurpleTypingState state = PURPLE_TYPING_STATE_NONE;
	gboolean found = FALSE;
	guint position = 0;

	state = purple_conversation_member_get_typing_state(member);

	found = g_list_store_find(members->active_typers, member, &position);
	if(found && state != PURPLE_TYPING_STATE_TYPING) {
		g_list_store_remove(members->active_typers, position);
	}

	if(!found && state == PURPLE_TYPING_STATE_TYPING) {
		g_list_store_append(members->active_typers, member);
	}
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_conversation_members_get_item_type(G_GNUC_UNUSED GListModel *list) {
	return PURPLE_TYPE_CONVERSATION_MEMBER;
}

static guint
purple_conversation_members_get_n_items(GListModel *list) {
	PurpleConversationMembers *members = PURPLE_CONVERSATION_MEMBERS(list);

	return members->members->len;
}

static gpointer
purple_conversation_members_get_item(GListModel *list, guint position) {
	PurpleConversationMembers *members = PURPLE_CONVERSATION_MEMBERS(list);
	PurpleConversationMember *member = NULL;

	if(position < members->members->len) {
		member = g_ptr_array_index(members->members, position);
		g_object_ref(member);
	}

	return member;
}

static void
purple_conversation_members_list_model_init(GListModelInterface *iface) {
	iface->get_item_type = purple_conversation_members_get_item_type;
	iface->get_n_items = purple_conversation_members_get_n_items;
	iface->get_item = purple_conversation_members_get_item;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE_WITH_CODE(
	PurpleConversationMembers,
	purple_conversation_members,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
	                      purple_conversation_members_list_model_init))

static void
purple_conversation_members_finalize(GObject *obj) {
	PurpleConversationMembers *members = PURPLE_CONVERSATION_MEMBERS(obj);

	g_clear_pointer(&members->members, g_ptr_array_unref);
	g_clear_object(&members->active_typers);

	G_OBJECT_CLASS(purple_conversation_members_parent_class)->finalize(obj);
}

static void
purple_conversation_members_get_property(GObject *obj, guint param_id,
                                         GValue *value, GParamSpec *pspec)
{
	PurpleConversationMembers *members = PURPLE_CONVERSATION_MEMBERS(obj);

	switch(param_id) {
	case PROP_ITEM_TYPE:
		g_value_set_gtype(value,
		                  g_list_model_get_item_type(G_LIST_MODEL(obj)));
		break;
	case PROP_N_ITEMS:
		g_value_set_uint(value, g_list_model_get_n_items(G_LIST_MODEL(obj)));
		break;
	case PROP_ACTIVE_TYPERS:
		g_value_set_object(value,
		                   purple_conversation_members_get_active_typers(members));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_conversation_members_init(PurpleConversationMembers *members) {
	/* We allocate 2 as dms have at least 2 members. */
	members->members = g_ptr_array_new_full(2, g_object_unref);

	members->active_typers = g_list_store_new(PURPLE_TYPE_CONVERSATION_MEMBER);
}

static void
purple_conversation_members_class_init(PurpleConversationMembersClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_conversation_members_finalize;
	obj_class->get_property = purple_conversation_members_get_property;

	/**
	 * PurpleConversationMembers:item-type:
	 *
	 * The type of items. See [iface@Gio.ListModel.get_item_type].
	 *
	 * Since: 3.0
	 */
	properties[PROP_ITEM_TYPE] = g_param_spec_gtype(
		"item-type", NULL, NULL,
		G_TYPE_OBJECT,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversationMembers:n-items:
	 *
	 * The number of items. See [iface@Gio.ListModel.get_n_items].
	 *
	 * Since: 3.0
	 */
	properties[PROP_N_ITEMS] = g_param_spec_uint(
		"n-items", NULL, NULL,
		0, G_MAXUINT, 0,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConversationMembers:active-typers:
	 *
	 * The list of [class@ConversationMember]s that are actively typing.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ACTIVE_TYPERS] = g_param_spec_object(
		"active-typers", NULL, NULL,
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleConversationMembers::member-added:
	 * @members: The instance.
	 * @member: The [class@Purple.ConversationMember] instance.
	 * @announce: Whether or not this addition should be announced.
	 * @message: (nullable): An optional message to use in the announcement.
	 *
	 * Emitted when a new member is added to this collection.
	 *
	 * Since: 3.0
	 */
	signals[SIG_MEMBER_ADDED] = g_signal_new_class_handler(
		"member-added",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		3,
		PURPLE_TYPE_CONVERSATION_MEMBER,
		G_TYPE_BOOLEAN,
		G_TYPE_STRING);

	/**
	 * PurpleConversationMembers::member-removed:
	 * @members: The instance.
	 * @member: The [class@Purple.ConversationMember] instance.
	 * @announce: Whether or not this removal should be announced.
	 * @message: (nullable): An optional message to use in the announcement.
	 *
	 * Emitted when member is removed from this collection.
	 *
	 * Since: 3.0
	 */
	signals[SIG_MEMBER_REMOVED] = g_signal_new_class_handler(
		"member-removed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		3,
		PURPLE_TYPE_CONVERSATION_MEMBER,
		G_TYPE_BOOLEAN,
		G_TYPE_STRING);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleConversationMember *
purple_conversation_members_add_member(PurpleConversationMembers *members,
                                       PurpleContactInfo *info,
                                       gboolean announce,
                                       const char *message)
{
	PurpleConversationMember *member = NULL;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBERS(members), NULL);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(info), NULL);

	member = purple_conversation_members_find_member(members, info);
	if(PURPLE_IS_CONVERSATION_MEMBER(member)) {
		return member;
	}

	member = purple_conversation_member_new(info);
	g_ptr_array_add(members->members, member);

	/* Add a callback for notify::name-for-display and typing-state on info. */
	g_signal_connect_object(member, "notify::name-for-display",
	                        G_CALLBACK(purple_conversation_members_member_changed_cb),
	                        members, G_CONNECT_DEFAULT);
	g_signal_connect_object(member, "notify::typing-state",
	                        G_CALLBACK(purple_conversation_members_typing_changed_cb),
	                        members, G_CONNECT_DEFAULT);

	g_signal_emit(members, signals[SIG_MEMBER_ADDED], 0, member, announce,
	              message);
	g_list_model_items_changed(G_LIST_MODEL(members),
	                           members->members->len - 1, 0, 1);

	return member;
}

PurpleConversationMember *
purple_conversation_members_find_member(PurpleConversationMembers *members,
                                        PurpleContactInfo *info)
{
	PurpleConversationMember *member = NULL;
	guint position = 0;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBERS(members), NULL);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(info), NULL);

	if(purple_conversation_members_has_member(members, info, &position)) {
		member = g_ptr_array_index(members->members, position);
	}

	return member;
}

gboolean
purple_conversation_members_has_member(PurpleConversationMembers *members,
                                       PurpleContactInfo *info,
                                       guint *position)
{
	PurpleConversationMember *needle = NULL;
	gboolean found = FALSE;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBERS(members), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(info), FALSE);

	needle = purple_conversation_member_new(info);
	found = g_ptr_array_find_with_equal_func(members->members,
	                                         needle,
	                                         purple_conversation_members_check_member_equal,
	                                         position);

	g_clear_object(&needle);

	return found;
}

PurpleConversationMembers *
purple_conversation_members_new(void) {
	return g_object_new(PURPLE_TYPE_CONVERSATION_MEMBERS, NULL);
}

gboolean
purple_conversation_members_remove_member(PurpleConversationMembers *members,
                                          PurpleContactInfo *info,
                                          gboolean announce,
                                          const char *message)
{
	PurpleConversationMember *member = NULL;
	guint position = 0;

	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBERS(members), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(info), FALSE);

	if(!purple_conversation_members_has_member(members, info, &position)) {
		return FALSE;
	}

	member = g_ptr_array_steal_index(members->members, position);

	/* Remove our signal handlers for the member. */
	g_signal_handlers_disconnect_by_func(member,
	                                     purple_conversation_members_member_changed_cb,
	                                     members);


	g_signal_emit(members, signals[SIG_MEMBER_REMOVED], 0, member,
	              announce, message);
	g_list_model_items_changed(G_LIST_MODEL(members), position, 1, 0);

	g_clear_object(&member);

	return TRUE;
}

GListModel *
purple_conversation_members_get_active_typers(PurpleConversationMembers *members)
{
	g_return_val_if_fail(PURPLE_IS_CONVERSATION_MEMBERS(members), NULL);

	return G_LIST_MODEL(members->active_typers);
}
