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


#ifndef __GST_LP_BIN_H__
#define __GST_LP_BIN_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_LP_BIN (gst_lp_bin_get_type())
#define GST_LP_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_LP_BIN,GstLpBin))
#define GST_LP_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_LP_BIN,GstLpBinClass))
#define GST_IS_LP_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_LP_BIN))
#define GST_IS_LP_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_LP_BIN))
#define GST_LP_BIN_CAST(obj) ((GstLpBin *)(obj))
#define GST_LP_BIN_GET_LOCK(bin) (&((GstLpBin*)(bin))->lock)
#define GST_LP_BIN_LOCK(bin) (g_rec_mutex_lock (GST_LP_BIN_GET_LOCK(bin)))
#define GST_LP_BIN_UNLOCK(bin) (g_rec_mutex_unlock (GST_LP_BIN_GET_LOCK(bin)))
#define REMOVE_SIGNAL(obj,id)                             \
if (id) {                                                 \
  if (g_signal_handler_is_connected (G_OBJECT (obj), id)) \
    g_signal_handler_disconnect (G_OBJECT (obj), id);     \
  id = 0;                                                 \
}
typedef struct _GstLpBin GstLpBin;
typedef struct _GstLpBinClass GstLpBinClass;

struct _GstLpBin
{
  GstPipeline parent;

  GRecMutex lock;               /* to protect group switching */

  GstElement *uridecodebin;
  GstElement *fcbin;
  GstElement *lpsink;
  /* the last activated source */
  GstElement *source;
  GstFlowReturn ret;

  guint64 buffer_duration;      /* When buffering, the max buffer duration (ns) */
  guint buffer_size;            /* When buffering, the max buffer size (bytes) */

  GstBus *bus;

  gchar *uri;
  gulong bus_msg_cb_id;
  gulong pad_added_id;
  gulong pad_removed_id;
  gulong no_more_pads_id;
  gulong source_element_id;
  gulong drained_id;
  gulong unknown_type_id;
  gulong autoplug_factories_id;
  gulong autoplug_continue_id;
  gulong caps_video_id;
  gulong audio_tags_changed_id;
  gulong video_tags_changed_id;
  gulong text_tags_changed_id;

  guint naudio;
  guint nvideo;
  guint ntext;

  GstPad *video_pad;            /* contains srcpads of input-selectors for connect with lpsink */
  GstPad *audio_pad;
  GstPad *text_pad;

  GstElement *audio_sink;       /* configured audio sink, or NULL  */
  GstElement *video_sink;       /* configured video sink, or NULL */

  GMutex elements_lock;
  guint32 elements_cookie;
  GList *elements;              /* factories we can use for selecting elements */

  gboolean thumbnail_mode;
  gboolean use_buffering;

  guint video_resource;
  guint audio_resource;

  /* This table contains property name and value */
  GHashTable *property_pairs;

  /* this string contains all of elements from decodebin */
  gchar *elements_str;

  GPtrArray *video_channels;    /* links to input-selector pads */
  GPtrArray *audio_channels;    /* links to input-selector pads */
  GPtrArray *text_channels;     /* links to input-selector pads */

  gboolean audio_only;          /* Whether or not audio and video stream exist */

  /* Whether or not audio, video and text chain is linked */
  gboolean *audio_chain_linked;
  gboolean *video_chain_linked;
  gboolean *text_chain_linked;
};

struct _GstLpBinClass
{
  GstPipelineClass parent_class;

  /* notify app that the current uri finished decoding and it is possible to
   * queue a new one for gapless playback */
  void (*about_to_finish) (GstLpBin * lpbin);

  /* signal fired to know if we continue trying to decode the given caps */
    gboolean (*autoplug_continue) (GstElement * element, GstPad * pad,
      GstCaps * caps);

  /* signal fired to get a list of factories to try to autoplug */
  GValueArray *(*autoplug_factories) (GstElement * element, GstPad * pad,
      GstCaps * caps);

  /* get the thumbnail image */
  GstBuffer *(*retrieve_thumbnail) (GstLpBin * lpbin, gint width, gint height,
      gchar * format);

  /* notify app that the tags of audio/video/text streams changed */
  void (*video_tags_changed) (GstLpBin * lpbin, gint stream);
  void (*audio_tags_changed) (GstLpBin * lpbin, gint stream);
  void (*text_tags_changed) (GstLpBin * lpbin, gint stream);

  /* get audio/video/text tags for a stream */
  GstTagList *(*get_video_tags) (GstLpBin * lpbin, gint stream);
  GstTagList *(*get_audio_tags) (GstLpBin * lpbin, gint stream);
  GstTagList *(*get_text_tags) (GstLpBin * lpbin, gint stream);

  /* get audio/video pad for a stream */
  GstPad *(*get_video_pad) (GstLpBin * lpbin, gint stream);
  GstPad *(*get_audio_pad) (GstLpBin * lpbin, gint stream);
  GstPad *(*get_text_pad) (GstLpBin * lpbin, gint stream);

  /* get the caps of video sink element's sinkpad */
  GstStructure *(*caps_video) (GstLpBin * lpbin);
};

enum
{
  LPBIN_STREAM_AUDIO = 0,
  LPBIN_STREAM_VIDEO,
  LPBIN_STREAM_TEXT,
  LPBIN_STREAM_LAST
};

GType gst_lp_bin_get_type (void);

G_END_DECLS
#endif // __GST_LP_BIN_H__
