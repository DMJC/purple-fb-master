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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_PROTOCOL_IM_H
#define PURPLE_PROTOCOL_IM_H

#include <glib.h>
#include <glib-object.h>

#include "purpleprotocol.h"
#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_IM (purple_protocol_im_get_type())

/**
 * PurpleProtocolIM:
 *
 * #PurpleProtocolIM describes the API that protocols need to implement for
 * handling one on one conversations.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolIM, purple_protocol_im, PURPLE, PROTOCOL_IM,
                    PurpleProtocol)

#include "connection.h"
#include "purpleconversation.h"
#include "purpleimconversation.h"

struct _PurpleProtocolIMInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	gint (*send)(PurpleProtocolIM *im, PurpleConnection *connection,
	             PurpleConversation *conversation, PurpleMessage *msg);

	guint (*send_typing)(PurpleProtocolIM *im, PurpleConnection *connection,
	                     const gchar *name, PurpleIMTypingState state);

	/*< private >*/
	gpointer reserved[4];
};

G_BEGIN_DECLS

/**
 * purple_protocol_im_send:
 * @im: The #PurpleProtocolIM instance.
 * @connection: The #PurpleConnection to use.
 * @conversation: The [class@Conversation] this message is for.
 * @msg: The #PurpleMessage to send.
 *
 * Sends @msg out over @connection.
 *
 * Returns: >= 0 on success, or < 0 on error.  If 0 is returned the message
 *          should not be output locally.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gint purple_protocol_im_send(PurpleProtocolIM *im,
                             PurpleConnection *connection,
                             PurpleConversation *conversation,
                             PurpleMessage *msg);

/**
 * purple_protocol_im_send_typing:
 * @im: The #PurpleProtocolIM instance.
 * @connection: The #PurpleConversation to use.
 * @name: The name of the user to send a typing notification to.
 * @state: The #PurpleIMTypingState Value.
 *
 * If this protocol requires the #PURPLE_IM_TYPING message to be sent
 * repeatedly to signify that the user is still typing, then the protocol
 * should return the number of seconds to wait before sending a subsequent
 * notification. Otherwise the protocol should return 0.
 *
 * Returns: A quiet-period, specified in seconds, where Purple will not send
 *          any additional typing notification messages.  Most protocols should
 *          return 0, which means that no additional #PURPLE_IM_TYPING messages
 *          need to be sent.  If this is 5, for example, then Purple will wait
 *          five seconds, and if the Purple user is still typing then Purple
 *          will send another #PURPLE_IM_TYPING message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
guint purple_protocol_im_send_typing(PurpleProtocolIM *im,
                                     PurpleConnection *connection,
                                     const gchar *name,
                                     PurpleIMTypingState state);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_IM_H */

