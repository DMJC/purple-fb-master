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

#ifndef PURPLE_REQUEST_FIELD_LABEL_H
#define PURPLE_REQUEST_FIELD_LABEL_H

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

/**
 * PurpleRequestFieldLabel:
 *
 * A label request field.
 */
typedef struct _PurpleRequestFieldLabel PurpleRequestFieldLabel;

#include "purplerequestfield.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_REQUEST_FIELD_LABEL (purple_request_field_label_get_type())
G_DECLARE_FINAL_TYPE(PurpleRequestFieldLabel, purple_request_field_label,
                     PURPLE, REQUEST_FIELD_LABEL, PurpleRequestField)

/**
 * purple_request_field_label_new:
 * @id:   The field ID.
 * @text: The label of the field.
 *
 * Creates a label field.
 *
 * Returns: (transfer full): The new field.
 */
PurpleRequestField *purple_request_field_label_new(const char *id, const char *text);

G_END_DECLS

#endif /* PURPLE_REQUEST_FIELD_LABEL_H */
