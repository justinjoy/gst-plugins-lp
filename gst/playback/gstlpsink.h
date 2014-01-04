/* GStreamer Lightweight Playback Plugins
 *
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *	Author : Jeongseok Kim <jeongseok.kim@lge.com>
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


#ifndef __GST_LP_SINK_H__
#define __GST_LP_SINK_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_LP_SINK (gst_lp_sink_get_type())
#define GST_LP_SINK(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_LP_SINK,GstLpSink))
#define GST_LP_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_LP_SINK,GstLpSinkClass))
#define GST_IS_LP_SINK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_LP_SINK))
#define GST_IS_LP_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_LP_SINK))
#define GST_LP_SINK_CAST(obj) ((GstLpSink*)(obj))
#define GST_LP_SINK_GET_LOCK(bin) (&((GstLpSink*)(bin))->lock)
#define GST_LP_SINK_LOCK(bin) (g_rec_mutex_lock (GST_LP_SINK_GET_LOCK(bin)))
#define GST_LP_SINK_UNLOCK(bin) (g_rec_mutex_unlock (GST_LP_SINK_GET_LOCK(bin)))
#define GST_SINK_CHAIN(c) ((GstSinkChain *)(c))
#define GST_AV_SINK_CHAIN(c) ((GstAVSinkChain *)(c))
typedef struct _GstSinkChain GstSinkChain;
typedef struct _GstAVSinkChain GstAVSinkChain;
typedef struct _GstLpSink GstLpSink;
typedef struct _GstLpSinkClass GstLpSinkClass;

struct _GstLpSink
{
  GstBin parent;

  GRecMutex lock;               /* to protect group switching */

  GstAVSinkChain *avchain;

  GList *video_chains;
  GList *audio_chains;
  GList *text_chains;

  GstElement *video_streamid_demux;
  GstElement *audio_streamid_demux;
  GstElement *text_streamid_demux;

  GstElement *video_sink;
  GstElement *audio_sink;
  GstElement *text_sinkbin;
  GstPad *audio_pad;
  GstPad *video_pad;
  GstPad *text_pad;

  gulong audio_block_id;
  gulong video_block_id;
  gulong text_block_id;

  gboolean audio_pad_blocked;
  gboolean video_pad_blocked;
  gboolean text_pad_blocked;

  guint32 pending_blocked_pads;

  GstFlowReturn ret;

  gboolean thumbnail_mode;
  gint interleaving_type;
  guint video_resource;
  guint audio_resource;

  gboolean video_multiple_stream;
  gboolean audio_multiple_stream;

  guint nb_video_bin;
  guint nb_audio_bin;
  guint nb_text_bin;

  gboolean audio_only;
  gboolean need_async_start;
  gboolean async_pending;

  gdouble rate;
};

struct _GstLpSinkClass
{
  GstBinClass parent_class;
  void (*pad_blocked) (GstLpSink * lpsink, gchar * steram_id, gboolean blocked);
  gboolean *(*unblock_sinkpads) (GstLpSink * lpsink);
};

typedef enum
{
  GST_LP_SINK_TYPE_AUDIO = 0,
  GST_LP_SINK_TYPE_VIDEO = 1,
  GST_LP_SINK_TYPE_TEXT = 2,
  GST_LP_SINK_TYPE_AV = 3,
  GST_LP_SINK_TYPE_LAST = 9,
  GST_LP_SINK_TYPE_FLUSHING = 10
} GstLpSinkType;

struct _GstSinkChain
{
  GstBin *bin;
  GstLpSink *lpsink;
  GstElement *queue;
  GstElement *sink;
  GstLpSinkType type;
  GstPad *bin_ghostpad;

  gulong block_id;
  GstPad *peer_srcpad_queue;
  gboolean peer_srcpad_blocked;
  GstCaps *caps;

};

struct _GstAVSinkChain
{
  GstSinkChain sinkchain;
  GstElement *video_queue;
  GstElement *audio_queue;
  GstPad *video_ghostpad;
  GstPad *audio_ghostpad;

  GstPad *video_pad;            /* store audio/video sink's sinkpad */
  GstPad *audio_pad;

  gulong video_block_id;
  gulong audio_block_id;
  GstPad *video_peer_srcpad_queue;
  GstPad *audio_peer_srcpad_queue;
  gboolean video_peer_srcpad_blocked;
  gboolean audio_peer_srcpad_blocked;
  GstCaps *video_caps;
  GstCaps *audio_caps;
};

GType gst_lp_sink_get_type (void);

void gst_lp_sink_set_sink (GstLpSink * lpsink, GstLpSinkType type,
    GstElement * sink);
GstElement *gst_lp_sink_get_sink (GstLpSink * lpsink, GstLpSinkType type);
void gst_lp_sink_set_thumbnail_mode (GstLpSink * lpsink,
    gboolean thumbnail_mode);
void gst_lp_sink_set_interleaving_type (GstLpSink * lpsink,
    gint interleaving_type);

gboolean gst_lp_sink_reconfigure (GstLpSink * lpsink);
void gst_lp_sink_set_all_pads_blocked (GstLpSink * lpsink);

G_END_DECLS
#endif // __GST_LP_SINK_H__
