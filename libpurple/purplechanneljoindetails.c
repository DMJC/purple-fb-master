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

#include "purplechanneljoindetails.h"
#include "util.h"

enum {
	PROP_0,
	PROP_NAME,
	PROP_NICKNAME,
	PROP_NICKNAME_SUPPORTED,
	PROP_PASSWORD,
	PROP_PASSWORD_SUPPORTED,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

struct _PurpleChannelJoinDetails {
	GObject parent;

	char *name;
	char *nickname;
	gboolean nickname_supported;
	char *password;
	gboolean password_supported;
};

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_channel_join_details_set_nickname_supported(PurpleChannelJoinDetails *details,
                                                   gboolean nickname_supported)
{
	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	if(details->nickname_supported != nickname_supported) {
		details->nickname_supported = nickname_supported;

		g_object_notify_by_pspec(G_OBJECT(details),
		                         properties[PROP_NICKNAME_SUPPORTED]);
	}
}

static void
purple_channel_join_details_set_password_supported(PurpleChannelJoinDetails *details,
                                                   gboolean password_supported)
{
	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	if(details->password_supported != password_supported) {
		details->password_supported = password_supported;

		g_object_notify_by_pspec(G_OBJECT(details),
		                         properties[PROP_PASSWORD_SUPPORTED]);
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleChannelJoinDetails, purple_channel_join_details,
                    G_TYPE_OBJECT)

static void
purple_channel_join_details_finalize(GObject *obj) {
	PurpleChannelJoinDetails *details = PURPLE_CHANNEL_JOIN_DETAILS(obj);

	g_clear_pointer(&details->name, g_free);
	g_clear_pointer(&details->nickname, g_free);
	g_clear_pointer(&details->password, g_free);

	G_OBJECT_CLASS(purple_channel_join_details_parent_class)->finalize(obj);
}

static void
purple_channel_join_details_get_property(GObject *obj, guint param_id,
                                         GValue *value, GParamSpec *pspec)
{
	PurpleChannelJoinDetails *details = PURPLE_CHANNEL_JOIN_DETAILS(obj);

	switch(param_id) {
	case PROP_NAME:
		g_value_set_string(value,
		                   purple_channel_join_details_get_name(details));
		break;
	case PROP_NICKNAME:
		g_value_set_string(value,
		                   purple_channel_join_details_get_nickname(details));
		break;
	case PROP_NICKNAME_SUPPORTED:
		g_value_set_boolean(value,
		                    purple_channel_join_details_get_nickname_supported(details));
		break;
	case PROP_PASSWORD:
		g_value_set_string(value,
		                   purple_channel_join_details_get_password(details));
		break;
	case PROP_PASSWORD_SUPPORTED:
		g_value_set_boolean(value,
		                    purple_channel_join_details_get_password_supported(details));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_channel_join_details_set_property(GObject *obj, guint param_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
	PurpleChannelJoinDetails *details = PURPLE_CHANNEL_JOIN_DETAILS(obj);

	switch(param_id) {
	case PROP_NAME:
		purple_channel_join_details_set_name(details,
		                                     g_value_get_string(value));
		break;
	case PROP_NICKNAME:
		purple_channel_join_details_set_nickname(details,
		                                         g_value_get_string(value));
		break;
	case PROP_NICKNAME_SUPPORTED:
		purple_channel_join_details_set_nickname_supported(details,
		                                                   g_value_get_boolean(value));
		break;
	case PROP_PASSWORD:
		purple_channel_join_details_set_password(details,
		                                         g_value_get_string(value));
		break;
	case PROP_PASSWORD_SUPPORTED:
		purple_channel_join_details_set_password_supported(details,
		                                                   g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_channel_join_details_init(G_GNUC_UNUSED PurpleChannelJoinDetails *details)
{
}

static void
purple_channel_join_details_class_init(PurpleChannelJoinDetailsClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_channel_join_details_finalize;
	obj_class->get_property = purple_channel_join_details_get_property;
	obj_class->set_property = purple_channel_join_details_set_property;

	/**
	 * PurpleChannelJoinDetails:name:
	 *
	 * The name of the channel to join.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NAME] = g_param_spec_string(
		"name", "name",
		"The name of the channel to join.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleChannelJoinDetails:nickname:
	 *
	 * The nickname of the user in the channel.
	 *
	 * Not all protocols support a channel-specific username, so user
	 * interfaces should check [property@ChannelJoinDetails:nickname-supported]
	 * before displaying this option to the user.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NICKNAME] = g_param_spec_string(
		"nickname", "nickname",
		"The channel-specific nickname for the user.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleChannelJoinDetails:nickname-supported:
	 *
	 * Whether or not the protocol supports channel-specific nicknames.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NICKNAME_SUPPORTED] = g_param_spec_boolean(
		"nickname-supported", "nickname-supported",
		"Whether or not the protocol supports channel-specific nicknames.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleChannelJoinDetails:password:
	 *
	 * The password to use to join the channel.
	 *
	 * This is protocol specific so be sure to check
	 * [property@PurpleChannelJoinDetails:password-supported] before displaying
	 * this option to the user.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PASSWORD] = g_param_spec_string(
		"password", "password",
		"The password to join this channel.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleChannelJoinDetails:password-supported:
	 *
	 * Whether or not the protocol supports channel passwords.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PASSWORD_SUPPORTED] = g_param_spec_boolean(
		"password-supported", "password-supported",
		"Whether or not the protocol supports channel passwords.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleChannelJoinDetails *
purple_channel_join_details_new(gboolean nickname_supported,
                                gboolean password_supported)
{
	return g_object_new(
		PURPLE_TYPE_CHANNEL_JOIN_DETAILS,
		"nickname-supported", nickname_supported,
		"password-supported", password_supported,
		NULL);
}

const char *
purple_channel_join_details_get_name(PurpleChannelJoinDetails *details) {
	g_return_val_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details), NULL);

	return details->name;
}

void
purple_channel_join_details_set_name(PurpleChannelJoinDetails *details,
                                     const char *name)
{
	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	if(g_set_str(&details->name, name)) {
		g_object_notify_by_pspec(G_OBJECT(details), properties[PROP_NAME]);
	}
}

const char *
purple_channel_join_details_get_nickname(PurpleChannelJoinDetails *details) {
	g_return_val_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details), NULL);

	return details->nickname;
}

void
purple_channel_join_details_set_nickname(PurpleChannelJoinDetails *details,
                                         const char *nickname)
{
	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	if(g_set_str(&details->nickname, nickname)) {
		g_object_notify_by_pspec(G_OBJECT(details), properties[PROP_NICKNAME]);
	}
}

gboolean
purple_channel_join_details_get_nickname_supported(PurpleChannelJoinDetails *details) {
	g_return_val_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details), FALSE);

	return details->nickname_supported;
}

const char *
purple_channel_join_details_get_password(PurpleChannelJoinDetails *details) {
	g_return_val_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details), NULL);

	return details->password;
}

void
purple_channel_join_details_set_password(PurpleChannelJoinDetails *details,
                                         const char *password)
{
	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	if(g_set_str(&details->password, password)) {
		g_object_notify_by_pspec(G_OBJECT(details), properties[PROP_PASSWORD]);
	}
}

gboolean
purple_channel_join_details_get_password_supported(PurpleChannelJoinDetails *details)
{
	g_return_val_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details), FALSE);

	return details->password_supported;
}

void
purple_channel_join_details_merge(PurpleChannelJoinDetails *source,
                                  PurpleChannelJoinDetails *destination)
{
	GObject *obj = NULL;
	const char *value = NULL;
	gboolean supported = FALSE;

	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(source));
	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(destination));

	obj = G_OBJECT(destination);
	g_object_freeze_notify(obj);

	value = purple_channel_join_details_get_name(source);
	purple_channel_join_details_set_name(destination, value);

	supported = purple_channel_join_details_get_nickname_supported(source);
	purple_channel_join_details_set_nickname_supported(destination, supported);

	value = purple_channel_join_details_get_nickname(source);
	purple_channel_join_details_set_nickname(destination, value);

	supported = purple_channel_join_details_get_password_supported(source);
	purple_channel_join_details_set_password_supported(destination, supported);

	value = purple_channel_join_details_get_password(source);
	purple_channel_join_details_set_password(destination, value);

	g_object_thaw_notify(obj);
}
