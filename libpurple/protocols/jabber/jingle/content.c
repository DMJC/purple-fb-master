/**
 * @file content.c
 *
 * purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
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
 */

#include "internal.h"
#include <purple.h>

#include "content.h"
#include "jingle.h"

#include <string.h>

typedef struct
{
	JingleSession *session;
	gchar *description_type;
	gchar *creator;
	gchar *disposition;
	gchar *name;
	gchar *senders;
	JingleTransport *transport;
	JingleTransport *pending_transport;
} JingleContentPrivate;

enum {
	PROP_0,
	PROP_SESSION,
	PROP_CREATOR,
	PROP_DISPOSITION,
	PROP_NAME,
	PROP_SENDERS,
	PROP_TRANSPORT,
	PROP_PENDING_TRANSPORT,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
	JingleContent,
	jingle_content,
	G_TYPE_OBJECT,
	0,
	G_ADD_PRIVATE_DYNAMIC(JingleContent)
);

/******************************************************************************
 * JingleContent Implementation
 *****************************************************************************/
static JingleContent *
jingle_content_parse_internal(PurpleXmlNode *content)
{
	PurpleXmlNode *description = purple_xmlnode_get_child(content, "description");
	const gchar *type = purple_xmlnode_get_namespace(description);
	const gchar *creator = purple_xmlnode_get_attrib(content, "creator");
	const gchar *disposition = purple_xmlnode_get_attrib(content, "disposition");
	const gchar *senders = purple_xmlnode_get_attrib(content, "senders");
	const gchar *name = purple_xmlnode_get_attrib(content, "name");
	JingleTransport *transport =
			jingle_transport_parse(purple_xmlnode_get_child(content, "transport"));
	if (transport == NULL)
		return NULL;

	if (senders == NULL)
		senders = "both";

	return jingle_content_create(type, creator, disposition, name, senders, transport);
}

static PurpleXmlNode *
jingle_content_to_xml_internal(JingleContent *content, PurpleXmlNode *jingle, JingleActionType action)
{
	PurpleXmlNode *node = purple_xmlnode_new_child(jingle, "content");
	gchar *creator = jingle_content_get_creator(content);
	gchar *name = jingle_content_get_name(content);
	gchar *senders = jingle_content_get_senders(content);
	gchar *disposition = jingle_content_get_disposition(content);

	purple_xmlnode_set_attrib(node, "creator", creator);
	purple_xmlnode_set_attrib(node, "name", name);
	purple_xmlnode_set_attrib(node, "senders", senders);
	if (!purple_strequal("session", disposition))
		purple_xmlnode_set_attrib(node, "disposition", disposition);

	g_free(disposition);
	g_free(senders);
	g_free(name);
	g_free(creator);

	if (action != JINGLE_CONTENT_REMOVE) {
		JingleTransport *transport;

		if (action != JINGLE_TRANSPORT_ACCEPT &&
				action != JINGLE_TRANSPORT_INFO &&
				action != JINGLE_TRANSPORT_REJECT &&
				action != JINGLE_TRANSPORT_REPLACE) {
			PurpleXmlNode *description = purple_xmlnode_new_child(node, "description");

			purple_xmlnode_set_namespace(description,
					jingle_content_get_description_type(content));
		}

		if (action != JINGLE_TRANSPORT_REJECT && action == JINGLE_TRANSPORT_REPLACE)
			transport = jingle_content_get_pending_transport(content);
		else
			transport = jingle_content_get_transport(content);

		jingle_transport_to_xml(transport, node, action);
		g_object_unref(transport);
	}

	return node;
}

/******************************************************************************
 * GObject Stuff
 *****************************************************************************/
static void
jingle_content_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	JingleContent *content = JINGLE_CONTENT(object);
	JingleContentPrivate *priv = jingle_content_get_instance_private(content);

	switch (prop_id) {
		case PROP_SESSION:
			priv->session = g_value_get_object(value);
			break;
		case PROP_CREATOR:
			g_free(priv->creator);
			priv->creator = g_value_dup_string(value);
			break;
		case PROP_DISPOSITION:
			g_free(priv->disposition);
			priv->disposition = g_value_dup_string(value);
			break;
		case PROP_NAME:
			g_free(priv->name);
			priv->name = g_value_dup_string(value);
			break;
		case PROP_SENDERS:
			g_free(priv->senders);
			priv->senders = g_value_dup_string(value);
			break;
		case PROP_TRANSPORT:
			if (priv->transport)
				g_object_unref(priv->transport);
			priv->transport = g_value_get_object(value);
			break;
		case PROP_PENDING_TRANSPORT:
			if (priv->pending_transport)
				g_object_unref(priv->pending_transport);
			priv->pending_transport = g_value_get_object(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
jingle_content_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	JingleContent *content = JINGLE_CONTENT(object);
	JingleContentPrivate *priv = jingle_content_get_instance_private(content);

	switch (prop_id) {
		case PROP_SESSION:
			g_value_set_object(value, priv->session);
			break;
		case PROP_CREATOR:
			g_value_set_string(value, priv->creator);
			break;
		case PROP_DISPOSITION:
			g_value_set_string(value, priv->disposition);
			break;
		case PROP_NAME:
			g_value_set_string(value, priv->name);
			break;
		case PROP_SENDERS:
			g_value_set_string(value, priv->senders);
			break;
		case PROP_TRANSPORT:
			g_value_set_object(value, priv->transport);
			break;
		case PROP_PENDING_TRANSPORT:
			g_value_set_object(value, priv->pending_transport);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
jingle_content_init (JingleContent *content)
{
}

static void
jingle_content_finalize (GObject *content)
{
	JingleContentPrivate *priv = jingle_content_get_instance_private(JINGLE_CONTENT(content));

	purple_debug_info("jingle","jingle_content_finalize\n");

	g_free(priv->description_type);
	g_free(priv->creator);
	g_free(priv->disposition);
	g_free(priv->name);
	g_free(priv->senders);
	g_object_unref(priv->transport);
	if (priv->pending_transport)
		g_object_unref(priv->pending_transport);

	G_OBJECT_CLASS(jingle_content_parent_class)->finalize(content);
}


static void
jingle_content_class_finalize (JingleContentClass *klass)
{
}

static void
jingle_content_class_init (JingleContentClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = jingle_content_finalize;
	obj_class->set_property = jingle_content_set_property;
	obj_class->get_property = jingle_content_get_property;

	klass->to_xml = jingle_content_to_xml_internal;
	klass->parse = jingle_content_parse_internal;

	properties[PROP_SESSION] = g_param_spec_object("session",
			"Jingle Session",
			"The jingle session parent of this content.",
			JINGLE_TYPE_SESSION,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_CREATOR] = g_param_spec_string("creator",
			"Creator",
			"The participant that created this content.",
			NULL,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_DISPOSITION] = g_param_spec_string("disposition",
			"Disposition",
			"The disposition of the content.",
			NULL,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_NAME] = g_param_spec_string("name",
			"Name",
			"The name of this content.",
			NULL,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_SENDERS] = g_param_spec_string("senders",
			"Senders",
			"The sender of this content.",
			NULL,
			G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_TRANSPORT] = g_param_spec_object("transport",
			"transport",
			"The transport of this content.",
			JINGLE_TYPE_TRANSPORT,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_PENDING_TRANSPORT] = g_param_spec_object("pending-transport",
			"Pending transport",
			"The pending transport contained within this content",
			JINGLE_TYPE_TRANSPORT,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, PROP_LAST, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
void
jingle_content_register(PurplePlugin *plugin)
{
	jingle_content_register_type(G_TYPE_MODULE(plugin));
}

JingleContent *
jingle_content_create(const gchar *type, const gchar *creator,
		const gchar *disposition, const gchar *name,
		const gchar *senders, JingleTransport *transport)
{


	JingleContent *content = g_object_new(jingle_get_type(type),
			"creator", creator,
			"disposition", disposition != NULL ? disposition : "session",
			"name", name,
			"senders", senders != NULL ? senders : "both",
			"transport", transport,
			NULL);
	return content;
}

JingleSession *jingle_content_get_session(JingleContent *content)
{
	JingleSession *session;
	g_object_get(content, "session", &session, NULL);
	return session;
}

const gchar *
jingle_content_get_description_type(JingleContent *content)
{
	return JINGLE_CONTENT_GET_CLASS(content)->description_type;
}

gchar *
jingle_content_get_creator(JingleContent *content)
{
	gchar *creator;
	g_object_get(content, "creator", &creator, NULL);
	return creator;
}

gchar *
jingle_content_get_disposition(JingleContent *content)
{
	gchar *disposition;
	g_object_get(content, "disposition", &disposition, NULL);
	return disposition;
}

gchar *
jingle_content_get_name(JingleContent *content)
{
	gchar *name;
	g_object_get(content, "name", &name, NULL);
	return name;
}

gchar *
jingle_content_get_senders(JingleContent *content)
{
	gchar *senders;
	g_object_get(content, "senders", &senders, NULL);
	return senders;
}

JingleTransport *
jingle_content_get_transport(JingleContent *content)
{
	JingleTransport *transport;
	g_object_get(content, "transport", &transport, NULL);
	return transport;
}

JingleTransport *
jingle_content_get_pending_transport(JingleContent *content)
{
	JingleTransport *pending_transport;
	g_object_get(content, "pending-transport", &pending_transport, NULL);
	return pending_transport;
}

void
jingle_content_set_session(JingleContent *content, JingleSession *session)
{
	g_return_if_fail(JINGLE_IS_CONTENT(content));
	g_return_if_fail(JINGLE_IS_SESSION(session));
	g_object_set(content, "session", session, NULL);
}

void
jingle_content_set_pending_transport(JingleContent *content, JingleTransport *transport)
{
	g_object_set(content, "pending-transport", transport, NULL);
}

void
jingle_content_accept_transport(JingleContent *content)
{
	JingleContentPrivate *priv = NULL;
	GObject *obj;

	g_return_if_fail(JINGLE_IS_CONTENT(content));

	priv = jingle_content_get_instance_private(content);

	if (priv->transport)
		g_object_unref(priv->transport);

	priv->transport = priv->pending_transport;
	priv->pending_transport = NULL;

	obj = G_OBJECT(content);
	g_object_freeze_notify(obj);
	g_object_notify_by_pspec(obj, properties[PROP_TRANSPORT]);
	g_object_notify_by_pspec(obj, properties[PROP_PENDING_TRANSPORT]);
	g_object_thaw_notify(obj);
}

void
jingle_content_remove_pending_transport(JingleContent *content)
{
	JingleContentPrivate *priv = NULL;

	g_return_if_fail(JINGLE_IS_CONTENT(content));

	priv = jingle_content_get_instance_private(content);

	if (priv->pending_transport) {
		g_object_unref(priv->pending_transport);
		priv->pending_transport = NULL;
	}

	g_object_notify_by_pspec(G_OBJECT(content), properties[PROP_PENDING_TRANSPORT]);
}

void
jingle_content_modify(JingleContent *content, const gchar *senders)
{
	g_object_set(content, "senders", senders, NULL);
}

JingleContent *
jingle_content_parse(PurpleXmlNode *content)
{
	const gchar *type = purple_xmlnode_get_namespace(purple_xmlnode_get_child(content, "description"));
	GType jingle_type = jingle_get_type(type);

	if (jingle_type != G_TYPE_NONE) {
		return JINGLE_CONTENT_CLASS(g_type_class_ref(jingle_type))->parse(content);
	} else {
		return NULL;
	}
}

PurpleXmlNode *
jingle_content_to_xml(JingleContent *content, PurpleXmlNode *jingle, JingleActionType action)
{
	g_return_val_if_fail(content != NULL, NULL);
	g_return_val_if_fail(JINGLE_IS_CONTENT(content), NULL);
	return JINGLE_CONTENT_GET_CLASS(content)->to_xml(content, jingle, action);
}

void
jingle_content_handle_action(JingleContent *content, PurpleXmlNode *xmlcontent, JingleActionType action)
{
	g_return_if_fail(content != NULL);
	g_return_if_fail(JINGLE_IS_CONTENT(content));
	JINGLE_CONTENT_GET_CLASS(content)->handle_action(content, xmlcontent, action);
}

