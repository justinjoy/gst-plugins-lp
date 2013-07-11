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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "gstlpbin.h"
#include "gstlpsink.h"

GST_DEBUG_CATEGORY_STATIC (gst_lp_bin_debug);
#define GST_CAT_DEFAULT gst_lp_bin_debug

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
  PROP_THUMBNAIL_MODE,
  PROP_USE_BUFFERING,
  PROP_LAST
};

enum
{
  SIGNAL_ABOUT_TO_FINISH,
  SIGNAL_RETRIEVE_THUMBNAIL,
  SIGNAL_SOURCE_SETUP,
  SIGNAL_AUTOPLUG_CONTINUE,
  SIGNAL_AUTOPLUG_FACTORIES,
  SIGNAL_CAPS_VIDEO,
  LAST_SIGNAL
};

#define DEFAULT_THUMBNAIL_MODE FALSE
#define DEFAULT_USE_BUFFERING FALSE

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
static void pad_removec_cb (GstElement * decodebin, GstPad * pad,
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
static void gst_lp_bin_deactive_signal_handler (GstLpBin * lpbin);
static GstBuffer *gst_lp_bin_retrieve_thumbnail (GstLpBin * lpbin, gint width,
    gint height, gchar * format);
static void gst_lp_bin_set_thumbnail_mode (GstLpBin * lpbin,
    gboolean thumbnail_mode);
static GstStructure *gst_lp_bin_caps_video (GstLpBin * lpbin);

static GstElementClass *parent_class;

static guint gst_lp_bin_signals[LAST_SIGNAL] = { 0 };

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
      "Justin Joy <justin.joy.9to5@gmail.com>");

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

  g_object_class_install_property (gobject_klass, PROP_VIDEO_SINK,
      g_param_spec_object ("video-sink", "Video Sink",
          "the video output element to use (NULL = default sink)",
          GST_TYPE_ELEMENT, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_AUDIO_SINK,
      g_param_spec_object ("audio-sink", "Audio Sink",
          "the audio output element to use (NULL = default sink)",
          GST_TYPE_ELEMENT, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_THUMBNAIL_MODE,
      g_param_spec_boolean ("thumbnail-mode", "Thumbnail mode",
          "Thumbnail mode", DEFAULT_THUMBNAIL_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_USE_BUFFERING,
      g_param_spec_boolean ("use-buffering", "Use buffering",
          "set use-buffering property at multiqueue", DEFAULT_USE_BUFFERING,
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

  gst_lp_bin_signals[SIGNAL_CAPS_VIDEO] =
      g_signal_new ("caps-video", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstLpBinClass,
          caps_video), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_STRUCTURE, 0, G_TYPE_NONE);

  gstelement_klass->change_state = GST_DEBUG_FUNCPTR (gst_lp_bin_change_state);
  gstelement_klass->query = GST_DEBUG_FUNCPTR (gst_lp_bin_query);

  gstbin_klass->handle_message = GST_DEBUG_FUNCPTR (gst_lp_bin_handle_message);

  klass->autoplug_continue = GST_DEBUG_FUNCPTR (gst_lp_bin_autoplug_continue);
  klass->autoplug_factories = GST_DEBUG_FUNCPTR (gst_lp_bin_autoplug_factories);
  klass->retrieve_thumbnail = GST_DEBUG_FUNCPTR (gst_lp_bin_retrieve_thumbnail);
  klass->caps_video = GST_DEBUG_FUNCPTR (gst_lp_bin_caps_video);
}

static gboolean
gst_lp_bin_bus_cb (GstBus * bus, GstMessage * message, gpointer data)
{
  GstLpBin *lpbin = (GstLpBin *) data;
  GST_DEBUG_OBJECT (lpbin, "gst_lp_bin_bus_cb");
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED:
      GST_DEBUG_OBJECT (lpbin,
          "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, element name = %s",
          GST_OBJECT_NAME (GST_MESSAGE_SRC (message)));
      GstState oldstate, newstate, pending;
      gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);

      GST_DEBUG_OBJECT (lpbin,
          "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, state-changed: %s -> %s, pending-state = %s",
          gst_element_state_get_name (oldstate),
          gst_element_state_get_name (newstate),
          gst_element_state_get_name (pending));

      if (newstate == GST_STATE_READY
          && GST_IS_ELEMENT (GST_MESSAGE_SRC (message))) {
        GstElement *elem = NULL;
        gchar *elem_name = NULL;
        GstElementFactory *factory = NULL;
        const gchar *klass = NULL;

        elem = GST_ELEMENT (GST_MESSAGE_SRC (message));
        factory = gst_element_get_factory (elem);
        klass = gst_element_factory_get_klass (factory);

        if (strstr (klass, "Demux")) {
          gchar *elem_name = gst_element_get_name (elem);
          GST_DEBUG_OBJECT (lpbin,
              "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, element_name = %s",
              elem_name);

          if (g_object_class_find_property (G_OBJECT_GET_CLASS (elem),
                  "thumbnail-mode"))
            g_object_set (elem, "thumbnail-mode", TRUE, NULL);
          else
            GST_WARNING_OBJECT (lpbin,
                "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, %s doesn't thumbnail-mode property.",
                elem_name);
        } else if (strstr (klass, "Parse")) {
          gchar *elem_name = gst_element_get_name (elem);
          GST_DEBUG_OBJECT (lpbin,
              "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, element_name = %s",
              elem_name);

          /*if (g_object_class_find_property (G_OBJECT_GET_CLASS (elem), "thumbnail-mode"))
             g_object_set (elem, "thumbnail-mode", TRUE, NULL);
             else
             GST_WARNING_OBJECT (lpbin, "gstBusCallbackHandle : GST_MESSAGE_STATE_CHANGED, %s doesn't thumbnail-mode property.", elem_name); */
        } else if (strstr (klass, "Sink/Image")) {
          gchar *elem_name = gst_element_get_name (elem);
          GST_DEBUG_OBJECT (lpbin,
              "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, element_name = %s, klass = %s",
              elem_name, klass);
          if (g_object_class_find_property (G_OBJECT_GET_CLASS (elem),
                  "thumbnail-mode")) {
            if (lpbin->thumbnail_mode) {
              GST_DEBUG_OBJECT (lpbin,
                  "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, Sink/Image, set thumbnail-mode as TRUE");
              lpbin->video_sink = elem;
              g_object_set (elem, "thumbnail-mode", TRUE, NULL);
            }
          } else
            GST_DEBUG_OBJECT (lpbin,
                "gst_lp_bin_bus_cb : GST_MESSAGE_STATE_CHANGED, %s doesn't thumbnail-mode property.",
                elem_name);
        }

        g_free (elem_name);
      }
      break;
  }
  return TRUE;
}

static void
gst_lp_bin_init (GstLpBin * lpbin)
{
  GST_DEBUG_CATEGORY_INIT (gst_lp_bin_debug, "lpbin", 0,
      "Lightweight Play Bin");

  lpbin->bus = gst_pipeline_get_bus (GST_PIPELINE (lpbin));
  if (lpbin->bus) {
    gst_bus_add_signal_watch (lpbin->bus);
    lpbin->bus_msg_cb_id =
        g_signal_connect (lpbin->bus, "message", G_CALLBACK (gst_lp_bin_bus_cb),
        lpbin);
    gst_object_unref (lpbin->bus);
  }

  lpbin->caps_video_id =
      g_signal_connect (lpbin, "caps-video", G_CALLBACK (gst_lp_bin_caps_video),
      lpbin);

  g_rec_mutex_init (&lpbin->lock);

  /* first filter out the interesting element factories */
  g_mutex_init (&lpbin->elements_lock);

  lpbin->uridecodebin = NULL;
  lpbin->fcbin = NULL;
  lpbin->lpsink = NULL;
  lpbin->source = NULL;

  lpbin->naudio = 0;
  lpbin->nvideo = 0;
  lpbin->ntext = 0;

  lpbin->video_pad = NULL;
  lpbin->audio_pad = NULL;

  lpbin->thumbnail_mode = DEFAULT_THUMBNAIL_MODE;
  lpbin->use_buffering = DEFAULT_USE_BUFFERING;
}

static void
gst_lp_bin_finalize (GObject * obj)
{
  GstLpBin *lpbin;

  lpbin = GST_LP_BIN (obj);

  if (lpbin->source)
    gst_object_unref (lpbin->source);

  if (lpbin->video_sink) {
    gst_element_set_state (lpbin->video_sink, GST_STATE_NULL);
    gst_object_unref (lpbin->video_sink);
  }

  if (lpbin->audio_sink) {
    gst_element_set_state (lpbin->audio_sink, GST_STATE_NULL);
    gst_object_unref (lpbin->audio_sink);
  }

  g_rec_mutex_clear (&lpbin->lock);
  g_mutex_clear (&lpbin->elements_lock);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gboolean
gst_lp_bin_query (GstElement * element, GstQuery * query)
{
  GstLpBin *lpbin = GST_LP_BIN (element);

  gboolean ret;

  GST_LP_BIN_LOCK (lpbin);

  ret = GST_ELEMENT_CLASS (parent_class)->query (element, query);

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
  GST_DEBUG_OBJECT (lpbin,
      "retrieve_thumbnail : width = %d, height = %d, format = %s", width,
      height, format);
  GstBuffer *result = NULL;
  GstCaps *caps;

  if (!lpbin->video_sink) {
    GST_DEBUG_OBJECT (lpbin, "no video sink");
    return NULL;
  }

  caps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      "format", G_TYPE_STRING, format,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, NULL);

  GST_DEBUG_OBJECT (lpbin, "retrieve_thumbnail : video_sink name = %s",
      gst_element_get_name (lpbin->video_sink));
  g_signal_emit_by_name (G_OBJECT (lpbin->video_sink), "convert-sample", caps,
      &result);
  GST_DEBUG_OBJECT (lpbin, "retrieve_thumbnail : result = %p", result);
  gst_caps_unref (caps);
  return result;
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
    case PROP_VIDEO_SINK:
      gst_lp_bin_set_sink (lpbin, &lpbin->video_sink, "video",
          g_value_get_object (value));
      break;
    case PROP_AUDIO_SINK:
      gst_lp_bin_set_sink (lpbin, &lpbin->audio_sink, "audio",
          g_value_get_object (value));
      break;
    case PROP_THUMBNAIL_MODE:
      lpbin->thumbnail_mode = g_value_get_boolean (value);
      break;
    case PROP_USE_BUFFERING:
      lpbin->use_buffering = g_value_get_boolean (value);
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
      gint n_video;

      GST_LP_BIN_LOCK (lpbin);
      g_object_get (lpbin->fcbin, "n-video", &n_video, NULL);
      g_value_set_int (value, n_video);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_CURRENT_VIDEO:
    {
      gint current_video;

      GST_LP_BIN_LOCK (lpbin);
      g_object_get (lpbin->fcbin, "current-video", &current_video, NULL);
      g_value_set_int (value, current_video);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_N_AUDIO:
    {
      gint n_audio;

      GST_LP_BIN_LOCK (lpbin);
      g_object_get (lpbin->fcbin, "n-audio", &n_audio, NULL);
      g_value_set_int (value, n_audio);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_CURRENT_AUDIO:
    {
      gint current_audio;

      GST_LP_BIN_LOCK (lpbin);
      g_object_get (lpbin->fcbin, "current-audio", &current_audio, NULL);
      g_value_set_int (value, current_audio);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_N_TEXT:
    {
      gint n_text;

      GST_LP_BIN_LOCK (lpbin);
      g_object_get (lpbin->fcbin, "n-text", &n_text, NULL);
      g_value_set_int (value, n_text);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_CURRENT_TEXT:
    {
      gint current_text;

      GST_LP_BIN_LOCK (lpbin);
      g_object_get (lpbin->fcbin, "current-text", &current_text, NULL);
      g_value_set_int (value, current_text);
      GST_LP_BIN_UNLOCK (lpbin);
      break;
    }
    case PROP_THUMBNAIL_MODE:
      g_value_set_boolean (value, lpbin->thumbnail_mode);
      break;
    case PROP_USE_BUFFERING:
      g_value_set_boolean (value, lpbin->use_buffering);
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
  GstPadTemplate *tmpl;


  caps = gst_pad_query_caps (pad, NULL);
  s = gst_caps_get_structure (caps, 0);
  name = gst_structure_get_name (s);

  tmpl = gst_pad_template_new (name, GST_PAD_SINK, GST_PAD_REQUEST, caps);

  GST_DEBUG_OBJECT (lpbin,
      "pad %s:%s with caps %" GST_PTR_FORMAT " added",
      GST_DEBUG_PAD_NAME (pad), caps);

  fcbin_sinkpad = gst_element_request_pad (lpbin->fcbin, tmpl, name, caps);
  gst_pad_link (pad, fcbin_sinkpad);

  fcbin_srcpad = g_object_get_data (G_OBJECT (fcbin_sinkpad), "fcbin.srcpad");

  if (!lpbin->video_pad && g_str_has_prefix (name, "video/")) {
    lpbin->video_pad = fcbin_srcpad;
  } else if (!lpbin->audio_pad && g_str_has_prefix (name, "audio/")) {
    lpbin->audio_pad = fcbin_srcpad;
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
  GST_DEBUG_OBJECT (lpbin, "no more pads callback");
  GstPad *lpsink_sinkpad;

  GST_OBJECT_LOCK (lpbin);
  if (lpbin->audio_sink) {
    GST_DEBUG_OBJECT (lpbin, "setting custom audio sink %" GST_PTR_FORMAT,
        lpbin->audio_sink);
    gst_lp_sink_set_sink (lpbin->lpsink, GST_LP_SINK_TYPE_AUDIO,
        lpbin->audio_sink);
  }

  if (lpbin->video_sink) {
    GST_DEBUG_OBJECT (lpbin, "setting custom video sink %" GST_PTR_FORMAT,
        lpbin->video_sink);
    gst_lp_sink_set_sink (lpbin->lpsink, GST_LP_SINK_TYPE_VIDEO,
        lpbin->video_sink);
  }
  GST_OBJECT_UNLOCK (lpbin);

  if (lpbin->video_pad) {
    lpsink_sinkpad = gst_element_get_request_pad (lpbin->lpsink, "video_sink");
    gst_pad_link (lpbin->video_pad, lpsink_sinkpad);
  }

  if (lpbin->audio_pad) {
    lpsink_sinkpad = gst_element_get_request_pad (lpbin->lpsink, "audio_sink");
    gst_pad_link (lpbin->audio_pad, lpsink_sinkpad);
  }
}

static void
notify_source_cb (GstElement * decodebin, GParamSpec * pspec, GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "notify_source_cb");
  GstElement *source;

  g_object_get (lpbin->uridecodebin, "source", &source, NULL);

  GST_OBJECT_LOCK (lpbin);
  if ((lpbin->source != NULL) && (GST_IS_ELEMENT (lpbin->source))) {
    gst_object_unref (GST_OBJECT (lpbin->source));
  }
  lpbin->source = source;
  GST_OBJECT_UNLOCK (lpbin);

  g_object_notify (G_OBJECT (lpbin), "source");
  g_signal_emit (lpbin, gst_lp_bin_signals[SIGNAL_SOURCE_SETUP], 0,
      lpbin->source);
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

static void
gst_lp_bin_set_thumbnail_mode (GstLpBin * lpbin, gboolean thumbnail_mode)
{
  GST_DEBUG_OBJECT (lpbin, "set thumbnail mode to lpsink as %d",
      thumbnail_mode);
  gst_lp_sink_set_thumbnail_mode (lpbin->lpsink, thumbnail_mode);
}

static gboolean
gst_lp_bin_setup_element (GstLpBin * lpbin)
{
  GstCaps *fd_caps;

  fd_caps = gst_caps_from_string ("video/x-fd; audio/x-fd");

  lpbin->uridecodebin = gst_element_factory_make ("uridecodebin", NULL);

  g_object_set (lpbin->uridecodebin,
      "caps", fd_caps, "uri", lpbin->uri, NULL);

  if(lpbin->use_buffering)
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

  lpbin->lpsink = gst_element_factory_make ("lpsink", NULL);
  if (lpbin->lpsink && lpbin->thumbnail_mode) {
    GST_DEBUG_OBJECT (lpbin,
        "setup_element : set lpsink thumbnail-mode as TRUE");
    gst_lp_bin_set_thumbnail_mode (lpbin, lpbin->thumbnail_mode);
  }
  gst_bin_add (GST_BIN_CAST (lpbin), lpbin->lpsink);

  g_object_unref (fd_caps);

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
gst_lp_bin_deactive_signal_handler (GstLpBin * lpbin)
{
  GST_DEBUG_OBJECT (lpbin, "deactive_signal_handler");
  REMOVE_SIGNAL (lpbin->bus, lpbin->bus_msg_cb_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->pad_added_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->pad_removed_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->no_more_pads_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->source_element_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->drained_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->unknown_type_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->autoplug_factories_id);
  REMOVE_SIGNAL (lpbin->uridecodebin, lpbin->autoplug_continue_id);
  REMOVE_SIGNAL (lpbin, lpbin->caps_video_id);
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
    case GST_STATE_CHANGE_READY_TO_NULL:
      if (lpbin->audio_sink)
        gst_element_set_state (lpbin->audio_sink, GST_STATE_NULL);
      if (lpbin->video_sink)
        gst_element_set_state (lpbin->video_sink, GST_STATE_NULL);
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
    {
      gst_lp_bin_deactive_signal_handler (lpbin);

      if (lpbin->audio_sink)
        lpbin->audio_sink = NULL;
      if (lpbin->video_sink)
        lpbin->video_sink = NULL;
      break;
    }
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

static gboolean
autoplug_continue_signal (GstElement * element, GstPad * pad, GstCaps * caps,
    GstLpBin * lpbin)
{
  GST_LOG_OBJECT (lpbin, "autoplug_continue_notify");

  gboolean result;

  g_signal_emit (lpbin,
      gst_lp_bin_signals[SIGNAL_AUTOPLUG_CONTINUE], 0, pad, caps, &result);

  GST_LOG_OBJECT (lpbin, "autoplug_continue_notify, result = %d", result);

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
  GST_LOG_OBJECT (lpbin, "gst_lp_bin_autoplug_continue");

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

  GST_LOG_OBJECT (lpbin,
      "continue autoplugging lpbin %p for %s:%s, %" GST_PTR_FORMAT ": %d",
      lpbin, GST_DEBUG_PAD_NAME (pad), caps, ret);

  return ret;
}

static GValueArray *
gst_lp_bin_autoplug_factories (GstElement * element, GstPad * pad,
    GstCaps * caps)
{
  GList *mylist, *tmp;
  GValueArray *result;
  GstLpBin *lpbin = GST_LP_BIN_CAST (element);

  GST_LOG_OBJECT (lpbin, "factories lpbin %p for %s:%s, %" GST_PTR_FORMAT,
      lpbin, GST_DEBUG_PAD_NAME (pad), caps);

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

static GstStructure *
gst_lp_bin_caps_video (GstLpBin * lpbin)
{
  GstStructure *result;
  GstCaps *caps;

  //TODO At future, video_sink will be used instead of video_pad.
  if (!lpbin->video_pad)
    goto end;

  caps = gst_pad_get_current_caps (lpbin->video_pad);

  if (!caps) {
    GST_WARNING_OBJECT (lpbin, "video pad caps is not exist");
    goto end;
  }
  GST_DEBUG_OBJECT (lpbin, "video pad caps are %" GST_PTR_FORMAT, caps);

  result = gst_caps_get_structure (caps, 0);
  GST_DEBUG_OBJECT (lpbin, "video pad caps structure is %" GST_PTR_FORMAT,
      result);

  gst_caps_unref (caps);

end:
  return result;
}
