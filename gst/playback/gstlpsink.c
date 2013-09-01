/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Jeongseok Kim <jeongseok.kim@lge.com>
 *	         Wonchul Lee <wonchul86.lee@lge.com>
 *	         Hoonhee Lee <hoonhee.lee@lge.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstlpsink.h"

GST_DEBUG_CATEGORY_STATIC (gst_lp_sink_debug);
#define GST_CAT_DEFAULT gst_lp_sink_debug

static GstStaticPadTemplate audiotemplate =
GST_STATIC_PAD_TEMPLATE ("audio_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate videotemplate =
GST_STATIC_PAD_TEMPLATE ("video_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate texttemplate = GST_STATIC_PAD_TEMPLATE ("text_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

/* props */
enum
{
  PROP_0,
  PROP_VIDEO_SINK,
  PROP_AUDIO_SINK,
  PROP_VIDEO_RESOURCE,
  PROP_AUDIO_RESOURCE,
  PROP_LAST
};

#define DEFAULT_THUMBNAIL_MODE FALSE

static void gst_lp_sink_dispose (GObject * object);
static void gst_lp_sink_finalize (GObject * object);
static void gst_lp_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_lp_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);

static GstPad *gst_lp_sink_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_lp_sink_release_request_pad (GstElement * element,
    GstPad * pad);
static gboolean gst_lp_sink_send_event (GstElement * element, GstEvent * event);
static gboolean gst_lp_sink_send_event_to_sink (GstLpSink * lpsink,
    GstEvent * event);
static GstStateChangeReturn gst_lp_sink_change_state (GstElement * element,
    GstStateChange transition);

static void gst_lp_sink_handle_message (GstBin * bin, GstMessage * message);

void gst_lp_sink_set_sink (GstLpSink * lpsink, GstLpSinkType type,
    GstElement * sink);
GstElement *gst_lp_sink_get_sink (GstLpSink * lpsink, GstLpSinkType type);
static GstFlowReturn gst_lp_sink_new_sample (GstElement * sink);
static void src_pad_added_cb (GstElement * osel, GstPad * pad,
    GstLpSink * lpsink);
static gboolean gst_lp_sink_do_configure_sync (GstLpSink * lpsink,
    GstPad * pad);
static GstPad *gst_lp_sink_do_configure_sink_bin (GstLpSink * lpsink,
    GstPad * pad, GstSinkChain * sink_chain);
static void gst_lp_sink_do_configure_sink_element (GstLpSink * lpsink,
    gchar * bin_name);
static gboolean gst_lp_sink_osel_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_lp_sink_bin_chain_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static void
_do_init (GType type)
{
  // TODO
}

G_DEFINE_TYPE_WITH_CODE (GstLpSink, gst_lp_sink, GST_TYPE_BIN,
    _do_init (g_define_type_id));

#define parent_class gst_lp_sink_parent_class

static void
gst_lp_sink_class_init (GstLpSinkClass * klass)
{
  GObjectClass *gobject_klass;
  GstElementClass *gstelement_klass;
  GstBinClass *gstbin_klass;

  gobject_klass = (GObjectClass *) klass;
  gstelement_klass = (GstElementClass *) klass;
  gstbin_klass = (GstBinClass *) klass;

  gobject_klass->dispose = gst_lp_sink_dispose;
  gobject_klass->finalize = gst_lp_sink_finalize;
  gobject_klass->set_property = gst_lp_sink_set_property;
  gobject_klass->get_property = gst_lp_sink_get_property;

  /**
   * GstPlaySink:video-sink:
   *
   * Set the used video sink element. NULL will use the default sink. playsink
   * must be in %GST_STATE_NULL
   *
   */
  g_object_class_install_property (gobject_klass, PROP_VIDEO_SINK,
      g_param_spec_object ("video-sink", "Video Sink",
          "the video output element to use (NULL = default sink)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstPlaySink:audio-sink:
   *
   * Set the used audio sink element. NULL will use the default sink. playsink
   * must be in %GST_STATE_NULL
   *
   */
  g_object_class_install_property (gobject_klass, PROP_AUDIO_SINK,
      g_param_spec_object ("audio-sink", "Audio Sink",
          "the audio output element to use (NULL = default sink)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_VIDEO_RESOURCE,
      g_param_spec_uint ("video-resource", "Acquired video resource",
          "Acquired video resource", 0, 2, 0,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_AUDIO_RESOURCE,
      g_param_spec_uint ("audio-resource", "Acquired audio resource",
          "Acquired audio resource (the most significant bit - 0: ADEC, 1: MIX / the remains - channel number)",
          0, G_MAXUINT, 0, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));


  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&audiotemplate));
  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&videotemplate));
  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&texttemplate));
  gst_element_class_set_static_metadata (gstelement_klass,
      "Lightweight Player Sink", "Lightweight/Bin/Sink",
      "Convenience sink for multiple streams in a restricted system",
      "Jeongseok Kim <jeongseok.kim@lge.com>");

  gstelement_klass->change_state = GST_DEBUG_FUNCPTR (gst_lp_sink_change_state);

  gstelement_klass->send_event = GST_DEBUG_FUNCPTR (gst_lp_sink_send_event);
  gstelement_klass->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_lp_sink_request_new_pad);
  gstelement_klass->release_pad =
      GST_DEBUG_FUNCPTR (gst_lp_sink_release_request_pad);

  gstbin_klass->handle_message = GST_DEBUG_FUNCPTR (gst_lp_sink_handle_message);
}

static void
gst_lp_sink_init (GstLpSink * lpsink)
{
  GST_DEBUG_CATEGORY_INIT (gst_lp_sink_debug, "lpsink", 0,
      "Lightweight Play Sink");
  g_rec_mutex_init (&lpsink->lock);
  GST_OBJECT_FLAG_SET (lpsink, GST_ELEMENT_FLAG_SINK);

  lpsink->audio_sink = NULL;
  lpsink->video_sink = NULL;
  lpsink->text_sink = NULL;
  lpsink->video_pad = NULL;
  lpsink->audio_pad = NULL;
  lpsink->text_pad = NULL;
  lpsink->thumbnail_mode = DEFAULT_THUMBNAIL_MODE;

  lpsink->video_resource = 0;
  lpsink->audio_resource = 0;

  lpsink->video_osel = NULL;
  lpsink->audio_osel = NULL;
  lpsink->text_osel = NULL;

  lpsink->stream_synchronizer = NULL;

  lpsink->video_multiple_stream = FALSE;
  lpsink->audio_multiple_stream = FALSE;

  lpsink->sink_chain_list = NULL;

  lpsink->nb_video_bin = 0;
  lpsink->nb_audio_bin = 0;
  lpsink->nb_text_bin = 0;
}

static void
gst_lp_sink_dispose (GObject * obj)
{
  GstLpSink *lpsink;
  lpsink = GST_LP_SINK (obj);

  if (lpsink->audio_sink != NULL) {
    //gst_element_set_state (lpsink->audio_sink, GST_STATE_NULL);
    //gst_object_unref (lpsink->audio_sink);
    lpsink->audio_sink = NULL;
  }
  if (lpsink->video_sink != NULL) {
    //gst_element_set_state (lpsink->video_sink, GST_STATE_NULL);
    //gst_object_unref (lpsink->video_sink);
    lpsink->video_sink = NULL;
  }
  if (lpsink->text_sink != NULL) {
    //gst_element_set_state (lpsink->text_sink, GST_STATE_NULL);
    //gst_object_unref (lpsink->text_sink);
    lpsink->text_sink = NULL;
  }
  if (lpsink->audio_pad) {
    gst_object_unref (lpsink->audio_pad);
    lpsink->audio_pad = NULL;
  }
  if (lpsink->video_pad) {
    gst_object_unref (lpsink->video_pad);
    lpsink->video_pad = NULL;
  }
  if (lpsink->text_pad) {
    gst_object_unref (lpsink->text_pad);
    lpsink->text_pad = NULL;
  }
  if (lpsink->video_osel) {
    //gst_object_unref (lpsink->video_osel);
    lpsink->video_osel = NULL;
  }
  if (lpsink->audio_osel) {
    //gst_object_unref (lpsink->audio_osel);
    lpsink->audio_osel = NULL;
  }
  if (lpsink->text_osel) {
    //gst_object_unref (lpsink->text_osel);
    lpsink->text_osel = NULL;
  }
  if (lpsink->stream_synchronizer) {
    //gst_object_unref (lpsink->stream_synchronizer);
    lpsink->stream_synchronizer = NULL;
  }
  //g_list_foreach (lpsink->sink_chain_list, (GFunc) gst_object_unref, NULL);
  g_list_free (lpsink->sink_chain_list);
  lpsink->sink_chain_list = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gst_lp_sink_finalize (GObject * obj)
{
  GstLpSink *lpsink;

  lpsink = GST_LP_SINK (obj);

  g_rec_mutex_clear (&lpsink->lock);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

void
gst_lp_sink_set_thumbnail_mode (GstLpSink * lpsink, gboolean thumbnail_mode)
{
  GST_DEBUG_OBJECT (lpsink, "set thumbnail mode as %d", thumbnail_mode);
  lpsink->thumbnail_mode = thumbnail_mode;
}

void
gst_lp_sink_set_multiple_stream (GstLpSink * lpsink, gchar * type,
    gboolean multiple_stream)
{
  GST_DEBUG_OBJECT (lpsink,
      "gst_lp_sink_set_multiple_stream : type = %s, multiple_stream = %d", type,
      multiple_stream);

  GST_LP_SINK_LOCK (lpsink);
  if (!strcmp (type, "audio")) {
    lpsink->audio_multiple_stream = multiple_stream;
  } else if (!strcmp (type, "video")) {
    lpsink->video_multiple_stream = multiple_stream;
  }
  GST_LP_SINK_UNLOCK (lpsink);
}

static void
gst_lp_sink_do_configure_sink_element (GstLpSink * lpsink, gchar * bin_name)
{
  GST_DEBUG_OBJECT (lpsink,
      "gst_lp_sink_configure_sink_element : bin_name = %s", bin_name);
  GstSinkChain *sink_chain = NULL;
  GstElement *bin = NULL;
  GstElement *queue = NULL;
  GstElement *sink_element = NULL;
  GstPad *queue_srcpad = NULL;
  GstPad *sinkpad = NULL;
  GstLpSinkType type = -1;

  GST_LP_SINK_LOCK (lpsink);
  if (lpsink->sink_chain_list) {
    GList *walk = lpsink->sink_chain_list;

    while (walk) {
      sink_chain = (GstSinkChain *) walk->data;
      bin = GST_ELEMENT (sink_chain->bin);
      if (strstr (bin_name, gst_element_get_name (bin))) {
        type = sink_chain->type;
        queue = sink_chain->queue;
        sink_element = sink_chain->sink;
        break;
      }
      sink_chain = NULL;
      bin = NULL;
      walk = g_list_next (walk);
    }
  }

  queue_srcpad = gst_element_get_static_pad (queue, "src");
  sinkpad = gst_element_get_static_pad (sink_element, "sink");
  if (gst_pad_link_full (queue_srcpad, sinkpad,
          GST_PAD_LINK_CHECK_NOTHING) != GST_PAD_LINK_OK) {
    GST_INFO_OBJECT (sinkpad, "Failed to set target");
    if (type == GST_LP_SINK_TYPE_AUDIO) {
      gst_object_unref (sinkpad);

      GST_INFO_OBJECT (sinkpad, "A fakesink will be deployed for audio sink.");

      gst_bin_remove (GST_BIN_CAST (bin), sink_element);
      sink_element = gst_element_factory_make ("fakesink", NULL);

      gst_bin_add (GST_BIN_CAST (bin), sink_element);
      gst_element_set_state (sink_element, GST_STATE_PAUSED);

      sinkpad = gst_element_get_static_pad (sink_element, "sink");
      gst_pad_link_full (queue_srcpad, sinkpad, GST_PAD_LINK_CHECK_NOTHING);
    }
  }
  if (type == GST_LP_SINK_TYPE_AUDIO)
    lpsink->audio_sink = sink_element;
  else if (type == GST_LP_SINK_TYPE_VIDEO)
    lpsink->video_sink = sink_element;
  else
    lpsink->text_sink = sink_element;

  gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PLAYING);
  GST_LP_SINK_UNLOCK (lpsink);
}

static gboolean
gst_lp_sink_osel_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = TRUE;
  GstElement *osel = NULL;
  GstLpSink *lpsink = NULL;
  GstPad *srcpad = NULL;
  GstCaps *outcaps = NULL;

  osel = GST_ELEMENT (parent);
  lpsink = GST_ELEMENT_PARENT (osel);
  srcpad = gst_element_get_static_pad (osel, "src_0");

  GST_DEBUG_OBJECT (lpsink, "gst_lp_sink_osel_sink_event : event = %s",
      gst_event_type_get_name (GST_EVENT_TYPE (event)));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      gst_event_parse_caps (event, &caps);
      outcaps = gst_caps_copy (caps);
      gst_pad_set_caps (srcpad, outcaps);
      if (gst_pad_is_linked (srcpad) == FALSE)
        gst_lp_sink_do_configure_sync (lpsink, srcpad);
      res = gst_pad_event_default (pad, parent, event);
      gst_event_unref (event);
      break;
    }
    default:
    {
      res = gst_pad_event_default (pad, parent, event);
      break;
    }
  }

  if (outcaps)
    gst_caps_unref (outcaps);

  return res;
}

static gboolean
gst_lp_sink_bin_chain_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = TRUE;
  GstElement *bin = NULL;
  GstLpSink *lpsink = NULL;

  bin = GST_ELEMENT (parent);
  lpsink = GST_ELEMENT_PARENT (bin);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      gst_event_parse_caps (event, &caps);
      gst_lp_sink_do_configure_sink_element (lpsink,
          gst_element_get_name (bin));
      res = gst_pad_event_default (pad, parent, event);
      gst_event_unref (event);
      break;
    }
    default:
    {
      res = gst_pad_event_default (pad, parent, event);
      break;
    }
  }

  return res;
}

static gboolean
gst_lp_sink_do_configure_sync (GstLpSink * lpsink, GstPad * pad)
{
  gboolean ret = TRUE;

  GstSinkChain *sink_chain = NULL;
  GstPad *bin_sinkpad;
  GstIterator *it;
  GValue item = G_VALUE_INIT;

  GST_LP_SINK_LOCK (lpsink);
  if (!lpsink->stream_synchronizer) {
    lpsink->stream_synchronizer =
        gst_element_factory_make ("streamsynchronizer", NULL);
    gst_bin_add (GST_BIN_CAST (lpsink), lpsink->stream_synchronizer);
    gst_element_set_state (lpsink->stream_synchronizer, GST_STATE_PAUSED);
  }

  sink_chain = g_slice_alloc0 (sizeof (GstSinkChain));

  sink_chain->sinkpad_stream_synchronizer =
      gst_element_get_request_pad (lpsink->stream_synchronizer, "sink_%u");
  it = gst_pad_iterate_internal_links (sink_chain->sinkpad_stream_synchronizer);
  g_assert (it);
  gst_iterator_next (it, &item);
  sink_chain->srcpad_stream_synchronizer = g_value_dup_object (&item);
  g_value_unset (&item);
  g_assert (sink_chain->srcpad_stream_synchronizer);
  gst_iterator_free (it);

  bin_sinkpad = gst_lp_sink_do_configure_sink_bin (lpsink, pad, sink_chain);
  gst_pad_link_full (pad, sink_chain->sinkpad_stream_synchronizer,
      GST_PAD_LINK_CHECK_NOTHING);
  gst_pad_link_full (sink_chain->srcpad_stream_synchronizer, bin_sinkpad,
      GST_PAD_LINK_CHECK_NOTHING);
  GST_LP_SINK_UNLOCK (lpsink);

  return ret;
}

static GstPad *
gst_lp_sink_do_configure_sink_bin (GstLpSink * lpsink, GstPad * pad,
    GstSinkChain * sink_chain)
{
  GST_DEBUG_OBJECT (lpsink,
      "gst_lp_sink_do_configure_sink_bin : pad = %s, caps = %s",
      gst_pad_get_name (pad),
      gst_caps_to_string (gst_pad_get_current_caps (pad)));

  gchar *bin_name = NULL;
  GstCaps *caps = NULL;
  GstPad *ghostpad = NULL;
  GstBin *bin = NULL;
  GstElement *queue = NULL;
  GstPad *queue_sinkpad = NULL;
  guint type = -1;

  gchar *sink_name = NULL;
  GstElement *sink_element = NULL;

  caps = gst_pad_get_current_caps (pad);

  if (g_str_has_prefix (gst_caps_to_string (caps), "audio/")) {
    type = GST_LP_SINK_TYPE_AUDIO;
    bin_name = g_strdup_printf ("abin%d", lpsink->nb_audio_bin++);
    sink_name = "adecsink";
  } else if (g_str_has_prefix (gst_caps_to_string (caps), "video/")) {
    type = GST_LP_SINK_TYPE_VIDEO;
    bin_name = g_strdup_printf ("vbin%d", lpsink->nb_video_bin++);
    sink_name = "vdecsink";
  } else {
    type = GST_LP_SINK_TYPE_TEXT;
    bin_name = g_strdup_printf ("tbin%d", lpsink->nb_text_bin++);
    sink_name = "appsink";
  }

  GST_LP_SINK_LOCK (lpsink);
  bin = gst_bin_new (bin_name);
  gst_bin_add (GST_BIN_CAST (lpsink), bin);
  gst_element_set_state (GST_ELEMENT_CAST (bin), GST_STATE_PAUSED);

  queue = gst_element_factory_make ("queue", NULL);
  if (type = GST_LP_SINK_TYPE_VIDEO)
    g_object_set (G_OBJECT (queue), "max-size-buffers", 3, "max-size-bytes", 0,
        "max-size-time", (gint64) 0, "silent", TRUE, NULL);
  gst_bin_add (GST_BIN_CAST (bin), queue);
  gst_element_set_state (queue, GST_STATE_PAUSED);

  sink_element = gst_element_factory_make (sink_name, NULL);

  if (!sink_element) {
    goto beach;
  }

  /* FIXME: this code is only for LG SIC sink */
  GST_DEBUG_OBJECT (lpsink, "Setting hardware resource");
  switch (type) {
    case GST_LP_SINK_TYPE_AUDIO:
      if (lpsink->audio_resource & (1 << 31)) {
        g_object_set (sink_element, "mixer", TRUE, NULL);
      } else {
        g_object_set (sink_element, "mixer", FALSE, NULL);
      }

      g_object_set (sink_element, "index",
          (lpsink->audio_resource & ~(1 << 31)), NULL);
      GST_DEBUG_OBJECT (sink_element, "Request to acquire [%s:%x]",
          (lpsink->audio_resource & (1 << 31)) ? "MIXER" : "ADEC",
          (lpsink->audio_resource & ~(1 << 31)));
      break;
    case GST_LP_SINK_TYPE_VIDEO:
      GST_DEBUG_OBJECT (sink_element,
          "Passing vdec ch property[%x] into vdecsink", lpsink->video_resource);
      g_object_set (sink_element, "vdec-ch", lpsink->video_resource, NULL);
      break;
    case GST_LP_SINK_TYPE_TEXT:
      g_object_set (sink_element, "emit-signals", TRUE, NULL);
      g_signal_connect (sink_element, "new-sample",
          G_CALLBACK (gst_lp_sink_new_sample), NULL);
      break;
  }

  gst_bin_add (GST_BIN_CAST (bin), sink_element);
  gst_element_set_state (sink_element, GST_STATE_PAUSED);

  ghostpad = gst_ghost_pad_new_no_target ("sink", GST_PAD_SINK);
  gst_pad_set_event_function (ghostpad,
      GST_DEBUG_FUNCPTR (gst_lp_sink_bin_chain_event));
  gst_pad_set_active (ghostpad, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (bin), ghostpad);

  queue_sinkpad = gst_element_get_static_pad (queue, "sink");
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (ghostpad), queue_sinkpad);

  sink_chain->bin = bin;
  sink_chain->queue = queue;
  sink_chain->caps = caps;
  sink_chain->type = type;
  sink_chain->sink = sink_element;
  lpsink->sink_chain_list =
      g_list_append (lpsink->sink_chain_list, (GstSinkChain *) sink_chain);

  if (bin_name)
    g_free (bin_name);

beach:
  GST_LP_SINK_UNLOCK (lpsink);
  return ghostpad;
}

static void
src_pad_added_cb (GstElement * osel, GstPad * pad, GstLpSink * lpsink)
{
  GST_DEBUG_OBJECT (lpsink, "src_pad_added_cb : osel = %s, pad = %s, caps = %s",
      gst_element_get_name (osel), gst_pad_get_name (pad),
      gst_caps_to_string (gst_pad_get_current_caps (pad)));

  gst_lp_sink_do_configure_sync (lpsink, pad);
}

/**
 * gst_lp_sink_request_pad
 * @lpsink: a #GstLpSink
 * @type: a #GstLpSinkType
 *
 * Create or return a pad of @type.
 *
 * Returns: a #GstPad of @type or %NULL when the pad could not be created.
 */
GstPad *
gst_lp_sink_request_pad (GstLpSink * lpsink, GstLpSinkType type)
{
  GstPad *res = NULL;
  GstPad *sinkpad, *srcpad;
  const gchar *pad_name;
  GstElement *osel;

  GST_LP_SINK_LOCK (lpsink);

  switch (type) {
    case GST_LP_SINK_TYPE_AUDIO:
      pad_name = "audio_sink";
      lpsink->audio_osel = gst_element_factory_make ("output-selector", NULL);
      osel = gst_object_ref (lpsink->audio_osel);
      break;
    case GST_LP_SINK_TYPE_VIDEO:
      pad_name = "video_sink";
      lpsink->video_osel = gst_element_factory_make ("output-selector", NULL);
      osel = gst_object_ref (lpsink->video_osel);
      break;
    case GST_LP_SINK_TYPE_TEXT:
      pad_name = "text_sink";
      lpsink->text_osel = gst_element_factory_make ("output-selector", NULL);
      osel = gst_object_ref (lpsink->text_osel);
      break;
    default:
      res = NULL;
  }

  if (!osel) {
    goto beach;
  }
  g_object_set (osel, "resend-latest", TRUE, NULL);
  sinkpad = gst_element_get_static_pad (osel, "sink");

  switch (type) {
    case GST_LP_SINK_TYPE_AUDIO:
      if (lpsink->audio_multiple_stream) {
        g_object_set (osel, "reverse-funnel-mode", TRUE, NULL);
        lpsink->src_pad_added_id = g_signal_connect (osel, "src-pad-added",
            G_CALLBACK (src_pad_added_cb), lpsink);
      } else {
        srcpad = gst_element_get_request_pad (osel, "src_%u");
        gst_pad_set_event_function (sinkpad,
            GST_DEBUG_FUNCPTR (gst_lp_sink_osel_sink_event));
      }
      break;
    case GST_LP_SINK_TYPE_VIDEO:
      if (lpsink->video_multiple_stream) {
        g_object_set (osel, "reverse-funnel-mode", TRUE, NULL);
        lpsink->src_pad_added_id = g_signal_connect (osel, "src-pad-added",
            G_CALLBACK (src_pad_added_cb), lpsink);
      } else {
        srcpad = gst_element_get_request_pad (osel, "src_%u");
        gst_pad_set_event_function (sinkpad,
            GST_DEBUG_FUNCPTR (gst_lp_sink_osel_sink_event));
      }
      break;
    case GST_LP_SINK_TYPE_TEXT:
      //if (lpsink->text_multiple_stream) {
      g_object_set (osel, "reverse-funnel-mode", TRUE, NULL);
      lpsink->src_pad_added_id = g_signal_connect (osel, "src-pad-added",
          G_CALLBACK (src_pad_added_cb), lpsink);
      //} else {
      //  gst_element_get_request_pad (osel, "src_%u");
      //}
      break;
  }

  gst_bin_add (GST_BIN_CAST (lpsink), osel);
  gst_element_set_state (osel, GST_STATE_PAUSED);
  res = gst_ghost_pad_new_no_target (pad_name, GST_PAD_SINK);

  if (type == GST_LP_SINK_TYPE_AUDIO) {
    lpsink->audio_pad = res;
  } else if (type == GST_LP_SINK_TYPE_VIDEO) {
    lpsink->video_pad = res;
  } else {
    lpsink->text_pad = res;
  }

  gst_pad_set_active (res, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (lpsink), res);
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (res), sinkpad);

  if (sinkpad)
    gst_object_unref (sinkpad);
  if (srcpad)
    gst_object_unref (srcpad);
  if (osel)
    gst_object_unref (osel);
beach:
  GST_LP_SINK_UNLOCK (lpsink);

  return res;
}

static GstPad *
gst_lp_sink_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name, const GstCaps * caps)
{
  GstLpSink *lpsink;
  const gchar *tplname;
  GstPad *pad;
  GstLpSinkType type;

  g_return_val_if_fail (templ != NULL, NULL);
  GST_DEBUG_OBJECT (element, "name: %s", name);

  lpsink = GST_LP_SINK (element);
  tplname = GST_PAD_TEMPLATE_NAME_TEMPLATE (templ);

  if (!strncmp ("audio_sink", tplname, 10))
    type = GST_LP_SINK_TYPE_AUDIO;
  else if (!strncmp ("video_sink", tplname, 10))
    type = GST_LP_SINK_TYPE_VIDEO;
  else if (!strncmp ("text_sink", tplname, 9))
    type = GST_LP_SINK_TYPE_TEXT;
  else
    goto unknown_template;

  pad = gst_lp_sink_request_pad (lpsink, type);
  return pad;

unknown_template:
  GST_WARNING_OBJECT (element, "Unknown pad template");
  return NULL;
}

void
gst_lp_sink_release_pad (GstLpSink * lpsink, GstPad * pad)
{
  GstPad **res = NULL;
  gboolean untarget = TRUE;

  GST_DEBUG_OBJECT (lpsink, "release pad %" GST_PTR_FORMAT, pad);

  GST_LP_SINK_LOCK (lpsink);
  if (pad == lpsink->video_pad) {
    res = &lpsink->video_pad;
  } else if (pad == lpsink->audio_pad) {
    res = &lpsink->audio_pad;
  } else if (pad == lpsink->video_pad) {
    res = &lpsink->video_pad;
  } else {
    res = &pad;
    untarget = FALSE;
  }

  GST_LP_SINK_UNLOCK (lpsink);
  if (*res) {
    GST_DEBUG_OBJECT (lpsink, "deactivate pad %" GST_PTR_FORMAT, *res);
    // TODO
    gst_pad_set_active (*res, FALSE);
    if (untarget)
      gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (*res), NULL);
    GST_DEBUG_OBJECT (lpsink, "remove pad %" GST_PTR_FORMAT, *res);
    gst_element_remove_pad (GST_ELEMENT_CAST (lpsink), *res);
    *res = NULL;
  }
}

static void
gst_lp_sink_release_request_pad (GstElement * element, GstPad * pad)
{
  GstLpSink *lpsink = GST_LP_SINK (element);

  gst_lp_sink_release_pad (lpsink, pad);
}

static void
gst_lp_sink_handle_message (GstBin * bin, GstMessage * message)
{
//  GstLpSink *lpsink;
//  lpsink = GST_LP_SINK_CAST (bin);

  switch (GST_MESSAGE_TYPE (message)) {
    default:
      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
  }
}

static void
gst_lp_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec)
{
  GstLpSink *lpsink = GST_LP_SINK (object);

  switch (prop_id) {
    case PROP_VIDEO_SINK:
      gst_lp_sink_set_sink (lpsink, GST_LP_SINK_TYPE_VIDEO,
          g_value_get_object (value));
      break;
    case PROP_AUDIO_SINK:
      gst_lp_sink_set_sink (lpsink, GST_LP_SINK_TYPE_AUDIO,
          g_value_get_object (value));
      break;
    case PROP_VIDEO_RESOURCE:
      lpsink->video_resource = g_value_get_uint (value);
      break;
    case PROP_AUDIO_RESOURCE:
      lpsink->audio_resource = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
      break;
  }
}

static void
gst_lp_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec)
{
  GstLpSink *lpsink = GST_LP_SINK (object);
  switch (prop_id) {
    case PROP_VIDEO_SINK:
      g_value_take_object (value, gst_lp_sink_get_sink (lpsink,
              GST_LP_SINK_TYPE_VIDEO));
      break;
    case PROP_AUDIO_SINK:
      g_value_take_object (value, gst_lp_sink_get_sink (lpsink,
              GST_LP_SINK_TYPE_AUDIO));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
      break;
  }
}

/* We only want to send the event to a single sink (overriding GstBin's
 * behaviour), but we want to keep GstPipeline's behaviour - wrapping seek
 * events appropriately. So, this is a messy duplication of code. */
static gboolean
gst_lp_sink_send_event (GstElement * element, GstEvent * event)
{
  gboolean res = FALSE;
  GstEventType event_type = GST_EVENT_TYPE (event);
  GstLpSink *lpsink = GST_LP_SINK (element);

  switch (event_type) {
    case GST_EVENT_SEEK:
      GST_DEBUG_OBJECT (element, "Sending event to a sink");
      res = gst_lp_sink_send_event_to_sink (lpsink, event);
      break;
    default:
      res = GST_ELEMENT_CLASS (parent_class)->send_event (element, event);
      break;
  }
  return res;
}

static gboolean
gst_lp_sink_send_event_to_sink (GstLpSink * lpsink, GstEvent * event)
{
  gboolean res = TRUE;

  if (lpsink->video_sink) {
    gst_event_ref (event);
    if ((res = gst_element_send_event (lpsink->video_sink, event))) {
      GST_DEBUG_OBJECT (lpsink, "Sent event successfully to video sink");
      goto done;
    }
    GST_DEBUG_OBJECT (lpsink, "Event failed when sent to video sink");
  }
  if (lpsink->audio_sink) {
    gst_event_ref (event);
    if ((res = gst_element_send_event (lpsink->audio_sink, event))) {
      GST_DEBUG_OBJECT (lpsink, "Sent event successfully to audio sink");
      goto done;
    }
    GST_DEBUG_OBJECT (lpsink, "Event failed when sent to audio sink");
  }

done:
  gst_event_unref (event);
  return res;
}

static gboolean
add_chain (GstSinkChain * chain, gboolean add)
{
  gst_bin_remove (GST_BIN_CAST (GST_ELEMENT_PARENT (chain->bin)), chain->bin);
  /* we don't want to lose our sink status */
  //GST_OBJECT_FLAG_SET (GST_ELEMENT_PARENT (chain->bin), GST_ELEMENT_FLAG_SINK);

  return TRUE;
}

static void
free_chain (GstSinkChain * chain)
{
  if (chain) {
    if (chain->bin)
      gst_object_unref (chain->bin);
    //g_free (chain);
  }
}

static gboolean
activate_chain (GstSinkChain * chain, gboolean activate)
{
  gst_element_set_state (chain->bin, GST_STATE_NULL);
  return TRUE;
}

static GstStateChangeReturn
gst_lp_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstStateChangeReturn bret;
  GstLpSink *lpsink;
  lpsink = GST_LP_SINK (element);

  switch (transition) {
    default:
      /* all other state changes return SUCCESS by default, this value can be
       * overridden by the result of the children */
      ret = GST_STATE_CHANGE_SUCCESS;
      break;
  }

  /* do the state change of the children */
  bret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /* now look at the result of our children and adjust the return value */
  switch (bret) {
    case GST_STATE_CHANGE_FAILURE:
      /* failure, we stop */
      goto activate_failed;
    case GST_STATE_CHANGE_NO_PREROLL:
      /* some child returned NO_PREROLL. This is strange but we never know. We
       * commit our async state change (if any) and return the NO_PREROLL */
//      do_async_done (playsink);
      ret = bret;
      break;
    case GST_STATE_CHANGE_ASYNC:
      /* some child was async, return this */
      ret = bret;
      break;
    default:
      /* return our previously configured return value */
      break;
  }

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:{
      GST_LP_SINK_LOCK (lpsink);
      if (lpsink->sink_chain_list) {
        GList *walk = lpsink->sink_chain_list;

        while (walk) {
          GstSinkChain *sink_chain = (GstSinkChain *) walk->data;
          gst_element_release_request_pad (GST_ELEMENT_CAST
              (lpsink->stream_synchronizer),
              sink_chain->sinkpad_stream_synchronizer);
          gst_object_unref (sink_chain->sinkpad_stream_synchronizer);
          sink_chain->sinkpad_stream_synchronizer = NULL;
          gst_object_unref (sink_chain->srcpad_stream_synchronizer);
          sink_chain->srcpad_stream_synchronizer = NULL;

          walk = g_list_next (walk);
        }
      }
      GST_LP_SINK_UNLOCK (lpsink);
      break;
    }
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_lp_sink_release_pad (lpsink, lpsink->audio_pad);
      gst_lp_sink_release_pad (lpsink, lpsink->video_pad);
      gst_lp_sink_release_pad (lpsink, lpsink->text_pad);
      if (lpsink->sink_chain_list) {
        GList *walk = lpsink->sink_chain_list;

        while (walk) {
          GstSinkChain *sink_chain = (GstSinkChain *) walk->data;
          activate_chain (sink_chain, FALSE);
          add_chain (sink_chain, FALSE);
          gst_bin_remove (GST_BIN_CAST (sink_chain->bin), sink_chain->sink);
          gst_bin_remove (GST_BIN_CAST (sink_chain->bin), sink_chain->queue);
          if (sink_chain->sink != NULL) {
            gst_element_set_state (sink_chain->sink, GST_STATE_NULL);
            gst_bin_remove (GST_BIN_CAST (sink_chain->bin), sink_chain->sink);
            sink_chain->sink = NULL;
          }

          free_chain ((GstSinkChain *) sink_chain);
          walk = g_list_next (walk);
        }

      }

      /*if (lpsink->audio_sink != NULL) {
         gst_element_set_state (lpsink->audio_sink, GST_STATE_NULL);
         gst_bin_remove (GST_BIN_CAST (lpsink), lpsink->audio_sink);
         //g_assert (lpsink->audio_sink != NULL);
         lpsink->audio_sink = NULL;
         }
         if (lpsink->video_sink != NULL) {
         gst_element_set_state (lpsink->video_sink, GST_STATE_NULL);
         gst_bin_remove (GST_BIN_CAST (lpsink), lpsink->video_sink);
         //g_assert (lpsink->video_sink != NULL);
         lpsink->video_sink = NULL;
         } */
      break;
    default:
      break;
  }
  return ret;
  /* ERRORS */
activate_failed:
  {
    GST_DEBUG_OBJECT (element,
        "element failed to change states -- activation problem?");
    return GST_STATE_CHANGE_FAILURE;
  }
}

void
gst_lp_sink_set_sink (GstLpSink * lpsink, GstLpSinkType type, GstElement * sink)
{
  GstElement **elem = NULL, *old = NULL;

  GST_DEBUG_OBJECT (lpsink, "Setting sink %" GST_PTR_FORMAT " as sink type %d",
      sink, type);

  GST_LP_SINK_LOCK (lpsink);
  switch (type) {
    case GST_LP_SINK_TYPE_AUDIO:
      elem = &lpsink->audio_sink;
      break;
    case GST_LP_SINK_TYPE_VIDEO:
      elem = &lpsink->video_sink;
      break;
    default:
      break;
  }

  if (elem) {
    old = *elem;
    if (sink)
      gst_object_ref (sink);
    *elem = sink;
  }
  GST_LP_SINK_UNLOCK (lpsink);

  if (old) {
    if (old != sink)
      gst_element_set_state (old, GST_STATE_NULL);
    gst_object_unref (old);
  }
}

GstElement *
gst_lp_sink_get_sink (GstLpSink * lpsink, GstLpSinkType type)
{
  GstElement *result = NULL;
  GstElement *elem = NULL;

  GST_LP_SINK_LOCK (lpsink);
  switch (type) {
    case GST_LP_SINK_TYPE_AUDIO:
      elem = lpsink->audio_sink;
      break;
    case GST_LP_SINK_TYPE_VIDEO:
      elem = lpsink->video_sink;
      break;
    default:
      break;
  }

  if (elem)
    result = gst_object_ref (elem);
  GST_LP_SINK_UNLOCK (lpsink);

  return result;
}

static GstFlowReturn
gst_lp_sink_new_sample (GstElement * sink)
{
  GstSample *sample;
  GstBuffer *buffer;
  GstCaps *caps;
  GstStructure *structure;

  GstPad *pad;

  pad = gst_element_get_static_pad (sink, "sink");

  g_signal_emit_by_name (sink, "pull-sample", &sample);

  if (sample) {
    GstSample *out_sample;
    out_sample =
        GST_SAMPLE_CAST (gst_mini_object_copy (GST_MINI_OBJECT_CONST_CAST
            (sample)));

    structure =
        gst_structure_new ("subtitle_data", "sample", GST_TYPE_SAMPLE,
        out_sample, NULL);

    gst_element_post_message (sink,
        gst_message_new_application (GST_OBJECT_CAST (sink), structure));

    gst_sample_unref (sample);
  }

  return GST_FLOW_OK;
}
