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

#ifndef PURPLE_TYPING_H
#define PURPLE_TYPING_H

#include "purpleversion.h"

/**
 * PurpleTypingState:
 * @PURPLE_TYPING_STATE_NONE: The user is not currently typing and has nothing
 *                            in their buffer.
 * @PURPLE_TYPING_STATE_TYPING: The user is currently typing.
 * @PURPLE_TYPING_STATE_PAUSED: The user has typed some text, but stopped
 *                              without deleting it.
 *
 * Defines the state of a user composing a message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_TYPE_IN_3_0
typedef enum {
    PURPLE_TYPING_STATE_NONE,
    PURPLE_TYPING_STATE_TYPING,
    PURPLE_TYPING_STATE_PAUSED,
} PurpleTypingState;

#endif /* PURPLE_TYPING_H */
