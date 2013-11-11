/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Justin Joy <justin.joy.9to5@gmail.com> 
 *	         Wonchul Lee <wonchul86.lee@lge.com> 
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

#include "gstfcbin.h"

#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#include <stdlib.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (fc_bin_debug);
#define GST_CAT_DEFAULT fc_bin_debug

#define parent_class gst_fc_bin_parent_class
G_DEFINE_TYPE (GstFCBin, gst_fc_bin, GST_TYPE_BIN);

#define DEFAULT_N_VIDEO         0
#define DEFAULT_CURRENT_VIDEO   0
#define DEFAULT_N_AUDIO         0
#define DEFAULT_CURRENT_AUDIO   0
#define DEFAULT_N_TEXT          0

enum
{
  PROP_0,
  PROP_N_VIDEO,
  PROP_CURRENT_VIDEO,
  PROP_N_AUDIO,
  PROP_CURRENT_AUDIO,
  PROP_N_TEXT,
  PROP_LAST
};

/* signals */
enum
{
  SIGNAL_VIDEO_TAGS_CHANGED,
  SIGNAL_AUDIO_TAGS_CHANGED,
  SIGNAL_TEXT_TAGS_CHANGED,
  SIGNAL_ELEMENT_CONFIGURED,
  LAST_SIGNAL
};

typedef struct
{
  GstFCBin *fcbin;
  gint stream_id;
  GstLpSinkType type;
} NotifyTagsData;

static guint gst_fc_bin_signals[LAST_SIGNAL] = { 0 };

static void gst_fc_bin_finalize (GObject * obj);
static void gst_fc_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_fc_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);
static GstPad *gst_fc_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static gboolean array_has_value (const gchar * values[], const gchar * value);
static gboolean gst_fc_bin_funnel_pad_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
/*static GstStateChangeReturn gst_fc_bin_change_state (GstElement * element,
    GstStateChange transition);*/

static GstStaticPadTemplate gst_fc_bin_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_vsink_pad_template =
GST_STATIC_PAD_TEMPLATE ("video_sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_vsrc_pad_template =
GST_STATIC_PAD_TEMPLATE ("video_src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_asink_pad_template =
GST_STATIC_PAD_TEMPLATE ("audio_sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_asrc_pad_template =
GST_STATIC_PAD_TEMPLATE ("audio_src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_tsink_pad_template =
GST_STATIC_PAD_TEMPLATE ("text_sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_tsrc_pad_template =
GST_STATIC_PAD_TEMPLATE ("text_src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);



static void
gst_fc_bin_class_init (GstFCBinClass * klass)
{
  GObjectClass *gobject_klass;
  GstElementClass *element_class = (GstElementClass *) klass;

  gobject_klass = (GObjectClass *) klass;

  gobject_klass->set_property = gst_fc_bin_set_property;
  gobject_klass->get_property = gst_fc_bin_get_property;

  gobject_klass->finalize = gst_fc_bin_finalize;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_sink_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_asrc_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_asink_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_vsrc_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_vsink_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_tsrc_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_tsink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Flow Controller",
      "Lightweight/Controller/Flow",
      "data flow controller behind Fake Decoder",
      "Justin Joy <justin.joy.9to5@gmail.com, Wonchul Lee <wonchul86.lee@lge.com>");

  /**
   * GstFcBin:n-video
   *
   * Get the total number of available video streams.
   */
  g_object_class_install_property (gobject_klass, PROP_N_VIDEO,
      g_param_spec_int ("n-video", "Number Video",
          "Total number of video streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  /**
   * GstFcBin:current-video
   *
   * Get or set the currently playing video stream. By default the first video
   * stream with data is played.
   */
  g_object_class_install_property (gobject_klass, PROP_CURRENT_VIDEO,
      g_param_spec_int ("current-video", "Current Video",
          "Currently playing video stream (-1 = auto)",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstFcBin:n-audio
   *
   * Get the total number of available audio streams.
   */
  g_object_class_install_property (gobject_klass, PROP_N_AUDIO,
      g_param_spec_int ("n-audio", "Number Audio",
          "Total number of audio streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  /**
   * GstFcBin:current-audio
   *
   * Get or set the currently playing audio stream. By default the first audio
   * stream with data is played.
   */
  g_object_class_install_property (gobject_klass, PROP_CURRENT_AUDIO,
      g_param_spec_int ("current-audio", "Current audio",
          "Currently playing audio stream (-1 = auto)",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstFcBin:n-text
   *
   * Get the total number of available subtitle streams.
   */
  g_object_class_install_property (gobject_klass, PROP_N_TEXT,
      g_param_spec_int ("n-text", "Number Text",
          "Total number of text streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * GstFCBin::video-tags-changed
   * @fcbin: a #GstFCBin
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
  gst_fc_bin_signals[SIGNAL_VIDEO_TAGS_CHANGED] =
      g_signal_new ("video-tags-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstFCBinClass, video_tags_changed), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * GstFCBin::audio-tags-changed
   * @fcbin: a #GstFCBin
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
  gst_fc_bin_signals[SIGNAL_AUDIO_TAGS_CHANGED] =
      g_signal_new ("audio-tags-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstFCBinClass, audio_tags_changed), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * GstFCBin::text-tags-changed
   * @fcbin: a #GstFCBin
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
  gst_fc_bin_signals[SIGNAL_TEXT_TAGS_CHANGED] =
      g_signal_new ("text-tags-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstFCBinClass, text_tags_changed), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * GstFCBin::element-configured
   * @fcbin: a #GstFCBin
   *
   * This signal is emitted after input-selector or funnel element has been created and linked.
   *
   */
  gst_fc_bin_signals[SIGNAL_ELEMENT_CONFIGURED] =
      g_signal_new ("element-configured", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstFCBinClass, element_configured), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 3, G_TYPE_INT, GST_TYPE_PAD,
      GST_TYPE_PAD);

  //element_class->change_state = GST_DEBUG_FUNCPTR (gst_fc_bin_change_state);
  element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_fc_bin_request_new_pad);

  GST_DEBUG_CATEGORY_INIT (fc_bin_debug, "fcbin", 0, "Flow Controller");
}

static void
gst_fc_bin_init (GstFCBin * fcbin)
{
  GST_DEBUG_OBJECT (fcbin, "initializing");

  g_rec_mutex_init (&fcbin->lock);

  fcbin->select[GST_FC_BIN_STREAM_AUDIO].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_AUDIO].media_list[0] = "audio/";
  fcbin->select[GST_FC_BIN_STREAM_AUDIO].type = GST_LP_SINK_TYPE_AUDIO;
  fcbin->select[GST_FC_BIN_STREAM_VIDEO].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_VIDEO].media_list[0] = "video/";
  fcbin->select[GST_FC_BIN_STREAM_VIDEO].type = GST_LP_SINK_TYPE_VIDEO;
  fcbin->select[GST_FC_BIN_STREAM_TEXT].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[0] = "text/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[1] =
      "application/x-subtitle";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[2] = "application/x-ssa";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[3] = "application/x-ass";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[4] = "subpicture/x-dvd";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[5] = "subpicture/x-dvb";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[6] = "subpicture/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[7] = "subtitle/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].type = GST_LP_SINK_TYPE_TEXT;

  fcbin->current_video = DEFAULT_CURRENT_VIDEO;
  fcbin->current_audio = DEFAULT_CURRENT_AUDIO;
}



static void
gst_fc_bin_finalize (GObject * obj)
{
  GstFCBin *fcbin;

  fcbin = GST_FC_BIN (obj);

  g_rec_mutex_clear (&fcbin->lock);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gboolean
gst_fc_bin_set_current_video_stream (GstFCBin * fcbin, gint stream)
{
  GPtrArray *channels;
  GstPad *sinkpad;

  GST_FC_BIN_LOCK (fcbin);

  GST_DEBUG_OBJECT (fcbin, "Changing current video stream %d -> %d",
      fcbin->current_video, stream);

  if (!(channels = fcbin->select[GST_FC_BIN_STREAM_VIDEO].channels))
    goto no_channels;

  if (stream == -1 || channels->len <= stream) {
    sinkpad = NULL;
  } else {
    /* take channel from selected stream */
    sinkpad = g_ptr_array_index (channels, stream);
  }

  if (sinkpad)
    gst_object_ref (sinkpad);
  GST_FC_BIN_UNLOCK (fcbin);

  if (sinkpad) {
    GstObject *selector;

    if ((selector = gst_pad_get_parent (sinkpad))) {
      GstPad *old_sinkpad;

      g_object_get (selector, "active-pad", &old_sinkpad, NULL);

      if (old_sinkpad != sinkpad) {
        /*if (gst_play_bin_send_custom_event (selector,
           "playsink-custom-video-flush"))
           fcbin->video_pending_flush_finish = TRUE; */

        /* activate the selected pad */
        g_object_set (selector, "active-pad", sinkpad, NULL);
      }

      gst_object_unref (selector);
    }
    gst_object_unref (sinkpad);
  }
  return TRUE;

no_channels:
  {
    GST_FC_BIN_UNLOCK (fcbin);
    GST_WARNING_OBJECT (fcbin, "can't switch video, we have no channels");
    return FALSE;
  }
}

static gboolean
gst_fc_bin_set_current_audio_stream (GstFCBin * fcbin, gint stream)
{
  GPtrArray *channels;
  GstPad *sinkpad;

  GST_FC_BIN_LOCK (fcbin);

  GST_DEBUG_OBJECT (fcbin, "Changing current audio stream %d -> %d",
      fcbin->current_audio, stream);

  if (!(channels = fcbin->select[GST_FC_BIN_STREAM_AUDIO].channels))
    goto no_channels;

  if (stream == -1 || channels->len <= stream) {
    sinkpad = NULL;
  } else {
    /* take channel from selected stream */
    sinkpad = g_ptr_array_index (channels, stream);
  }

  if (sinkpad)
    gst_object_ref (sinkpad);
  GST_FC_BIN_UNLOCK (fcbin);

  if (sinkpad) {
    GstObject *selector;

    if ((selector = gst_pad_get_parent (sinkpad))) {
      GstPad *old_sinkpad;

      g_object_get (selector, "active-pad", &old_sinkpad, NULL);

      if (old_sinkpad != sinkpad) {
        /*if (gst_play_bin_send_custom_event (selector,
           "playsink-custom-audio-flush"))
           fcbin->audio_pending_flush_finish = TRUE; */

        /* activate the selected pad */
        g_object_set (selector, "active-pad", sinkpad, NULL);
      }

      gst_object_unref (selector);
    }
    gst_object_unref (sinkpad);
  }
  return TRUE;

no_channels:
  {
    GST_FC_BIN_UNLOCK (fcbin);
    GST_WARNING_OBJECT (fcbin, "can't switch audio, we have no channels");
    return FALSE;
  }
}

static void
gst_fc_bin_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstFCBin *fcbin = GST_FC_BIN (object);

  switch (prop_id) {
    case PROP_N_VIDEO:
    {
      gint n_video;
      GPtrArray *channels_video;

      GST_FC_BIN_LOCK (fcbin);
      channels_video =
          g_ptr_array_ref (fcbin->select[GST_FC_BIN_STREAM_VIDEO].channels);
      n_video = (channels_video ? channels_video->len : 0);
      g_value_set_int (value, n_video);

      g_ptr_array_unref (channels_video);
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    }
    case PROP_CURRENT_VIDEO:
      GST_FC_BIN_LOCK (fcbin);
      g_value_set_int (value, fcbin->current_video);
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    case PROP_N_AUDIO:
    {
      gint n_audio;
      GPtrArray *channels_audio;

      GST_FC_BIN_LOCK (fcbin);
      channels_audio =
          g_ptr_array_ref (fcbin->select[GST_FC_BIN_STREAM_AUDIO].channels);
      n_audio = (channels_audio ? channels_audio->len : 0);
      g_value_set_int (value, n_audio);

      g_ptr_array_unref (channels_audio);
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    }
    case PROP_CURRENT_AUDIO:
      GST_FC_BIN_LOCK (fcbin);
      g_value_set_int (value, fcbin->current_audio);
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    case PROP_N_TEXT:
    {
      gint n_text;
      GPtrArray *channels_text;

      GST_FC_BIN_LOCK (fcbin);
      channels_text =
          g_ptr_array_ref (fcbin->select[GST_FC_BIN_STREAM_TEXT].channels);
      n_text = (channels_text ? channels_text->len : 0);
      g_value_set_int (value, n_text);

      g_ptr_array_unref (channels_text);
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_fc_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstFCBin *fcbin = GST_FC_BIN (object);

  switch (prop_id) {
    case PROP_CURRENT_VIDEO:
      gst_fc_bin_set_current_video_stream (fcbin, g_value_get_int (value));
      break;
    case PROP_CURRENT_AUDIO:
      gst_fc_bin_set_current_audio_stream (fcbin, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static int
get_current_stream_number (GstFCBin * fcbin, GPtrArray * channels)
{
  /* Internal API cleanup would make this easier... */
  int i;
  GstPad *pad, *current;
  GstObject *selector = NULL;
  int ret = -1;

  for (i = 0; i < channels->len; i++) {
    pad = g_ptr_array_index (channels, i);
    if ((selector = gst_pad_get_parent (pad))) {
      g_object_get (selector, "active-pad", &current, NULL);
      gst_object_unref (selector);

      if (pad == current) {
        gst_object_unref (current);
        ret = i;
        break;
      }

      if (current)
        gst_object_unref (current);
    }
  }

  return ret;
}


static void
selector_active_pad_changed (GObject * selector, GParamSpec * pspec,
    GstFCBin * fcbin)
{
  const gchar *property;
  GstFCSelect *select = NULL;
  gint i;

  GST_FC_BIN_LOCK (fcbin);

  for (i = 0; i < GST_FC_BIN_STREAM_LAST; i++) {
    if (selector == G_OBJECT (fcbin->select[i].selector)) {
      select = &fcbin->select[i];
    }
  }

  if (!select) {
    GST_FC_BIN_UNLOCK (fcbin);
    return;
  }

  switch (select->type) {
    case GST_LP_SINK_TYPE_VIDEO:
      property = "current-video";
      fcbin->current_video =
          get_current_stream_number (fcbin, select->channels);
      break;
    case GST_LP_SINK_TYPE_AUDIO:
      property = "current-audio";
      fcbin->current_audio =
          get_current_stream_number (fcbin, select->channels);
      break;
    case GST_LP_SINK_TYPE_TEXT:
      property = "current-text";
      fcbin->current_text = get_current_stream_number (fcbin, select->channels);
      break;

    default:
      property = NULL;
  }
  GST_FC_BIN_UNLOCK (fcbin);

  if (property)
    g_object_notify (G_OBJECT (fcbin), property);
}

static void
notify_tags_cb (GObject * object, GParamSpec * pspec, gpointer user_data)
{
  NotifyTagsData *ntdata = (NotifyTagsData *) user_data;
  gint signal;

  GST_DEBUG_OBJECT (ntdata->fcbin,
      "notify_tags_cb : ntdata->type = %d, ntdata->stream_id = %d",
      ntdata->type, ntdata->stream_id);

  switch (ntdata->type) {
    case GST_LP_SINK_TYPE_VIDEO:
      signal = SIGNAL_VIDEO_TAGS_CHANGED;
      break;
    case GST_LP_SINK_TYPE_AUDIO:
      signal = SIGNAL_AUDIO_TAGS_CHANGED;
      break;
    default:
      signal = -1;
      break;
  }

  if (signal >= 0) {
    g_signal_emit (G_OBJECT (ntdata->fcbin), gst_fc_bin_signals[signal], 0,
        ntdata->stream_id);
  }
}

static gboolean
gst_fc_bin_funnel_pad_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
    {
      GstTagList *tags, *oldtags, *newtags;
      NotifyTagsData *ntdata;

      gst_event_parse_tag (event, &tags);
      oldtags = g_object_get_data (G_OBJECT (pad), "funnel.taglist");
      newtags = gst_tag_list_merge (oldtags, tags, GST_TAG_MERGE_REPLACE);

      g_object_set_data (G_OBJECT (pad), "funnel.taglist", newtags);

      ntdata = g_object_get_data (G_OBJECT (pad), "funnel.tagdata");
      g_signal_emit (G_OBJECT (ntdata->fcbin),
          gst_fc_bin_signals[SIGNAL_TEXT_TAGS_CHANGED], 0, ntdata->stream_id);

      if (oldtags)
        gst_tag_list_unref (oldtags);
      GST_DEBUG_OBJECT (pad, "received tags %" GST_PTR_FORMAT, newtags);
    }
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static void
gst_fc_bin_do_configure (GstFCBin * fcbin, GstPad * ghost_sinkpad,
    GstLpSinkType type, gboolean multiple_stream)
{
  GstFCSelect *select = NULL;
  GstPad *sinkpad = NULL;

  if (type == GST_LP_SINK_TYPE_AUDIO) {
    select = &fcbin->select[GST_FC_BIN_STREAM_AUDIO];
  } else if (type == GST_LP_SINK_TYPE_VIDEO) {
    select = &fcbin->select[GST_FC_BIN_STREAM_VIDEO];
  } else if (type == GST_LP_SINK_TYPE_TEXT) {
    select = &fcbin->select[GST_FC_BIN_STREAM_TEXT];
  } else {
    GST_ERROR_OBJECT (fcbin, "unknown type for pad %s",
        GST_DEBUG_PAD_NAME (ghost_sinkpad));
    return;
  }

  if (select->selector == NULL) {
    if (multiple_stream)
      select->selector = gst_element_factory_make ("funnel", NULL);
    else
      select->selector = gst_element_factory_make ("input-selector", NULL);

    if (select->selector == NULL) {
      /* gst_element_post_message (GST_ELEMENT_CAST (fcbin), 
         gst_missing_element_message_new (GST_ELEMENT_CAST (fcbin),
         "input-selector")); */
    } else {
      if (!multiple_stream) {
        g_object_set (select->selector, "sync-streams", TRUE, NULL);

        g_signal_connect (select->selector, "notify::active-pad",
            G_CALLBACK (selector_active_pad_changed), fcbin);
      }

      GST_DEBUG_OBJECT (fcbin, "adding new selector %p", select->selector);
      gst_bin_add (GST_BIN_CAST (fcbin), select->selector);
      gst_element_set_state (select->selector, GST_STATE_PAUSED);
    }
  }

  if (select->srcpad == NULL) {
    if (select->selector) {
      GstPad *sel_srcpad;

      sel_srcpad = gst_element_get_static_pad (select->selector, "src");
      select->srcpad = gst_ghost_pad_new (NULL, sel_srcpad);
      gst_pad_set_active (select->srcpad, TRUE);
      gst_element_add_pad (GST_ELEMENT (fcbin), select->srcpad);
    }
  }
  if (select->selector) {
    gchar *pad_name = g_strdup_printf ("sink_%d", select->channels->len);
    if ((sinkpad = gst_element_get_request_pad (select->selector, pad_name))) {
      g_object_set_data (G_OBJECT (sinkpad), "fcbin.select", select);

      gulong notify_tags_handler = 0;
      NotifyTagsData *ntdata;

      ntdata = g_new0 (NotifyTagsData, 1);
      ntdata->fcbin = fcbin;
      ntdata->stream_id = select->channels->len;
      ntdata->type = type;

      if (multiple_stream) {
        gst_pad_set_event_function (sinkpad,
            GST_DEBUG_FUNCPTR (gst_fc_bin_funnel_pad_event));
        g_object_set_data (G_OBJECT (sinkpad), "funnel.tagdata", ntdata);
      }

      if (g_object_class_find_property (G_OBJECT_GET_CLASS (sinkpad), "tags")) {
        notify_tags_handler =
            g_signal_connect_data (G_OBJECT (sinkpad), "notify::tags",
            G_CALLBACK (notify_tags_cb), ntdata, (GClosureNotify) g_free,
            (GConnectFlags) 0);
      }

      gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (ghost_sinkpad), sinkpad);
      //gst_pad_set_active (ghost_sinkpad, TRUE);
      //gst_element_add_pad (element, ghost_sinkpad);
      g_object_set_data (G_OBJECT (ghost_sinkpad), "fcbin.srcpad",
          select->srcpad);

      g_signal_emit (G_OBJECT (fcbin),
          gst_fc_bin_signals[SIGNAL_ELEMENT_CONFIGURED], 0, type, sinkpad,
          select->srcpad);

      g_ptr_array_add (select->channels, sinkpad);
    }
    g_free (pad_name);
  }

}

static void
caps_notify_cb (GstPad * pad, GParamSpec * unused, GstFCBin * fcbin)
{
  GstCaps *caps = NULL;
  gchar *caps_str = NULL;
  GstStructure *s = NULL;
  gboolean multiple_stream = FALSE;

  g_object_get (pad, "caps", &caps, NULL);
  s = gst_caps_get_structure (caps, 0);
  caps_str = gst_caps_to_string (caps);

  GST_INFO_OBJECT (fcbin, "caps_notify_cb : caps = %s", caps_str);

  if (gst_structure_has_field (s, "multiple-stream")) {
    multiple_stream =
        g_value_get_boolean (gst_structure_get_value (s, "multiple-stream"));
  }

  GST_INFO_OBJECT (fcbin, "caps_notify_cb : multiple_stream = %d",
      multiple_stream);

  if (gst_ghost_pad_get_target (GST_GHOST_PAD (pad)) != NULL) {
    GST_DEBUG_OBJECT (fcbin,
        "caps_notify_cb : pad = %s already has target",
        GST_DEBUG_PAD_NAME (pad));
    goto done;
  }

  GST_FC_BIN_LOCK (fcbin);
  if (g_str_has_prefix (caps_str, "video/")
      || g_str_has_prefix (caps_str, "image/jpeg")) {
    gst_fc_bin_do_configure (fcbin, pad, GST_LP_SINK_TYPE_VIDEO,
        multiple_stream);
  } else if (g_str_has_prefix (caps_str, "audio/")) {
    gst_fc_bin_do_configure (fcbin, pad, GST_LP_SINK_TYPE_AUDIO,
        multiple_stream);
  } else if (g_str_has_prefix (caps_str, "text/")
      || g_str_has_prefix (caps_str, "application/")
      || g_str_has_prefix (caps_str, "subpicture/")) {
    gst_fc_bin_do_configure (fcbin, pad, GST_LP_SINK_TYPE_TEXT, TRUE);
  }
  GST_FC_BIN_UNLOCK (fcbin);

done:
  if (caps_str)
    g_free (caps_str);
  if (caps)
    gst_caps_unref (caps);
}

static GstPad *
gst_fc_bin_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name, const GstCaps * caps)
{
  GstFCBin *fcbin;
  GstPad *ghost_sinkpad = NULL;
  const GstStructure *s;
  const gchar *in_name;

  fcbin = GST_FC_BIN (element);

  s = gst_caps_get_structure (caps, 0);
  in_name = gst_structure_get_name (s);

  ghost_sinkpad = gst_ghost_pad_new_no_target (NULL, GST_PAD_SINK);
  g_signal_connect (G_OBJECT (ghost_sinkpad), "notify::caps",
      G_CALLBACK (caps_notify_cb), fcbin);

  gst_pad_set_active (ghost_sinkpad, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (fcbin), ghost_sinkpad);

  return ghost_sinkpad;
}

//TODO need to implement resource deallocation code.
/*
static void
gst_fc_bin_release_pad (GstElement * element, GstPad *pad)
{
	GstFCBin * fcbin;
	GstFCSelect *select;
	GstPad *sinkpad;
	GstFCSelect *select;

	fcbin = GST_FC_BIN (element);

	sinkpad = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad));
	select = g_object_get_data (G_OBJECT(sinkpad), "fcbin.select");
	
	//gst_element_release_request_pad (select->selector, ); //FIXME 
	
}
*/

void
gst_fc_bin_release_pad (GstFCBin * fcbin, GstPad * pad)
{
  GstPad **res = NULL;
  GstFCSelect *select;
  GstPad *sinkpad;
  GstPad *srcpad;

  GST_DEBUG_OBJECT (fcbin, "release pad %" GST_PTR_FORMAT, pad);

  GST_FC_BIN_LOCK (fcbin);

  sinkpad = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad));
  select = g_object_get_data (G_OBJECT (sinkpad), "fcbin.select");
  srcpad = g_object_get_data (G_OBJECT (pad), "fcbin.srcpad");

  gst_element_release_request_pad (select->selector, sinkpad);
  gst_object_unref (sinkpad);

  res = &pad;

  if (pad) {
    GST_DEBUG_OBJECT (fcbin, "deactivate pad %" GST_PTR_FORMAT, *res);
    gst_pad_set_active (*res, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (fcbin), *res);
    *res = NULL;
  }

  if (srcpad) {
    gst_pad_set_active (srcpad, FALSE);
    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (srcpad), NULL);
    gst_element_remove_pad (GST_ELEMENT_CAST (fcbin), srcpad);
    srcpad = NULL;
  }

  if (select->selector) {
    gst_element_set_state (select->selector, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (fcbin), select->selector);
    select->selector = NULL;
  }

  GST_FC_BIN_UNLOCK (fcbin);
}

static gboolean
array_has_value (const gchar * values[], const gchar * value)
{
  gint i;

  for (i = 0; values[i]; i++) {
    if (g_str_has_prefix (value, values[i]))
      return TRUE;
  }
  return FALSE;
}

/*
static GstStateChangeReturn
gst_fc_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstFCBin *fcbin;

  fcbin = GST_FC_BIN (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
    {
      GstIterator *it;
      GstPad *fcbin_sinkpad;
      gboolean done = FALSE;
      GValue item = { 0, };
      gchar *fcbin_sinkpad_name;

      GST_FC_BIN_LOCK (fcbin);
      it = gst_element_iterate_sink_pads (fcbin);

      while (!done) {
        switch (gst_iterator_next (it, &item)) {
          case GST_ITERATOR_OK:
            fcbin_sinkpad = g_value_get_object (&item);
            fcbin_sinkpad_name = GST_PAD_NAME (fcbin_sinkpad);
            gst_fc_bin_release_pad (fcbin, fcbin_sinkpad);
            g_value_reset (&item);
            break;
          case GST_ITERATOR_RESYNC:
            gst_iterator_resync (it);
            break;
          case GST_ITERATOR_ERROR:
            done = TRUE;
            break;
          case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        }
      }
      g_free (fcbin_sinkpad_name);
      g_value_unset (&item);
      gst_iterator_free (it);
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    }
    default:
      break;
  }
  return ret;*/

  /* ERRORS */
/*failure:
  {
    return ret;
  }

}
*/
