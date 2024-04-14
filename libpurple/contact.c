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

#include "contact.h"

#include "buddylist.h"
#include "prefs.h"
#include "purpleconversationmanager.h"
#include "purpleprivate.h"
#include "signals.h"
#include "util.h"

typedef struct _PurpleMetaContactPrivate    PurpleMetaContactPrivate;

struct _PurpleMetaContactPrivate {
	char *alias;                  /* The user-set alias of the contact  */
};

enum
{
	PROP_0,
	PROP_ALIAS,
	N_PROPERTIES,
};

/******************************************************************************
 * Globals
 *****************************************************************************/
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(PurpleMetaContact, purple_meta_contact,
		PURPLE_TYPE_COUNTING_NODE);

/******************************************************************************
 * API
 *****************************************************************************/
void
purple_meta_contact_set_alias(PurpleMetaContact *contact, const char *alias)
{
	PurpleMetaContactPrivate *priv = NULL;
	char *old_alias;
	char *new_alias = NULL;

	g_return_if_fail(PURPLE_IS_META_CONTACT(contact));
	priv = purple_meta_contact_get_instance_private(contact);

	if ((alias != NULL) && (*alias != '\0'))
		new_alias = purple_utf8_strip_unprintables(alias);

	if (!purple_strequal(priv->alias, new_alias)) {
		g_free(new_alias);
		return;
	}

	old_alias = priv->alias;

	if ((new_alias != NULL) && (*new_alias != '\0')) {
		priv->alias = new_alias;
	} else {
		priv->alias = NULL;
		g_free(new_alias); /* could be "\0" */
	}

	g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_ALIAS]);

	purple_blist_save_node(purple_blist_get_default(),
	                       PURPLE_BLIST_NODE(contact));
	purple_blist_update_node(purple_blist_get_default(),
	                         PURPLE_BLIST_NODE(contact));

	purple_signal_emit(purple_blist_get_handle(), "blist-node-aliased",
	                   contact, old_alias);
	g_free(old_alias);
}

const char *purple_meta_contact_get_alias(PurpleMetaContact* contact)
{
	PurpleMetaContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_META_CONTACT(contact), NULL);

	priv = purple_meta_contact_get_instance_private(contact);
	if (priv->alias)
		return priv->alias;

	return NULL;
}

/**************************************************************************
 * GObject Stuff
 **************************************************************************/
/* Set method for GObject properties */
static void
purple_meta_contact_set_property(GObject *obj, guint param_id, const GValue *value,
		GParamSpec *pspec)
{
	PurpleMetaContact *contact = PURPLE_META_CONTACT(obj);

	switch (param_id) {
		case PROP_ALIAS:
			purple_meta_contact_set_alias(contact, g_value_get_string(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

/* Get method for GObject properties */
static void
purple_meta_contact_get_property(GObject *obj, guint param_id, GValue *value,
		GParamSpec *pspec)
{
	PurpleMetaContact *contact = PURPLE_META_CONTACT(obj);
	PurpleMetaContactPrivate *priv =
			purple_meta_contact_get_instance_private(contact);

	switch (param_id) {
		case PROP_ALIAS:
			g_value_set_string(value, priv->alias);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

/* GObject initialization function */
static void
purple_meta_contact_init(PurpleMetaContact *contact)
{
	purple_blist_new_node(purple_blist_get_default(),
	                      PURPLE_BLIST_NODE(contact));
}

/* GObject finalize function */
static void
purple_meta_contact_finalize(GObject *object)
{
	PurpleMetaContactPrivate *priv = purple_meta_contact_get_instance_private(
			PURPLE_META_CONTACT(object));

	g_free(priv->alias);

	G_OBJECT_CLASS(purple_meta_contact_parent_class)->finalize(object);
}

/* Class initializer function */
static void purple_meta_contact_class_init(PurpleMetaContactClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_meta_contact_finalize;

	/* Setup properties */
	obj_class->get_property = purple_meta_contact_get_property;
	obj_class->set_property = purple_meta_contact_set_property;

	/**
	 * PurpleMetaContact:alias:
	 *
	 * The alias for the contact.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ALIAS] = g_param_spec_string(
		"alias",
		"Alias",
		"The alias for the contact.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
	);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

PurpleMetaContact *
purple_meta_contact_new(void)
{
	return g_object_new(PURPLE_TYPE_META_CONTACT, NULL);
}

