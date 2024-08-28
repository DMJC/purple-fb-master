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

#include "purpleaddcontactrequest.h"

struct _PurpleAddContactRequest {
	GObject parent;

	PurpleContact *contact;
	char *message;

	/* This tracks whether add has been called. */
	gboolean handled;
};

enum {
	SIG_ADD,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

enum {
	PROP_0,
	PROP_CONTACT,
	PROP_MESSAGE,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_add_contact_request_set_contact(PurpleAddContactRequest *request,
                                       PurpleContact *contact)
{
	g_return_if_fail(PURPLE_IS_ADD_CONTACT_REQUEST(request));

	if(g_set_object(&request->contact, contact)) {
		g_object_notify_by_pspec(G_OBJECT(request), properties[PROP_CONTACT]);
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleAddContactRequest, purple_add_contact_request,
                    G_TYPE_OBJECT)

static void
purple_add_contact_request_get_property(GObject *obj, guint param_id,
                                        GValue *value, GParamSpec *pspec)
{
	PurpleAddContactRequest *request = PURPLE_ADD_CONTACT_REQUEST(obj);

	switch(param_id) {
	case PROP_CONTACT:
		g_value_set_object(value,
		                   purple_add_contact_request_get_contact(request));
		break;
	case PROP_MESSAGE:
		g_value_set_string(value,
		                   purple_add_contact_request_get_message(request));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_add_contact_request_set_property(GObject *obj, guint param_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
	PurpleAddContactRequest *request = PURPLE_ADD_CONTACT_REQUEST(obj);

	switch(param_id) {
	case PROP_CONTACT:
		purple_add_contact_request_set_contact(request,
		                                       g_value_get_object(value));
		break;
	case PROP_MESSAGE:
		purple_add_contact_request_set_message(request,
		                                       g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_add_contact_request_dispose(GObject *obj) {
	PurpleAddContactRequest *request = PURPLE_ADD_CONTACT_REQUEST(obj);

	g_clear_object(&request->contact);

	G_OBJECT_CLASS(purple_add_contact_request_parent_class)->dispose(obj);
}

static void
purple_add_contact_request_finalize(GObject *obj) {
	PurpleAddContactRequest *request = PURPLE_ADD_CONTACT_REQUEST(obj);

	g_clear_pointer(&request->message, g_free);

	G_OBJECT_CLASS(purple_add_contact_request_parent_class)->finalize(obj);
}

static void
purple_add_contact_request_init(G_GNUC_UNUSED PurpleAddContactRequest *request)
{
}

static void
purple_add_contact_request_class_init(PurpleAddContactRequestClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_add_contact_request_get_property;
	obj_class->set_property = purple_add_contact_request_set_property;
	obj_class->dispose = purple_add_contact_request_dispose;
	obj_class->finalize = purple_add_contact_request_finalize;

	/**
	 * PurpleAddContactRequest:contact:
	 *
	 * The contact that this request is for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONTACT] = g_param_spec_object(
		"contact", NULL, NULL,
		PURPLE_TYPE_CONTACT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleAddContactRequest:message:
	 *
	 * The optional message sent from the remote user.
	 *
	 * Since: 3.0
	 */
	properties[PROP_MESSAGE] = g_param_spec_string(
		"message", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleAddContactRequest::add:
	 * @request: The [class@AddContactRequest] instance.
	 *
	 * Emitted when the user has told the ui to add the contact. This is
	 * typically emitted by the user interface calling
	 * [method@AddContactRequest.add].
	 *
	 * Since: 3.0
	 */
	signals[SIG_ADD] = g_signal_new_class_handler(
		"add",
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
PurpleAddContactRequest *
purple_add_contact_request_new(PurpleContact *contact) {
	g_return_val_if_fail(PURPLE_IS_CONTACT(contact), NULL);

	return g_object_new(
		PURPLE_TYPE_ADD_CONTACT_REQUEST,
		"contact", contact,
		NULL);
}

PurpleContact *
purple_add_contact_request_get_contact(PurpleAddContactRequest *request) {
	g_return_val_if_fail(PURPLE_IS_ADD_CONTACT_REQUEST(request), NULL);

	return request->contact;
}

void
purple_add_contact_request_set_message(PurpleAddContactRequest *request,
                                       const char *message)
{
	g_return_if_fail(PURPLE_IS_ADD_CONTACT_REQUEST(request));

	if(g_set_str(&request->message, message)) {
		g_object_notify_by_pspec(G_OBJECT(request), properties[PROP_MESSAGE]);
	}
}

const char *
purple_add_contact_request_get_message(PurpleAddContactRequest *request) {
	g_return_val_if_fail(PURPLE_IS_ADD_CONTACT_REQUEST(request), NULL);

	return request->message;
}

void
purple_add_contact_request_add(PurpleAddContactRequest *request) {
	g_return_if_fail(PURPLE_IS_ADD_CONTACT_REQUEST(request));

	/* Calling this multiple times is a programming error. */
	g_return_if_fail(request->handled == FALSE);

	request->handled = TRUE;

	g_signal_emit(request, signals[SIG_ADD], 0);
}
