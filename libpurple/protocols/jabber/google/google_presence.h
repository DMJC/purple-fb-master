/**
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

#ifndef PURPLE_JABBER_GOOGLE_PRESENCE_H_
#define PURPLE_JABBER_GOOGLE_PRESENCE_H_

#include <purple.h>

#include "jabber.h"
#include "buddy.h"

void jabber_google_presence_incoming(JabberStream *js, const char *who, JabberBuddyResource *jbr);
char *jabber_google_presence_outgoing(PurpleStatus *tune);


#endif /* PURPLE_JABBER_GOOGLE_PRESENCE_H_ */