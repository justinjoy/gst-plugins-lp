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
typedef struct _GstSinkChain GstSinkChain;
typedef struct _GstLpSink GstLpSink;
typedef struct _GstLpSinkClass GstLpSinkClass;

struct _GstLpSink
{
  GstBin parent;

  GRecMutex lock;               /* to protect group switching */

  GstElement *video_rfunnel;
  GstElement *audio_rfunnel;
  GstElement *text_rfunnel;

  GstElement *video_sink;
  GstElement *audio_sink;
  GstElement *text_sinkbin;
  GstPad *audio_pad;
  GstPad *video_pad;
  GstPad *text_pad;

  GstFlowReturn ret;

  gboolean thumbnail_mode;
  guint video_resource;
  guint audio_resource;

  gboolean video_multiple_stream;
  gboolean audio_multiple_stream;

  GList *sink_chain_list;
  guint nb_video_bin;
  guint nb_audio_bin;
  guint nb_text_bin;

  gboolean audio_only;
  gboolean need_async_start;
  gboolean async_pending;
};

struct _GstLpSinkClass
{
  GstBinClass parent_class;
};

typedef enum
{
  GST_LP_SINK_TYPE_AUDIO = 0,
  GST_LP_SINK_TYPE_VIDEO = 1,
  GST_LP_SINK_TYPE_TEXT = 2,
  GST_LP_SINK_TYPE_LAST = 9,
  GST_LP_SINK_TYPE_FLUSHING = 10
} GstLpSinkType;

struct _GstSinkChain
{
  GstBin *bin;
  GstLpSink *lpsink;
  GstElement *queue;
  GstElement *sink;
  GstCaps *caps;
  //GstLpSinkType type;
  GstPad *bin_ghostpad;
};

GType gst_lp_sink_get_type (void);

void gst_lp_sink_set_sink (GstLpSink * lpsink, GstLpSinkType type,
    GstElement * sink);
GstElement *gst_lp_sink_get_sink (GstLpSink * lpsink, GstLpSinkType type);
void gst_lp_sink_set_thumbnail_mode (GstLpSink * lpsink,
    gboolean thumbnail_mode);

G_END_DECLS
#endif // __GST_LP_SINK_H__
