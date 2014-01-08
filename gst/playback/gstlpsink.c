/* GStreamer Lightweight Playback Plugins
 *
 * Copyright (C) 2013-2014 LG Electronics, Inc.
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

/* signals */
enum
{
  SIGNAL_PAD_BLOCKED,
  SIGNAL_UNBLOCK_SINKPADS,
  LAST_SIGNAL
};

/* props */
enum
{
  PROP_0,
  PROP_VIDEO_SINK,
  PROP_AUDIO_SINK,
  PROP_VIDEO_RESOURCE,
  PROP_AUDIO_RESOURCE,
  PROP_AUDIO_ONLY,
  PROP_LAST
};

static guint gst_lp_sink_signals[LAST_SIGNAL] = { 0 };

#define DEFAULT_THUMBNAIL_MODE FALSE

#define PENDING_FLAG_SET(lpsink, flagtype) \
  ((lpsink->pending_blocked_pads) |= ( 1 << flagtype))
#define PENDING_FLAG_UNSET(lpsink, flagtype) \
  ((lpsink->pending_blocked_pads) &= ~( 1 << flagtype))
#define PENDING_FLAG_IS_SET(lpsink, flagtype) \
  ((lpsink->pending_blocked_pads) & ( 1 << flagtype))
#define PENDING_VIDEO_BLOCK(lpsink) \
  PENDING_FLAG_IS_SET(lpsink, GST_LP_SINK_TYPE_VIDEO)
#define PENDING_AUDIO_BLOCK(lpsink) \
  PENDING_FLAG_IS_SET(lpsink, GST_LP_SINK_TYPE_AUDIO)
#define PENDING_TEXT_BLOCK(lpsink) \
  PENDING_FLAG_IS_SET(lpsink, GST_LP_SINK_TYPE_TEXT)

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
static gboolean gst_lp_sink_query (GstElement * element, GstQuery * query);
static GstStateChangeReturn gst_lp_sink_change_state (GstElement * element,
    GstStateChange transition);

static void gst_lp_sink_handle_message (GstBin * bin, GstMessage * message);

void gst_lp_sink_set_sink (GstLpSink * lpsink, GstLpSinkType type,
    GstElement * sink);
GstElement *gst_lp_sink_get_sink (GstLpSink * lpsink, GstLpSinkType type);
static void gst_lp_sink_do_reconfigure (GstLpSink * lpsink);
static gboolean add_chain (GstSinkChain * chain, gboolean add);
static gboolean activate_chain (GstSinkChain * chain, gboolean activate);
static void video_set_blocked (GstLpSink * lpsink, gboolean blocked);
static void audio_set_blocked (GstLpSink * lpsink, gboolean blocked);
static void text_set_blocked (GstLpSink * lpsink, gboolean blocked);
static gboolean gst_lp_sink_unblock_sinkpads (GstLpSink * lpsink);
static GstSinkChain *gen_av_chain (GstLpSink * lpsink, GstSinkChain * vchain,
    GstSinkChain * achain, GstPad * video_sink_sinkpad,
    GstPad * audio_sink_sinkpad);

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
   * GstLpSink:video-sink:
   *
   * Set the used video sink element. NULL will use the default sink. lpsink
   * must be in %GST_STATE_NULL
   *
   */
  g_object_class_install_property (gobject_klass, PROP_VIDEO_SINK,
      g_param_spec_object ("video-sink", "Video Sink",
          "the video output element to use (NULL = default sink)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstLpSink:audio-sink:
   *
   * Set the used audio sink element. NULL will use the default sink. lpsink
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

  g_object_class_install_property (gobject_klass, PROP_AUDIO_ONLY,
      g_param_spec_boolean ("audio-only", "Audio only stream",
          "Audio only stream", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstLpSink::pad-blocked
   * @lpsink: a #GstLpSink
   *
   * This signal is emitted after srcpad of queue has been blocked by probe function.
   *
   */
  gst_lp_sink_signals[SIGNAL_PAD_BLOCKED] =
      g_signal_new ("pad-blocked", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstLpSinkClass, pad_blocked), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 2, G_TYPE_STRING,
      G_TYPE_BOOLEAN);

  gst_lp_sink_signals[SIGNAL_UNBLOCK_SINKPADS] =
      g_signal_new ("unblock-sinkpads", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpSinkClass, unblock_sinkpads), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_BOOLEAN, 0);

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
  gstelement_klass->query = GST_DEBUG_FUNCPTR (gst_lp_sink_query);
  gstelement_klass->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_lp_sink_request_new_pad);
  gstelement_klass->release_pad =
      GST_DEBUG_FUNCPTR (gst_lp_sink_release_request_pad);

  klass->unblock_sinkpads = GST_DEBUG_FUNCPTR (gst_lp_sink_unblock_sinkpads);

  gstbin_klass->handle_message = GST_DEBUG_FUNCPTR (gst_lp_sink_handle_message);
}

static void
gst_lp_sink_init (GstLpSink * lpsink)
{
  GST_DEBUG_CATEGORY_INIT (gst_lp_sink_debug, "lpsink", 0,
      "Lightweight Play Sink");

  g_rec_mutex_init (&lpsink->lock);
  lpsink->audio_sink = NULL;
  lpsink->video_sink = NULL;
  lpsink->video_pad = NULL;
  lpsink->audio_pad = NULL;
  lpsink->thumbnail_mode = DEFAULT_THUMBNAIL_MODE;
  lpsink->interleaving_type = 0;

  lpsink->video_resource = 0;
  lpsink->audio_resource = 0;

  lpsink->video_streamid_demux = NULL;
  lpsink->audio_streamid_demux = NULL;
  lpsink->text_streamid_demux = NULL;

  lpsink->video_multiple_stream = FALSE;
  lpsink->audio_multiple_stream = FALSE;

  lpsink->nb_video_bin = 0;
  lpsink->nb_audio_bin = 0;
  lpsink->nb_text_bin = 0;

  lpsink->audio_only = FALSE;

  lpsink->video_chains = NULL;
  lpsink->audio_chains = NULL;
  lpsink->text_chains = NULL;

  lpsink->rate = 0.0;
}

static void
gst_lp_sink_dispose (GObject * obj)
{
  GstLpSink *lpsink;
  lpsink = GST_LP_SINK (obj);

  if (lpsink->audio_sink != NULL) {
    gst_element_set_state (lpsink->audio_sink, GST_STATE_NULL);
    gst_object_unref (lpsink->audio_sink);
    lpsink->audio_sink = NULL;
  }

  if (lpsink->video_sink != NULL) {
    gst_element_set_state (lpsink->video_sink, GST_STATE_NULL);
    gst_object_unref (lpsink->video_sink);
    lpsink->video_sink = NULL;
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
  if (lpsink->video_streamid_demux) {
    //gst_object_unref (lpsink->video_streamid_demux);
    lpsink->video_streamid_demux = NULL;
  }
  if (lpsink->audio_streamid_demux) {
    //gst_object_unref (lpsink->audio_streamid_demux);
    lpsink->audio_streamid_demux = NULL;
  }
  if (lpsink->text_streamid_demux) {
    //gst_object_unref (lpsink->text_streamid_demux);
    lpsink->text_streamid_demux = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gst_lp_sink_finalize (GObject * obj)
{
  GstLpSink *lpsink;

  lpsink = GST_LP_SINK (obj);

  g_rec_mutex_clear (&lpsink->lock);

  if (lpsink->audio_sink) {
    g_object_unref (lpsink->audio_sink);
    lpsink->audio_sink = NULL;
  }
  if (lpsink->video_sink) {
    g_object_unref (lpsink->video_sink);
    lpsink->video_sink = NULL;
  }

  if (lpsink->audio_pad) {
    gst_object_unref (lpsink->audio_pad);
    lpsink->audio_pad = NULL;
  }

  if (lpsink->video_pad) {
    gst_object_unref (lpsink->video_pad);
    lpsink->video_pad = NULL;
  }
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

void
gst_lp_sink_set_thumbnail_mode (GstLpSink * lpsink, gboolean thumbnail_mode)
{
  GST_DEBUG_OBJECT (lpsink, "set thumbnail mode as %d", thumbnail_mode);
  lpsink->thumbnail_mode = thumbnail_mode;
}

void
gst_lp_sink_set_interleaving_type (GstLpSink * lpsink, gint interleaving_type)
{
  GST_DEBUG_OBJECT (lpsink, "set interleaving type as %d", interleaving_type);
  lpsink->interleaving_type = interleaving_type;
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
do_async_start (GstLpSink * lpsink)
{
  GstMessage *message;

  if (!lpsink->need_async_start) {
    GST_INFO_OBJECT (lpsink, "no async_start needed");
    return;
  }

  lpsink->async_pending = TRUE;

  GST_INFO_OBJECT (lpsink, "Sending async_start message");
  message = gst_message_new_async_start (GST_OBJECT_CAST (lpsink));
  GST_BIN_CLASS (gst_lp_sink_parent_class)->handle_message (GST_BIN_CAST
      (lpsink), message);
}

static void
do_async_done (GstLpSink * lpsink)
{
  GstMessage *message;

  if (lpsink->async_pending) {
    GST_INFO_OBJECT (lpsink, "Sending async_done message");
    message =
        gst_message_new_async_done (GST_OBJECT_CAST (lpsink),
        GST_CLOCK_TIME_NONE);
    GST_BIN_CLASS (gst_lp_sink_parent_class)->handle_message (GST_BIN_CAST
        (lpsink), message);

    lpsink->async_pending = FALSE;
  }

  lpsink->need_async_start = FALSE;
}

static GstElement *
try_element (GstLpSink * lpsink, GstElement * element, gboolean unref)
{
  GstStateChangeReturn ret;

  if (element) {
    ret = gst_element_set_state (element, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      GST_DEBUG_OBJECT (lpsink, "failed state change..");
      gst_element_set_state (element, GST_STATE_NULL);
      if (unref)
        gst_object_unref (element);
      element = NULL;
    }
  }
  return element;
}

static GstSinkChain *
gen_audio_chain (GstLpSink * lpsink, GstSinkChain * chain)
{
  gchar *bin_name = NULL;
  GstBin *bin = NULL;
  GstPad *queue_sinkpad = NULL;
  GstElement *sink_element = NULL;
  const gchar *elem_name = NULL;

  chain->lpsink = lpsink;

  if (lpsink->thumbnail_mode)
    elem_name = "fakesink";
  else
    elem_name = "adecsink";

  sink_element = gst_element_factory_make (elem_name, NULL);
  if (sink_element == NULL) {
    gchar *msg =
        g_strdup_printf ("missing element '%s' - check your environment",
        elem_name);
    GST_ELEMENT_ERROR (lpsink, CORE, MISSING_PLUGIN, (msg),
        ("gen_audio_chain fail"));
    g_free (msg);
    return NULL;
  }

  g_object_set (sink_element, "mixer",
      (lpsink->audio_resource & (1 << 31)), NULL);

  g_object_set (sink_element, "index",
      (lpsink->audio_resource & ~(1 << 31)), NULL);
  GST_DEBUG_OBJECT (sink_element, "Request to acquire [%s:%x]",
      (lpsink->audio_resource & (1 << 31)) ? "MIXER" : "ADEC",
      (lpsink->audio_resource & ~(1 << 31)));

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (sink_element),
          "audio-only")) {
    gchar *elem_name = gst_element_get_name (sink_element);

    g_object_set (sink_element, "audio-only", lpsink->audio_only, NULL);
    GST_INFO_OBJECT (lpsink, "Audio-only property is set on %s to %s",
        elem_name, ((lpsink->audio_only) ? "TRUE" : "FALSE"));

    g_free (elem_name);
  }

  chain->sink = try_element (lpsink, sink_element, TRUE);

  //FIXME
  if (chain->sink)
    lpsink->audio_sink = gst_object_ref (chain->sink);

  bin_name = g_strdup_printf ("abin%d", lpsink->nb_audio_bin++);
  chain->bin = GST_BIN_CAST (gst_bin_new (bin_name));
  bin = GST_BIN_CAST (chain->bin);
  gst_object_ref_sink (bin);
  gst_bin_add (bin, chain->sink);

  chain->queue = gst_element_factory_make ("queue", NULL);
  g_object_set (chain->queue, "silent", TRUE, NULL);
  gst_bin_add (bin, chain->queue);

  if (gst_element_link_pads_full (chain->queue, "src", chain->sink, NULL,
          GST_PAD_LINK_CHECK_TEMPLATE_CAPS) == FALSE) {
    GST_INFO_OBJECT (lpsink, "A fakesink will be deployed for audio sink.");

    gst_bin_remove (GST_BIN_CAST (bin), chain->sink);
    sink_element = gst_element_factory_make ("fakesink", NULL);
    chain->sink = try_element (lpsink, sink_element, TRUE);

    gst_bin_add (GST_BIN_CAST (bin), chain->sink);
    gst_element_link_pads_full (chain->queue, "src", chain->sink, NULL,
        GST_PAD_LINK_CHECK_TEMPLATE_CAPS);
  }

  queue_sinkpad = gst_element_get_static_pad (chain->queue, "sink");
  chain->bin_ghostpad = gst_ghost_pad_new ("sink", queue_sinkpad);

  gst_object_unref (queue_sinkpad);
  gst_element_add_pad (GST_ELEMENT_CAST (chain->bin), chain->bin_ghostpad);

  return chain;
}

static GstSinkChain *
gen_video_chain (GstLpSink * lpsink, GstSinkChain * vchain)
{
  gchar *bin_name = NULL;
  GstBin *bin = NULL;
  GstPad *queue_sinkpad = NULL;
  GstElement *sink_element = NULL;
  GstPadTemplate *tmpl;
  GstPad *queue_srcpad = NULL;
  GstPad *video_sink_sinkpad = NULL;
  GstPad *audio_sink_sinkpad = NULL;
  GList *item = NULL;

  vchain->lpsink = lpsink;

  sink_element = gst_element_factory_make ("vdecsink", NULL);
  if (sink_element == NULL) {
    GST_ELEMENT_ERROR (lpsink, CORE, MISSING_PLUGIN,
        ("missing element 'vdecsink' - check your environment"),
        ("gen_video_chain fail"));
    return NULL;
  }
  //TODO: thumbnail case handling
  if (lpsink->thumbnail_mode
      && g_object_class_find_property (G_OBJECT_GET_CLASS (sink_element),
          "thumbnail-mode")) {
    GST_INFO_OBJECT (sink_element, "gen_video_chain : thumbnail mode set as %d",
        lpsink->thumbnail_mode);
    g_object_set (sink_element, "thumbnail-mode", lpsink->thumbnail_mode, NULL);
  }

  if (lpsink->interleaving_type > 0
      && g_object_class_find_property (G_OBJECT_GET_CLASS (sink_element),
          "interleaving-type")) {
    GST_INFO_OBJECT (sink_element,
        "gen_video_chain : interleaving type set as %d",
        lpsink->interleaving_type);
    g_object_set (sink_element, "interleaving-type", lpsink->interleaving_type,
        NULL);
  }

  GST_DEBUG_OBJECT (sink_element,
      "Passing vdec ch property[%x] into vdecsink", lpsink->video_resource);
  g_object_set (sink_element, "vdec-ch", lpsink->video_resource, NULL);

  vchain->sink = try_element (lpsink, sink_element, TRUE);

  tmpl =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (vchain->sink),
      "sink_%d");
  video_sink_sinkpad =
      gst_element_request_pad (vchain->sink, tmpl, NULL, vchain->caps);
  //video_sink_sinkpad = gst_element_get_static_pad (vchain->sink, "sink");
  GST_WARNING_OBJECT (lpsink, "caps = %s", gst_caps_to_string (vchain->caps));

  /* configure av sink chain if audio_sinkpad is exist */
  if (lpsink->audio_chains && (item = g_list_first (lpsink->audio_chains))) {
    GstSinkChain *achain = NULL;

    achain = (GstSinkChain *) item->data;
    audio_sink_sinkpad =
        gst_element_request_pad (vchain->sink, tmpl, NULL, achain->caps);

    if (audio_sink_sinkpad)
      return gen_av_chain (lpsink, vchain, achain, video_sink_sinkpad,
          audio_sink_sinkpad);
  }

  if (vchain->sink)
    lpsink->video_sink = gst_object_ref (vchain->sink);

  bin_name = g_strdup_printf ("vbin%d", lpsink->nb_video_bin++);
  vchain->bin = GST_BIN_CAST (gst_bin_new (bin_name));
  bin = GST_BIN_CAST (vchain->bin);
  gst_object_ref_sink (bin);
  gst_bin_add (bin, vchain->sink);

  vchain->queue = gst_element_factory_make ("queue", NULL);
  g_object_set (G_OBJECT (vchain->queue), "max-size-buffers", 3,
      "max-size-bytes", 0, "max-size-time", (gint64) 0, "silent", TRUE, NULL);
  gst_bin_add (bin, vchain->queue);

  queue_srcpad = gst_element_get_static_pad (vchain->queue, "src");

  if (gst_pad_link_full (queue_srcpad, video_sink_sinkpad,
          GST_PAD_LINK_CHECK_TEMPLATE_CAPS) != GST_PAD_LINK_OK) {
    GST_WARNING_OBJECT (vchain->queue,
        "failed to link video sink behind queue");
    goto link_failed;           // FIXME: vchain->bin should be treated properly
  }

  queue_sinkpad = gst_element_get_static_pad (vchain->queue, "sink");
  vchain->bin_ghostpad = gst_ghost_pad_new ("sink", queue_sinkpad);
  gst_element_add_pad (GST_ELEMENT_CAST (vchain->bin), vchain->bin_ghostpad);

link_failed:
  gst_object_unref (queue_sinkpad);
  gst_object_unref (queue_srcpad);
  gst_object_unref (video_sink_sinkpad);

  return vchain;
}

static GstSinkChain *
gen_av_chain (GstLpSink * lpsink, GstSinkChain * vchain, GstSinkChain * achain,
    GstPad * video_sink_sinkpad, GstPad * audio_sink_sinkpad)
{
  gchar *bin_name = NULL;
  GstBin *bin = NULL;
  GstPad *video_queue_sinkpad;
  GstPad *video_queue_srcpad;
  GstPad *audio_queue_sinkpad;
  GstPad *audio_queue_srcpad;
  GstAVSinkChain *avchain = g_slice_alloc0 (sizeof (GstAVSinkChain));
  GstSinkChain *chain = GST_SINK_CHAIN (avchain);

  chain->lpsink = vchain->lpsink;
  chain->sink = vchain->sink;
  chain->type = GST_LP_SINK_TYPE_AV;

  avchain->video_block_id = vchain->block_id;
  avchain->video_caps = vchain->caps;
  avchain->video_peer_srcpad_blocked = vchain->peer_srcpad_blocked;
  avchain->video_peer_srcpad_queue = vchain->peer_srcpad_queue;

  avchain->audio_block_id = achain->block_id;
  avchain->audio_caps = achain->caps;
  avchain->audio_peer_srcpad_blocked = achain->peer_srcpad_blocked;
  avchain->audio_peer_srcpad_queue = achain->peer_srcpad_queue;

  lpsink->video_chains = g_list_remove (lpsink->video_chains, vchain);
  g_slice_free (GstSinkChain, vchain);

  lpsink->audio_chains = g_list_remove (lpsink->audio_chains, achain);
  g_slice_free (GstSinkChain, achain);

  if (chain->sink)
    lpsink->video_sink = gst_object_ref (chain->sink);

  bin_name = g_strdup_printf ("avbin%d", lpsink->nb_video_bin++);
  chain->bin = GST_BIN_CAST (gst_bin_new (bin_name));
  bin = GST_BIN_CAST (chain->bin);
  gst_object_ref_sink (bin);
  gst_bin_add (bin, chain->sink);

  avchain->video_queue = gst_element_factory_make ("queue", NULL);
  g_object_set (G_OBJECT (avchain->video_queue), "max-size-buffers", 3,
      "max-size-bytes", 0, "max-size-time", (gint64) 0, "silent", TRUE, NULL);
  gst_bin_add (bin, avchain->video_queue);

  avchain->audio_queue = gst_element_factory_make ("queue", NULL);
  g_object_set (G_OBJECT (avchain->audio_queue), "silent", TRUE, NULL);
  gst_bin_add (bin, avchain->audio_queue);

  video_queue_srcpad = gst_element_get_static_pad (avchain->video_queue, "src");
  audio_queue_srcpad = gst_element_get_static_pad (avchain->audio_queue, "src");

  gst_pad_link_full (video_queue_srcpad, video_sink_sinkpad,
      GST_PAD_LINK_CHECK_TEMPLATE_CAPS);
  gst_pad_link_full (audio_queue_srcpad, audio_sink_sinkpad,
      GST_PAD_LINK_CHECK_TEMPLATE_CAPS);

  video_queue_sinkpad =
      gst_element_get_static_pad (avchain->video_queue, "sink");
  audio_queue_sinkpad =
      gst_element_get_static_pad (avchain->audio_queue, "sink");
  avchain->video_ghostpad =
      gst_ghost_pad_new ("video_sink", video_queue_sinkpad);
  avchain->audio_ghostpad =
      gst_ghost_pad_new ("audio_sink", audio_queue_sinkpad);
  gst_pad_set_active (avchain->video_ghostpad, TRUE);
  gst_pad_set_active (avchain->audio_ghostpad, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (chain->bin), avchain->video_ghostpad);
  gst_element_add_pad (GST_ELEMENT_CAST (chain->bin), avchain->audio_ghostpad);

  avchain->video_pad = video_sink_sinkpad;
  avchain->audio_pad = audio_sink_sinkpad;

  lpsink->video_chains = g_list_append (lpsink->video_chains, avchain);

  gst_object_unref (video_queue_sinkpad);
  gst_object_unref (video_queue_srcpad);
  gst_object_unref (audio_queue_sinkpad);
  gst_object_unref (audio_queue_srcpad);
  gst_object_unref (video_sink_sinkpad);
  gst_object_unref (audio_sink_sinkpad);

  return GST_SINK_CHAIN (avchain);
}

static GstPadProbeReturn
srcpad_blocked_cb (GstPad * blockedpad, GstPadProbeInfo * info,
    gpointer user_data);

#if 0
static void
caps_notify_cb_from_demux (GstPad * pad, GParamSpec * unused,
    GstLpSink * lpsink)
{
  GstCaps *caps = NULL;
  GstElement *element = NULL;
  GstPad *active_pad = NULL;
  GstElement *queue = NULL;
  GstPad *queue_sinkpad = NULL;
  GstPad *queue_srcpad = NULL;
  gulong *block_id = NULL;
  gboolean reconfigure = FALSE;
  GstSinkChain *chain = NULL;

  g_object_get (pad, "caps", &caps, NULL);
  element = gst_pad_get_parent_element (pad);
  g_object_get (element, "active-pad", &active_pad, NULL);

  if (gst_pad_is_linked (active_pad)) {
    return;
  }

  queue = gst_element_factory_make ("queue", NULL);
  g_object_set (queue, "silent", TRUE, NULL);
  gst_bin_add (GST_BIN_CAST (lpsink), queue);
  gst_element_set_state (queue, GST_STATE_PAUSED);

  queue_sinkpad = gst_element_get_static_pad (queue, "sink");

  gst_pad_link_full (active_pad, queue_sinkpad, GST_PAD_LINK_CHECK_NOTHING);
  gst_object_unref (queue_sinkpad);

  queue_srcpad = gst_element_get_static_pad (queue, "src");

  GST_LP_SINK_LOCK (lpsink);
  chain = g_slice_alloc0 (sizeof (GstSinkChain));
  block_id = &chain->block_id;
  chain->peer_srcpad_queue = gst_object_ref (queue_srcpad);
  chain->caps = gst_caps_ref (caps);

  if (element == lpsink->video_streamid_demux) {
    chain->type = GST_LP_SINK_TYPE_VIDEO;
    lpsink->video_chains = g_list_append (lpsink->video_chains, chain);
  } else if (element == lpsink->audio_streamid_demux) {
    chain->type = GST_LP_SINK_TYPE_AUDIO;
    lpsink->audio_chains = g_list_append (lpsink->audio_chains, chain);
  } else if (element == lpsink->text_streamid_demux) {
    chain->type = GST_LP_SINK_TYPE_TEXT;
    lpsink->text_chains = g_list_append (lpsink->text_chains, chain);
  }

  GST_LP_SINK_UNLOCK (lpsink);

  g_object_set_data (G_OBJECT (queue_srcpad), "lpsink.chain", chain);

  if (block_id && *block_id == 0) {
    *block_id =
        gst_pad_add_probe (queue_srcpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        srcpad_blocked_cb, lpsink, NULL);
    //PENDING_FLAG_SET (lpsink, type);
  }

  gst_object_unref (queue_srcpad);

  if (reconfigure)
    gst_lp_sink_reconfigure (lpsink);

  if (caps)
    gst_caps_unref (caps);
}
#endif

static void
pad_added_cb (GstElement * element, GstPad * pad, GstLpSink * lpsink)
{
  GstElement *queue = NULL;
  GstPad *queue_sinkpad = NULL;
  GstPad *queue_srcpad = NULL;
  GstPad *ghost_sinkpad = NULL;
  GstPad *sid_sinkpad = NULL;
  gulong *block_id = NULL;
  GstSinkChain *chain = NULL;
  GstCaps *caps = NULL;
  GstStructure *s = NULL;
  gchar *stream_id = NULL;
  GstQuery *query = NULL;

  queue = gst_element_factory_make ("queue", NULL);
  g_object_set (queue, "silent", TRUE, NULL);
  gst_bin_add (GST_BIN_CAST (lpsink), queue);
  gst_element_set_state (queue, GST_STATE_PAUSED);

  queue_sinkpad = gst_element_get_static_pad (queue, "sink");
  queue_srcpad = gst_element_get_static_pad (queue, "src");

  gst_pad_link_full (pad, queue_sinkpad, GST_PAD_LINK_CHECK_NOTHING);
  gst_object_unref (queue_sinkpad);


  sid_sinkpad = gst_element_get_static_pad (element, "sink");
  ghost_sinkpad = gst_pad_get_peer (sid_sinkpad);

  stream_id = gst_pad_get_stream_id (ghost_sinkpad);
  s = gst_structure_new ("get-caps-by-streamid",
      "stream-id", G_TYPE_STRING, stream_id, "caps", GST_TYPE_CAPS, caps, NULL);
  query = gst_query_new_custom (GST_QUERY_CUSTOM, s);


  if (gst_pad_query (pad, query)) {
    gchar *caps_name = NULL;
    const GstStructure *structure;

    structure = gst_query_get_structure (query);
    caps = g_value_get_boxed (gst_structure_get_value (structure, "caps"));
    caps_name = gst_caps_to_string (caps);

    GST_INFO_OBJECT (lpsink, "get-caps-by-streamid query result = %s",
        caps_name);

    g_free (caps_name);
  }

  GST_LP_SINK_LOCK (lpsink);
  chain = g_slice_alloc0 (sizeof (GstSinkChain));
  block_id = &chain->block_id;
  chain->peer_srcpad_queue = gst_object_ref (queue_srcpad);
  chain->caps = caps;

  if (element == lpsink->video_streamid_demux) {
    chain->type = GST_LP_SINK_TYPE_VIDEO;
    lpsink->video_chains = g_list_append (lpsink->video_chains, chain);
  } else if (element == lpsink->audio_streamid_demux) {
    chain->type = GST_LP_SINK_TYPE_AUDIO;
    lpsink->audio_chains = g_list_append (lpsink->audio_chains, chain);
  } else if (element == lpsink->text_streamid_demux) {
    chain->type = GST_LP_SINK_TYPE_TEXT;
    lpsink->text_chains = g_list_append (lpsink->text_chains, chain);
  }

  GST_LP_SINK_UNLOCK (lpsink);
  g_object_set_data (G_OBJECT (queue_srcpad), "lpsink.chain", chain);

  if (block_id && *block_id == 0) {
    *block_id =
        gst_pad_add_probe (queue_srcpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        srcpad_blocked_cb, lpsink, NULL);
  }

  gst_query_unref (query);
  g_free (stream_id);
  gst_object_unref (queue_srcpad);
  gst_object_unref (sid_sinkpad);
  gst_object_unref (ghost_sinkpad);
}

void
gst_lp_sink_set_all_pads_blocked (GstLpSink * lpsink)
{
  GST_DEBUG_OBJECT (lpsink, "all pads are blocked!");

  GST_LP_SINK_LOCK (lpsink);
  gst_lp_sink_do_reconfigure (lpsink);

  video_set_blocked (lpsink, FALSE);
  audio_set_blocked (lpsink, FALSE);
  text_set_blocked (lpsink, FALSE);
  GST_LP_SINK_UNLOCK (lpsink);
}

static void
gst_lp_sink_do_reconfigure (GstLpSink * lpsink)
{
  GList *item = NULL;
  GstSinkChain *chain = NULL;

  GST_LP_SINK_LOCK (lpsink);

  for (item = g_list_first (lpsink->video_chains); item; item = item->next) {
    chain = (GstSinkChain *) item->data;

    if (chain->sink)
      continue;

    chain = gen_video_chain (lpsink, chain);
    if (chain == NULL)
      break;

    add_chain (chain, TRUE);
    activate_chain (chain, TRUE);

    if (chain->type == GST_LP_SINK_TYPE_AV) {
      GstAVSinkChain *avchain = GST_AV_SINK_CHAIN (chain);
      gst_pad_link_full (avchain->video_peer_srcpad_queue,
          avchain->video_ghostpad, GST_PAD_LINK_CHECK_NOTHING);
      gst_pad_link_full (avchain->audio_peer_srcpad_queue,
          avchain->audio_ghostpad, GST_PAD_LINK_CHECK_NOTHING);
      GST_DEBUG_OBJECT (lpsink, "av chains added");
      break;
    } else {
      gst_pad_link_full (chain->peer_srcpad_queue, chain->bin_ghostpad,
          GST_PAD_LINK_CHECK_NOTHING);
    }

    GST_DEBUG_OBJECT (lpsink, "video chains added");
  }

  for (item = g_list_first (lpsink->audio_chains); item; item = item->next) {
    chain = (GstSinkChain *) item->data;

    chain = gen_audio_chain (lpsink, chain);
    if (chain == NULL)
      break;

    add_chain (chain, TRUE);
    activate_chain (chain, TRUE);

    gst_pad_link_full (chain->peer_srcpad_queue, chain->bin_ghostpad,
        GST_PAD_LINK_CHECK_NOTHING);

    GST_DEBUG_OBJECT (lpsink, "audio chains added");
  }

  for (item = g_list_first (lpsink->text_chains); item; item = item->next) {
    GstPad *sink_sinkpad;
    chain = (GstSinkChain *) item->data;

    sink_sinkpad =
        gst_element_get_request_pad (lpsink->text_sinkbin, "text_sink%d");

    gst_pad_link_full (chain->peer_srcpad_queue, sink_sinkpad,
        GST_PAD_LINK_CHECK_NOTHING);

    GST_DEBUG_OBJECT (lpsink, "text chain added");
  }

  do_async_done (lpsink);
  GST_LP_SINK_UNLOCK (lpsink);

/*no_chain:
  GST_DEBUG_OBJECT (lpsink, "failed to setup chain");
  GST_LP_SINK_UNLOCK (lpsink);
  return FALSE;*/
}

static void
gst_lp_sink_setup_element (GstLpSink * lpsink, GstElement ** streamid_demux,
    GstPad ** ghost_sinkpad, const gchar * pad_name)
{
  GstPad *demux_sinkpad = NULL;

  *streamid_demux = gst_element_factory_make ("streamiddemux", NULL);

  gst_bin_add (GST_BIN_CAST (lpsink), *streamid_demux);

  demux_sinkpad = gst_element_get_static_pad (*streamid_demux, "sink");
  *ghost_sinkpad = gst_ghost_pad_new (pad_name, demux_sinkpad);
  g_signal_connect (G_OBJECT (*streamid_demux), "pad-added",
      G_CALLBACK (pad_added_cb), lpsink);

  gst_element_set_state (*streamid_demux, GST_STATE_PAUSED);

  if (*ghost_sinkpad == lpsink->video_pad)
    lpsink->video_streamid_demux = *streamid_demux;
  else if (*ghost_sinkpad == lpsink->audio_pad)
    lpsink->audio_streamid_demux = *streamid_demux;
  else if (*ghost_sinkpad == lpsink->text_pad)
    lpsink->text_streamid_demux = *streamid_demux;

  gst_object_unref (demux_sinkpad);
}

static void
video_set_blocked (GstLpSink * lpsink, gboolean blocked)
{
  GList *item = NULL;
  GstSinkChain *chain = NULL;

  if (!lpsink->video_chains) {
    GST_DEBUG_OBJECT (lpsink, "no video chain");
    return;
  }

  for (item = g_list_first (lpsink->video_chains); item; item = item->next) {
    chain = (GstSinkChain *) item->data;

    if (blocked && chain->block_id == 0) {
      chain->block_id =
          gst_pad_add_probe (chain->peer_srcpad_queue,
          GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, srcpad_blocked_cb, lpsink, NULL);
    } else if (!blocked && chain->block_id) {
      gst_pad_remove_probe (chain->peer_srcpad_queue, chain->block_id);
      chain->block_id = 0;
      chain->peer_srcpad_blocked = FALSE;
    }

    if (chain->type == GST_LP_SINK_TYPE_AV) {
      GstAVSinkChain *avchain = GST_AV_SINK_CHAIN (chain);
      if (blocked && avchain->video_block_id == 0) {
        avchain->video_block_id =
            gst_pad_add_probe (avchain->video_peer_srcpad_queue,
            GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, srcpad_blocked_cb, lpsink,
            NULL);
      } else if (!blocked && avchain->video_block_id) {
        gst_pad_remove_probe (avchain->video_peer_srcpad_queue,
            avchain->video_block_id);
        avchain->video_block_id = 0;
        avchain->video_peer_srcpad_blocked = FALSE;
      }

      if (blocked && avchain->audio_block_id == 0) {
        avchain->audio_block_id =
            gst_pad_add_probe (avchain->audio_peer_srcpad_queue,
            GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, srcpad_blocked_cb, lpsink,
            NULL);
      } else if (!blocked && avchain->audio_block_id) {
        gst_pad_remove_probe (avchain->audio_peer_srcpad_queue,
            avchain->audio_block_id);
        avchain->audio_block_id = 0;
        avchain->audio_peer_srcpad_blocked = FALSE;
      }
    }

  }
}

static void
audio_set_blocked (GstLpSink * lpsink, gboolean blocked)
{
  GList *item = NULL;
  GstSinkChain *chain = NULL;

  if (!lpsink->audio_chains) {
    GST_DEBUG_OBJECT (lpsink, "no audio chain");
    return;
  }

  for (item = g_list_first (lpsink->audio_chains); item; item = item->next) {
    chain = (GstSinkChain *) item->data;

    if (blocked && chain->block_id == 0) {
      chain->block_id =
          gst_pad_add_probe (chain->peer_srcpad_queue,
          GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, srcpad_blocked_cb, lpsink, NULL);
    } else if (!blocked && chain->block_id) {
      gst_pad_remove_probe (chain->peer_srcpad_queue, chain->block_id);
      chain->block_id = 0;
      chain->peer_srcpad_blocked = FALSE;
    }
  }
}

static void
text_set_blocked (GstLpSink * lpsink, gboolean blocked)
{
  GList *item = NULL;
  GstSinkChain *chain = NULL;

  if (!lpsink->text_chains) {
    GST_DEBUG_OBJECT (lpsink, "no text chain");
    return;
  }

  for (item = g_list_first (lpsink->text_chains); item; item = item->next) {
    chain = (GstSinkChain *) item->data;

    if (blocked && chain->block_id == 0) {
      chain->block_id =
          gst_pad_add_probe (chain->peer_srcpad_queue,
          GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, srcpad_blocked_cb, lpsink, NULL);
    } else if (!blocked && chain->block_id) {
      gst_pad_remove_probe (chain->peer_srcpad_queue, chain->block_id);
      chain->block_id = 0;
      chain->peer_srcpad_blocked = FALSE;
    }
  }
}

static GstPadProbeReturn
srcpad_blocked_cb (GstPad * blockedpad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstLpSink *lpsink = (GstLpSink *) user_data;
  GstSinkChain *chain;
  const gchar *pad_type = NULL;
  gchar *stream_id = NULL;

  GST_LP_SINK_LOCK (lpsink);

  stream_id = gst_pad_get_stream_id (blockedpad);
  chain = g_object_get_data (G_OBJECT (blockedpad), "lpsink.chain");
  chain->peer_srcpad_blocked = TRUE;

  if (chain->type == GST_LP_SINK_TYPE_VIDEO)
    pad_type = "video";
  else if (chain->type == GST_LP_SINK_TYPE_AUDIO)
    pad_type = "audio";
  else if (chain->type == GST_LP_SINK_TYPE_TEXT)
    pad_type = "text";

  GST_DEBUG_OBJECT (blockedpad, "%s pad(%s) blocked", pad_type,
      GST_PAD_NAME (blockedpad));

  if (stream_id) {
    g_signal_emit (G_OBJECT (lpsink),
        gst_lp_sink_signals[SIGNAL_PAD_BLOCKED], 0, stream_id, TRUE);
    g_free (stream_id);
  }

  GST_LP_SINK_UNLOCK (lpsink);

  return GST_PAD_PROBE_OK;
}

gboolean
gst_lp_sink_reconfigure (GstLpSink * lpsink)
{
  GST_LOG_OBJECT (lpsink, "Triggering reconfiguration");

  GST_LP_SINK_LOCK (lpsink);
  video_set_blocked (lpsink, TRUE);
  audio_set_blocked (lpsink, TRUE);
  text_set_blocked (lpsink, TRUE);
  GST_LP_SINK_UNLOCK (lpsink);

  return TRUE;
}

static gboolean
gst_lp_sink_unblock_sinkpads (GstLpSink * lpsink)
{
  GstPad *opad = NULL;

  if (lpsink->video_pad) {
    opad =
        GST_PAD_CAST (gst_proxy_pad_get_internal (GST_PROXY_PAD
            (lpsink->video_pad)));
    gst_pad_remove_probe (opad, lpsink->video_block_id);
    lpsink->video_block_id = 0;
  }

  if (lpsink->audio_pad) {
    opad =
        GST_PAD_CAST (gst_proxy_pad_get_internal (GST_PROXY_PAD
            (lpsink->audio_pad)));
    gst_pad_remove_probe (opad, lpsink->audio_block_id);
    lpsink->audio_block_id = 0;
  }

  if (lpsink->text_pad) {
    opad =
        GST_PAD_CAST (gst_proxy_pad_get_internal (GST_PROXY_PAD
            (lpsink->text_pad)));
    gst_pad_remove_probe (opad, lpsink->text_block_id);
    lpsink->text_block_id = 0;
  }

  gst_object_unref (opad);

  return TRUE;
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
  gulong *block_id = NULL;

  GST_LP_SINK_LOCK (lpsink);

  switch (type) {
    case GST_LP_SINK_TYPE_AUDIO:
      if (!lpsink->audio_pad)
        gst_lp_sink_setup_element (lpsink, &lpsink->audio_streamid_demux,
            &lpsink->audio_pad, "audio_sink");

      res = lpsink->audio_pad;
      block_id = &lpsink->audio_block_id;
      break;
    case GST_LP_SINK_TYPE_VIDEO:
      if (!lpsink->video_pad)
        gst_lp_sink_setup_element (lpsink, &lpsink->video_streamid_demux,
            &lpsink->video_pad, "video_sink");

      res = lpsink->video_pad;
      block_id = &lpsink->video_block_id;
      break;
    case GST_LP_SINK_TYPE_TEXT:
      if (!lpsink->text_pad) {
        gst_lp_sink_setup_element (lpsink, &lpsink->text_streamid_demux,
            &lpsink->text_pad, "text_sink");

        lpsink->text_sinkbin = gst_element_factory_make ("lptsinkbin", NULL);
        gst_bin_add (GST_BIN_CAST (lpsink), lpsink->text_sinkbin);
        GST_OBJECT_FLAG_SET (lpsink->text_sinkbin, GST_ELEMENT_FLAG_SINK);
      }

      res = lpsink->text_pad;
      block_id = &lpsink->text_block_id;
      break;
    default:
      res = NULL;
  }
  GST_LP_SINK_UNLOCK (lpsink);

  if (res) {
    gst_pad_set_active (res, TRUE);
    gst_element_add_pad (GST_ELEMENT_CAST (lpsink), res);

    if (block_id && *block_id == 0) {
      GstPad *blockpad =
          GST_PAD_CAST (gst_proxy_pad_get_internal (GST_PROXY_PAD (res)));

      *block_id =
          gst_pad_add_probe (blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
          NULL, NULL, NULL);
      //PENDING_FLAG_SET (playsink, type);
      gst_object_unref (blockpad);
    }
  }

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
    /*TODO: g_signal_handler_disconnect (lpsink->video_pad,
       lpsink->video_notify_caps_id); */
    //video_set_blocked (lpsink, FALSE);
  } else if (pad == lpsink->audio_pad) {
    res = &lpsink->audio_pad;
    /*TODO: g_signal_handler_disconnect (lpsink->audio_pad,
       lpsink->audio_notify_caps_id); */
    //audio_set_blocked (lpsink, FALSE);
  } else if (pad == lpsink->text_pad) {
    res = &lpsink->text_pad;
    /*TODO: g_signal_handler_disconnect (lpsink->text_pad,
       lpsink->text_notify_caps_id); */
    //text_set_blocked (lpsink, FALSE);
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
    case PROP_AUDIO_ONLY:
      lpsink->audio_only = g_value_get_boolean (value);
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
    case PROP_AUDIO_ONLY:
      g_value_set_boolean (value, lpsink->audio_only);
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
      gdouble rate;
      res = gst_lp_sink_send_event_to_sink (lpsink, event);
      if (res) {
        gst_event_parse_seek (event, &rate, NULL, NULL, NULL, NULL, NULL, NULL);
        if (lpsink->rate != rate) {
          lpsink->rate = rate;
          GST_INFO_OBJECT (lpsink,
              "GST_EVENT_SEEK, set playrate %lf" G_GUINT64_FORMAT, rate);
        }
      }
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
gst_lp_sink_query (GstElement * element, GstQuery * query)
{
  GstLpSink *lpsink = GST_LP_SINK (element);
  gboolean ret;

  if (GST_QUERY_TYPE (query) == GST_QUERY_POSITION && lpsink->video_sink
      && ABS ((gint64) lpsink->rate) >= 4) {
    GST_INFO_OBJECT (lpsink,
        "GST_QUERY_POSITION, trying to get current pts from vdecsink");
    guint64 current_pts = 0;
    GstFormat format;
    gst_query_parse_position (query, &format, NULL);
    g_object_get (lpsink->video_sink, "current-pts", &current_pts, NULL);
    gst_query_set_position (query, format, current_pts);
    ret = TRUE;
  } else {
    ret = GST_ELEMENT_CLASS (parent_class)->query (element, query);
  }

  return ret;
}

static gboolean
add_chain (GstSinkChain * chain, gboolean add)
{
  //if (chain->added == add)
  //return TRUE;

  if (add)
    gst_bin_add (GST_BIN_CAST (chain->lpsink), GST_ELEMENT_CAST (chain->bin));
  else {
    gst_bin_remove (GST_BIN_CAST (chain->lpsink),
        GST_ELEMENT_CAST (chain->bin));
    /* we don't want to lose our sink status */
    GST_OBJECT_FLAG_SET (chain->lpsink, GST_ELEMENT_FLAG_SINK);
  }

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
  GstState state;

  //if (chain->activated == activate)
  //return TRUE;

  GST_OBJECT_LOCK (chain->lpsink);
  state = GST_STATE_TARGET (chain->lpsink);
  GST_OBJECT_UNLOCK (chain->lpsink);
  if (activate)
    gst_element_set_state (GST_ELEMENT_CAST (chain->bin), state);
  else
    gst_element_set_state (GST_ELEMENT_CAST (chain->bin), GST_STATE_NULL);

  //chain->activated = activate;

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
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      lpsink->need_async_start = TRUE;
      /* we want to go async to PAUSED until we managed to configure and add the
       * sinks */
      do_async_start (lpsink);
      ret = GST_STATE_CHANGE_ASYNC;

      /* block all pads here */
      /*if (!gst_lp_sink_reconfigure (lpsink))
         ret = GST_STATE_CHANGE_FAILURE; */
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_LP_SINK_LOCK (lpsink);
      video_set_blocked (lpsink, FALSE);
      audio_set_blocked (lpsink, FALSE);
      text_set_blocked (lpsink, FALSE);
      GST_LP_SINK_UNLOCK (lpsink);
      break;
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
      do_async_done (lpsink);
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
      /* FIXME Release audio device when we implement that */
      lpsink->need_async_start = TRUE;
      break;
    }
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_lp_sink_release_pad (lpsink, lpsink->audio_pad);
      gst_lp_sink_release_pad (lpsink, lpsink->video_pad);
      gst_lp_sink_release_pad (lpsink, lpsink->text_pad);

      if (lpsink->video_chains) {
        GList *walk = lpsink->video_chains;

        while (walk) {
          GstSinkChain *chain = (GstSinkChain *) walk->data;

          if (chain->type == GST_LP_SINK_TYPE_AV) {
            gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (GST_AV_SINK_CHAIN
                    (chain)->video_ghostpad), NULL);
            gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (GST_AV_SINK_CHAIN
                    (chain)->video_ghostpad), NULL);

            gst_element_unlink (GST_AV_SINK_CHAIN (chain)->audio_queue,
                GST_AV_SINK_CHAIN (chain)->sinkchain.sink);
            gst_element_unlink (GST_AV_SINK_CHAIN (chain)->video_queue,
                GST_AV_SINK_CHAIN (chain)->sinkchain.sink);

            gst_element_release_request_pad (GST_AV_SINK_CHAIN
                (chain)->sinkchain.sink, GST_AV_SINK_CHAIN (chain)->audio_pad);
            gst_element_release_request_pad (GST_AV_SINK_CHAIN
                (chain)->sinkchain.sink, GST_AV_SINK_CHAIN (chain)->video_pad);
            gst_object_unref (GST_AV_SINK_CHAIN (chain)->video_pad);
            gst_object_unref (GST_AV_SINK_CHAIN (chain)->audio_pad);
          } else {
            gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (chain->bin_ghostpad),
                NULL);
            gst_element_unlink (chain->queue, chain->sink);
          }
          activate_chain (chain, FALSE);
          add_chain (chain, FALSE);

          gst_bin_remove (chain->bin, chain->sink);

          if (chain->sink != NULL) {
            gst_element_set_state (chain->sink, GST_STATE_NULL);
            gst_bin_remove (chain->bin, chain->sink);
            chain->sink = NULL;
          }

          free_chain ((GstSinkChain *) chain);

          walk = g_list_next (walk);
        }
      }

      if (lpsink->audio_chains) {
        GList *walk = lpsink->audio_chains;

        while (walk) {
          GstSinkChain *chain = (GstSinkChain *) walk->data;

          gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (chain->bin_ghostpad),
              NULL);
          gst_element_unlink (chain->queue, chain->sink);

          activate_chain (chain, FALSE);
          add_chain (chain, FALSE);

          gst_bin_remove (chain->bin, chain->sink);

          if (chain->sink != NULL) {
            gst_element_set_state (chain->sink, GST_STATE_NULL);
            gst_bin_remove (chain->bin, chain->sink);
            chain->sink = NULL;
          }

          free_chain ((GstSinkChain *) chain);

          walk = g_list_next (walk);
        }
      }

      if (lpsink->text_chains) {
        GList *walk = lpsink->text_chains;

        while (walk) {
          GstSinkChain *chain = (GstSinkChain *) walk->data;

          //TODO: 

          walk = g_list_next (walk);
        }
      }

      do_async_done (lpsink);
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
