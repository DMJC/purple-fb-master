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

#include "buddylist.h"
#include "debug.h"
#include "notify.h"
#include "prefs.h"
#include "purpleaccount.h"
#include "purpleaccountmanager.h"
#include "purpleprivate.h"
#include "purpleprotocol.h"
#include "purpleprotocolclient.h"
#include "purpleconversation.h"
#include "signals.h"
#include "util.h"
#include "xmlnode.h"

/* Private data for a buddy list. */
typedef struct  {
	PurpleBlistNode *root;
	GHashTable *buddies;  /* Every buddy in this list */
} PurpleBuddyListPrivate;

static GType buddy_list_type = G_TYPE_INVALID;
static PurpleBuddyList *purplebuddylist = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(PurpleBuddyList, purple_buddy_list, G_TYPE_OBJECT);

/*
 * A hash table used for efficient lookups of buddies by name.
 * PurpleAccount* => GHashTable*, with the inner hash table being
 * struct _purple_hbuddy => PurpleBuddy*
 */
static GHashTable *buddies_cache = NULL;

/*
 * A hash table used for efficient lookups of groups by name.
 * UTF-8 collate-key => PurpleGroup*.
 */
static GHashTable *groups_cache = NULL;

static guint          save_timer = 0;
static gboolean       blist_loaded = FALSE;
static gchar *localized_default_group_name = NULL;

/*********************************************************************
 * Private utility functions                                         *
 *********************************************************************/

static gchar *
purple_blist_fold_name(const gchar *name)
{
	gchar *res, *tmp;

	if (name == NULL)
		return NULL;

	tmp = g_utf8_casefold(name, -1);
	res = g_utf8_collate_key(tmp, -1);
	g_free(tmp);

	return res;
}

static PurpleBlistNode *purple_blist_get_last_sibling(PurpleBlistNode *node)
{
	PurpleBlistNode *n = node;
	if (!n)
		return NULL;
	while (n->next)
		n = n->next;
	return n;
}

PurpleBlistNode *_purple_blist_get_last_child(PurpleBlistNode *node)
{
	if (!node)
		return NULL;
	return purple_blist_get_last_sibling(node->child);
}

struct _list_account_buddies {
	GSList *list;
	PurpleAccount *account;
};

struct _purple_hbuddy {
	char *name;
	PurpleAccount *account;
	PurpleBlistNode *group;
};

/* This function must not use purple_normalize */
static guint _purple_blist_hbuddy_hash(struct _purple_hbuddy *hb)
{
	return g_str_hash(hb->name) ^ g_direct_hash(hb->group) ^ g_direct_hash(hb->account);
}

/* This function must not use purple_normalize */
static guint _purple_blist_hbuddy_equal(struct _purple_hbuddy *hb1, struct _purple_hbuddy *hb2)
{
	return (hb1->group == hb2->group &&
	        hb1->account == hb2->account &&
	        purple_strequal(hb1->name, hb2->name));
}

static void _purple_blist_hbuddy_free_key(struct _purple_hbuddy *hb)
{
	g_free(hb->name);
	g_free(hb);
}

static void
purple_blist_buddies_cache_add_account(PurpleAccount *account)
{
	GHashTable *account_buddies = g_hash_table_new_full((GHashFunc)_purple_blist_hbuddy_hash,
						(GEqualFunc)_purple_blist_hbuddy_equal,
						(GDestroyNotify)_purple_blist_hbuddy_free_key, NULL);
	g_hash_table_insert(buddies_cache, account, account_buddies);
}

static void
purple_buddy_list_account_added_cb(G_GNUC_UNUSED PurpleAccountManager *manager,
                                   PurpleAccount *account,
                                   G_GNUC_UNUSED gpointer data)
{
	purple_blist_buddies_cache_add_account(account);
}

static void
purple_blist_buddies_cache_remove_account(const PurpleAccount *account)
{
	g_hash_table_remove(buddies_cache, account);
}

static void
purple_buddy_list_account_removed_cb(G_GNUC_UNUSED PurpleAccountManager *manager,
                                     PurpleAccount *account,
                                     G_GNUC_UNUSED gpointer data)
{
	purple_blist_buddies_cache_remove_account(account);
}

/*********************************************************************
 * Writing to disk                                                   *
 *********************************************************************/

static void
value_to_xmlnode(gpointer key, gpointer hvalue, gpointer user_data)
{
	const char *name;
	GValue *value;
	PurpleXmlNode *node, *child;
	char buf[21];

	name    = (const char *)key;
	value   = (GValue *)hvalue;
	node    = (PurpleXmlNode *)user_data;

	g_return_if_fail(value != NULL);

	child = purple_xmlnode_new_child(node, "setting");
	purple_xmlnode_set_attrib(child, "name", name);

	if (G_VALUE_HOLDS_INT(value)) {
		purple_xmlnode_set_attrib(child, "type", "int");
		g_snprintf(buf, sizeof(buf), "%d", g_value_get_int(value));
		purple_xmlnode_insert_data(child, buf, -1);
	}
	else if (G_VALUE_HOLDS_STRING(value)) {
		purple_xmlnode_set_attrib(child, "type", "string");
		purple_xmlnode_insert_data(child, g_value_get_string(value), -1);
	}
	else if (G_VALUE_HOLDS_BOOLEAN(value)) {
		purple_xmlnode_set_attrib(child, "type", "bool");
		g_snprintf(buf, sizeof(buf), "%d", g_value_get_boolean(value));
		purple_xmlnode_insert_data(child, buf, -1);
	}
}

static PurpleXmlNode *
contact_to_xmlnode(PurpleMetaContact *contact)
{
	PurpleXmlNode *node;
	gchar *alias;

	node = purple_xmlnode_new("contact");
	g_object_get(contact, "alias", &alias, NULL);

	if (alias != NULL)
	{
		purple_xmlnode_set_attrib(node, "alias", alias);
	}

	/* Write contact settings */
	g_hash_table_foreach(purple_blist_node_get_settings(PURPLE_BLIST_NODE(contact)),
			value_to_xmlnode, node);

	g_free(alias);
	return node;
}

static PurpleXmlNode *
group_to_xmlnode(PurpleGroup *group)
{
	PurpleXmlNode *node, *child;
	PurpleBlistNode *cnode;

	node = purple_xmlnode_new("group");
	if (group != purple_blist_get_default_group())
		purple_xmlnode_set_attrib(node, "name", purple_group_get_name(group));

	/* Write settings */
	g_hash_table_foreach(purple_blist_node_get_settings(PURPLE_BLIST_NODE(group)),
			value_to_xmlnode, node);

	/* Write contacts and chats */
	for (cnode = PURPLE_BLIST_NODE(group)->child; cnode != NULL; cnode = cnode->next)
	{
		if (purple_blist_node_is_transient(cnode))
			continue;
		if (PURPLE_IS_META_CONTACT(cnode))
		{
			child = contact_to_xmlnode(PURPLE_META_CONTACT(cnode));
			purple_xmlnode_insert_child(node, child);
		}
	}

	return node;
}

static PurpleXmlNode *
blist_to_xmlnode(void) {
	PurpleXmlNode *node, *child, *grandchild;
	PurpleBlistNode *gnode;
	const gchar *localized_default;

	node = purple_xmlnode_new("purple");
	purple_xmlnode_set_attrib(node, "version", "1.0");

	/* Write groups */
	child = purple_xmlnode_new_child(node, "blist");

	localized_default = localized_default_group_name;
	if (!purple_strequal(_("Buddies"), "Buddies"))
		localized_default = _("Buddies");
	if (localized_default != NULL) {
		purple_xmlnode_set_attrib(child,
			"localized-default-group", localized_default);
	}

	for (gnode = purple_blist_get_default_root(); gnode != NULL;
	     gnode = gnode->next) {
		if (purple_blist_node_is_transient(gnode))
			continue;
		if (PURPLE_IS_GROUP(gnode))
		{
			grandchild = group_to_xmlnode(PURPLE_GROUP(gnode));
			purple_xmlnode_insert_child(child, grandchild);
		}
	}

	return node;
}

static void
purple_blist_sync(void)
{
	PurpleXmlNode *node;
	char *data;

	if (!blist_loaded)
	{
		purple_debug_error("buddylist", "Attempted to save buddy list before it "
						 "was read!\n");
		return;
	}

	node = blist_to_xmlnode();
	data = purple_xmlnode_to_formatted_str(node, NULL);
	purple_util_write_data_to_config_file("blist.xml", data, -1);
	g_free(data);
	purple_xmlnode_free(node);
}

static gboolean
save_cb(G_GNUC_UNUSED gpointer data)
{
	purple_blist_sync();
	save_timer = 0;
	return FALSE;
}

static void
purple_blist_real_schedule_save(void)
{
	if (save_timer == 0)
		save_timer = g_timeout_add_seconds(5, save_cb, NULL);
}

static void
purple_blist_real_save_account(G_GNUC_UNUSED PurpleBuddyList *list,
                               G_GNUC_UNUSED PurpleAccount *account)
{
#if 1
	purple_blist_real_schedule_save();
#else
	if (account != NULL) {
		/* Save the buddies and privacy data for this account */
	} else {
		/* Save all buddies and privacy data */
	}
#endif
}

static void
purple_blist_real_save_node(G_GNUC_UNUSED PurpleBuddyList *list,
                            G_GNUC_UNUSED PurpleBlistNode *node)
{
	purple_blist_real_schedule_save();
}

void
purple_blist_schedule_save(void)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);

	/* Save everything */
	if (klass && klass->save_account) {
		klass->save_account(purplebuddylist, NULL);
	}
}

/*********************************************************************
 * Reading from disk                                                 *
 *********************************************************************/

static void
parse_setting(PurpleBlistNode *node, PurpleXmlNode *setting)
{
	const char *name = purple_xmlnode_get_attrib(setting, "name");
	const char *type = purple_xmlnode_get_attrib(setting, "type");
	char *value = purple_xmlnode_get_data(setting);

	if (!value)
		return;

	if (!type || purple_strequal(type, "string"))
		purple_blist_node_set_string(node, name, value);
	else if (purple_strequal(type, "bool"))
		purple_blist_node_set_bool(node, name, atoi(value));
	else if (purple_strequal(type, "int"))
		purple_blist_node_set_int(node, name, atoi(value));

	g_free(value);
}

static void
parse_contact(PurpleGroup *group, PurpleXmlNode *cnode)
{
	PurpleMetaContact *contact = purple_meta_contact_new();
	PurpleXmlNode *x;
	const char *alias;

	purple_blist_add_contact(contact, group,
			_purple_blist_get_last_child((PurpleBlistNode*)group));

	if ((alias = purple_xmlnode_get_attrib(cnode, "alias"))) {
		purple_meta_contact_set_alias(contact, alias);
	}

	for (x = cnode->child; x; x = x->next) {
		if (x->type != PURPLE_XMLNODE_TYPE_TAG)
			continue;
		if (purple_strequal(x->name, "setting"))
			parse_setting(PURPLE_BLIST_NODE(contact), x);
	}

	/* if the contact is empty, don't keep it around.  it causes problems */
	if (!PURPLE_BLIST_NODE(contact)->child)
		purple_blist_remove_contact(contact);
}

static void
parse_group(PurpleXmlNode *groupnode)
{
	const char *name = purple_xmlnode_get_attrib(groupnode, "name");
	PurpleGroup *group;
	PurpleXmlNode *cnode;

	group = purple_group_new(name);
	purple_blist_add_group(group, purple_blist_get_last_sibling(
	                                      purple_blist_get_default_root()));

	for (cnode = groupnode->child; cnode; cnode = cnode->next) {
		if (cnode->type != PURPLE_XMLNODE_TYPE_TAG)
			continue;
		if (purple_strequal(cnode->name, "setting"))
			parse_setting((PurpleBlistNode*)group, cnode);
		else if (purple_strequal(cnode->name, "contact") ||
				purple_strequal(cnode->name, "person"))
			parse_contact(group, cnode);
	}
}

static void
load_blist(void)
{
	PurpleXmlNode *purple, *blist;

	blist_loaded = TRUE;

	purple = purple_util_read_xml_from_config_file("blist.xml", _("buddy list"));

	if(purple == NULL) {
		return;
	}

	blist = purple_xmlnode_get_child(purple, "blist");
	if(blist) {
		PurpleXmlNode *groupnode;

		localized_default_group_name = g_strdup(
			purple_xmlnode_get_attrib(blist,
				"localized-default-group"));

		for(groupnode = purple_xmlnode_get_child(blist, "group"); groupnode != NULL;
				groupnode = purple_xmlnode_get_next_twin(groupnode)) {
			parse_group(groupnode);
		}
	} else {
		g_free(localized_default_group_name);
		localized_default_group_name = NULL;
	}

	purple_xmlnode_free(purple);
}

/*****************************************************************************
 * Public API functions                                                      *
 *****************************************************************************/

void
purple_blist_set_ui(GType type)
{
	g_return_if_fail(g_type_is_a(type, PURPLE_TYPE_BUDDY_LIST) ||
	                 type == G_TYPE_INVALID);
	buddy_list_type = type;
}

void
purple_blist_boot(void)
{
	GListModel *manager_model = NULL;
	PurpleBuddyList *gbl = g_object_new(buddy_list_type, NULL);
	guint n_items = 0;

	buddies_cache = g_hash_table_new_full(g_direct_hash, g_direct_equal,
					 NULL, (GDestroyNotify)g_hash_table_destroy);

	groups_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	manager_model = purple_account_manager_get_default_as_model();
	n_items = g_list_model_get_n_items(manager_model);
	for(guint index = 0; index < n_items; index++) {
		PurpleAccount *account = g_list_model_get_item(manager_model, index);
		purple_blist_buddies_cache_add_account(account);
		g_object_unref(account);
	}

	purplebuddylist = gbl;

	load_blist();
}

PurpleBuddyList *
purple_blist_get_default(void)
{
	return purplebuddylist;
}

PurpleBlistNode *
purple_blist_get_default_root(void)
{
	if (purplebuddylist) {
		PurpleBuddyListPrivate *priv =
		        purple_buddy_list_get_instance_private(purplebuddylist);
		return priv->root;
	}
	return NULL;
}

PurpleBlistNode *
purple_blist_get_root(PurpleBuddyList *list)
{
	PurpleBuddyListPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_BUDDY_LIST(list), NULL);
	priv = purple_buddy_list_get_instance_private(list);

	return priv->root;
}

static void
append_buddy(G_GNUC_UNUSED gpointer key, gpointer value, gpointer user_data)
{
	GSList **list = user_data;
	*list = g_slist_prepend(*list, value);
}

GSList *
purple_blist_get_buddies(void)
{
	PurpleBuddyListPrivate *priv;
	GSList *buddies = NULL;

	if (!purplebuddylist)
		return NULL;

	priv = purple_buddy_list_get_instance_private(purplebuddylist);
	g_hash_table_foreach(priv->buddies, append_buddy, &buddies);
	return buddies;
}

void
purple_blist_show(void)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));
	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);

	if (klass && klass->show) {
		klass->show(purplebuddylist);
	}
}

void purple_blist_set_visible(gboolean show)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));
	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);

	if (klass && klass->set_visible) {
		klass->set_visible(purplebuddylist, show);
	}
}

void purple_blist_update_groups_cache(PurpleGroup *group, const char *new_name)
{
		gchar* key;

		key = purple_blist_fold_name(purple_group_get_name(group));
		g_hash_table_remove(groups_cache, key);
		g_free(key);

		g_hash_table_insert(groups_cache,
			purple_blist_fold_name(new_name), group);
}

void purple_blist_add_contact(PurpleMetaContact *contact, PurpleGroup *group, PurpleBlistNode *node)
{
	PurpleBuddyListClass *klass = NULL;
	PurpleGroup *g;
	PurpleBlistNode *gnode, *cnode, *bnode;
	PurpleCountingNode *contact_counter, *group_counter;

	g_return_if_fail(PURPLE_IS_META_CONTACT(contact));
	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));

	if (PURPLE_BLIST_NODE(contact) == node)
		return;

	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);

	if (node && (PURPLE_IS_META_CONTACT(node)))
		g = PURPLE_GROUP(node->parent);
	else if (group)
		g = group;
	else
		g = purple_blist_get_default_group();

	gnode = (PurpleBlistNode*)g;
	cnode = (PurpleBlistNode*)contact;

	if (cnode->parent) {
		if (cnode->parent->child == cnode)
			cnode->parent->child = cnode->next;
		if (cnode->prev)
			cnode->prev->next = cnode->next;
		if (cnode->next)
			cnode->next->prev = cnode->prev;

		contact_counter = PURPLE_COUNTING_NODE(contact);
		group_counter = PURPLE_COUNTING_NODE(cnode->parent);

		if (purple_counting_node_get_online_count(contact_counter) > 0)
			purple_counting_node_change_online_count(group_counter, -1);
		if (purple_counting_node_get_current_size(contact_counter) > 0)
			purple_counting_node_change_current_size(group_counter, -1);
		purple_counting_node_change_total_size(group_counter, -1);

		if (klass && klass->remove) {
			klass->remove(purplebuddylist, cnode);
		}

		if (klass && klass->remove_node) {
			klass->remove_node(purplebuddylist, cnode);
		}
	}

	if (node && (PURPLE_IS_META_CONTACT(node))) {
		if (node->next)
			node->next->prev = cnode;
		cnode->next = node->next;
		cnode->prev = node;
		cnode->parent = node->parent;
		node->next = cnode;
	} else {
		if (gnode->child)
			gnode->child->prev = cnode;
		cnode->prev = NULL;
		cnode->next = gnode->child;
		gnode->child = cnode;
		cnode->parent = gnode;
	}

	contact_counter = PURPLE_COUNTING_NODE(contact);
	group_counter = PURPLE_COUNTING_NODE(g);

	if (purple_counting_node_get_online_count(contact_counter) > 0)
		purple_counting_node_change_online_count(group_counter, +1);
	if (purple_counting_node_get_current_size(contact_counter) > 0)
		purple_counting_node_change_current_size(group_counter, +1);
	purple_counting_node_change_total_size(group_counter, +1);

	if (klass && klass->save_node) {
		if (cnode->child) {
			klass->save_node(purplebuddylist, cnode);
		}
		for (bnode = cnode->child; bnode; bnode = bnode->next) {
			klass->save_node(purplebuddylist, bnode);
		}
	}

	if (klass && klass->update) {
		if (cnode->child) {
			klass->update(purplebuddylist, cnode);
		}

		for (bnode = cnode->child; bnode; bnode = bnode->next) {
			klass->update(purplebuddylist, bnode);
		}
	}
}

void purple_blist_add_group(PurpleGroup *group, PurpleBlistNode *node)
{
	PurpleBuddyListClass *klass = NULL;
	PurpleBuddyListPrivate *priv = NULL;
	PurpleBlistNode *gnode = (PurpleBlistNode*)group;
	gchar* key;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));
	g_return_if_fail(PURPLE_IS_GROUP(group));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);
	priv = purple_buddy_list_get_instance_private(purplebuddylist);

	/* if we're moving to overtop of ourselves, do nothing */
	if (gnode == node) {
		if (!priv->root) {
			node = NULL;
		} else {
			return;
		}
	}

	if (purple_blist_find_group(purple_group_get_name(group))) {
		/* This is just being moved */

		if (klass && klass->remove) {
			klass->remove(purplebuddylist,
			              (PurpleBlistNode *)group);
		}

		if (gnode == priv->root) {
			priv->root = gnode->next;
		}
		if (gnode->prev)
			gnode->prev->next = gnode->next;
		if (gnode->next)
			gnode->next->prev = gnode->prev;
	} else {
		key = purple_blist_fold_name(purple_group_get_name(group));
		g_hash_table_insert(groups_cache, key, group);
	}

	if (node && PURPLE_IS_GROUP(node)) {
		gnode->next = node->next;
		gnode->prev = node;
		if (node->next)
			node->next->prev = gnode;
		node->next = gnode;
	} else {
		if (priv->root) {
			priv->root->prev = gnode;
		}
		gnode->next = priv->root;
		gnode->prev = NULL;
		priv->root = gnode;
	}

	if (klass && klass->save_node) {
		klass->save_node(purplebuddylist, gnode);
		for (node = gnode->child; node; node = node->next) {
			klass->save_node(purplebuddylist, node);
		}
	}

	if (klass && klass->update) {
		klass->update(purplebuddylist, gnode);
		for (node = gnode->child; node; node = node->next) {
			klass->update(purplebuddylist, node);
		}
	}

	purple_signal_emit(purple_blist_get_handle(), "blist-node-added",
			gnode);
}

void purple_blist_remove_contact(PurpleMetaContact *contact)
{
	PurpleBuddyListClass *klass = NULL;
	PurpleBlistNode *node, *gnode;
	PurpleGroup *group;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));
	g_return_if_fail(PURPLE_IS_META_CONTACT(contact));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);
	node = (PurpleBlistNode *)contact;
	gnode = node->parent;
	group = PURPLE_GROUP(gnode);

	/* Remove the node from its parent */
	if (gnode->child == node)
		gnode->child = node->next;
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	purple_counting_node_change_total_size(PURPLE_COUNTING_NODE(group), -1);

	/* Update the UI */
	if (klass && klass->remove) {
		klass->remove(purplebuddylist, node);
	}

	if (klass && klass->remove_node) {
		klass->remove_node(purplebuddylist, node);
	}

	purple_signal_emit(purple_blist_get_handle(), "blist-node-removed",
			PURPLE_BLIST_NODE(contact));

	/* Delete the node */
	g_object_unref(contact);
}

void purple_blist_remove_group(PurpleGroup *group)
{
	PurpleAccountManager *manager = NULL;
	PurpleBuddyListClass *klass = NULL;
	PurpleBuddyListPrivate *priv = NULL;
	PurpleBlistNode *node;
	GList *accounts = NULL;
	gchar* key;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));
	g_return_if_fail(PURPLE_IS_GROUP(group));

	if (group == purple_blist_get_default_group())
		purple_debug_warning("buddylist", "cannot remove default group");

	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);
	priv = purple_buddy_list_get_instance_private(purplebuddylist);
	node = (PurpleBlistNode *)group;

	/* Make sure the group is empty */
	if (node->child)
		return;

	/* Remove the node from its parent */
	if (priv->root == node) {
		priv->root = node->next;
	}
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;

	key = purple_blist_fold_name(purple_group_get_name(group));
	g_hash_table_remove(groups_cache, key);
	g_free(key);

	/* Update the UI */
	if (klass && klass->remove) {
		klass->remove(purplebuddylist, node);
	}

	if (klass && klass->remove_node) {
		klass->remove_node(purplebuddylist, node);
	}

	purple_signal_emit(purple_blist_get_handle(), "blist-node-removed",
			PURPLE_BLIST_NODE(group));

	/* Remove the group from all accounts that are online */
	manager = purple_account_manager_get_default();
	accounts = purple_account_manager_get_connected(manager);
	while(accounts != NULL) {
		purple_account_remove_group(accounts->data, group);

		accounts = g_list_delete_link(accounts, accounts);
	}

	/* Delete the node */
	g_object_unref(group);
}

PurpleGroup *purple_blist_find_group(const char *name)
{
	gchar* key;
	PurpleGroup *group;

	g_return_val_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist), NULL);

	if (name == NULL || name[0] == '\0')
		name = PURPLE_BLIST_DEFAULT_GROUP_NAME;
	if (purple_strequal(name, "Buddies"))
		name = PURPLE_BLIST_DEFAULT_GROUP_NAME;
	if (purple_strequal(name, localized_default_group_name))
		name = PURPLE_BLIST_DEFAULT_GROUP_NAME;

	key = purple_blist_fold_name(name);
	group = g_hash_table_lookup(groups_cache, key);
	g_free(key);

	return group;
}

PurpleGroup *
purple_blist_get_default_group(void)
{
	PurpleGroup *group;

	group = purple_blist_find_group(PURPLE_BLIST_DEFAULT_GROUP_NAME);
	if (!group) {
		group = purple_group_new(PURPLE_BLIST_DEFAULT_GROUP_NAME);
		purple_blist_add_group(group, NULL);
	}

	return group;
}

void
purple_blist_walk(PurpleBlistWalkFunc group_func,
                  PurpleBlistWalkFunc meta_contact_func,
                  PurpleBlistWalkFunc contact_func,
                  gpointer data)
{
	PurpleBlistNode *group = NULL, *meta_contact = NULL, *contact = NULL;

	for (group = purple_blist_get_default_root(); group != NULL;
	     group = group->next) {
		if(group_func != NULL) {
			group_func(group, data);
		}

		for(meta_contact = group->child; meta_contact != NULL; meta_contact = meta_contact->next) {
			if(PURPLE_IS_META_CONTACT(meta_contact)) {
				if(meta_contact_func != NULL) {
					meta_contact_func(meta_contact, data);
				}

				if(contact_func != NULL) {
					for(contact = meta_contact->child; contact != NULL; contact = contact->next) {
						contact_func(contact, data);
					}
				}
			}
		}
	}
}

const gchar *
purple_blist_get_default_group_name(void) {
	return _("Buddies");
}


void
purple_blist_request_add_buddy(PurpleAccount *account, const char *username,
							 const char *group, const char *alias)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);
	if (klass != NULL && klass->request_add_buddy != NULL) {
		klass->request_add_buddy(purplebuddylist, account, username,
		                         group, alias);
	}
}

void
purple_blist_request_add_group(void)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(purplebuddylist));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(purplebuddylist);
	if (klass != NULL && klass->request_add_group != NULL) {
		klass->request_add_group(purplebuddylist);
	}
}

void
purple_blist_new_node(PurpleBuddyList *list, PurpleBlistNode *node)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(list));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(list);
	if (klass && klass->new_node) {
		klass->new_node(list, node);
	}
}

void
purple_blist_update_node(PurpleBuddyList *list, PurpleBlistNode *node)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(list));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(list);
	if (klass && klass->update) {
		klass->update(list, node);
	}
}

void
purple_blist_save_node(PurpleBuddyList *list, PurpleBlistNode *node)
{
	PurpleBuddyListClass *klass = NULL;

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(list));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(list);
	if (klass && klass->save_node) {
		klass->save_node(list, node);
	}
}

void
purple_blist_save_account(PurpleBuddyList *list, PurpleAccount *account)
{
	PurpleBuddyListClass *klass = NULL;

	/* XXX: There's a chicken and egg problem with the accounts api, where
	 * it'll call this function before purple_blist_init is called, this will
	 * cause the following g_return_if_fail to fail, and muck up the logs.  We
	 * need to find a better fix for this, but this gets rid of it for now.
	 */
	if(G_UNLIKELY(list == NULL && purplebuddylist == NULL)) {
		return;
	}

	g_return_if_fail(PURPLE_IS_BUDDY_LIST(list));

	klass = PURPLE_BUDDY_LIST_GET_CLASS(list);
	if (klass && klass->save_account) {
		klass->save_account(list, account);
	}
}

const gchar *
_purple_blist_get_localized_default_group_name(void)
{
	return localized_default_group_name;
}

void *
purple_blist_get_handle(void)
{
	static int handle;

	return &handle;
}

void
purple_blist_init(void)
{
	void *handle = purple_blist_get_handle();

	/* Set a default, which can't be done as a static initializer. */
	buddy_list_type = PURPLE_TYPE_BUDDY_LIST;

	purple_signal_register(handle, "blist-node-added",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 PURPLE_TYPE_BLIST_NODE);

	purple_signal_register(handle, "blist-node-removed",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 PURPLE_TYPE_BLIST_NODE);

	purple_signal_register(handle, "update-idle", purple_marshal_VOID,
						 G_TYPE_NONE, 0);

	purple_signal_register(handle, "blist-node-extended-menu",
			     purple_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2,
			     PURPLE_TYPE_BLIST_NODE,
			     G_TYPE_POINTER); /* (GList **) */

	purple_signal_register(handle, "blist-node-aliased",
						 purple_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2,
						 PURPLE_TYPE_BLIST_NODE, G_TYPE_STRING);
}

static void
blist_node_destroy(PurpleBuddyListClass *klass, PurpleBuddyList *list,
                   PurpleBlistNode *node)
{
	PurpleBlistNode *child, *next_child;

	child = node->child;
	while (child) {
		next_child = child->next;
		blist_node_destroy(klass, list, child);
		child = next_child;
	}

	/* Allow the UI to free data */
	node->parent = NULL;
	node->child  = NULL;
	node->next   = NULL;
	node->prev   = NULL;
	if (klass && klass->remove) {
		klass->remove(list, node);
	}

	g_object_unref(node);
}

void
purple_blist_uninit(void)
{
	/* This happens if we quit before purple_set_blist is called. */
	if (purplebuddylist == NULL)
		return;

	if(save_timer != 0) {
		g_clear_handle_id(&save_timer, g_source_remove);
		purple_blist_sync();
	}

	purple_debug_info("buddylist", "Destroying");

	g_clear_pointer(&buddies_cache, g_hash_table_destroy);
	g_clear_pointer(&groups_cache, g_hash_table_destroy);

	g_clear_object(&purplebuddylist);

	g_clear_pointer(&localized_default_group_name, g_free);

	purple_signals_disconnect_by_handle(purple_blist_get_handle());
	purple_signals_unregister_by_instance(purple_blist_get_handle());
}

/**************************************************************************
 * GObject code
 **************************************************************************/

/* GObject initialization function */
static void
purple_buddy_list_init(PurpleBuddyList *blist)
{
	PurpleBuddyListPrivate *priv = NULL;
	PurpleAccountManager *manager = NULL;

	priv = purple_buddy_list_get_instance_private(blist);

	priv->buddies = g_hash_table_new_full(
					 (GHashFunc)_purple_blist_hbuddy_hash,
					 (GEqualFunc)_purple_blist_hbuddy_equal,
					 (GDestroyNotify)_purple_blist_hbuddy_free_key, NULL);

	manager = purple_account_manager_get_default();
	g_signal_connect_object(manager, "added",
	                        G_CALLBACK(purple_buddy_list_account_added_cb),
	                        blist, 0);
	g_signal_connect_object(manager, "removed",
	                        G_CALLBACK(purple_buddy_list_account_removed_cb),
	                        blist, 0);
}

/* GObject finalize function */
static void
purple_buddy_list_finalize(GObject *object)
{
	PurpleBuddyList *list = PURPLE_BUDDY_LIST(object);
	PurpleBuddyListClass *klass = PURPLE_BUDDY_LIST_GET_CLASS(list);
	PurpleBuddyListPrivate *priv =
	        purple_buddy_list_get_instance_private(list);
	PurpleBlistNode *node, *next_node;

	g_hash_table_destroy(priv->buddies);

	node = priv->root;
	while (node) {
		next_node = node->next;
		blist_node_destroy(klass, list, node);
		node = next_node;
	}
	priv->root = NULL;

	G_OBJECT_CLASS(purple_buddy_list_parent_class)->finalize(object);
}

/* Class initializer function */
static void purple_buddy_list_class_init(PurpleBuddyListClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_buddy_list_finalize;

	klass->save_node = purple_blist_real_save_node;
	klass->remove_node = purple_blist_real_save_node;
	klass->save_account = purple_blist_real_save_account;
}
