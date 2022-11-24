/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "purplecontact.h"

#include "purpleenums.h"
#include "util.h"

typedef struct  {
	gchar *id;
	PurpleAccount *account;

	gchar *username;
	gchar *display_name;
	gchar *alias;

	GdkPixbuf *avatar;

	PurplePresence *presence;

	PurpleTags *tags;

	PurplePerson *person;

	PurpleContactPermission permission;
} PurpleContactPrivate;

enum {
	PROP_0,
	PROP_ID,
	PROP_ACCOUNT,
	PROP_USERNAME,
	PROP_DISPLAY_NAME,
	PROP_ALIAS,
	PROP_AVATAR,
	PROP_PRESENCE,
	PROP_TAGS,
	PROP_PERSON,
	PROP_PERMISSION,
	N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(PurpleContact, purple_contact, G_TYPE_OBJECT)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_contact_set_account(PurpleContact *contact, PurpleAccount *account) {
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	priv = purple_contact_get_instance_private(contact);

	if(g_set_object(&priv->account, account)) {
		g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_ACCOUNT]);
	}
}

static PurpleAccount *
purple_contact_default_get_account(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->account;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_contact_get_property(GObject *obj, guint param_id, GValue *value,
                            GParamSpec *pspec)
{
	PurpleContact *contact = PURPLE_CONTACT(obj);

	switch(param_id) {
		case PROP_ID:
			g_value_set_string(value, purple_contact_get_id(contact));
			break;
		case PROP_ACCOUNT:
			g_value_set_object(value, purple_contact_get_account(contact));
			break;
		case PROP_USERNAME:
			g_value_set_string(value, purple_contact_get_username(contact));
			break;
		case PROP_DISPLAY_NAME:
			g_value_set_string(value, purple_contact_get_display_name(contact));
			break;
		case PROP_ALIAS:
			g_value_set_string(value, purple_contact_get_alias(contact));
			break;
		case PROP_AVATAR:
			g_value_set_object(value, purple_contact_get_avatar(contact));
			break;
		case PROP_PRESENCE:
			g_value_set_object(value, purple_contact_get_presence(contact));
			break;
		case PROP_TAGS:
			g_value_set_object(value, purple_contact_get_tags(contact));
			break;
		case PROP_PERSON:
			g_value_set_object(value, purple_contact_get_person(contact));
			break;
		case PROP_PERMISSION:
			g_value_set_enum(value, purple_contact_get_permission(contact));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_contact_set_property(GObject *obj, guint param_id, const GValue *value,
                            GParamSpec *pspec)
{
	PurpleContact *contact = PURPLE_CONTACT(obj);

	switch(param_id) {
		case PROP_ID:
			purple_contact_set_id(contact, g_value_get_string(value));
			break;
		case PROP_ACCOUNT:
			purple_contact_set_account(contact, g_value_get_object(value));
			break;
		case PROP_USERNAME:
			purple_contact_set_username(contact, g_value_get_string(value));
			break;
		case PROP_DISPLAY_NAME:
			purple_contact_set_display_name(contact, g_value_get_string(value));
			break;
		case PROP_ALIAS:
			purple_contact_set_alias(contact, g_value_get_string(value));
			break;
		case PROP_AVATAR:
			purple_contact_set_avatar(contact, g_value_get_object(value));
			break;
		case PROP_PERSON:
			purple_contact_set_person(contact, g_value_get_object(value));
			break;
		case PROP_PERMISSION:
			purple_contact_set_permission(contact, g_value_get_enum(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_contact_dispose(GObject *obj) {
	PurpleContact *contact = PURPLE_CONTACT(obj);
	PurpleContactPrivate *priv = purple_contact_get_instance_private(contact);

	g_clear_object(&priv->account);
	g_clear_object(&priv->avatar);
	g_clear_object(&priv->presence);
	g_clear_object(&priv->tags);
	g_clear_object(&priv->person);

	G_OBJECT_CLASS(purple_contact_parent_class)->dispose(obj);
}

static void
purple_contact_finalize(GObject *obj) {
	PurpleContact *contact = PURPLE_CONTACT(obj);
	PurpleContactPrivate *priv = purple_contact_get_instance_private(contact);

	g_clear_pointer(&priv->id, g_free);
	g_clear_pointer(&priv->username, g_free);
	g_clear_pointer(&priv->display_name, g_free);
	g_clear_pointer(&priv->alias, g_free);

	G_OBJECT_CLASS(purple_contact_parent_class)->finalize(obj);
}

static void
purple_contact_constructed(GObject *obj) {
	PurpleContact *contact = NULL;
	PurpleContactPrivate *priv = NULL;

	G_OBJECT_CLASS(purple_contact_parent_class)->constructed(obj);

	contact = PURPLE_CONTACT(obj);
	priv = purple_contact_get_instance_private(contact);

	if(priv->id == NULL) {
		purple_contact_set_id(contact, NULL);
	}
}

static void
purple_contact_init(PurpleContact *contact) {
	PurpleContactPrivate *priv = purple_contact_get_instance_private(contact);

	priv->tags = purple_tags_new();
	priv->presence = g_object_new(PURPLE_TYPE_PRESENCE, NULL);
}

static void
purple_contact_class_init(PurpleContactClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	klass->get_account = purple_contact_default_get_account;

	obj_class->constructed = purple_contact_constructed;
	obj_class->dispose = purple_contact_dispose;
	obj_class->finalize = purple_contact_finalize;
	obj_class->get_property = purple_contact_get_property;
	obj_class->set_property = purple_contact_set_property;

	/**
	 * PurpleContact:id:
	 *
	 * The protocol specific id for the contact.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", "id",
		"The id of the contact",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:account:
	 *
	 * The account that this contact belongs to.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", "account",
		"The account this contact belongs to",
		PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:username:
	 *
	 * The username for this contact. In rare cases this can change, like when
	 * a user changes their "nick" on IRC which is their user name.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_USERNAME] = g_param_spec_string(
		"username", "username",
		"The username of the contact",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:display-name:
	 *
	 * The display name for this contact. This is generally set by the person
	 * the contact is representing and controlled via the protocol plugin.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_DISPLAY_NAME] = g_param_spec_string(
		"display-name", "display-name",
		"The display name of the contact",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:alias:
	 *
	 * The alias for this contact. This is controlled by the libpurple user and
	 * may be used by the protocol if it allows for aliasing.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ALIAS] = g_param_spec_string(
		"alias", "alias",
		"The alias of the contact.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:avatar:
	 *
	 * The avatar for this contact. This is typically controlled by the protocol
	 * and should only be read by others.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_AVATAR] = g_param_spec_object(
		"avatar", "avatar",
		"The avatar of the contact",
		GDK_TYPE_PIXBUF,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:presence:
	 *
	 * The [class@Purple.Presence] for this contact. This is typically
	 * controlled by the protocol and should only be read by others.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_PRESENCE] = g_param_spec_object(
		"presence", "presence",
		"The presence of the contact",
		PURPLE_TYPE_PRESENCE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:tags:
	 *
	 * The [class@Purple.Tags] for this contact.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TAGS] = g_param_spec_object(
		"tags", "tags",
		"The tags for the contact",
		PURPLE_TYPE_TAGS,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:person:
	 *
	 * The [class@Purple.Person] that this contact belongs to.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_PERSON] = g_param_spec_object(
		"person", "person",
		"The person this contact belongs to.",
		PURPLE_TYPE_PERSON,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleContact:permission:
	 *
	 * The permission level for the contact.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_PERMISSION] = g_param_spec_enum(
		"permission", "permission",
		"The permission level of the contact",
		PURPLE_TYPE_CONTACT_PERMISSION,
		PURPLE_CONTACT_PERMISSION_UNSET,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleContact *
purple_contact_new(PurpleAccount *account, const gchar *id) {
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return g_object_new(
		PURPLE_TYPE_CONTACT,
		"account", account,
		"id", id,
		NULL);
}

PurpleAccount *
purple_contact_get_account(PurpleContact *contact) {
	PurpleContactClass *klass = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	klass = PURPLE_CONTACT_GET_CLASS(contact);
	if(klass != NULL && klass->get_account != NULL) {
		return klass->get_account(contact);
	}

	return NULL;
}

const gchar *
purple_contact_get_id(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->id;
}

void
purple_contact_set_id(PurpleContact *contact, const gchar *id) {
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));

	priv = purple_contact_get_instance_private(contact);

	g_free(priv->id);
	priv->id = g_strdup(id);

	g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_ID]);
}

const gchar *
purple_contact_get_username(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->username;
}

void
purple_contact_set_username(PurpleContact *contact, const gchar *username) {
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));
	g_return_if_fail(username != NULL);

	priv = purple_contact_get_instance_private(contact);

	g_free(priv->username);
	priv->username = g_strdup(username);

	g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_USERNAME]);
}

const gchar *
purple_contact_get_display_name(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->display_name;
}

void
purple_contact_set_display_name(PurpleContact *contact,
                                const gchar *display_name)
{
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));

	priv = purple_contact_get_instance_private(contact);

	g_free(priv->display_name);
	priv->display_name = g_strdup(display_name);

	g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_DISPLAY_NAME]);
}

const gchar *
purple_contact_get_alias(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->alias;
}

void
purple_contact_set_alias(PurpleContact *contact, const gchar *alias) {
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));

	priv = purple_contact_get_instance_private(contact);

	g_free(priv->alias);
	priv->alias = g_strdup(alias);

	g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_ALIAS]);
}

GdkPixbuf *
purple_contact_get_avatar(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->avatar;
}

void
purple_contact_set_avatar(PurpleContact *contact, GdkPixbuf *avatar) {
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));

	priv = purple_contact_get_instance_private(contact);

	if(g_set_object(&priv->avatar, avatar)) {
		g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_AVATAR]);
	}
}

PurplePresence *
purple_contact_get_presence(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->presence;
}

PurpleTags *
purple_contact_get_tags(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->tags;
}

void
purple_contact_set_person(PurpleContact *contact, PurplePerson *person) {
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));

	priv = purple_contact_get_instance_private(contact);

	if(g_set_object(&priv->person, person)) {
		g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_PERSON]);
	}
}

PurplePerson *
purple_contact_get_person(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	return priv->person;
}

PurpleContactPermission
purple_contact_get_permission(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact),
	                     PURPLE_CONTACT_PERMISSION_UNSET);

	priv = purple_contact_get_instance_private(contact);

	return priv->permission;
}

void
purple_contact_set_permission(PurpleContact *contact,
                              PurpleContactPermission permission)
{
	PurpleContactPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONTACT(contact));

	priv = purple_contact_get_instance_private(contact);

	priv->permission = permission;

	g_object_notify_by_pspec(G_OBJECT(contact), properties[PROP_PERMISSION]);
}

const char *
purple_contact_get_name_for_display(PurpleContact *contact) {
	PurpleContactPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	priv = purple_contact_get_instance_private(contact);

	/* If contact is associated with a PurplePerson that has an alias set,
	 * return the alias of that PurplePerson.
	 */
	if(priv->person != NULL) {
		const char *alias = purple_person_get_alias(priv->person);

		if(alias != NULL && alias[0] != '\0') {
			return alias;
		}
	}

	/* If the purple user set an alias for the contact, return that. */
	if(priv->alias != NULL && priv->alias[0] != '\0') {
		return priv->alias;
	}

	/* If the contact has a display name set, return that. */
	if(priv->display_name != NULL && priv->display_name[0] != '\0') {
		return priv->display_name;
	}

	/* Fallback to the username if that is set. */
	if(priv->username != NULL && priv->username[0] != '\0') {
		return priv->username;
	}

	/* Finally, in a last ditch effort, return the id of the contact. */
	return priv->id;
}

int
purple_contact_compare(PurpleContact *a, PurpleContact *b) {
	PurplePerson *person_a = NULL;
	PurplePerson *person_b = NULL;
	const char *name_a = NULL;
	const char *name_b = NULL;

	/* Check for NULL values. */
	if(a != NULL && b == NULL) {
		return -1;
	} else if(a == NULL && b != NULL) {
		return 1;
	} else if(a == NULL && b == NULL) {
		return 0;
	}

	/* Check if the contacts have persons associated with them. */
	person_a = purple_contact_get_person(a);
	person_b = purple_contact_get_person(b);

	if(person_a != NULL && person_b == NULL) {
		return -1;
	} else if(person_a == NULL && person_b != NULL) {
		return 1;
	}

	/* Finally get the names for the displaying and compare those. */
	name_a = purple_contact_get_name_for_display(a);
	name_b = purple_contact_get_name_for_display(b);

	return purple_utf8_strcasecmp(name_a, name_b);
}
