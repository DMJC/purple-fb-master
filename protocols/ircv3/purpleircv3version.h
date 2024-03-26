/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#if !defined(PURPLE_IRCV3_GLOBAL_HEADER_INSIDE) && \
    !defined(PURPLE_IRCV3_COMPILATION)
# error "only <libpurple/protocols/ircv3.h> may be included directly"
#endif

#ifndef PURPLE_IRCV3_VERSION_H
#define PURPLE_IRCV3_VERSION_H

#include <glib.h>

#include <purple.h>

#if (defined(_WIN32) || defined(__CYGWIN__)) && \
	!defined(PURPLE_IRCV3_STATIC_COMPILATION)
#define _PURPLE_IRCV3_EXPORT __declspec(dllexport)
#define _PURPLE_IRCV3_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
#define _PURPLE_IRCV3_EXPORT __attribute__((visibility("default")))
#define _PURPLE_IRCV3_IMPORT
#else
#define _PURPLE_IRCV3_EXPORT
#define _PURPLE_IRCV3_IMPORT
#endif
#ifdef PURPLE_IRCV3_COMPILATION
#define _PURPLE_IRCV3_API _PURPLE_IRCV3_EXPORT
#else
#define _PURPLE_IRCV3_API _PURPLE_IRCV3_IMPORT
#endif

#define _PURPLE_IRCV3_EXTERN _PURPLE_IRCV3_API extern

#ifdef PURPLE_IRCV3_DISABLE_DEPRECATION_WARNINGS
#define PURPLE_IRCV3_DEPRECATED _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_DEPRECATED_FOR(f) _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_UNAVAILABLE(maj, min) _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_UNAVAILABLE_STATIC_INLINE(maj, min)
#define PURPLE_IRCV3_UNAVAILABLE_TYPE(maj, min)
#else
#define PURPLE_IRCV3_DEPRECATED G_DEPRECATED _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_UNAVAILABLE(maj, min) G_UNAVAILABLE(maj, min) _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_UNAVAILABLE_STATIC_INLINE(maj, min) G_UNAVAILABLE(maj, min)
#define PURPLE_IRCV3_UNAVAILABLE_TYPE(maj, min) G_UNAVAILABLE(maj, min)
#endif

/**
 * PURPLE_IRCV3_VERSION_CUR_STABLE:
 *
 * A macro that evaluates to the current stable version of the IRCv3 protocol
 * plugin, in a format that can be used by the C pre-processor.
 *
 * Since: 3.0
 */
#define PURPLE_IRCV3_VERSION_CUR_STABLE \
	(G_ENCODE_VERSION(PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION))

/* If the package sets PURPLE_IRCV3_VERSION_MIN_REQUIRED to some future
 * PURPLE_IRCV3_VERSION_X_Y value that we don't know about, it will compare as 0 in
 * preprocessor tests.
 */
#ifndef PURPLE_IRCV3_VERSION_MIN_REQUIRED
#define PURPLE_IRCV3_VERSION_MIN_REQUIRED (PURPLE_IRCV3_VERSION_CUR_STABLE)
#elif PURPLE_IRCV3_VERSION_MIN_REQUIRED == 0
#undef PURPLE_IRCV3_VERSION_MIN_REQUIRED
#define PURPLE_IRCV3_VERSION_MIN_REQUIRED (PURPLE_IRCV3_VERSION_CUR_STABLE + 1)
#endif /* PURPLE_IRCV3_VERSION_MIN_REQUIRED */

#if !defined(PURPLE_IRCV3_VERSION_MAX_ALLOWED) || (PURPLE_IRCV3_VERSION_MAX_ALLOWED == 0)
#undef PURPLE_IRCV3_VERSION_MAX_ALLOWED
#define PURPLE_IRCV3_VERSION_MAX_ALLOWED (PURPLE_IRCV3_VERSION_CUR_STABLE)
#endif /* PURPLE_IRCV3_VERSION_MAX_ALLOWED */

/* sanity checks */
#if PURPLE_IRCV3_VERSION_MIN_REQUIRED > PURPLE_IRCV3_VERSION_CUR_STABLE
#error "PURPLE_IRCV3_VERSION_MIN_REQUIRED must be <= PURPLE_IRCV3_VERSION_CUR_STABLE"
#endif
#if PURPLE_IRCV3_VERSION_MAX_ALLOWED < PURPLE_IRCV3_VERSION_MIN_REQUIRED
#error "PURPLE_IRCV3_VERSION_MAX_ALLOWED must be >= PURPLE_IRCV3_VERSION_MIN_REQUIRED"
#endif
#if PURPLE_IRCV3_VERSION_MIN_REQUIRED < G_ENCODE_VERSION(3, 0)
#error "PURPLE_IRCV3_VERSION_MIN_REQUIRED must be >= PURPLE_IRCV3_VERSION_3_0"
#endif

#define PURPLE_IRCV3_VAR _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_AVAILABLE_IN_ALL _PURPLE_IRCV3_EXTERN

/**
 * PURPLE_IRCV3_VERSION_3_0:
 *
 * A macro that evaluates to the 3.0 version of the IRCv3 protocol plugin, in a
 * format that can be used by the C pre-processor.
 *
 * Since: 3.0
 */
#define PURPLE_IRCV3_VERSION_3_0 (G_ENCODE_VERSION(3, 0))

#if PURPLE_IRCV3_VERSION_MAX_ALLOWED < PURPLE_IRCV3_VERSION_3_0
#define PURPLE_IRCV3_AVAILABLE_IN_3_0 PURPLE_IRCV3_UNAVAILABLE(3, 0)
#define PURPLE_IRCV3_AVAILABLE_STATIC_INLINE_IN_3_0 PURPLE_IRCV3_UNAVAILABLE_STATIC_INLINE(3, 0)
#define PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0 PURPLE_IRCV3_UNAVAILABLE_MACRO(3, 0)
#define PURPLE_IRCV3_AVAILABLE_ENUMERATOR_IN_3_0 PURPLE_IRCV3_UNAVAILABLE_ENUMERATOR(3, 0)
#define PURPLE_IRCV3_AVAILABLE_TYPE_IN_3_0 PURPLE_IRCV3_UNAVAILABLE_TYPE(3, 0)
#else
#define PURPLE_IRCV3_AVAILABLE_IN_3_0 _PURPLE_IRCV3_EXTERN
#define PURPLE_IRCV3_AVAILABLE_STATIC_INLINE_IN_3_0
#define PURPLE_IRCV3_AVAILABLE_MACRO_IN_3_0
#define PURPLE_IRCV3_AVAILABLE_ENUMERATOR_IN_3_0
#define PURPLE_IRCV3_AVAILABLE_TYPE_IN_3_0
#endif

#endif /* PURPLE_IRCV3_VERSION_H */
