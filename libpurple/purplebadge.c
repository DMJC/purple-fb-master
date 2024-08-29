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

#include "purplebadge.h"

#include "util.h"

struct _PurpleBadge {
	GObject parent;

	char *id;
	int priority;
	char *icon_name;
	char *mnemonic;
	char *tooltip_text;
	char *description;

	char *link_text;
	char *link_uri;
};

enum {
	PROP_0,
	PROP_ID,
	PROP_PRIORITY,
	PROP_ICON_NAME,
	PROP_MNEMONIC,
	PROP_TOOLTIP_TEXT,
	PROP_DESCRIPTION,
	PROP_LINK_TEXT,
	PROP_LINK_URI,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_badge_set_id(PurpleBadge *badge, const char *id) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(g_set_str(&badge->id, id)) {
		g_object_notify_by_pspec(G_OBJECT(badge), properties[PROP_ID]);
	}
}

static void
purple_badge_set_priority(PurpleBadge *badge, int priority) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(badge->priority != priority) {
		badge->priority = priority;

		g_object_notify_by_pspec(G_OBJECT(badge), properties[PROP_PRIORITY]);
	}
}

static void
purple_badge_set_icon_name(PurpleBadge *badge, const char *icon_name) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(g_set_str(&badge->icon_name, icon_name)) {
		g_object_notify_by_pspec(G_OBJECT(badge), properties[PROP_ICON_NAME]);
	}
}

static void
purple_badge_set_mnemonic(PurpleBadge *badge, const char *mnemonic) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(g_set_str(&badge->mnemonic, mnemonic)) {
		g_object_notify_by_pspec(G_OBJECT(badge), properties[PROP_MNEMONIC]);
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleBadge, purple_badge,
                    G_TYPE_OBJECT)

static void
purple_badge_get_property(GObject *obj, guint param_id, GValue *value,
                          GParamSpec *pspec)
{
	PurpleBadge *badge = PURPLE_BADGE(obj);

	switch(param_id) {
	case PROP_ID:
		g_value_set_string(value, purple_badge_get_id(badge));
		break;
	case PROP_PRIORITY:
		g_value_set_int(value, purple_badge_get_priority(badge));
		break;
	case PROP_ICON_NAME:
		g_value_set_string(value, purple_badge_get_icon_name(badge));
		break;
	case PROP_MNEMONIC:
		g_value_set_string(value, purple_badge_get_mnemonic(badge));
		break;
	case PROP_TOOLTIP_TEXT:
		g_value_set_string(value, purple_badge_get_tooltip_text(badge));
		break;
	case PROP_DESCRIPTION:
		g_value_set_string(value, purple_badge_get_description(badge));
		break;
	case PROP_LINK_TEXT:
		g_value_set_string(value, purple_badge_get_link_text(badge));
		break;
	case PROP_LINK_URI:
		g_value_set_string(value, purple_badge_get_link_uri(badge));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_badge_set_property(GObject *obj, guint param_id, const GValue *value,
                          GParamSpec *pspec)
{
	PurpleBadge *badge = PURPLE_BADGE(obj);

	switch(param_id) {
	case PROP_ID:
		purple_badge_set_id(badge, g_value_get_string(value));
		break;
	case PROP_PRIORITY:
		purple_badge_set_priority(badge, g_value_get_int(value));
		break;
	case PROP_ICON_NAME:
		purple_badge_set_icon_name(badge, g_value_get_string(value));
		break;
	case PROP_MNEMONIC:
		purple_badge_set_mnemonic(badge, g_value_get_string(value));
		break;
	case PROP_TOOLTIP_TEXT:
		purple_badge_set_tooltip_text(badge, g_value_get_string(value));
		break;
	case PROP_DESCRIPTION:
		purple_badge_set_description(badge, g_value_get_string(value));
		break;
	case PROP_LINK_TEXT:
		purple_badge_set_link_text(badge, g_value_get_string(value));
		break;
	case PROP_LINK_URI:
		purple_badge_set_link_uri(badge, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_badge_finalize(GObject *obj) {
	PurpleBadge *badge = PURPLE_BADGE(obj);

	g_clear_pointer(&badge->id, g_free);
	g_clear_pointer(&badge->icon_name, g_free);
	g_clear_pointer(&badge->mnemonic, g_free);
	g_clear_pointer(&badge->tooltip_text, g_free);
	g_clear_pointer(&badge->description, g_free);
	g_clear_pointer(&badge->link_text, g_free);
	g_clear_pointer(&badge->link_uri, g_free);

	G_OBJECT_CLASS(purple_badge_parent_class)->finalize(obj);
}

static void
purple_badge_init(G_GNUC_UNUSED PurpleBadge *badge) {
}

static void
purple_badge_class_init(PurpleBadgeClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_badge_get_property;
	obj_class->set_property = purple_badge_set_property;
	obj_class->finalize = purple_badge_finalize;

	/**
	 * PurpleBadge:id:
	 *
	 * The id for this badge.
	 *
	 * This should be name spaced to avoid collisions. For example, the IRCv3
	 * protocol would use ircv3-prefix-operator for operators.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadge:priority:
	 *
	 * The priority of this badge.
	 *
	 * Badges with higher priorities will be sorted first.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PRIORITY] = g_param_spec_int(
		"priority", NULL, NULL,
		G_MININT32, G_MAXINT32, 0,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadge:icon-name:
	 *
	 * The name of the icon for the badge.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ICON_NAME] = g_param_spec_string(
		"icon-name", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadge:mnemonic:
	 *
	 * The mnemonic for the badge.
	 *
	 * The mnemonic is a fallback in case that the icon can not be found or can
	 * be used by text interfaces.
	 *
	 * Since: 3.0
	 */
	properties[PROP_MNEMONIC] = g_param_spec_string(
		"mnemonic", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadge:tooltip-text:
	 *
	 * Text to be displayed in the tooltip for this badge.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TOOLTIP_TEXT] = g_param_spec_string(
		"tooltip-text", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadge:description:
	 *
	 * The description of the badge.
	 *
	 * This may be shown as part of the tooltip to describe to the end user
	 * what this badge is for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_DESCRIPTION] = g_param_spec_string(
		"description", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadge:link-text:
	 *
	 * The text to display for the link of the badge.
	 *
	 * Since: 3.0
	 */
	properties[PROP_LINK_TEXT] = g_param_spec_string(
		"link-text", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleBadge:link-uri:
	 *
	 * A URI for the badge that could link to a page describing more about it.
	 *
	 * Since: 3.0
	 */
	properties[PROP_LINK_URI] = g_param_spec_string(
		"link-uri", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleBadge *
purple_badge_new(const char *id, int priority, const char *icon_name,
                 const char *mnemonic)
{
	g_return_val_if_fail(!purple_strempty(id), NULL);
	g_return_val_if_fail(!purple_strempty(icon_name), NULL);
	g_return_val_if_fail(!purple_strempty(mnemonic), NULL);

	return g_object_new(
		PURPLE_TYPE_BADGE,
		"id", id,
		"priority", priority,
		"icon-name", icon_name,
		"mnemonic", mnemonic,
		NULL);
}

const char *
purple_badge_get_id(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), NULL);

	return badge->id;
}

int
purple_badge_get_priority(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), 0);

	return badge->priority;
}

const char *
purple_badge_get_icon_name(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), NULL);

	return badge->icon_name;
}

const char *
purple_badge_get_mnemonic(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), NULL);

	return badge->mnemonic;
}

const char *
purple_badge_get_tooltip_text(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), NULL);

	return badge->tooltip_text;
}

void
purple_badge_set_tooltip_text(PurpleBadge *badge, const char *tooltip_text) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(g_set_str(&badge->tooltip_text, tooltip_text)) {
		g_object_notify_by_pspec(G_OBJECT(badge),
		                         properties[PROP_TOOLTIP_TEXT]);
	}
}

const char *
purple_badge_get_description(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), NULL);

	return badge->description;
}

void
purple_badge_set_description(PurpleBadge *badge, const char *description) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(g_set_str(&badge->description, description)) {
		g_object_notify_by_pspec(G_OBJECT(badge),
		                         properties[PROP_DESCRIPTION]);
	}
}

const char *
purple_badge_get_link_text(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), NULL);

	return badge->link_text;
}

void
purple_badge_set_link_text(PurpleBadge *badge, const char *link_text) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(g_set_str(&badge->link_text, link_text)) {
		g_object_notify_by_pspec(G_OBJECT(badge), properties[PROP_LINK_TEXT]);
	}
}

const char *
purple_badge_get_link_uri(PurpleBadge *badge) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge), NULL);

	return badge->link_uri;
}

void
purple_badge_set_link_uri(PurpleBadge *badge, const char *link_uri) {
	g_return_if_fail(PURPLE_IS_BADGE(badge));

	if(g_set_str(&badge->link_uri, link_uri)) {
		g_object_notify_by_pspec(G_OBJECT(badge), properties[PROP_LINK_URI]);
	}
}

int
purple_badge_compare(PurpleBadge *badge1, PurpleBadge *badge2) {
	if(badge1 == NULL && badge2 == NULL) {
		return 0;
	}

	if(!PURPLE_IS_BADGE(badge1)) {
		return 1;
	}

	if(!PURPLE_IS_BADGE(badge2)) {
		return -1;
	}

	return badge2->priority - badge1->priority;
}

gboolean
purple_badge_equal(PurpleBadge *badge1, PurpleBadge *badge2) {
	g_return_val_if_fail(PURPLE_IS_BADGE(badge1), FALSE);
	g_return_val_if_fail(PURPLE_IS_BADGE(badge2), FALSE);

	return purple_strequal(badge1->id, badge2->id);
}
