/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#if !defined(PIDGIN_GLOBAL_HEADER_INSIDE) && !defined(PIDGIN_COMPILATION)
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PIDGIN_VERSION_H
#define PIDGIN_VERSION_H

#include <glib.h>

#include "pidginversionconsts.h"

#if (defined(_WIN32) || defined(__CYGWIN__)) && \
	!defined(PIDGIN_STATIC_COMPILATION)
#define _PIDGIN_EXPORT __declspec(dllexport)
#define _PIDGIN_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
#define _PIDGIN_EXPORT __attribute__((visibility("default")))
#define _PIDGIN_IMPORT
#else
#define _PIDGIN_EXPORT
#define _PIDGIN_IMPORT
#endif
#ifdef PIDGIN_COMPILATION
#define _PIDGIN_API _PIDGIN_EXPORT
#else
#define _PIDGIN_API _PIDGIN_IMPORT
#endif

#define _PIDGIN_EXTERN _PIDGIN_API extern

#ifdef PIDGIN_DISABLE_DEPRECATION_WARNINGS
#define PIDGIN_DEPRECATED _PIDGIN_EXTERN
#define PIDGIN_DEPRECATED_FOR(f) _PIDGIN_EXTERN
#define PIDGIN_UNAVAILABLE(maj, min) _PIDGIN_EXTERN
#define PIDGIN_UNAVAILABLE_STATIC_INLINE(maj, min)
#define PIDGIN_UNAVAILABLE_TYPE(maj, min)
#else
#define PIDGIN_DEPRECATED G_DEPRECATED _PIDGIN_EXTERN
#define PIDGIN_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _PIDGIN_EXTERN
#define PIDGIN_UNAVAILABLE(maj, min) G_UNAVAILABLE(maj, min) _PIDGIN_EXTERN
#define PIDGIN_UNAVAILABLE_STATIC_INLINE(maj, min) G_UNAVAILABLE(maj, min)
#define PIDGIN_UNAVAILABLE_TYPE(maj, min) G_UNAVAILABLE(maj, min)
#endif

/**
 * PIDGIN_VERSION_CUR_STABLE:
 *
 * A macro that evaluates to the current stable version of Pidgin, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_CUR_STABLE \
	(G_ENCODE_VERSION(PIDGIN_MAJOR_VERSION, PIDGIN_MINOR_VERSION))

/* If the package sets PIDGIN_VERSION_MIN_REQUIRED to some future
 * PIDGIN_VERSION_X_Y value that we don't know about, it will compare as 0 in
 * preprocessor tests.
 */
#ifndef PIDGIN_VERSION_MIN_REQUIRED
#define PIDGIN_VERSION_MIN_REQUIRED (PIDGIN_VERSION_CUR_STABLE)
#elif PIDGIN_VERSION_MIN_REQUIRED == 0
#undef PIDGIN_VERSION_MIN_REQUIRED
#define PIDGIN_VERSION_MIN_REQUIRED (PIDGIN_VERSION_CUR_STABLE + 1)
#endif /* PIDGIN_VERSION_MIN_REQUIRED */

#if !defined(PIDGIN_VERSION_MAX_ALLOWED) || (PIDGIN_VERSION_MAX_ALLOWED == 0)
#undef PIDGIN_VERSION_MAX_ALLOWED
#define PIDGIN_VERSION_MAX_ALLOWED (PIDGIN_VERSION_CUR_STABLE)
#endif /* PIDGIN_VERSION_MAX_ALLOWED */

/* sanity checks */
#if PIDGIN_VERSION_MIN_REQUIRED > PIDGIN_VERSION_CUR_STABLE
#error "PIDGIN_VERSION_MIN_REQUIRED must be <= PIDGIN_VERSION_CUR_STABLE"
#endif
#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_MIN_REQUIRED
#error "PIDGIN_VERSION_MAX_ALLOWED must be >= PIDGIN_VERSION_MIN_REQUIRED"
#endif
#if PIDGIN_VERSION_MIN_REQUIRED < G_ENCODE_VERSION(2, 0)
#error "PIDGIN_VERSION_MIN_REQUIRED must be >= PIDGIN_VERSION_2_0"
#endif

#define PIDGIN_VAR _PIDGIN_EXTERN
#define PIDGIN_AVAILABLE_IN_ALL _PIDGIN_EXTERN

/**
 * PIDGIN_VERSION_2_0:
 *
 * A macro that evaluates to the 2.0 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_0 (G_ENCODE_VERSION(2, 0))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_0
#define PIDGIN_AVAILABLE_IN_2_0 PIDGIN_UNAVAILABLE(2, 0)
#else
#define PIDGIN_AVAILABLE_IN_2_0 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_1:
 *
 * A macro that evaluates to the 2.1 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_1 (G_ENCODE_VERSION(2, 1))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_1
#define PIDGIN_AVAILABLE_IN_2_1 PIDGIN_UNAVAILABLE(2, 1)
#define PIDGIN_AVAILABLE_MACRO_IN_2_1 PIDGIN_UNAVAILABLE_MACRO(2, 1)
#define PIDGIN_AVAILABLE_TYPE_IN_2_1 PIDGIN_UNAVAILABLE_TYPE(2, 1)
#else
#define PIDGIN_AVAILABLE_IN_2_1 _PIDGIN_EXTERN
#define PIDGIN_AVAILABLE_MACRO_IN_2_1
#define PIDGIN_AVAILABLE_TYPE_IN_2_1
#endif

/**
 * PIDGIN_VERSION_2_2:
 *
 * A macro that evaluates to the 2.2 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_2 (G_ENCODE_VERSION(2, 2))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_2
#define PIDGIN_AVAILABLE_IN_2_2 PIDGIN_UNAVAILABLE(2, 2)
#else
#define PIDGIN_AVAILABLE_IN_2_2 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_3:
 *
 * A macro that evaluates to the 2.3 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_3 (G_ENCODE_VERSION(2, 3))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_3
#define PIDGIN_AVAILABLE_IN_2_3 PIDGIN_UNAVAILABLE(2, 3)
#else
#define PIDGIN_AVAILABLE_IN_2_3 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_4:
 *
 * A macro that evaluates to the 2.4 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_4 (G_ENCODE_VERSION(2, 4))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_4
#define PIDGIN_AVAILABLE_IN_2_4 PIDGIN_UNAVAILABLE(2, 4)
#else
#define PIDGIN_AVAILABLE_IN_2_4 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_5:
 *
 * A macro that evaluates to the 2.5 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_5 (G_ENCODE_VERSION(2, 5))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_5
#define PIDGIN_AVAILABLE_IN_2_5 PIDGIN_UNAVAILABLE(2, 5)
#else
#define PIDGIN_AVAILABLE_IN_2_5 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_6:
 *
 * A macro that evaluates to the 2.6 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_6 (G_ENCODE_VERSION(2, 6))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_6
#define PIDGIN_AVAILABLE_IN_2_6 PIDGIN_UNAVAILABLE(2, 6)
#else
#define PIDGIN_AVAILABLE_IN_2_6 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_7:
 *
 * A macro that evaluates to the 2.7 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_7 (G_ENCODE_VERSION(2, 7))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_7
#define PIDGIN_AVAILABLE_IN_2_7 PIDGIN_UNAVAILABLE(2, 7)
#else
#define PIDGIN_AVAILABLE_IN_2_7 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_8:
 *
 * A macro that evaluates to the 2.8 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_8 (G_ENCODE_VERSION(2, 8))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_8
#define PIDGIN_AVAILABLE_IN_2_8 PIDGIN_UNAVAILABLE(2, 8)
#else
#define PIDGIN_AVAILABLE_IN_2_8 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_9:
 *
 * A macro that evaluates to the 2.9 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_9 (G_ENCODE_VERSION(2, 9))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_9
#define PIDGIN_AVAILABLE_IN_2_9 PIDGIN_UNAVAILABLE(2, 9)
#else
#define PIDGIN_AVAILABLE_IN_2_9 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_10:
 *
 * A macro that evaluates to the 2.10 version of Pidgin, in a format that can
 * be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_10 (G_ENCODE_VERSION(2, 10))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_10
#define PIDGIN_AVAILABLE_IN_2_10 PIDGIN_UNAVAILABLE(2, 10)
#else
#define PIDGIN_AVAILABLE_IN_2_10 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_11:
 *
 * A macro that evaluates to the 2.11 version of Pidgin, in a format that can
 * be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_11 (G_ENCODE_VERSION(2, 11))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_11
#define PIDGIN_AVAILABLE_IN_2_11 PIDGIN_UNAVAILABLE(2, 11)
#else
#define PIDGIN_AVAILABLE_IN_2_11 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_12:
 *
 * A macro that evaluates to the 2.12 version of Pidgin, in a format that can
 * be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_12 (G_ENCODE_VERSION(2, 12))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_12
#define PIDGIN_AVAILABLE_IN_2_12 PIDGIN_UNAVAILABLE(2, 12)
#else
#define PIDGIN_AVAILABLE_IN_2_12 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_13:
 *
 * A macro that evaluates to the 2.13 version of Pidgin, in a format that can
 * be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_13 (G_ENCODE_VERSION(2, 13))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_13
#define PIDGIN_AVAILABLE_IN_2_13 PIDGIN_UNAVAILABLE(2, 13)
#else
#define PIDGIN_AVAILABLE_IN_2_13 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_2_14:
 *
 * A macro that evaluates to the 2.14 version of Pidgin, in a format that can
 * be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_2_14 (G_ENCODE_VERSION(2, 14))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_2_14
#define PIDGIN_AVAILABLE_IN_2_14 PIDGIN_UNAVAILABLE(2, 14)
#else
#define PIDGIN_AVAILABLE_IN_2_14 _PIDGIN_EXTERN
#endif

/**
 * PIDGIN_VERSION_3_0:
 *
 * A macro that evaluates to the 3.0 version of Pidgin, in a format that can be
 * used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PIDGIN_VERSION_3_0 (G_ENCODE_VERSION(3, 0))

#if PIDGIN_VERSION_MAX_ALLOWED < PIDGIN_VERSION_3_0
#define PIDGIN_AVAILABLE_IN_3_0 PIDGIN_UNAVAILABLE(3, 0)
#define PIDGIN_AVAILABLE_STATIC_INLINE_IN_3_0 PIDGIN_UNAVAILABLE_STATIC_INLINE(3, 0)
#define PIDGIN_AVAILABLE_MACRO_IN_3_0 PIDGIN_UNAVAILABLE_MACRO(3, 0)
#define PIDGIN_AVAILABLE_ENUMERATOR_IN_3_0 PIDGIN_UNAVAILABLE_ENUMERATOR(3, 0)
#define PIDGIN_AVAILABLE_TYPE_IN_3_0 PIDGIN_UNAVAILABLE_TYPE(3, 0)
#else
#define PIDGIN_AVAILABLE_IN_3_0 _PIDGIN_EXTERN
#define PIDGIN_AVAILABLE_STATIC_INLINE_IN_3_0
#define PIDGIN_AVAILABLE_MACRO_IN_3_0
#define PIDGIN_AVAILABLE_ENUMERATOR_IN_3_0
#define PIDGIN_AVAILABLE_TYPE_IN_3_0
#endif

#endif /* PIDGIN_VERSION_H */
