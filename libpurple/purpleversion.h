/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_VERSION_H
#define PURPLE_VERSION_H

#include <glib.h>

#include "purpleversionconsts.h"

#if (defined(_WIN32) || defined(__CYGWIN__)) && \
	!defined(PURPLE_STATIC_COMPILATION)
#define _PURPLE_EXPORT __declspec(dllexport)
#define _PURPLE_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
#define _PURPLE_EXPORT __attribute__((visibility("default")))
#define _PURPLE_IMPORT
#else
#define _PURPLE_EXPORT
#define _PURPLE_IMPORT
#endif
#ifdef PURPLE_COMPILATION
#define _PURPLE_API _PURPLE_EXPORT
#else
#define _PURPLE_API _PURPLE_IMPORT
#endif

#define _PURPLE_EXTERN _PURPLE_API extern

#ifdef PURPLE_DISABLE_DEPRECATION_WARNINGS
#define PURPLE_DEPRECATED _PURPLE_EXTERN
#define PURPLE_DEPRECATED_FOR(f) _PURPLE_EXTERN
#define PURPLE_UNAVAILABLE(maj, min) _PURPLE_EXTERN
#define PURPLE_UNAVAILABLE_STATIC_INLINE(maj, min)
#define PURPLE_UNAVAILABLE_TYPE(maj, min)
#else
#define PURPLE_DEPRECATED G_DEPRECATED _PURPLE_EXTERN
#define PURPLE_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _PURPLE_EXTERN
#define PURPLE_UNAVAILABLE(maj, min) G_UNAVAILABLE(maj, min) _PURPLE_EXTERN
#define PURPLE_UNAVAILABLE_STATIC_INLINE(maj, min) G_UNAVAILABLE(maj, min)
#define PURPLE_UNAVAILABLE_TYPE(maj, min) G_UNAVAILABLE(maj, min)
#endif

/**
 * PURPLE_VERSION_CUR_STABLE:
 *
 * A macro that evaluates to the current stable version of libpurple, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PURPLE_VERSION_CUR_STABLE \
	(G_ENCODE_VERSION(PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION))

/* If the package sets PURPLE_VERSION_MIN_REQUIRED to some future
 * PURPLE_VERSION_X_Y value that we don't know about, it will compare as 0 in
 * preprocessor tests.
 */
#ifndef PURPLE_VERSION_MIN_REQUIRED
#define PURPLE_VERSION_MIN_REQUIRED (PURPLE_VERSION_CUR_STABLE)
#elif PURPLE_VERSION_MIN_REQUIRED == 0
#undef PURPLE_VERSION_MIN_REQUIRED
#define PURPLE_VERSION_MIN_REQUIRED (PURPLE_VERSION_CUR_STABLE + 1)
#endif /* PURPLE_VERSION_MIN_REQUIRED */

#if !defined(PURPLE_VERSION_MAX_ALLOWED) || (PURPLE_VERSION_MAX_ALLOWED == 0)
#undef PURPLE_VERSION_MAX_ALLOWED
#define PURPLE_VERSION_MAX_ALLOWED (PURPLE_VERSION_CUR_STABLE)
#endif /* PURPLE_VERSION_MAX_ALLOWED */

/* sanity checks */
#if PURPLE_VERSION_MIN_REQUIRED > PURPLE_VERSION_CUR_STABLE
#error "PURPLE_VERSION_MIN_REQUIRED must be <= PURPLE_VERSION_CUR_STABLE"
#endif
#if PURPLE_VERSION_MAX_ALLOWED < PURPLE_VERSION_MIN_REQUIRED
#error "PURPLE_VERSION_MAX_ALLOWED must be >= PURPLE_VERSION_MIN_REQUIRED"
#endif
#if PURPLE_VERSION_MIN_REQUIRED < G_ENCODE_VERSION(0, 1)
#error "PURPLE_VERSION_MIN_REQUIRED must be >= PURPLE_VERSION_0_1"
#endif

#define PURPLE_VAR _PURPLE_EXTERN
#define PURPLE_AVAILABLE_IN_ALL _PURPLE_EXTERN

/**
 * PURPLE_VERSION_2_8:
 *
 * A macro that evaluates to the 2.8 version of libpurple, in a format that
 * can be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PURPLE_VERSION_2_8 (G_ENCODE_VERSION(2, 8))

#if PURPLE_VERSION_MAX_ALLOWED < PURPLE_VERSION_2_8
#define PURPLE_AVAILABLE_IN_2_8 PURPLE_UNAVAILABLE(2, 8)
#else
#define PURPLE_AVAILABLE_IN_2_8 _PURPLE_EXTERN
#endif

/**
 * PURPLE_VERSION_2_11:
 *
 * A macro that evaluates to the 2.11 version of libpurple, in a format that
 * can be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PURPLE_VERSION_2_11 (G_ENCODE_VERSION(2, 11))

#if PURPLE_VERSION_MAX_ALLOWED < PURPLE_VERSION_2_11
#define PURPLE_AVAILABLE_IN_2_11 PURPLE_UNAVAILABLE(2, 11)
#else
#define PURPLE_AVAILABLE_IN_2_11 _PURPLE_EXTERN
#endif

/**
 * PURPLE_VERSION_2_14:
 *
 * A macro that evaluates to the 2.14 version of libpurple, in a format that
 * can be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PURPLE_VERSION_2_14 (G_ENCODE_VERSION(2, 14))

#if PURPLE_VERSION_MAX_ALLOWED < PURPLE_VERSION_2_14
#define PURPLE_AVAILABLE_IN_2_14 PURPLE_UNAVAILABLE(2, 14)
#else
#define PURPLE_AVAILABLE_IN_2_14 _PURPLE_EXTERN
#endif

/**
 * PURPLE_VERSION_3_0:
 *
 * A macro that evaluates to the 3.0 version of libpurple, in a format that can
 * be used by the C pre-processor.
 *
 * Since: 3.0.0
 */
#define PURPLE_VERSION_3_0 (G_ENCODE_VERSION(3, 0))

#if PURPLE_VERSION_MAX_ALLOWED < PURPLE_VERSION_3_0
#define PURPLE_AVAILABLE_IN_3_0 PURPLE_UNAVAILABLE(3, 0)
#define PURPLE_AVAILABLE_STATIC_INLINE_IN_3_0 PURPLE_UNAVAILABLE_STATIC_INLINE(3, 0)
#define PURPLE_AVAILABLE_MACRO_IN_3_0 PURPLE_UNAVAILABLE_MACRO(3, 0)
#define PURPLE_AVAILABLE_ENUMERATOR_IN_3_0 PURPLE_UNAVAILABLE_ENUMERATOR(3, 0)
#define PURPLE_AVAILABLE_TYPE_IN_3_0 PURPLE_UNAVAILABLE_TYPE(3, 0)
#else
#define PURPLE_AVAILABLE_IN_3_0 _PURPLE_EXTERN
#define PURPLE_AVAILABLE_STATIC_INLINE_IN_3_0
#define PURPLE_AVAILABLE_MACRO_IN_3_0
#define PURPLE_AVAILABLE_ENUMERATOR_IN_3_0
#define PURPLE_AVAILABLE_TYPE_IN_3_0
#endif

/**
 * PURPLE_VERSION_CHECK:
 * @major: The major version to check for.
 * @minor: The minor version to check for.
 * @micro: The micro version to check for.
 *
 * Checks the version of libpurple being compiled against.  See
 * #purple_version_check for a runtime check.
 *
 * Returns: %TRUE if the version of libpurple is the same or newer than the
 *          passed-in version.
 */
#define PURPLE_VERSION_CHECK(major, minor, micro) ((major) == PURPLE_MAJOR_VERSION && \
                                                   ((minor) < PURPLE_MINOR_VERSION || \
                                                    ((minor) == PURPLE_MINOR_VERSION && (micro) <= PURPLE_MICRO_VERSION)))

G_BEGIN_DECLS

/**
 * purple_version_check:
 * @required_major: the required major version.
 * @required_minor: the required minor version.
 * @required_micro: the required micro version.
 *
 * Checks that the libpurple version is compatible with the requested
 * version
 *
 * Returns: %NULL if the versions are compatible, or a string describing
 *          the version mismatch if not compatible.
 */
const char *purple_version_check(guint required_major, guint required_minor, guint required_micro);

/**
 * purple_major_version:
 *
 * The major version of the running libpurple.  Contrast with
 * #PURPLE_MAJOR_VERSION, which expands at compile time to the major version of
 * libpurple being compiled against.
 */
extern const guint purple_major_version;

/**
 * purple_minor_version:
 *
 * The minor version of the running libpurple.  Contrast with
 * #PURPLE_MINOR_VERSION, which expands at compile time to the minor version of
 * libpurple being compiled against.
 */
extern const guint purple_minor_version;

/**
 * purple_micro_version:
 *
 * The micro version of the running libpurple.  Contrast with
 * #PURPLE_MICRO_VERSION, which expands at compile time to the micro version of
 * libpurple being compiled against.
 */
extern const guint purple_micro_version;

G_END_DECLS

#endif /* PURPLE_VERSION_H */
