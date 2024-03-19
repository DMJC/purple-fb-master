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

#include <gio/gio.h>

#include "purplecreateconversationdetails.h"

#include "purplecontact.h"

enum {
	PROP_0,
	PROP_MAX_PARTICIPANTS,
	PROP_PARTICIPANTS,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

struct _PurpleCreateConversationDetails {
	GObject parent;

	guint max_participants;
	GListModel *participants;
};

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_create_conversation_details_set_max_participants(PurpleCreateConversationDetails *details,
                                                        guint max_participants)
{
	g_return_if_fail(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details));

	if(details->max_participants != max_participants) {
		details->max_participants = max_participants;

		g_object_notify_by_pspec(G_OBJECT(details),
		                         properties[PROP_MAX_PARTICIPANTS]);
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_TYPE(PurpleCreateConversationDetails,
              purple_create_conversation_details,
              G_TYPE_OBJECT)

static void
purple_create_conversation_details_finalize(GObject *obj) {
	PurpleCreateConversationDetails *details = NULL;

	details = PURPLE_CREATE_CONVERSATION_DETAILS(obj);

	g_clear_object(&details->participants);

	G_OBJECT_CLASS(purple_create_conversation_details_parent_class)->finalize(obj);
}

static void
purple_create_conversation_details_get_property(GObject *obj, guint param_id,
                                                GValue *value,
                                                GParamSpec *pspec)
{
	PurpleCreateConversationDetails *details = NULL;

	details = PURPLE_CREATE_CONVERSATION_DETAILS(obj);

	switch(param_id) {
	case PROP_MAX_PARTICIPANTS:
		g_value_set_uint(value,
		                 purple_create_conversation_details_get_max_participants(details));
		break;
	case PROP_PARTICIPANTS:
		g_value_set_object(value,
		                   purple_create_conversation_details_get_participants(details));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_create_conversation_details_set_property(GObject *obj, guint param_id,
                                                const GValue *value,
                                                GParamSpec *pspec)
{
	PurpleCreateConversationDetails *details = NULL;

	details = PURPLE_CREATE_CONVERSATION_DETAILS(obj);

	switch(param_id) {
	case PROP_MAX_PARTICIPANTS:
		purple_create_conversation_details_set_max_participants(details,
		                                                        g_value_get_uint(value));
		break;
	case PROP_PARTICIPANTS:
		purple_create_conversation_details_set_participants(details,
		                                                    g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_create_conversation_details_init(G_GNUC_UNUSED PurpleCreateConversationDetails *details)
{
}

static void
purple_create_conversation_details_class_init(PurpleCreateConversationDetailsClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_create_conversation_details_finalize;
	obj_class->get_property = purple_create_conversation_details_get_property;
	obj_class->set_property = purple_create_conversation_details_set_property;

	/**
	 * PurpleCreateConversationDetails:max-participants:
	 *
	 * The maximum number of participants that can be supported by the
	 * protocol not including the libpurple user.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_MAX_PARTICIPANTS] = g_param_spec_uint(
		"max-participants", "max-participants",
		"The maximum number of participants that the protocol supports.",
		0, G_MAXUINT, 0,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleCreateConversationDetails:participants:
	 *
	 * A [iface@Gio.ListModel] of [class@Contact]s to add to the conversation
	 * not including the libpurple user.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_PARTICIPANTS] = g_param_spec_object(
		"participants", "participants",
		"The list of contacts to add to the conversation.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleCreateConversationDetails *
purple_create_conversation_details_new(guint max_participants) {
	return g_object_new(
		PURPLE_TYPE_CREATE_CONVERSATION_DETAILS,
		"max-participants", max_participants,
		NULL);
}

guint
purple_create_conversation_details_get_max_participants(PurpleCreateConversationDetails *details)
{
	g_return_val_if_fail(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details), 0);

	return details->max_participants;
}

GListModel *
purple_create_conversation_details_get_participants(PurpleCreateConversationDetails *details)
{
	g_return_val_if_fail(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details), NULL);

	return details->participants;
}

void
purple_create_conversation_details_set_participants(PurpleCreateConversationDetails *details,
                                                    GListModel *participants)
{
	g_return_if_fail(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details));

	if(G_IS_LIST_MODEL(participants)) {
		g_return_if_fail(g_list_model_get_item_type(participants) == PURPLE_TYPE_CONTACT);
	}

	if(g_set_object(&details->participants, participants)) {
		g_object_notify_by_pspec(G_OBJECT(details),
		                         properties[PROP_PARTICIPANTS]);
	}
}
