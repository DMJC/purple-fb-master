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

#ifndef PURPLE_MEDIA_GST_H
#define PURPLE_MEDIA_GST_H

#include "media.h"
#include "mediamanager.h"
#include "purpleversion.h"

#include <gst/gst.h>

#define PURPLE_TYPE_MEDIA_ELEMENT_TYPE  purple_media_element_type_get_type()

#define PURPLE_TYPE_MEDIA_ELEMENT_INFO  purple_media_element_info_get_type()

/**
 * PurpleMediaElementInfo:
 *
 * An opaque structure representing an audio/video source/sink.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_TYPE_IN_2_6
typedef struct _PurpleMediaElementInfo PurpleMediaElementInfo;

/**
 * PurpleMediaElementCreateCallback:
 * @info: The element.
 * @media: The media.
 * @session_id: The session id.
 * @participant: The participant.
 *
 * A callback that creates a GstElement for the given parameters.
 *
 * Returns: (transfer full): The new element.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_TYPE_IN_2_6
typedef GstElement *(*PurpleMediaElementCreateCallback)(
		PurpleMediaElementInfo *info, PurpleMedia *media,
		const gchar *session_id, const gchar *participant);

/**
 * PurpleMediaElementType:
 * @PURPLE_MEDIA_ELEMENT_NONE:         empty element
 * @PURPLE_MEDIA_ELEMENT_AUDIO:        supports audio
 * @PURPLE_MEDIA_ELEMENT_VIDEO:        supports video
 * @PURPLE_MEDIA_ELEMENT_AUDIO_VIDEO:  supports audio and video
 * @PURPLE_MEDIA_ELEMENT_NO_SRCS:      has no src pads
 * @PURPLE_MEDIA_ELEMENT_ONE_SRC:      has one src pad
 * @PURPLE_MEDIA_ELEMENT_MULTI_SRC:    has multiple src pads
 * @PURPLE_MEDIA_ELEMENT_REQUEST_SRC:  src pads must be requested
 * @PURPLE_MEDIA_ELEMENT_NO_SINKS:     has no sink pads
 * @PURPLE_MEDIA_ELEMENT_ONE_SINK:     has one sink pad
 * @PURPLE_MEDIA_ELEMENT_MULTI_SINK:   has multiple sink pads
 * @PURPLE_MEDIA_ELEMENT_REQUEST_SINK: sink pads must be requested
 * @PURPLE_MEDIA_ELEMENT_UNIQUE:       This element is unique and only one
 *                                     instance of it should be created at a
 *                                     time
 * @PURPLE_MEDIA_ELEMENT_SRC:          can be set as an active src
 * @PURPLE_MEDIA_ELEMENT_SINK:         can be set as an active sink
 * @PURPLE_MEDIA_ELEMENT_APPLICATION:  supports application data
 *
 * The possible types of media elements.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_TYPE_IN_2_6
typedef enum {
	PURPLE_MEDIA_ELEMENT_NONE = 0,
	PURPLE_MEDIA_ELEMENT_AUDIO = 1,
	PURPLE_MEDIA_ELEMENT_VIDEO = 1 << 1,
	PURPLE_MEDIA_ELEMENT_AUDIO_VIDEO = PURPLE_MEDIA_ELEMENT_AUDIO
			| PURPLE_MEDIA_ELEMENT_VIDEO,
	PURPLE_MEDIA_ELEMENT_NO_SRCS = 0,
	PURPLE_MEDIA_ELEMENT_ONE_SRC = 1 << 2,
	PURPLE_MEDIA_ELEMENT_MULTI_SRC = 1 << 3,
	PURPLE_MEDIA_ELEMENT_REQUEST_SRC = 1 << 4,
	PURPLE_MEDIA_ELEMENT_NO_SINKS = 0,
	PURPLE_MEDIA_ELEMENT_ONE_SINK = 1 << 5,
	PURPLE_MEDIA_ELEMENT_MULTI_SINK = 1 << 6,
	PURPLE_MEDIA_ELEMENT_REQUEST_SINK = 1 << 7,
	PURPLE_MEDIA_ELEMENT_UNIQUE = 1 << 8,
	PURPLE_MEDIA_ELEMENT_SRC = 1 << 9,
	PURPLE_MEDIA_ELEMENT_SINK = 1 << 10,
	PURPLE_MEDIA_ELEMENT_APPLICATION = 1 << 11,
} PurpleMediaElementType;

G_BEGIN_DECLS

PURPLE_AVAILABLE_IN_2_6
GType purple_media_element_type_get_type(void);

PURPLE_AVAILABLE_IN_2_6
G_DECLARE_FINAL_TYPE(PurpleMediaElementInfo, purple_media_element_info, PURPLE,
		MEDIA_ELEMENT_INFO, GObject)

/**
 * purple_media_get_src:
 * @media: The media object the session is in.
 * @sess_id: The session id of the session to get the source from.
 *
 * Gets the source from a session
 *
 * Returns: (transfer none): The source retrieved.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
GstElement *purple_media_get_src(PurpleMedia *media, const gchar *sess_id);

/**
 * purple_media_get_tee:
 * @media: The instance to get the tee from.
 * @session_id: The id of the session to get the tee from.
 * @participant: Optionally, the participant of the stream to get the tee from.
 *
 * Gets the tee from a given session/stream.
 *
 * Returns: (transfer none): The GstTee element from the chosen session/stream.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
GstElement *purple_media_get_tee(PurpleMedia *media,
		const gchar *session_id, const gchar *participant);


/**
 * purple_media_manager_get_pipeline:
 * @manager: The media manager to get the pipeline from.
 *
 * Gets the pipeline from the media manager.
 *
 * Returns: (transfer none): The pipeline.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
GstElement *purple_media_manager_get_pipeline(PurpleMediaManager *manager);

/**
 * purple_media_manager_get_element:
 * @manager: The media manager to use to obtain the source/sink.
 * @type: The type of source/sink to get.
 * @media: The media call this element is requested for.
 * @session_id: The id of the session this element is requested for or NULL.
 * @participant: The remote user this element is requested for or NULL.
 *
 * Gets the media element for the given session.
 *
 * Returns: (transfer full): A GStreamer source or sink for audio or video.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
GstElement *purple_media_manager_get_element(PurpleMediaManager *manager,
		PurpleMediaSessionType type, PurpleMedia *media,
		const gchar *session_id, const gchar *participant);

/**
 * purple_media_manager_enumerate_elements:
 * @manager: The media manager to use to obtain the element infos.
 * @type: The type of element infos to get.
 *
 * Enumerates the elements in @manager filtered by @type.
 *
 * Returns: (transfer container) (element-type PurpleMediaElementInfo): A #GList of registered #PurpleMediaElementInfo instances that match
 * @type.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GList *purple_media_manager_enumerate_elements(PurpleMediaManager *manager, PurpleMediaElementType type);

/**
 * purple_media_manager_get_element_info:
 * @manager: The #PurpleMediaManager instance
 * @name: The name of the element to get.
 *
 * Gets the element info for the element named @name in @manager.
 *
 * Returns: (transfer full): The #PurpleMediaElementInfo for @name or NULL.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
PurpleMediaElementInfo *purple_media_manager_get_element_info(
		PurpleMediaManager *manager, const gchar *name);

/**
 * purple_media_manager_register_element:
 * @manager: The instance.
 * @info: THe new element to register.
 *
 * Registers @info with @manager.
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
gboolean purple_media_manager_register_element(PurpleMediaManager *manager,
		PurpleMediaElementInfo *info);

/**
 * purple_media_manager_unregister_element:
 * @manager: The instance.
 * @name: The name of the element to unregister.
 *
 * Unregisters the elemented named @name from @manager.
 *
 * Returns: %TRUE if the element was found and removed, otherwise %FALSE.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
gboolean purple_media_manager_unregister_element(PurpleMediaManager *manager,
		const gchar *name);

/**
 * purple_media_manager_set_active_element:
 * @manager: The instance.
 * @info: The new element to activate.
 *
 * Attempts to activate @info as the active element.
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
gboolean purple_media_manager_set_active_element(PurpleMediaManager *manager,
		PurpleMediaElementInfo *info);

/**
 * purple_media_manager_get_active_element:
 * @manager: The #PurpleMediaManager instance
 * @type: The #PurpleMediaElementType who's info to get
 *
 * Gets the element that is currently active.
 *
 * Returns: (transfer none): The #PurpleMediaElementInfo for @type.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
PurpleMediaElementInfo *purple_media_manager_get_active_element(
		PurpleMediaManager *manager, PurpleMediaElementType type);

/**
 * purple_media_manager_set_video_caps:
 * @manager: The media manager to set the media formats.
 * @caps: Set of allowed media formats.
 *
 * Reduces media formats supported by the video source to given set.
 *
 * Useful to force negotiation of smaller picture resolution more suitable for
 * use with particular codec and communication protocol without rescaling.
 *
 * Since: 2.8
 */
PURPLE_AVAILABLE_IN_2_8
void purple_media_manager_set_video_caps(PurpleMediaManager *manager,
		GstCaps *caps);

/**
 * purple_media_manager_get_video_caps:
 * @manager: The media manager to get the media formats from.
 *
 * Returns current set of media formats limiting the output from video source.
 *
 * Returns: GstCaps limiting the video source's formats.
 *
 * Since: 2.8
 */
PURPLE_AVAILABLE_IN_2_8
GstCaps *purple_media_manager_get_video_caps(PurpleMediaManager *manager);

/**
 * purple_media_element_info_get_id:
 * @info: The instance.
 *
 * Gets the identifier from @info.
 *
 * Returns: The identifier.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
gchar *purple_media_element_info_get_id(PurpleMediaElementInfo *info);

/**
 * purple_media_element_info_get_name:
 * @info: The instance.
 *
 * Gets the name of @info.
 *
 * Returns: The name of @info.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
gchar *purple_media_element_info_get_name(PurpleMediaElementInfo *info);

/**
 * purple_media_element_info_get_element_type:
 * @info: The instance.
 *
 * Gets the element type of @info.
 *
 * Returns: The element type.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
PurpleMediaElementType purple_media_element_info_get_element_type(
		PurpleMediaElementInfo *info);

/**
 * purple_media_element_info_call_create:
 * @info: The #PurpleMediaElementInfo to create the element from
 * @media: The media instance.
 * @session_id: The session id.
 * @participant: The participant.
 *
 * Creates a new call.
 *
 * Returns: (transfer full): The new GstElement.
 *
 * Since: 2.6
 */
PURPLE_AVAILABLE_IN_2_6
GstElement *purple_media_element_info_call_create(
		PurpleMediaElementInfo *info, PurpleMedia *media,
		const gchar *session_id, const gchar *participant);

G_END_DECLS

#endif /* PURPLE_MEDIA_GST_H */
