/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Justin Joy <justin.joy.9to5@gmail.com> 
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


#ifndef __GST_FC_BIN_H__
#define __GST_FC_BIN_H__

#include <gst/gst.h>
#include "../playback/gstlpsink.h"

G_BEGIN_DECLS
#define GST_TYPE_FC_BIN (gst_fc_bin_get_type())
#define GST_FC_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_FC_BIN,GstFCBin))
#define GST_FC_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FC_BIN,GstFCBinClass))
#define GST_IS_FC_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FC_BIN))
#define GST_IS_FC_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FC_BIN))
#define GST_FC_BIN_GET_LOCK(bin) (&((GstFCBin*)(bin))->lock)
#define GST_FC_BIN_LOCK(bin) (g_rec_mutex_lock (GST_FC_BIN_GET_LOCK(bin)))
#define GST_FC_BIN_UNLOCK(bin) (g_rec_mutex_unlock (GST_FC_BIN_GET_LOCK(bin)))
typedef struct _GstFCSelect GstFCSelect;
typedef struct _GstFCBin GstFCBin;
typedef struct _GstFCBinClass GstFCBinClass;

enum
{
  GST_FC_BIN_STREAM_AUDIO = 0,
  GST_FC_BIN_STREAM_VIDEO,
  GST_FC_BIN_STREAM_TEXT,
  GST_FC_BIN_STREAM_LAST
};

struct _GstFCSelect
{
  const gchar *media_list[8];   /* the media types for the selector */
  GstElement *selector;         /* the selector */
  GstLpSinkType type;

  GPtrArray *channels;
  GstPad *srcpad;
  //GstPad *sinkpad;
};

struct _GstFCBin
{
  GstBin parent;

  GRecMutex lock;

  gint current_video;           /* the currently selected stream */
  gint current_audio;           /* the currently selected stream */
  gint current_text;            /* the currently selected stream */

  GstFCSelect select[GST_FC_BIN_STREAM_LAST];

  GstPad *audio_srcpad;
  GstPad *video_srcpad;
  GstPad *text_srcpad;

  gulong audio_block_id;
  gulong video_block_id;
  gulong text_block_id;

  gint nb_streams;
  gint nb_current_stream;
  gint nb_video;
  gint nb_audio;
  gint nb_text;
  GPtrArray *video_pads;
  GPtrArray *audio_pads;
  GPtrArray *text_pads;

  GHashTable *caps_pairs;
};

struct _GstFCBinClass
{
  GstBinClass parent_class;

  /* notify lpbin that the tags of audio/video/text streams changed */
  void (*audio_tags_changed) (GstFCBin * fcbin, gint stream);
  void (*video_tags_changed) (GstFCBin * fcbin, gint stream);
  void (*text_tags_changed) (GstFCBin * fcbin, gint stream);

  void (*element_configured) (GstFCBin * fcbin, gint type, GstPad * sinkpad,
      GstPad * srcpad, gchar * stream_id);

  gboolean *(*unblock_sinkpads) (GstFCBin * fcbin);
};

GType gst_fc_bin_get_type (void);

G_END_DECLS
#endif // __GST_FC_BIN_H__
