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
#if !defined(PURPLE_IRCV3_GLOBAL_HEADER_INSIDE) && \
    !defined(PURPLE_IRCV3_COMPILATION)
# error "only <libpurple/protocols/ircv3.h> may be included directly"
#endif

#ifndef PURPLE_IRCV3_CONSTANTS_H
#define PURPLE_IRCV3_CONSTANTS_H

/**
 * PURPLE_IRCV3_CTCP_ACTION:
 *
 * A constant representing the CTCP ACTION command.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_CTCP_ACTION ("ACTION") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_CTCP_DELIMITER:
 *
 * The delimiter used for CTCP messages.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_CTCP_DELIMITER (0x1) PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_CTCP_VERSION:
 *
 * A constant representing the CTCP VERSION command.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_CTCP_VERSION ("VERSION") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_ERR_NICKLOCKED:
 *
 * A constant for the IRC %NICKLOCKED error.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_ERR_NICKLOCKED ("902") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_ERR_SASLABORTED:
 *
 * A constant for the IRC %SASLABORTED error.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_ERR_SASLABORTED ("906") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_ERR_SASLALREADY:
 *
 * A constant for the IRC %SASLALREADY error.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_ERR_SASLALREADY ("907") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_ERR_SASLFAIL:
 *
 * A constant for the IRC %SASLFAIL error.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_ERR_SASLFAIL ("904") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_ERR_SASLTOOLONG:
 *
 * A constant for the IRC %SASLTOOLONG error.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_ERR_SASLTOOLONG ("905") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_MSG_AUTHENTICATE:
 *
 * A constant for the IRC %AUTHENTICATE message.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_MSG_AUTHENTICATE ("AUTHENTICATE") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_MSG_CAP:
 *
 * A constant for the IRC %CAP message.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_MSG_CAP ("CAP") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_MSG_JOIN:
 *
 * A constant for the IRC %JOIN message.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_MSG_JOIN ("JOIN") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_MSG_NOTICE:
 *
 * A constant for the IRC %NOTICE message.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_MSG_NOTICE ("NOTICE") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_MSG_PING:
 *
 * A constant for the IRC %PING message.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_MSG_PING ("PING") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_MSG_PRIVMSG:
 *
 * A constant for the IRC %PRIVMSG message.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_MSG_PRIVMSG ("PRIVMSG") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_MSG_TOPIC:
 *
 * A constant for the IRC %TOPIC message.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_MSG_TOPIC ("TOPIC") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_CREATED:
 *
 * A constant for the IRC %CREATED reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_CREATED ("003") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_ENDOFMOTD:
 *
 * A constant for the IRC %ENDOFMOTD reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_ENDOFMOTD ("376") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_LOGGEDIN:
 *
 * A constant for the IRC %LOGGEDIN reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_LOGGEDIN ("900") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_LOGGEDOUT:
 *
 * A constant for the IRC %LOGGEDOUT reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_LOGGEDOUT ("901") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_LUSERCHANNELS:
 *
 * A constant for the IRC %LUSERCHANNELS reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_LUSERCHANNELS ("254") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_LUSERCLIENT:
 *
 * A constant for the IRC %LUSERCLIENT reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_LUSERCLIENT ("251") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_LUSERME:
 *
 * A constant for the IRC %LUSERME reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_LUSERME ("255") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_LUSEROP:
 *
 * A constant for the IRC %LUSEROP reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_LUSEROP ("252") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_LUSERUNKNOWN:
 *
 * A constant for the IRC %LUSERUNKNOWN reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_LUSERUNKNOWN ("253") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_MYINFO:
 *
 * A constant for the IRC %MYINFO reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_MYINFO ("004") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_MOTD:
 *
 * A constant for the IRC %MOTD reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_MOTD ("372") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_MOTDSTART:
 *
 * A constant for the IRC %MOTDSTART reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_MOTDSTART ("375") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_NOTOPIC:
 *
 * A constant for the IRC %NOTOPIC reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_NOTOPIC ("331") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_SASLMECHS:
 *
 * A constant for the IRC %SASLMECHS reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_SASLMECHS ("908") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_SASLSUCCESS:
 *
 * A constant for the IRC %SASLSUCCESS reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_SASLSUCCESS ("903") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_TOPIC:
 *
 * A constant for the IRC %TOPIC reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_TOPIC ("332") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_WELCOME:
 *
 * A constant for the IRC %WELCOME reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_WELCOME ("001") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

/**
 * PURPLE_IRCV3_RPL_YOURHOST:
 *
 * A constant for the IRC %YOURHOST reply.
 *
 * Since: 3.0.0
 */
#define PURPLE_IRCV3_RPL_YOURHOST ("002") PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0

#endif /* PURPLE_IRCV3_CONSTANTS_H */
