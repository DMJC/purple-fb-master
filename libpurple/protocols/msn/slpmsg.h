/**
 * @file slpmsg.h SLP Message functions
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
#ifndef _MSN_SLPMSG_H_
#define _MSN_SLPMSG_H_

typedef struct _MsnSlpMessage MsnSlpMessage;

#include "imgstore.h"

#include "slpcall.h"
#include "slplink.h"
#include "session.h"
#include "msg.h"

#include "slp.h"

/**
 * A SLP Message  This contains everything that we will need to send a SLP
 * Message even if has to be sent in several parts.
 */
struct _MsnSlpMessage
{
	MsnSlpCall *slpcall; /**< The slpcall to which this slp message belongs (if applicable). */
	MsnSlpLink *slplink; /**< The slplink through which this slp message is being sent. */
	MsnSession *session;

	MsnP2PHeader *header;
	MsnP2PFooter *footer;

	long session_id;
	long id;
	long ack_id;
	long ack_sub_id;
	long long ack_size;

	gboolean sip; /**< A flag that states if this is a SIP slp message. */
	long flags;

	gboolean ft;
	PurpleStoredImage *img;
	guchar *buffer;

	/**
	 * For outgoing messages this is the number of bytes from buffer that
	 * have already been sent out.  For incoming messages this is the
	 * number of bytes that have been written to buffer.
	 */
	long long offset;

	/**
	 * This is the size of buffer, unless this is an outgoing file transfer,
	 * in which case this is the size of the file.
	 */
	long long size;

	GList *msgs; /**< The real messages. */

#if 1
	MsnMessage *msg; /**< The temporary real message that will be sent. */
#endif

	const char *info;
	gboolean text_body;
};

/**
 * Creates a new slp message
 *
 * @param slplink The slplink through which this slp message will be sent.
 * If it's set to NULL, it is a temporary SlpMessage.
 * @return The created slp message.
 */
MsnSlpMessage *msn_slpmsg_new(MsnSlpLink *slplink);

/**
 * Creates a MsnSlpMessage without a MsnSlpLink by parsing the raw data.
 *
 * @param data 		The raw data with the slp message.
 * @param data_len 	The len of the data
 *
 * @return The createed slp message.
 */
MsnSlpMessage *msn_slpmsg_new_from_data(const char *data, size_t data_len);

/**
 * Destroys a slp message
 *
 * @param slpmsg The slp message to destory.
 */
void msn_slpmsg_destroy(MsnSlpMessage *slpmsg);

void msn_slpmsg_set_body(MsnSlpMessage *slpmsg, const char *body,
						 long long size);
void msn_slpmsg_set_image(MsnSlpMessage *slpmsg, PurpleStoredImage *img);
void msn_slpmsg_open_file(MsnSlpMessage *slpmsg,
						  const char *file_name);
MsnSlpMessage * msn_slpmsg_sip_new(MsnSlpCall *slpcall, int cseq,
								   const char *header,
								   const char *branch,
								   const char *content_type,
								   const char *content);

void msn_slpmsg_show(MsnMessage *msg);

/**
 * Serialize the MsnSlpMessage in a way it can be used to be transmited
 *
 * @param slpmsg 	The MsnSlpMessage.
 * @param ret_size 	The size of the buffer cointaining the message.
 *
 * @return a buffer with the serialized data.
 */
char *msn_slpmsg_serialize(MsnSlpMessage *slpmsg, size_t *ret_size);

#endif /* _MSN_SLPMSG_H_ */
