/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013 LG Electronics.
 *  Author : Jeongseok Kim <jeongseok.kim@lge.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_FD_CAPS_H__
#define __GST_FD_CAPS_H__

G_BEGIN_DECLS
#define FD_VIDEO_CAPS \
    "video/x-divx;" \
    "video/x-h264;" \
    "video/x-intel-h263;" \
    "video/x-h263;" \
    "video/mpeg;" \
    "video/x-wmv;" \
    "video/x-msmpeg;" \
    "video/x-pn-realvideo;" \
    "video/x-pn-realvideo;" \
    "video/x-svq;" \
    "video/x-ffv;" \
    "video/x-3ivx;" \
    "video/x-vp8;" \
    "video/x-xvid;" \
    "video/x-flash-video;" \
    "video/x-raw"
#define FD_AUDIO_CAPS \
    "audio/mpeg;"\
    "audio/x-dts;" \
    "audio/x-ac3;" \
    "audio/x-eac3;" \
    "audio/x-private1-ac3;" \
    "audio/x-wma;" \
    "audio/x-pn-realaudio;" \
    "audio/x-raw;" \
    "audio/x-lpcm-1;" \
    "audio/x-lpcm;" \
    "audio/x-private-lg-lpcm;" \
    "audio/x-private1-lpcm;" \
    "audio/x-private-ts-lpcm;" \
    "audio/x-adpcm;" \
    "audio/x-vorbis;" \
    "audio/AMR;" \
    "audio/AMR-WB;" \
    "audio/x-flac;" \
    "audio/x-mulaw;" \
    "audio/x-alaw;" \
    "audio/x-private1-dts"
    G_END_DECLS
#endif // __GST_FD_CAPS_H__
