/* GStreamer Lightweight Playback Plugins
 *
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *	Author : Jeongseok Kim <jeongseok.kim@lge.com>
 *               Wonchul Lee <wonchul86.lee@lge.com>
 *               HoonHee Lee <hoonhee.lee@lge.com>
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

#include <gst/gst.h>

#include "gstlpbin.h"
#include "gstlpsink.h"

GST_DEBUG_CATEGORY_STATIC (gst_lp_bin_debug);
#define GST_CAT_DEFAULT gst_lp_bin_debug

#define LPBIN_SUPPORTED_CAPS "\
video/x-fd; audio/x-fd; \
text/x-avi-internal; text/x-avi-unknown; text/x-raw; \
application/x-ass; application/x-ssa; \
subpicture/x-dvd; subpicture/x-dvb; subpicture/x-xsub; \
"

enum
{
  PROP_0,
  PROP_URI,
  PROP_SOURCE,
  PROP_N_VIDEO,
  PROP_CURRENT_VIDEO,
  PROP_N_AUDIO,
  PROP_CURRENT_AUDIO,
  PROP_N_TEXT,
  PROP_CURRENT_TEXT,
  PROP_AUDIO_SINK,
  PROP_VIDEO_SINK,
  PROP_USE_BUFFERING,
  PROP_MUTE,
  PROP_SMART_PROPERTIES,
  PROP_BUFFER_SIZE,
  PROP_BUFFER_DURATION,
  PROP_LAST
};

enum
{
  SIGNAL_ABOUT_TO_FINISH,
  SIGNAL_RETRIEVE_THUMBNAIL,
  SIGNAL_SOURCE_SETUP,
  SIGNAL_AUTOPLUG_CONTINUE,
  SIGNAL_AUTOPLUG_FACTORIES,
  SIGNAL_VIDEO_TAGS_CHANGED,
  SIGNAL_AUDIO_TAGS_CHANGED,
  SIGNAL_TEXT_TAGS_CHANGED,
  SIGNAL_GET_VIDEO_TAGS,
  SIGNAL_GET_AUDIO_TAGS,
  SIGNAL_GET_TEXT_TAGS,
  SIGNAL_GET_VIDEO_PAD,
  SIGNAL_GET_AUDIO_PAD,
  SIGNAL_GET_TEXT_PAD,
  SIGNAL_STREAMS_READY,
  SIGNAL_STREAM_UNLOCK,
  LAST_SIGNAL
};

#define DEFAULT_USE_BUFFERING FALSE
#define DEFAULT_BUFFER_DURATION   -1
#define DEFAULT_BUFFER_SIZE       -1

#define DEFAULT_USE_STREAM_LOCK FALSE

/* GstObject overriding */
static void gst_lp_bin_class_init (GstLpBinClass * klass);
static void gst_lp_bin_init (GstLpBin * lpbin);
static void gst_lp_bin_finalize (GObject * object);

static void gst_lp_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_lp_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);
static GstStateChangeReturn gst_lp_bin_change_state (GstElement * element,
    GstStateChange transition);
static void gst_lp_bin_handle_message (GstBin * bin, GstMessage * message);
static gboolean gst_lp_bin_query (GstElement * element, GstQuery * query);

/* signal callbacks */
static void no_more_pads_cb (GstElement * decodebin, GstLpBin * lpbin);
static void pad_added_cb (GstElement * decodebin, GstPad * pad,
    GstLpBin * lpbin);
static void pad_removed_cb (GstElement * decodebin, GstPad * pad,
    GstLpBin * lpbin);
static void notify_source_cb (GstElement * decodebin, GParamSpec * pspec,
    GstLpBin * lpbin);
static void drained_cb (GstElement * decodebin, GstLpBin * lpbin);
static void unknown_type_cb (GstElement * decodebin, GstPad * pad,
    GstCaps * caps, GstLpBin * lpbin);
static gboolean autoplug_continue_signal (GstElement * element, GstPad * pad,
    GstCaps * caps, GstLpBin * lpbin);
static GValueArray *autoplug_factories_signal (GstElement * decodebin,
    GstPad * pad, GstCaps * caps, GstLpBin * lpbin);
static void audio_tags_changed_cb (GstElement * fcbin,
    gint stream_id, GstLpBin * lpbin);
static void video_tags_changed_cb (GstElement * fcbin,
    gint stream_id, GstLpBin * lpbin);
static void text_tags_changed_cb (GstElement * fcbin,
    gint stream_id, GstLpBin * lpbin);
static void element_configured_cb (GstElement * fcbin,
    gint type, GstPad * sinkpad, GstPad * srcpad, gchar * stream_id,
    GstLpBin * lpbin);
static void pad_blocked_cb (GstElement * lpsink, gchar * stream_id,
    gboolean blocked, GstLpBin * lpbin);
static void pad_added_cb_from_fcbin (GstElement * fcbin, GstPad * pad,
    GstLpBin * lpbin);
static void no_more_pads_cb_from_fcbin (GstElement * fcbin, GstLpBin * lpbin);

/* private functions */
static gboolean gst_lp_bin_setup_element (GstLpBin * lpbin);
static gboolean gst_lp_bin_make_link (GstLpBin * lpbin);
static void gst_lp_bin_set_sink (GstLpBin * lpbin, GstElement ** elem,
    const gchar * dbg, GstElement * sink);
static GstElement *gst_lp_bin_get_current_sink (GstLpBin * lpbin,
    GstElement ** elem, const gchar * dbg, GstLpSinkType type);
static gint compare_factories_func (gconstpointer p1, gconstpointer p2);
static void gst_lp_bin_update_elements_list (GstLpBin * lpbin);
static gboolean _factory_can_sink_caps (GstElementFactory * factory,
    GstCaps * caps);
static gboolean sink_accepts_caps (GstElement * sink, GstCaps * caps);
static gboolean _gst_boolean_accumulator (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer dummy);
static gboolean _gst_array_accumulator (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer dummy);
static gboolean gst_lp_bin_autoplug_continue (GstElement * element,
    GstPad * pad, GstCaps * caps);
static GValueArray *gst_lp_bin_autoplug_factories (GstElement * element,
    GstPad * pad, GstCaps * caps);
static void gst_lp_bin_deactive (GstLpBin * lpbin);
static GstBuffer *gst_lp_bin_retrieve_thumbnail (GstLpBin * lpbin, gint width,
    gint height, gchar * format);
static gboolean gst_lp_bin_stream_unlock (GstLpBin * lpbin);
static void gst_lp_bin_element_added_cb (GstBin * lpbin, GstElement * element,
    gpointer user_data);

static GstTagList *gst_lp_bin_get_video_tags (GstLpBin * lpbin, gint stream);
static GstTagList *gst_lp_bin_get_audio_tags (GstLpBin * lpbin, gint stream);
static GstTagList *gst_lp_bin_get_text_tags (GstLpBin * lpbin, gint stream);
static GstPad *gst_lp_bin_get_video_pad (GstLpBin * lpbin, gint stream);
static GstPad *gst_lp_bin_get_audio_pad (GstLpBin * lpbin, gint stream);
static GstPad *gst_lp_bin_get_text_pad (GstLpBin * lpbin, gint stream);
static void replace_stream_id_blocked_table (gchar * stream_id,
    gboolean blocked, GstLpBin * lpbin);

static GstElementClass *parent_class;

static guint gst_lp_bin_signals[LAST_SIGNAL] = { 0 };

static GRWLock lock;

GType
gst_lp_bin_get_type (void)
{
  static GType gst_lp_bin_type = 0;

  if (!gst_lp_bin_type) {
    static const GTypeInfo gst_lp_bin_info = {
      sizeof (GstLpBinClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_lp_bin_class_init,
      NULL,
      NULL,
      sizeof (GstLpBin),
      0,
      (GInstanceInitFunc) gst_lp_bin_init,
      NULL
    };

    gst_lp_bin_type = g_type_register_static (GST_TYPE_PIPELINE,
        "GstLpBin", &gst_lp_bin_info, 0);
  }

  return gst_lp_bin_type;
}

static void
gst_lp_bin_class_init (GstLpBinClass * klass)
{
  GObjectClass *gobject_klass;
  GstElementClass *gstelement_klass;
  GstBinClass *gstbin_klass;

  gobject_klass = (GObjectClass *) klass;
  gstelement_klass = (GstElementClass *) klass;
  gstbin_klass = (GstBinClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_klass->set_property = gst_lp_bin_set_property;
  gobject_klass->get_property = gst_lp_bin_get_property;

  gobject_klass->finalize = gst_lp_bin_finalize;

  gst_element_class_set_static_metadata (gstelement_klass,
      "Lightweight Play Bin", "Lightweight/Bin/Player",
      "Autoplug and play media for Restricted systems",
      "Jeongseok Kim <jeongseok.kim@lge.com>");

  g_object_class_install_property (gobject_klass, PROP_URI,
      g_param_spec_string ("uri", "URI", "URI of the media to play",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_SOURCE,
      g_param_spec_object ("source", "Source", "Source element",
          GST_TYPE_ELEMENT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  /**
   * LPBin:n-video
   *
   * Get the total number of available video streams.
   */
  g_object_class_install_property (gobject_klass, PROP_N_VIDEO,
      g_param_spec_int ("n-video", "Number Video",
          "Total number of video streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  /**
   * LPBin:current-video
   *
   * Get or set the currently playing video stream. By default the first video
   * stream with data is played.
   */
  g_object_class_install_property (gobject_klass, PROP_CURRENT_VIDEO,
      g_param_spec_int ("current-video", "Current Video",
          "Currently playing video stream (-1 = auto)",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * LPBin:n-audio
   *
   * Get the total number of available audio streams.
   */
  g_object_class_install_property (gobject_klass, PROP_N_AUDIO,
      g_param_spec_int ("n-audio", "Number Audio",
          "Total number of audio streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  /**
   * LPBin:current-audio
   *
   * Get or set the currently playing audio stream. By default the first audio
   * stream with data is played.
   */
  g_object_class_install_property (gobject_klass, PROP_CURRENT_AUDIO,
      g_param_spec_int ("current-audio", "Current audio",
          "Currently playing audio stream (-1 = auto)",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * LPBin:n-text
   *
   * Get the total number of available subtitle streams.
   */
  g_object_class_install_property (gobject_klass, PROP_N_TEXT,
      g_param_spec_int ("n-text", "Number Text",
          "Total number of text streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  /**
   * LPBin:current-text:
   *
   * Get or set the currently playing subtitle stream. By default the first
   * subtitle stream with data is played.
   */
  g_object_class_install_property (gobject_klass, PROP_CURRENT_TEXT,
      g_param_spec_int ("current-text", "Current Text",
          "Currently playing text stream (-1 = auto)",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_MUTE,
      g_param_spec_boolean ("mute", "Mute",
          "Mute the audio channel without changing the volume", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_VIDEO_SINK,
      g_param_spec_object ("video-sink", "Video Sink",
          "the video output element to use (NULL = default sink)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_AUDIO_SINK,
      g_param_spec_object ("audio-sink", "Audio Sink",
          "the audio output element to use (NULL = default sink)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_USE_BUFFERING,
      g_param_spec_boolean ("use-buffering", "Use buffering",
          "set use-buffering property at multiqueue", DEFAULT_USE_BUFFERING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstLpBin::smart-properties:
   *
   * This property makes to be possible that all of elements in uridecodebin
   * can set directly their's properties.
   * A property should be comprised of key and value and split by ':' delimiter.
   * All of properties should be split by ',' delimiter.
   * ex ) smart-properties="program-number:3,emit-stats:TRUE"
   *
   */
  g_object_class_install_property (gobject_klass, PROP_SMART_PROPERTIES,
      g_param_spec_boxed ("smart-properties", "Smart Properties",
          "Information of properties in such key and value",
          GST_TYPE_STRUCTURE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_BUFFER_SIZE,
      g_param_spec_int ("buffer-size", "Buffer size (bytes)",
          "Buffer size when buffering network streams",
          -1, G_MAXINT, DEFAULT_BUFFER_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_BUFFER_DURATION,
      g_param_spec_int64 ("buffer-duration", "Buffer duration (ns)",
          "Buffer duration when buffering network streams",
          -1, G_MAXINT64, DEFAULT_BUFFER_DURATION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_lp_bin_signals[SIGNAL_ABOUT_TO_FINISH] =
      g_signal_new ("about-to-finish", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstLpBinClass, about_to_finish), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 0, G_TYPE_NONE);

  gst_lp_bin_signals[SIGNAL_RETRIEVE_THUMBNAIL] =
      g_signal_new ("retrieve-thumbnail", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, retrieve_thumbnail), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_BUFFER, 3, G_TYPE_INT, G_TYPE_INT,
      G_TYPE_STRING);

  gst_lp_bin_signals[SIGNAL_SOURCE_SETUP] =
      g_signal_new ("source-setup", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, GST_TYPE_ELEMENT);

  gst_lp_bin_signals[SIGNAL_AUTOPLUG_CONTINUE] =
      g_signal_new ("autoplug-continue", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstLpBinClass,
          autoplug_continue), _gst_boolean_accumulator, NULL,
      g_cclosure_marshal_generic, G_TYPE_BOOLEAN, 2, GST_TYPE_PAD,
      GST_TYPE_CAPS);

  gst_lp_bin_signals[SIGNAL_AUTOPLUG_FACTORIES] =
      g_signal_new ("autoplug-factories", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstLpBinClass,
          autoplug_factories), _gst_array_accumulator, NULL,
      g_cclosure_marshal_generic, G_TYPE_VALUE_ARRAY, 2,
      GST_TYPE_PAD, GST_TYPE_CAPS);

  /**
   * GstLpBin::video-tags-changed
   * @lpbin: a #GstLpBin
   * @stream: stream index with changed tags
   *
   * This signal is emitted whenever the tags of a video stream have changed.
   * The application will most likely want to get the new tags.
   *
   * This signal may be emitted from the context of a GStreamer streaming thread.
   * You can use gst_message_new_application() and gst_element_post_message()
   * to notify your application's main thread.
   *
   */
  gst_lp_bin_signals[SIGNAL_VIDEO_TAGS_CHANGED] =
      g_signal_new ("video-tags-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstLpBinClass, video_tags_changed), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * GstLpBin::audio-tags-changed
   * @lpbin: a #GstLpBin
   * @stream: stream index with changed tags
   *
   * This signal is emitted whenever the tags of an audio stream have changed.
   * The application will most likely want to get the new tags.
   *
   * This signal may be emitted from the context of a GStreamer streaming thread.
   * You can use gst_message_new_application() and gst_element_post_message()
   * to notify your application's main thread.
   *
   */
  gst_lp_bin_signals[SIGNAL_AUDIO_TAGS_CHANGED] =
      g_signal_new ("audio-tags-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstLpBinClass, audio_tags_changed), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * GstLpBin::text-tags-changed
   * @lpbin: a #GstLpBin
   * @stream: stream index with changed tags
   *
   * This signal is emitted whenever the tags of a video stream have changed.
   * The application will most likely want to get the new tags.
   *
   * This signal may be emitted from the context of a GStreamer streaming thread.
   * You can use gst_message_new_application() and gst_element_post_message()
   * to notify your application's main thread.
   *
   */
  gst_lp_bin_signals[SIGNAL_TEXT_TAGS_CHANGED] =
      g_signal_new ("text-tags-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstLpBinClass, text_tags_changed), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * GstLpBin::get-video-tags
   * @lpbin: a #GstLpBin
   * @stream: a video stream number
   *
   * Action signal to retrieve the tags of a specific video stream number.
   * This information can be used to select a stream.
   *
   * Returns: a GstTagList with tags or NULL when the stream number does not
   * exist.
   */
  gst_lp_bin_signals[SIGNAL_GET_VIDEO_TAGS] =
      g_signal_new ("get-video-tags", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, get_video_tags), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_TAG_LIST, 1, G_TYPE_INT);

  /**
   * GstLpBin::get-audio-tags
   * @lpbin: a #GstLpBin
   * @stream: an audio stream number
   *
   * Action signal to retrieve the tags of a specific audio stream number.
   * This information can be used to select a stream.
   *
   * Returns: a GstTagList with tags or NULL when the stream number does not
   * exist.
   */
  gst_lp_bin_signals[SIGNAL_GET_AUDIO_TAGS] =
      g_signal_new ("get-audio-tags", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, get_audio_tags), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_TAG_LIST, 1, G_TYPE_INT);

  /**
   * GstLpBin::get-text-tags
   * @lpbin: a #GstLpBin
   * @stream: a text stream number
   *
   * Action signal to retrieve the tags of a specific text stream number.
   * This information can be used to select a stream.
   *
   * Returns: a GstTagList with tags or NULL when the stream number does not
   * exist.
   */
  gst_lp_bin_signals[SIGNAL_GET_TEXT_TAGS] =
      g_signal_new ("get-text-tags", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, get_text_tags), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_TAG_LIST, 1, G_TYPE_INT);


  /**
   * GstLpBin::get-video-pad
   * @lpbin: a #GstLpBin
   * @stream: a video stream number
   *
   * Action signal to retrieve the stream-combiner sinkpad for a specific
   * video stream.
   * This pad can be used for notifications of caps changes, stream-specific
   * queries, etc.
   *
   * Returns: a #GstPad, or NULL when the stream number does not exist.
   */
  gst_lp_bin_signals[SIGNAL_GET_VIDEO_PAD] =
      g_signal_new ("get-video-pad", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, get_video_pad), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_PAD, 1, G_TYPE_INT);

  /**
   * GstLpBin::get-audio-pad
   * @lpbin: a #GstLpBin
   * @stream: an audio stream number
   *
   * Action signal to retrieve the stream-combiner sinkpad for a specific
   * audio stream.
   * This pad can be used for notifications of caps changes, stream-specific
   * queries, etc.
   *
   * Returns: a #GstPad, or NULL when the stream number does not exist.
   */
  gst_lp_bin_signals[SIGNAL_GET_AUDIO_PAD] =
      g_signal_new ("get-audio-pad", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, get_audio_pad), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_PAD, 1, G_TYPE_INT);

  /**
   * GstLpBin::get-text-pad
   * @lpbin: a #GstLpBin
   * @stream: a text stream number
   *
   * Action signal to retrieve the stream-combiner sinkpad for a specific
   * text stream.
   * This pad can be used for notifications of caps changes, stream-specific
   * queries, etc.
   *
   * Returns: a #GstPad, or NULL when the stream number does not exist.
   */
  gst_lp_bin_signals[SIGNAL_GET_TEXT_PAD] =
      g_signal_new ("get-text-pad", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, get_text_pad), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_PAD, 1, G_TYPE_INT);

  gst_lp_bin_signals[SIGNAL_STREAMS_READY] =
      g_signal_new ("streams-ready", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 6,
      G_TYPE_PTR_ARRAY, G_TYPE_PTR_ARRAY, G_TYPE_PTR_ARRAY,
      G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);

  gst_lp_bin_signals[SIGNAL_STREAM_UNLOCK] =
      g_signal_new ("stream-unlock", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstLpBinClass, stream_unlock), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_BOOLEAN, 0);

  gstelement_klass->change_state = GST_DEBUG_FUNCPTR (gst_lp_bin_change_state);
  gstelement_klass->query = GST_DEBUG_FUNCPTR (gst_lp_bin_query);

  gstbin_klass->handle_message = GST_DEBUG_FUNCPTR (gst_lp_bin_handle_message);

  klass->autoplug_continue = GST_DEBUG_FUNCPTR (gst_lp_bin_autoplug_continue);
  klass->autoplug_factories = GST_DEBUG_FUNCPTR (gst_lp_bin_autoplug_factories);
  klass->retrieve_thumbnail = GST_DEBUG_FUNCPTR (gst_lp_bin_retrieve_thumbnail);

  klass->get_video_tags = GST_DEBUG_FUNCPTR (gst_lp_bin_get_video_tags);
  klass->get_audio_tags = GST_DEBUG_FUNCPTR (gst_lp_bin_get_audio_tags);
  klass->get_text_tags = GST_DEBUG_FUNCPTR (gst_lp_bin_get_text_tags);
  klass->get_video_pad = GST_DEBUG_FUNCPTR (gst_lp_bin_get_video_pad);
  klass->get_audio_pad = GST_DEBUG_FUNCPTR (gst_lp_bin_get_audio_pad);
  klass->get_text_pad = GST_DEBUG_FUNCPTR (gst_lp_bin_get_text_pad);

  klass->stream_unlock = GST_DEBUG_FUNCPTR (gst_lp_bin_stream_unlock);
}

static void
gst_lp_bin_init (GstLpBin * lpbin)
{
  GST_DEBUG_CATEGORY_INIT (gst_lp_bin_debug, "lpbin", 0,
      "Lightweight Play Bin");

  lpbin->element_added_id =
      g_signal_connect (lpbin, "element-added",
      G_CALLBACK (gst_lp_bin_element_added_cb), lpbin);

  lpbin->video_channels = g_ptr_array_new ();
  lpbin->audio_channels = g_ptr_array_new ();
  lpbin->text_channels = g_ptr_array_new ();

  g_rec_mutex_init (&lpbin->lock);

  /* first filter out the interesting element factories */
  g_mutex_init (&lpbin->elements_lock);

  /* add sink */
  lpbin->uridecodebin = NULL;
  lpbin->fcbin = NULL;
  lpbin->lpsink = NULL;
  lpbin->source = NULL;

  lpbin->naudio = 0;
  lpbin->nvideo = 0;
  lpbin->ntext = 0;

  lpbin->video_pad = NULL;
  lpbin->audio_pad = NULL;
  lpbin->text_pad = NULL;

  lpbin->use_buffering = DEFAULT_USE_BUFFERING;
  lpbin->use_stream_lock = DEFAULT_USE_STREAM_LOCK;

  g_rw_lock_init (&lock);
  lpbin->smart_prop = NULL;
  lpbin->elements_str = NULL;

  lpbin->buffer_duration = DEFAULT_BUFFER_DURATION;
  lpbin->buffer_size = DEFAULT_BUFFER_SIZE;

  lpbin->audio_only = TRUE;

  lpbin->audio_chain_linked = FALSE;
  lpbin->video_chain_linked = FALSE;
  lpbin->text_chain_linked = FALSE;

  lpbin->stream_id_blocked =
      g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free,
      NULL);

  lpbin->stream_id_blocked = NULL;

  lpbin->all_pads_blocked = FALSE;
}

static void
gst_lp_bin_finalize (GObject * obj)
{
  GstLpBin *lpbin;

  lpbin = GST_LP_BIN (obj);

  g_ptr_array_free (lpbin->video_channels, TRUE);
  g_ptr_array_free (lpbin->audio_channels, TRUE);
  g_ptr_array_free (lpbin->text_channels, TRUE);

  if (lpbin->source)
    gst_object_unref (lpbin->source);

  if (lpbin->video_pad)
    gst_object_unref (lpbin->video_pad);

  if (lpbin->audio_pad)
    gst_object_unref (lpbin->audio_pad);

  if (lpbin->text_pad)
    gst_object_unref (lpbin->text_pad);

  if (lpbin->video_sink) {
    gst_element_set_state (lpbin->video_sink, GST_STATE_NULL);
    gst_object_unref (lpbin->video_sink);
  }

  if (lpbin->audio_sink) {
    gst_element_set_state (lpbin->audio_sink, GST_STATE_NULL);
    gst_object_unref (lpbin->audio_sink);
  }

  if (lpbin->smart_prop) {
    gst_structure_free (lpbin->smart_prop);
    lpbin->smart_prop = NULL;
  }

  if (lpbin->elements_str) {
    g_free (lpbin->elements_str);
  }

  if (lpbin->stream_id_blocked) {
    g_hash_table_remove_all (lpbin->stream_id_blocked);
    g_hash_table_destroy (lpbin->stream_id_blocked);
    lpbin->stream_id_blocked = NULL;
  }

  g_rec_mutex_clear (&lpbin->lock);
  g_mutex_clear (&lpbin->elements_lock);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gboolean
gst_lp_bin_query (GstElement * element, GstQuery * query)
{
  GstLpBin *lpbin = GST_LP_BIN (element);
  GstFormat format;
  GstElementFactory *factory = NULL;
  const gchar *klass = NULL;
  gboolean ret;

  GST_LP_BIN_LOCK (lpbin);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
      gst_query_parse_duration (query, &format, NULL);

      if (format == GST_FORMAT_BYTES) {
        GST_DEBUG_OBJECT (lpbin,
            "source element will directly answer for BYTE format duration query.");
        ret = gst_element_query (lpbin->source, query);
      } else {
        ret = GST_ELEMENT_CLASS (parent_class)->query (element, query);
      }
      break;
    default:
      ret = GST_ELEMENT_CLASS (parent_class)->query (element, query);
      break;
  }

  GST_LP_BIN_UNLOCK (lpbin);

  return ret;
}

static void
gst_lp_bin_handle_message (GstBin * bin, GstMessage * msg)
{
//  GstLpBin *lpbin = GST_LP_BIN(bin);

  if (msg)
    GST_BIN_CLASS (parent_class)->handle_message (bin, msg);
}

static GstBuffer *
gst_lp_bin_retrieve_thumbnail (GstLpBin * lpbin, gint width, gint height,
    gchar * format)
{
  GST_INFO_OBJECT (lpbin,
      "width = %d, height = %d, format = %s", width, height, format);
  GstBuffer *result = NULL;
  GstCaps *caps;

  lpbin->video_sink =
      gst_lp_bin_get_current_sink (lpbin, &lpbin->video_sink, "video",
      GST_LP_SINK_TYPE_VIDEO);
  if (!lpbin->video_sink) {
    GST_DEBUG_OBJECT (lpbin, "no video sink");
    return NULL;
  }

  caps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      "format", G_TYPE_STRING, format,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, NULL);

  GST_DEBUG_OBJECT (lpbin, "video_sink name = %s",
      GST_ELEMENT_NAME (lpbin->video_sink));
  g_signal_emit_by_name (G_OBJECT (lpbin->video_sink), "convert-frame", caps,
      &result);
  GST_DEBUG_OBJECT (lpbin, "result = %p", result);
  gst_caps_unref (caps);
  return result;
}

static gboolean
gst_lp_bin_stream_unlock (GstLpBin * lpbin)
{

  if (!lpbin->lpsink)
    GST_ELEMENT_ERROR (lpbin, STREAM, FAILED, (NULL),
        ("lpsink element does not exist."));

  if (!lpbin->use_stream_lock)
    GST_ELEMENT_ERROR (lpbin, STREAM, FAILED, (NULL),
        ("use_stream_lock is disabled."));

  gboolean ret = FALSE;

  g_signal_emit_by_name (lpbin->lpsink, "unblock-sinkpads", &ret, NULL);
  GST_INFO_OBJECT (lpbin,
      "received unblock-sinkpads result=%d by signal action", ret);

  return ret;
}

static gboolean
set_smart_properties (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstStructure *smart_prop = (GstStructure *) user_data;

  gst_structure_id_set_value (smart_prop, field_id, value);
  return TRUE;
}

static void
gst_lp_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstLpBin *lpbin = GST_LP_BIN (object);

  switch (prop_id) {
    case PROP_URI:
      lpbin->uri = g_strdup (g_value_get_string (value));
      break;
    case PROP_CURRENT_VIDEO:
      g_object_set (lpbin->fcbin, "current-video", g_value_get_int (value),
          NULL);
      break;
    case PROP_CURRENT_AUDIO:
      g_object_set (lpbin->fcbin, "current-audio", g_value_get_int (value),
          NULL);
      break;
    case PROP_CURRENT_TEXT:
      g_object_set (lpbin->fcbin, "current-text", g_value_get_int (value),
          NULL);
      break;
    case PROP_MUTE:
      break;
    case PROP_VIDEO_SINK:
      gst_lp_bin_set_sink (lpbin, &lpbin->video_sink, "video",
          g_value_get_object (value));
      break;
    case PROP_AUDIO_SINK:
      gst_lp_bin_set_sink (lpbin, &lpbin->audio_sink, "audio",
          g_value_get_object (value));
      break;
    case PROP_USE_BUFFERING:
      lpbin->use_buffering = g_value_get_boolean (value);
      break;
    case PROP_SMART_PROPERTIES:
    {
      const GstStructure *s = gst_value_get_structure (value);

      GST_INFO_OBJECT (lpbin,
          "set smart-properties, structure = %" GST_PTR_FORMAT, s);

      if (!gst_structure_has_name (s, "smart-properties"))
        break;

      if (lpbin->smart_prop)
        gst_structure_foreach (s, set_smart_properties, lpbin->smart_prop);
      else
        lpbin->smart_prop = gst_structure_copy (s);

      if (lpbin->source)
        g_object_set (lpbin->source, "smart-properties", lpbin->smart_prop,
            NULL);
      break;
    }
    case PROP_BUFFER_SIZE:
      lpbin->buffer_size = g_value_get_int (value);
      break;
    case PROP_BUFFER_DURATION:
      lpbin->buffer_duration = g_value_get_int64 (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_lp_bin_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstLpBin *lpbin = GST_LP_BIN (object);

  switch (prop_id) {
    case PROP_URI:
      GST_LP_BIN_LOCK (lpbin);
      g_value_set_string (value, lpbin->uri);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    case PROP_SOURCE:
    {
      GST_OBJECT_LOCK (lpbin);
      g_value_set_object (value, lpbin->source);
      GST_OBJECT_UNLOCK (lpbin);
      break;
    }
    case PROP_N_VIDEO:
    {
      gint n_video = 0;

      GST_LP_BIN_LOCK (lpbin);
      if (lpbin->fcbin)
        g_object_get (lpbin->fcbin, "n-video", &n_video, NULL);
      g_value_set_int (value, n_video);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_CURRENT_VIDEO:
    {
      gint current_video = -1;

      GST_LP_BIN_LOCK (lpbin);
      if (lpbin->fcbin)
        g_object_get (lpbin->fcbin, "current-video", &current_video, NULL);
      g_value_set_int (value, current_video);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_N_AUDIO:
    {
      gint n_audio = 0;

      GST_LP_BIN_LOCK (lpbin);
      if (lpbin->fcbin)
        g_object_get (lpbin->fcbin, "n-audio", &n_audio, NULL);
      g_value_set_int (value, n_audio);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_CURRENT_AUDIO:
    {
      gint current_audio = -1;

      GST_LP_BIN_LOCK (lpbin);
      if (lpbin->fcbin)
        g_object_get (lpbin->fcbin, "current-audio", &current_audio, NULL);
      g_value_set_int (value, current_audio);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_N_TEXT:
    {
      gint n_text = 0;

      GST_LP_BIN_LOCK (lpbin);
      if (lpbin->fcbin)
        g_object_get (lpbin->fcbin, "n-text", &n_text, NULL);
      g_value_set_int (value, n_text);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_CURRENT_TEXT:
    {
      gint current_text = -1;

      GST_LP_BIN_LOCK (lpbin);
      if (lpbin->fcbin)
        g_object_get (lpbin->fcbin, "current-text", &current_text, NULL);
      g_value_set_int (value, current_text);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_VIDEO_SINK:
      g_value_take_object (value,
          gst_lp_bin_get_current_sink (lpbin, &lpbin->video_sink,
              "video", GST_LP_SINK_TYPE_VIDEO));
      break;
    case PROP_AUDIO_SINK:
      g_value_take_object (value,
          gst_lp_bin_get_current_sink (lpbin, &lpbin->audio_sink,
              "audio", GST_LP_SINK_TYPE_AUDIO));
      break;
    case PROP_MUTE:
      break;
    case PROP_USE_BUFFERING:
      g_value_set_boolean (value, lpbin->use_buffering);
      break;
    case PROP_BUFFER_SIZE:
      GST_OBJECT_LOCK (lpbin);
      g_value_set_int (value, lpbin->buffer_size);
      GST_OBJECT_UNLOCK (lpbin);
      break;
    case PROP_BUFFER_DURATION:
      GST_OBJECT_LOCK (lpbin);
      g_value_set_int64 (value, lpbin->buffer_duration);
      GST_OBJECT_UNLOCK (lpbin);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
pad_added_cb (GstElement * decodebin, GstPad * pad, GstLpBin * lpbin)
{
  GstCaps *caps;
  const GstStructure *s;
  const gchar *name;
  GstPad *fcbin_sinkpad, *fcbin_srcpad;
  GstPad *sinkpad = NULL;
  GstPadTemplate *tmpl;

  caps = gst_pad_query_caps (pad, NULL);
  s = gst_caps_get_structure (caps, 0);
  name = gst_structure_get_name (s);

  tmpl = gst_pad_template_new (name, GST_PAD_SINK, GST_PAD_REQUEST, caps);

  GST_DEBUG_OBJECT (pad, "pad with caps %" GST_PTR_FORMAT " added", caps);

  fcbin_sinkpad = gst_element_request_pad (lpbin->fcbin, tmpl, name, caps);
  gst_pad_link (pad, fcbin_sinkpad);

  fcbin_srcpad = g_object_get_data (G_OBJECT (fcbin_sinkpad), "fcbin.srcpad");

  if (g_str_has_prefix (name, "video/")) {
    lpbin->audio_only = FALSE;
  }

  g_object_unref (tmpl);
}

/* called when a pad is removed from the uridecodebin. We unlink the pad from
 * the selector. This will make the selector select a new pad. */
static void
pad_removed_cb (GstElement * decodebin, GstPad * pad, GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "pad removed callback");
  // TODO
}

static void
no_more_pads_cb (GstElement * decodebin, GstLpBin * lpbin)
{
  GST_INFO_OBJECT (lpbin, "no more pads callback");

  if (lpbin->audio_only) {
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (lpbin->lpsink),
            "audio-only"))
      g_object_set (lpbin->lpsink, "audio-only", TRUE, NULL);
    GST_INFO_OBJECT (lpbin, "audio-only set as TRUE");
  }

  if (lpbin->fcbin) {
    gboolean ret = FALSE;
    g_signal_emit_by_name (lpbin->fcbin, "unblock-sinkpads", &ret, NULL);
  }
}

static void
pad_added_cb_from_fcbin (GstElement * fcbin, GstPad * pad, GstLpBin * lpbin)
{
  GstPad *lpsink_sinkpad;
  guint type = -1;

  type = (guintptr) g_object_get_data (G_OBJECT (pad), "type");
  GST_INFO_OBJECT (lpbin, "type = %d", type);

  if (gst_pad_get_direction (pad) == GST_PAD_SRC) {
    if (type == GST_LP_SINK_TYPE_VIDEO) {
      lpbin->video_pad = gst_object_ref (pad);
      lpsink_sinkpad =
          gst_element_get_request_pad (lpbin->lpsink, "video_sink");
      gst_pad_link (pad, lpsink_sinkpad);
      g_object_set_data (G_OBJECT (lpbin->video_pad), "lpsink.sinkpad",
          lpsink_sinkpad);
    }

    if (type == GST_LP_SINK_TYPE_AUDIO) {
      lpbin->audio_pad = gst_object_ref (pad);
      lpsink_sinkpad =
          gst_element_get_request_pad (lpbin->lpsink, "audio_sink");
      gst_pad_link (pad, lpsink_sinkpad);
      g_object_set_data (G_OBJECT (lpbin->audio_pad), "lpsink.sinkpad",
          lpsink_sinkpad);
    }

    if (type == GST_LP_SINK_TYPE_TEXT) {
      lpbin->text_pad = gst_object_ref (pad);
      lpsink_sinkpad = gst_element_get_request_pad (lpbin->lpsink, "text_sink");
      gst_pad_link (pad, lpsink_sinkpad);
      g_object_set_data (G_OBJECT (lpbin->text_pad), "lpsink.sinkpad",
          lpsink_sinkpad);
    }
  }

}

static void
no_more_pads_cb_from_fcbin (GstElement * fcbin, GstLpBin * lpbin)
{
  GPtrArray *video_caps = g_ptr_array_new ();
  GPtrArray *audio_caps = g_ptr_array_new ();
  GPtrArray *text_caps = g_ptr_array_new ();
  gint n_video = 0;
  gint n_audio = 0;
  gint n_text = 0;
  gint cur_video = -1;
  gint cur_audio = -1;
  gint cur_text = -1;
  GstIterator *it = NULL;
  gboolean ret = FALSE;
  gboolean it_done = FALSE;
  GValue item = { 0, };

  if (!lpbin->lpsink)
    goto no_lpsink;

  if (!lpbin->fcbin)
    goto no_fcbin;

  g_object_get (lpbin->fcbin, "n-video", &n_video, NULL);
  g_object_get (lpbin->fcbin, "n-audio", &n_audio, NULL);
  g_object_get (lpbin->fcbin, "n-text", &n_text, NULL);

  g_object_get (lpbin->fcbin, "current-video", &cur_video, NULL);
  g_object_get (lpbin->fcbin, "current-audio", &cur_audio, NULL);
  g_object_get (lpbin->fcbin, "current-text", &cur_text, NULL);

  GST_INFO_OBJECT (lpbin,
      "building caps for streams-ready signal, v=%d, a=%d, t=%d", n_video,
      n_audio, n_text);

  it = gst_element_iterate_sink_pads (lpbin->fcbin);
  if (!it)
    goto done;

  while (!it_done) {
    GstPad *sinkpad = NULL;
    GstCaps *caps = NULL;
    gchar *caps_str = NULL;
    GValue val = { 0, };

    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        sinkpad = g_value_get_object (&item);
        caps = gst_pad_get_current_caps (sinkpad);
        caps_str = gst_caps_to_string (caps);

        if (g_str_has_prefix (caps_str, "video")
            || g_str_has_prefix (caps_str, "image")) {
          g_ptr_array_add (video_caps, caps);
        } else if (g_str_has_prefix (caps_str, "audio")) {
          g_ptr_array_add (audio_caps, caps);
        } else if (g_str_has_prefix (caps_str, "text/")
            || g_str_has_prefix (caps_str, "application/")
            || g_str_has_prefix (caps_str, "subpicture/")) {
          g_ptr_array_add (text_caps, caps);
        }

        g_value_reset (&item);
        g_free (caps_str);
        break;
      case GST_ITERATOR_DONE:
        GST_DEBUG_OBJECT (lpbin->fcbin, "finished sinkpad iteration");
        it_done = TRUE;
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING_OBJECT (lpbin->fcbin, "aborted sinkpad iteration");
        it_done = TRUE;
        break;
    }
  }

  g_value_unset (&item);
  gst_iterator_free (it);

  if (n_video != video_caps->len || n_audio != audio_caps->len
      || n_text != text_caps->len)
    goto invalid_caps;

  // FIXME: this function has too many roles.
  // When detecting no_more_pad from fcbin, unlocking pads of lpsink is sufficient.
  // However, the next logic is required in order to support resource manager mechanism.
  // It's doubt whether the logic is located here.
  if (lpbin->use_stream_lock)
    goto emit_streams_ready;

  g_signal_emit_by_name (lpbin->lpsink, "unblock-sinkpads", &ret, NULL);
  GST_DEBUG_OBJECT (lpbin, "received unblock-sinkpads result=%d", ret);

done:
  g_ptr_array_foreach (video_caps, (GFunc) gst_caps_unref, NULL);
  g_ptr_array_foreach (audio_caps, (GFunc) gst_caps_unref, NULL);
  g_ptr_array_foreach (text_caps, (GFunc) gst_caps_unref, NULL);
  g_ptr_array_free (video_caps, TRUE);
  g_ptr_array_free (audio_caps, TRUE);
  g_ptr_array_free (text_caps, TRUE);
  return;

no_lpsink:
  GST_ERROR_OBJECT (lpbin, "*CANNOT* detect lpsink, check object lifecycle");
  goto done;

no_fcbin:
  GST_ERROR_OBJECT (lpbin, "*CANNOT* detect fcbin, check object lifecycle");
  goto done;

emit_streams_ready:
  /* Application should deallocate ptrArray and decrease reference count each of
   * caps after use it. */
  g_signal_emit_by_name (lpbin, "streams-ready", video_caps, audio_caps,
      text_caps, cur_video, cur_audio, cur_text, NULL);
  GST_INFO_OBJECT (lpbin, "stream-lock is enabled, streams-ready is emitted");
  return;

invalid_caps:
  GST_ELEMENT_ERROR (lpbin, STREAM, FAILED, (NULL),
      ("number of video(%d)/audio(%d)/text(%d) is not matched.",
          video_caps->len, audio_caps->len, text_caps->len));
  goto done;
}

static void
foreach_check_blocked (gpointer key, gpointer value, gpointer user_data)
{
  gchar *stream_id = (gchar *) key;
  gboolean blocked = (gboolean) value;
  GstLpBin *lpbin = (GstLpBin *) user_data;

  GST_INFO_OBJECT (lpbin, "stream_id = %s, blocked = %d", stream_id, blocked);
  if (blocked == FALSE) {
    lpbin->all_pads_blocked = FALSE;
  }
}

static void
replace_stream_id_blocked_table (gchar * stream_id, gboolean blocked,
    GstLpBin * lpbin)
{
  lpbin->all_pads_blocked = TRUE;

  GST_OBJECT_LOCK (lpbin);
  g_hash_table_replace (lpbin->stream_id_blocked, stream_id, blocked);
  GST_OBJECT_UNLOCK (lpbin);

  g_hash_table_foreach (lpbin->stream_id_blocked, foreach_check_blocked, lpbin);

  if (lpbin->all_pads_blocked) {
    gst_lp_sink_set_all_pads_blocked (lpbin->lpsink);
  }
}

static void
pad_blocked_cb (GstElement * lpsink, gchar * stream_id, gboolean blocked,
    GstLpBin * lpbin)
{
  replace_stream_id_blocked_table (stream_id, blocked, lpbin);
}

static void
element_configured_cb (GstElement * fcbin, gint type, GstPad * sinkpad,
    GstPad * srcpad, gchar * stream_id, GstLpBin * lpbin)
{
  GstPad *lpsink_sinkpad = NULL;

  GST_INFO_OBJECT (lpbin, "type = %d, stream_id = %s", type, stream_id);

  if (lpbin->stream_id_blocked == NULL) {
    lpbin->stream_id_blocked = g_hash_table_new (g_str_hash, g_str_equal);
  }

  if (stream_id) {
    GST_OBJECT_LOCK (lpbin);
    g_hash_table_insert (lpbin->stream_id_blocked, g_strdup (stream_id), FALSE);
    GST_OBJECT_UNLOCK (lpbin);
  }
  if (type == GST_LP_SINK_TYPE_AUDIO) {
    GST_INFO_OBJECT (lpbin, "AUDIO");
    g_ptr_array_add (lpbin->audio_channels, sinkpad);
  } else if (type == GST_LP_SINK_TYPE_VIDEO) {
    GST_INFO_OBJECT (lpbin, "VIDEO");
    g_ptr_array_add (lpbin->video_channels, sinkpad);
  } else if (type == GST_LP_SINK_TYPE_TEXT) {
    GST_INFO_OBJECT (lpbin, "TEXT");
    g_ptr_array_add (lpbin->text_channels, sinkpad);
  }
}

static void
notify_source_cb (GstElement * decodebin, GParamSpec * pspec, GstLpBin * lpbin)
{
  GstElement *source;
  GstQuery *query;
  GstStructure *s;

  g_object_get (lpbin->uridecodebin, "source", &source, NULL);

  GST_OBJECT_LOCK (lpbin);
  if ((lpbin->source != NULL) && (GST_IS_ELEMENT (lpbin->source))) {
    gst_object_unref (GST_OBJECT (lpbin->source));
  }
  lpbin->source = source;
  GST_OBJECT_UNLOCK (lpbin);

  g_object_set (lpbin->source, "smart-properties", lpbin->smart_prop, NULL);

  g_object_notify (G_OBJECT (lpbin), "source");
  g_signal_emit (lpbin, gst_lp_bin_signals[SIGNAL_SOURCE_SETUP], 0,
      lpbin->source);

  /* custom query for get use-stream-lock value */
  s = gst_structure_new ("smart-properties",
      "use-stream-lock", G_TYPE_BOOLEAN, NULL, NULL);
  query = gst_query_new_custom (GST_QUERY_CUSTOM, s);

  if (gst_element_query (source, query)) {
    const GstStructure *structure = gst_query_get_structure (query);
    gst_structure_get (structure, "use-stream-lock", G_TYPE_BOOLEAN,
        &lpbin->use_stream_lock, NULL);
  }

  gst_query_unref (query);
}

static void
drained_cb (GstElement * decodebin, GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "drained cb");

  /* after this call, we should have a next group to activate or we EOS */
  g_signal_emit (G_OBJECT (lpbin),
      gst_lp_bin_signals[SIGNAL_ABOUT_TO_FINISH], 0, NULL);

  // TODO
}

static void
unknown_type_cb (GstElement * decodebin, GstPad * pad, GstCaps * caps,
    GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "unknown type cb");

  // TODO
}

static gboolean
gst_lp_bin_setup_element (GstLpBin * lpbin)
{
  GstCaps *fd_caps;

  /* FIXME: Using fixed value caps is not a good idea. */
  fd_caps = gst_caps_from_string (LPBIN_SUPPORTED_CAPS);

  lpbin->uridecodebin = gst_element_factory_make ("uridecodebin", NULL);

  g_object_set (lpbin->uridecodebin, "caps", fd_caps, "uri", lpbin->uri,
      /* configure buffering parameters */
      "buffer-duration", lpbin->buffer_duration,
      "buffer-size", lpbin->buffer_size, NULL);

  if (lpbin->use_buffering)
    g_object_set (lpbin->uridecodebin, "use-buffering", TRUE, NULL);

  lpbin->pad_added_id = g_signal_connect (lpbin->uridecodebin, "pad-added",
      G_CALLBACK (pad_added_cb), lpbin);
  lpbin->no_more_pads_id =
      g_signal_connect (lpbin->uridecodebin, "no-more-pads",
      G_CALLBACK (no_more_pads_cb), lpbin);

  lpbin->pad_removed_id = g_signal_connect (lpbin->uridecodebin, "pad-removed",
      G_CALLBACK (pad_removed_cb), lpbin);

  lpbin->source_element_id =
      g_signal_connect (lpbin->uridecodebin, "notify::source",
      G_CALLBACK (notify_source_cb), lpbin);

  /* is called when the uridecodebin is out of data and we can switch to the
   * next uri */
  lpbin->drained_id = g_signal_connect (lpbin->uridecodebin, "drained",
      G_CALLBACK (drained_cb), lpbin);

  lpbin->unknown_type_id =
      g_signal_connect (lpbin->uridecodebin, "unknown-type",
      G_CALLBACK (unknown_type_cb), lpbin);
  gst_bin_add (GST_BIN_CAST (lpbin), lpbin->uridecodebin);

  /* will be called when a new media type is found. We return a list of decoders
   * including sinks for decodebin to try */
  lpbin->autoplug_factories_id =
      g_signal_connect (lpbin->uridecodebin, "autoplug-factories",
      G_CALLBACK (autoplug_factories_signal), lpbin);

  lpbin->autoplug_continue_id =
      g_signal_connect (lpbin->uridecodebin, "autoplug-continue",
      G_CALLBACK (autoplug_continue_signal), lpbin);

  lpbin->fcbin = gst_element_factory_make ("fcbin", NULL);
  gst_bin_add (GST_BIN_CAST (lpbin), lpbin->fcbin);

  lpbin->fcbin_pad_added_id = g_signal_connect (lpbin->fcbin, "pad-added",
      G_CALLBACK (pad_added_cb_from_fcbin), lpbin);
  lpbin->fcbin_no_more_pads_id =
      g_signal_connect (lpbin->fcbin, "no-more-pads",
      G_CALLBACK (no_more_pads_cb_from_fcbin), lpbin);

  lpbin->audio_tags_changed_id =
      g_signal_connect (lpbin->fcbin, "audio-tags-changed",
      G_CALLBACK (audio_tags_changed_cb), lpbin);

  lpbin->video_tags_changed_id =
      g_signal_connect (lpbin->fcbin, "video-tags-changed",
      G_CALLBACK (video_tags_changed_cb), lpbin);

  lpbin->text_tags_changed_id =
      g_signal_connect (lpbin->fcbin, "text-tags-changed",
      G_CALLBACK (text_tags_changed_cb), lpbin);

  g_signal_connect (lpbin->fcbin, "element-configured",
      G_CALLBACK (element_configured_cb), lpbin);

  lpbin->lpsink = gst_element_factory_make ("lpsink", NULL);
  lpbin->pad_blocked_id =
      g_signal_connect (lpbin->lpsink, "pad-blocked",
      G_CALLBACK (pad_blocked_cb), lpbin);

  gst_bin_add (GST_BIN_CAST (lpbin), lpbin->lpsink);

  gst_caps_unref (fd_caps);

  return TRUE;
}

static gboolean
gst_lp_bin_make_link (GstLpBin * lpbin)
{
//  lpsink_sinkpad = gst_element_get_request_pad (lpbin->lpsink, sink_name);
//  ret = gst_pad_link (pad, lpsink_sinkpad);

  return TRUE;
}

static void
gst_lp_bin_deactive (GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "deactive");

  GstPad *lpsink_sinkpad;

  if (lpbin->video_pad) {
    GST_LOG_OBJECT (lpbin, "unlinking video pad in fcbin from sink");
    lpsink_sinkpad =
        g_object_get_data (G_OBJECT (lpbin->video_pad), "lpsink.sinkpad");
    gst_pad_unlink (lpbin->video_pad, lpsink_sinkpad);

    /* release back */
    GST_LOG_OBJECT (lpbin, "release sink pad for video");
    gst_lp_sink_release_pad (lpbin->lpsink, lpsink_sinkpad);

    gst_object_unref (lpbin->video_pad);
    lpbin->video_pad = NULL;
    lpsink_sinkpad = NULL;
  }

  if (lpbin->audio_pad) {
    GST_LOG_OBJECT (lpbin, "unlinking audio pad in fcbin from sink");
    lpsink_sinkpad =
        g_object_get_data (G_OBJECT (lpbin->audio_pad), "lpsink.sinkpad");
    gst_pad_unlink (lpbin->audio_pad, lpsink_sinkpad);

    /* release back */
    GST_LOG_OBJECT (lpbin, "release sink pad for audio");
    gst_lp_sink_release_pad (lpbin->lpsink, lpsink_sinkpad);

    gst_object_unref (lpbin->audio_pad);
    lpbin->audio_pad = NULL;
    lpsink_sinkpad = NULL;
  }

  if (lpbin->text_pad) {
    GST_LOG_OBJECT (lpbin, "unlinking text pad in fcbin from sink");
    lpsink_sinkpad =
        g_object_get_data (G_OBJECT (lpbin->text_pad), "lpsink.sinkpad");
    gst_pad_unlink (lpbin->text_pad, lpsink_sinkpad);

    /* release back */
    GST_LOG_OBJECT (lpbin, "release sink pad for text");
    gst_lp_sink_release_pad (lpbin->lpsink, lpsink_sinkpad);

    gst_object_unref (lpbin->text_pad);
    lpbin->text_pad = NULL;
  }

  if (lpsink_sinkpad)
    gst_object_unref (lpsink_sinkpad);

  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->pad_added_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->pad_removed_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->no_more_pads_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->source_element_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->drained_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->unknown_type_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->autoplug_factories_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->autoplug_continue_id);
  REMOVE_SIGNAL (lpbin, lpbin->audio_tags_changed_id);
  REMOVE_SIGNAL (lpbin, lpbin->video_tags_changed_id);
  REMOVE_SIGNAL (lpbin, lpbin->text_tags_changed_id);
  REMOVE_SIGNAL (lpbin, lpbin->pad_blocked_id);
  REMOVE_SIGNAL (lpbin, lpbin->element_added_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->fcbin_pad_added_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->fcbin_no_more_pads_id);

  if (lpbin->fcbin) {
    gst_element_set_state (GST_ELEMENT_CAST (lpbin->fcbin), GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (lpbin), lpbin->fcbin);
    lpbin->fcbin = NULL;
  }

  if (lpbin->uridecodebin) {
    gst_element_set_state (GST_ELEMENT_CAST (lpbin->uridecodebin),
        GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (lpbin), lpbin->uridecodebin);
    lpbin->uridecodebin = NULL;
  }
}

static GstStateChangeReturn
gst_lp_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstLpBin *lpbin;

  lpbin = GST_LP_BIN (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      gst_lp_bin_setup_element (lpbin);
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
//      gst_lp_bin_make_link(lpbin);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_lp_bin_deactive (lpbin);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;

  /* ERRORS */
failure:
  {
    return ret;
  }

}

static void
gst_lp_bin_set_sink (GstLpBin * lpbin, GstElement ** elem, const gchar * dbg,
    GstElement * sink)
{
  GST_DEBUG_OBJECT (lpbin, "Setting %s sink to %" GST_PTR_FORMAT, dbg, sink);

  GST_OBJECT_LOCK (lpbin);
  if (*elem != sink) {
    GstElement *old;

    old = *elem;
    if (sink)
      gst_object_ref_sink (sink);
    *elem = sink;
    if (old)
      gst_object_unref (old);
  }
  GST_DEBUG_OBJECT (lpbin, "%s sink now %" GST_PTR_FORMAT, dbg, *elem);
  GST_OBJECT_UNLOCK (lpbin);
}

static GstElement *
gst_lp_bin_get_current_sink (GstLpBin * lpbin, GstElement ** elem,
    const gchar * dbg, GstLpSinkType type)
{
  if (!lpbin->lpsink)
    return NULL;

  GstElement *sink = gst_lp_sink_get_sink (lpbin->lpsink, type);

  GST_LOG_OBJECT (lpbin, "play_sink_get_sink() returned %s sink %"
      GST_PTR_FORMAT ", the originally set %s sink is %" GST_PTR_FORMAT,
      dbg, sink, dbg, *elem);

  if (sink == NULL) {
    GST_OBJECT_LOCK (lpbin);
    if ((sink = *elem))
      gst_object_ref (sink);
    GST_OBJECT_UNLOCK (lpbin);
  }

  return sink;
}

static void
gst_lp_bin_element_added_cb (GstBin * bin, GstElement * element,
    gpointer user_data)
{
  GstState state;
  GstElementFactory *factory = NULL;
  const gchar *klass = NULL;
  gchar *elem_name = NULL;
  GstLpBin *lpbin = (GstLpBin *) user_data;

  factory = gst_element_get_factory (element);
  klass = gst_element_factory_get_klass (factory);
  elem_name = gst_element_get_name (element);
  state = GST_STATE (element);

  if (g_strrstr (klass, "Bin"))
    g_signal_connect (element, "element-added",
        G_CALLBACK (gst_lp_bin_element_added_cb), lpbin);

  GST_INFO_OBJECT (GST_ELEMENT_CAST (lpbin), "%s element added, (state = %d)",
      elem_name, state);

  g_free (elem_name);
}

static gboolean
autoplug_continue_signal (GstElement * element, GstPad * pad, GstCaps * caps,
    GstLpBin * lpbin)
{
  GstPad *opad = NULL;
  GstElement *elem = NULL;
  gboolean result;

  g_signal_emit (lpbin,
      gst_lp_bin_signals[SIGNAL_AUTOPLUG_CONTINUE], 0, pad, caps, &result);

  GST_LOG_OBJECT (lpbin, "autoplug_continue_notify, result = %d", result);

  if (opad)
    gst_object_unref (opad);
  return result;
}

static gint
compare_factories_func (gconstpointer p1, gconstpointer p2)
{
  GstPluginFeature *f1, *f2;
  gint diff;
  gboolean is_sink1, is_sink2;
  gboolean is_parser1, is_parser2;

  f1 = (GstPluginFeature *) p1;
  f2 = (GstPluginFeature *) p2;

  is_sink1 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f1),
      GST_ELEMENT_FACTORY_TYPE_SINK);
  is_sink2 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f2),
      GST_ELEMENT_FACTORY_TYPE_SINK);
  is_parser1 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f1),
      GST_ELEMENT_FACTORY_TYPE_PARSER);
  is_parser2 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f2),
      GST_ELEMENT_FACTORY_TYPE_PARSER);

  /* First we want all sinks as we prefer a sink if it directly
   * supports the current caps */
  if (is_sink1 && !is_sink2)
    return -1;
  else if (!is_sink1 && is_sink2)
    return 1;

  /* Then we want all parsers as we always want to plug parsers
   * before decoders */
  if (is_parser1 && !is_parser2)
    return -1;
  else if (!is_parser1 && is_parser2)
    return 1;

  /* And if it's a both a parser or sink we first sort by rank
   * and then by factory name */
  diff = gst_plugin_feature_get_rank (f2) - gst_plugin_feature_get_rank (f1);
  if (diff != 0)
    return diff;

  diff = strcmp (GST_OBJECT_NAME (f2), GST_OBJECT_NAME (f1));

  return diff;
}

/* Must be called with elements lock! */
static void
gst_lp_bin_update_elements_list (GstLpBin * lpbin)
{
  GList *res, *tmp;
  guint cookie;

  cookie = gst_registry_get_feature_list_cookie (gst_registry_get ());
  if (!lpbin->elements || lpbin->elements_cookie != cookie) {
    if (lpbin->elements)
      gst_plugin_feature_list_free (lpbin->elements);
    res =
        gst_element_factory_list_get_elements
        (GST_ELEMENT_FACTORY_TYPE_DECODABLE, GST_RANK_MARGINAL);
    tmp =
        gst_element_factory_list_get_elements
        (GST_ELEMENT_FACTORY_TYPE_AUDIOVIDEO_SINKS, GST_RANK_MARGINAL);
    lpbin->elements = g_list_concat (res, tmp);
    lpbin->elements = g_list_sort (lpbin->elements, compare_factories_func);
    lpbin->elements_cookie = cookie;
  }
}

/* Like gst_element_factory_can_sink_any_caps() but doesn't
 * allow ANY caps on the sinkpad template */
static gboolean
_factory_can_sink_caps (GstElementFactory * factory, GstCaps * caps)
{
  const GList *templs;

  templs = gst_element_factory_get_static_pad_templates (factory);

  while (templs) {
    GstStaticPadTemplate *templ = (GstStaticPadTemplate *) templs->data;

    if (templ->direction == GST_PAD_SINK) {
      GstCaps *templcaps = gst_static_caps_get (&templ->static_caps);

      if (!gst_caps_is_any (templcaps)
          && gst_caps_can_intersect (templcaps, caps)) {
        gst_caps_unref (templcaps);
        return TRUE;
      }
      gst_caps_unref (templcaps);
    }
    templs = g_list_next (templs);
  }

  return FALSE;
}

static GValueArray *
autoplug_factories_signal (GstElement * decodebin, GstPad * pad,
    GstCaps * caps, GstLpBin * lpbin)
{
  GST_LOG_OBJECT (lpbin, "autoplug_factories_notify");

  GValueArray *result;

  g_signal_emit (lpbin, gst_lp_bin_signals[SIGNAL_AUTOPLUG_FACTORIES], 0, pad,
      caps, &result);

  GST_LOG_OBJECT (lpbin, "autoplug_factories_notify, result = %p", result);

  return result;
}

static gboolean
sink_accepts_caps (GstElement * sink, GstCaps * caps)
{
  GstPad *sinkpad;

  /* ... activate it ... We do this before adding it to the bin so that we
   * don't accidentally make it post error messages that will stop
   * everything. */
  if (GST_STATE (sink) < GST_STATE_READY &&
      gst_element_set_state (sink,
          GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
    return FALSE;
  }

  if ((sinkpad = gst_element_get_static_pad (sink, "sink"))) {
    /* Got the sink pad, now let's see if the element actually does accept the
     * caps that we have */
    if (!gst_pad_query_accept_caps (sinkpad, caps)) {
      gst_object_unref (sinkpad);
      return FALSE;
    }
    gst_object_unref (sinkpad);
  }

  return TRUE;
}

static gboolean
_gst_boolean_accumulator (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer dummy)
{
  gboolean myboolean;

  myboolean = g_value_get_boolean (handler_return);
  if (!(ihint->run_type & G_SIGNAL_RUN_CLEANUP))
    g_value_set_boolean (return_accu, myboolean);

  /* stop emission if FALSE */
  return myboolean;
}

static gboolean
_gst_array_accumulator (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer dummy)
{
  gpointer array;

  array = g_value_get_boxed (handler_return);
  if (!(ihint->run_type & G_SIGNAL_RUN_CLEANUP))
    g_value_set_boxed (return_accu, array);

  return FALSE;
}

static gboolean
gst_lp_bin_autoplug_continue (GstElement * element, GstPad * pad,
    GstCaps * caps)
{
  gboolean ret = TRUE;
  GstElement *sink;
  GstPad *sinkpad = NULL;
  GstLpBin *lpbin = GST_LP_BIN_CAST (element);

  GST_OBJECT_LOCK (lpbin);

  if ((sink = lpbin->audio_sink)) {
    sinkpad = gst_element_get_static_pad (sink, "sink");
    if (sinkpad) {
      GstCaps *sinkcaps;

      /* Ignore errors here, if a custom sink fails to go
       * to READY things are wrong and will error out later
       */
      if (GST_STATE (sink) < GST_STATE_READY)
        gst_element_set_state (sink, GST_STATE_READY);

      sinkcaps = gst_pad_query_caps (sinkpad, NULL);
      if (!gst_caps_is_any (sinkcaps))
        ret = !gst_pad_query_accept_caps (sinkpad, caps);
      gst_caps_unref (sinkcaps);
      gst_object_unref (sinkpad);
    }
  }
  if (!ret)
    goto done;

  if ((sink = lpbin->video_sink)) {
    sinkpad = gst_element_get_static_pad (sink, "sink");
    if (sinkpad) {
      GstCaps *sinkcaps;

      /* Ignore errors here, if a custom sink fails to go
       * to READY things are wrong and will error out later
       */
      if (GST_STATE (sink) < GST_STATE_READY)
        gst_element_set_state (sink, GST_STATE_READY);

      sinkcaps = gst_pad_query_caps (sinkpad, NULL);
      if (!gst_caps_is_any (sinkcaps))
        ret = !gst_pad_query_accept_caps (sinkpad, caps);
      gst_caps_unref (sinkcaps);
      gst_object_unref (sinkpad);
    }
  }

done:
  GST_OBJECT_UNLOCK (lpbin);

  GST_LOG_OBJECT (pad,
      "continue autoplugging lpbin %p for %" GST_PTR_FORMAT ": %d",
      lpbin, caps, ret);

  return ret;
}

static GValueArray *
gst_lp_bin_autoplug_factories (GstElement * element, GstPad * pad,
    GstCaps * caps)
{
  GList *mylist, *tmp;
  GValueArray *result;
  GstLpBin *lpbin = GST_LP_BIN_CAST (element);

  GST_LOG_OBJECT (pad, "factories lpbin %p for %" GST_PTR_FORMAT, lpbin, caps);

  /* filter out the elements based on the caps. */
  g_mutex_lock (&lpbin->elements_lock);
  gst_lp_bin_update_elements_list (lpbin);
  mylist =
      gst_element_factory_list_filter (lpbin->elements, caps, GST_PAD_SINK,
      FALSE);
  g_mutex_unlock (&lpbin->elements_lock);

  GST_LOG_OBJECT (lpbin, "found factories %p", mylist);
  GST_PLUGIN_FEATURE_LIST_DEBUG (mylist);

  /* 2 additional elements for the already set audio/video sinks */
  result = g_value_array_new (g_list_length (mylist) + 2);

  /* Check if we already have an audio/video sink and if this is the case
   * put it as the first element of the array */
  if (lpbin->audio_sink) {
    GstElementFactory *factory = gst_element_get_factory (lpbin->audio_sink);

    if (factory && _factory_can_sink_caps (factory, caps)) {
      GValue val = { 0, };

      g_value_init (&val, G_TYPE_OBJECT);
      g_value_set_object (&val, factory);
      result = g_value_array_append (result, &val);
      g_value_unset (&val);
    }
  }

  if (lpbin->video_sink) {
    GstElementFactory *factory = gst_element_get_factory (lpbin->video_sink);

    if (factory && _factory_can_sink_caps (factory, caps)) {
      GValue val = { 0, };

      g_value_init (&val, G_TYPE_OBJECT);
      g_value_set_object (&val, factory);
      result = g_value_array_append (result, &val);
      g_value_unset (&val);
    }
  }

  for (tmp = mylist; tmp; tmp = tmp->next) {
    GstElementFactory *factory = GST_ELEMENT_FACTORY_CAST (tmp->data);
    GST_LOG_OBJECT (lpbin, "factories, factory name = %s",
        GST_OBJECT_NAME (factory));
    GValue val = { 0, };

    if ( /*lpbin->audio_sink && */ gst_element_factory_list_is_type (factory,
            GST_ELEMENT_FACTORY_TYPE_SINK |
            GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)) {
      continue;
    }

    if ( /*lpbin->video_sink && */ gst_element_factory_list_is_type (factory,
            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO
            | GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE)) {
      continue;
    }

    g_value_init (&val, G_TYPE_OBJECT);
    g_value_set_object (&val, factory);
    g_value_array_append (result, &val);
    GST_LOG_OBJECT (lpbin, "factories, factory name = %s added",
        GST_OBJECT_NAME (factory));
    g_value_unset (&val);
  }
  gst_plugin_feature_list_free (mylist);

  return result;
}

static void
audio_tags_changed_cb (GstElement * fcbin, gint stream_id, GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "stream_id = %d", stream_id);

  g_signal_emit (G_OBJECT (lpbin),
      gst_lp_bin_signals[SIGNAL_AUDIO_TAGS_CHANGED], 0, stream_id);
}

static void
video_tags_changed_cb (GstElement * fcbin, gint stream_id, GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "stream_id = %d", stream_id);

  g_signal_emit (G_OBJECT (lpbin),
      gst_lp_bin_signals[SIGNAL_VIDEO_TAGS_CHANGED], 0, stream_id);
}

static void
text_tags_changed_cb (GstElement * fcbin, gint stream_id, GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "stream_id = %d", stream_id);

  g_signal_emit (G_OBJECT (lpbin),
      gst_lp_bin_signals[SIGNAL_TEXT_TAGS_CHANGED], 0, stream_id);
}

static GstTagList *
get_tags (GstLpBin * lpbin, gint type, gint stream)
{
  GstTagList *result;
  GPtrArray *channels;
  GstPad *sinkpad;

  switch (type) {
    case LPBIN_STREAM_AUDIO:
      channels = lpbin->audio_channels;
      break;
    case LPBIN_STREAM_VIDEO:
      channels = lpbin->video_channels;
      break;
    case LPBIN_STREAM_TEXT:
      channels = lpbin->text_channels;
      break;
    default:
      channels = NULL;
      break;
  }

  if (!channels || stream >= channels->len) {
    GST_WARNING_OBJECT (lpbin, "get_tags : channels is empty");
    return NULL;
  }

  sinkpad = g_ptr_array_index (channels, stream);
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (sinkpad), "tags")) {
    GST_DEBUG_OBJECT (lpbin, "get_tags : %s has tags property",
        GST_PAD_NAME (sinkpad));
    g_object_get (sinkpad, "tags", &result, NULL);
  } else {
    GST_DEBUG_OBJECT (lpbin, "get_tags : there is a taglist in funnel : %s",
        GST_PAD_NAME (sinkpad));
    result = g_object_get_data (G_OBJECT (sinkpad), "funnel.taglist");
  }
  GST_DEBUG_OBJECT (lpbin, "get_tags : result = %p", result);
  return result;
}

static GstTagList *
gst_lp_bin_get_video_tags (GstLpBin * lpbin, gint stream)
{
  GstTagList *result;

  GST_LP_BIN_LOCK (lpbin);
  result = get_tags (lpbin, LPBIN_STREAM_VIDEO, stream);
  GST_LP_BIN_UNLOCK (lpbin);

  return result;
}

static GstTagList *
gst_lp_bin_get_audio_tags (GstLpBin * lpbin, gint stream)
{
  GstTagList *result;

  GST_LP_BIN_LOCK (lpbin);
  result = get_tags (lpbin, LPBIN_STREAM_AUDIO, stream);
  GST_LP_BIN_UNLOCK (lpbin);

  return result;
}

static GstTagList *
gst_lp_bin_get_text_tags (GstLpBin * lpbin, gint stream)
{
  GstTagList *result;

  GST_LP_BIN_LOCK (lpbin);
  result = get_tags (lpbin, LPBIN_STREAM_TEXT, stream);
  GST_LP_BIN_UNLOCK (lpbin);

  return result;
}

static GstPad *
gst_lp_bin_get_video_pad (GstLpBin * lpbin, gint stream)
{
  GstPad *sinkpad = NULL;

  GST_LP_BIN_LOCK (lpbin);
  if (stream < lpbin->video_channels->len) {
    sinkpad = g_ptr_array_index (lpbin->video_channels, stream);
    gst_object_ref (sinkpad);
  }
  GST_LP_BIN_UNLOCK (lpbin);

  return sinkpad;
}

static GstPad *
gst_lp_bin_get_audio_pad (GstLpBin * lpbin, gint stream)
{
  GstPad *sinkpad = NULL;

  GST_LP_BIN_LOCK (lpbin);
  if (stream < lpbin->audio_channels->len) {
    sinkpad = g_ptr_array_index (lpbin->audio_channels, stream);
    gst_object_ref (sinkpad);
  }
  GST_LP_BIN_UNLOCK (lpbin);

  return sinkpad;
}

static GstPad *
gst_lp_bin_get_text_pad (GstLpBin * lpbin, gint stream)
{
  GstPad *sinkpad = NULL;

  GST_LP_BIN_LOCK (lpbin);
  if (stream < lpbin->text_channels->len) {
    sinkpad = g_ptr_array_index (lpbin->text_channels, stream);
    gst_object_ref (sinkpad);
  }
  GST_LP_BIN_UNLOCK (lpbin);

  return sinkpad;
}
