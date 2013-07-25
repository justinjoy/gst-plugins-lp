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
#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (fc_bin_debug);
#define GST_CAT_DEFAULT fc_bin_debug

#define parent_class gst_fc_bin_parent_class
G_DEFINE_TYPE (GstFCBin, gst_fc_bin, GST_TYPE_BIN);

#define DEFAULT_N_VIDEO         0
#define DEFAULT_CURRENT_VIDEO   0
#define DEFAULT_N_AUDIO         0
#define DEFAULT_CURRENT_AUDIO   0
#define DEFAULT_N_TEXT          0
#define DEFAULT_CURRENT_TEXT    0
#define DEFAULT_VIDEO_BYPASS FALSE
#define DEFAULT_AUDIO_BYPASS FALSE
#define DEFAULT_TEXT_BYPASS FALSE

enum
{
  PROP_0,
  PROP_N_VIDEO,
  PROP_CURRENT_VIDEO,
  PROP_N_AUDIO,
  PROP_CURRENT_AUDIO,
  PROP_N_TEXT,
  PROP_CURRENT_TEXT,
  PROP_VIDEO_BYPASS,
  PROP_AUDIO_BYPASS,
  PROP_TEXT_BYPASS,
  PROP_LAST
};

static void gst_fc_bin_finalize (GObject * obj);
static void gst_fc_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_fc_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);
static GstPad *gst_fc_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static gboolean array_has_value (const gchar * values[], const gchar * value);
static GstStateChangeReturn gst_fc_bin_change_state (GstElement * element,
    GstStateChange transition);
static GstIterator *gst_fc_bin_pad_iterate_linked_pads (GstPad * pad,
    GstObject * parent);
static GstFlowReturn gst_fc_bin_chain_bypass (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);
static gboolean gst_fc_bin_pad_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

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
   * GstFcBin:current-text:
   *
   * Get or set the currently playing subtitle stream. By default the first
   * subtitle stream with data is played.
   */
  g_object_class_install_property (gobject_klass, PROP_CURRENT_TEXT,
      g_param_spec_int ("current-text", "Current Text",
          "Currently playing text stream (-1 = auto)",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_VIDEO_BYPASS,
      g_param_spec_boolean ("video-bypass", "video by-pass",
          "Video by-pass mode", DEFAULT_VIDEO_BYPASS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_AUDIO_BYPASS,
      g_param_spec_boolean ("audio-bypass", "audio by-pass",
          "Audio by-pass mode", DEFAULT_AUDIO_BYPASS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_TEXT_BYPASS,
      g_param_spec_boolean ("text-bypass", "text by-pass",
          "Text by-pass mode", DEFAULT_TEXT_BYPASS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_fc_bin_change_state);
  element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_fc_bin_request_new_pad);

  GST_DEBUG_CATEGORY_INIT (fc_bin_debug, "fcbin", 0, "Flow Controller");
}

static void
gst_fc_bin_init (GstFCBin * fcbin)
{
  GST_DEBUG_OBJECT (fcbin, "initializing");

  g_rec_mutex_init (&fcbin->lock);
  g_cond_init (&fcbin->cond);
  fcbin->blocked = FALSE;
  fcbin->flushing = FALSE;

  fcbin->select[GST_FC_BIN_STREAM_AUDIO].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_AUDIO].media_list[0] = "audio/";
  fcbin->select[GST_FC_BIN_STREAM_VIDEO].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_VIDEO].media_list[0] = "video/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[0] = "text/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[1] =
      "application/x-subtitle";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[2] = "application/x-ssa";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[3] = "application/x-ass";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[4] = "subpicture/x-dvd";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[5] = "subpicture/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[6] = "subtitle/";

  fcbin->current_video = DEFAULT_CURRENT_VIDEO;
  fcbin->current_audio = DEFAULT_CURRENT_AUDIO;
  fcbin->current_text = DEFAULT_CURRENT_TEXT;

  fcbin->video_bypass = DEFAULT_VIDEO_BYPASS;
  fcbin->audio_bypass = DEFAULT_AUDIO_BYPASS;
  fcbin->text_bypass = DEFAULT_TEXT_BYPASS;

  fcbin->setup_caps = FALSE;
}



static void
gst_fc_bin_finalize (GObject * obj)
{
  GstFCBin *fcbin;

  fcbin = GST_FC_BIN (obj);

  g_list_foreach (fcbin->video_caps_list, (GFunc) gst_object_unref, NULL);
  g_list_free (fcbin->video_caps_list);
  fcbin->video_caps_list = NULL;

  g_list_foreach (fcbin->audio_caps_list, (GFunc) gst_object_unref, NULL);
  g_list_free (fcbin->audio_caps_list);
  fcbin->audio_caps_list = NULL;

  g_list_foreach (fcbin->text_caps_list, (GFunc) gst_object_unref, NULL);
  g_list_free (fcbin->text_caps_list);
  fcbin->text_caps_list = NULL;

  g_rec_mutex_clear (&fcbin->lock);
  g_cond_clear (&fcbin->cond);

  fcbin->blocked = FALSE;

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

static gboolean
gst_fc_bin_set_current_text_stream (GstFCBin * fcbin, gint stream)
{
  GPtrArray *channels;
  GstPad *sinkpad;

  GST_FC_BIN_LOCK (fcbin);

  GST_DEBUG_OBJECT (fcbin, "Changing current text stream %d -> %d",
      fcbin->current_text, stream);

  if (!(channels = fcbin->select[GST_FC_BIN_STREAM_TEXT].channels))
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
    GST_WARNING_OBJECT (fcbin, "can't switch text, we have no channels");
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
    case PROP_CURRENT_TEXT:
      GST_FC_BIN_LOCK (fcbin);
      g_value_set_int (value, fcbin->current_text);
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    case PROP_VIDEO_BYPASS:
      g_value_set_boolean (value, fcbin->video_bypass);
      break;
    case PROP_AUDIO_BYPASS:
      g_value_set_boolean (value, fcbin->audio_bypass);
      break;
    case PROP_TEXT_BYPASS:
      g_value_set_boolean (value, fcbin->text_bypass);
      break;
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
    case PROP_CURRENT_TEXT:
      gst_fc_bin_set_current_text_stream (fcbin, g_value_get_int (value));
      break;
    case PROP_VIDEO_BYPASS:
      fcbin->video_bypass = g_value_get_boolean (value);
      break;
    case PROP_AUDIO_BYPASS:
      fcbin->audio_bypass = g_value_get_boolean (value);
      break;
    case PROP_TEXT_BYPASS:
      fcbin->text_bypass = g_value_get_boolean (value);
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

guint
get_next_sinkpad_idx (GstFCBin * fcbin)
{
  gchar *pad_name = NULL;
  GstPad *sinkpad = NULL;
  gboolean sinkpad_done = FALSE;
  guint idx = 0;

  while (sinkpad_done == FALSE) {
    pad_name = g_strdup_printf ("sink%u", idx);
    sinkpad = gst_element_get_static_pad (GST_ELEMENT (fcbin), pad_name);

    if (sinkpad == NULL)
      sinkpad_done = TRUE;
    else
      idx++;
  }
  g_free (pad_name);
  GST_DEBUG_OBJECT (fcbin, "get_next_sinkpad_idx : idx = %d", idx);
  return idx;
}

static gboolean
gst_fc_bin_pad_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = TRUE;
  GstFCBin *fcbin;
  GstPad *srcpad = NULL;

  fcbin = GST_FC_BIN (parent);

  GST_FC_BIN_LOCK (fcbin);
  srcpad = g_object_get_data (G_OBJECT (pad), "fcbin.srcpad");

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
      GST_DEBUG_OBJECT (fcbin, "gst_fc_bin_pad_event : GST_EVENT_FLUSH_START");
      /* Unblock the pad if it's waiting */
      guint flushing = 1;
      g_object_set_data (G_OBJECT (pad), "flushing",
          GINT_TO_POINTER (flushing));
      GST_FC_BIN_BROADCAST (fcbin);
      break;
    default:
      break;
  }

  GST_FC_BIN_UNLOCK (fcbin);

  res = gst_pad_push_event (srcpad, event);
  GST_DEBUG_OBJECT (fcbin,
      "gst_fc_bin_pad_event : GST_EVENT_FLUSH_START res = %d", res);
  return res;
}

/* must be called with the SELECTOR_LOCK, will block while the pad is blocked 
 * or return TRUE when flushing */
static gboolean
gst_fc_bin_wait (GstFCBin * fcbin, GstPad * pad)
{
  guint flushing;
  flushing = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pad), "flushing"));
  while (fcbin->blocked && !fcbin->flushing && flushing == 0) {
    GST_DEBUG_OBJECT (fcbin, "gst_fc_bin_wait");
    /* we can be unlocked here when we are shutting down (flushing) or when we
     * get unblocked */
    flushing = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pad), "flushing"));
    GST_FC_BIN_WAIT (fcbin);
  }
  GST_DEBUG_OBJECT (fcbin, "gst_fc_bin_wait : return fcbin->flushing = %d",
      fcbin->flushing);
  return fcbin->flushing;
}

static GstFlowReturn
gst_fc_bin_chain_bypass (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFCBin *fcbin;
  GstFlowReturn ret;
  GstPad *srcpad = NULL;
  GstClockTime start_time;

  fcbin = GST_FC_BIN (parent);

  GST_DEBUG_OBJECT (fcbin,
      "gst_fc_bin_chain_bypass : entering chain for buf %p with timestamp %"
      GST_TIME_FORMAT, buffer, GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

  GST_FC_BIN_LOCK (fcbin);

  /* wait or check for flushing */
  if (gst_fc_bin_wait (fcbin, pad)) {
    GST_FC_BIN_UNLOCK (fcbin);
    goto flushing;
  }

  srcpad = g_object_get_data (G_OBJECT (pad), "fcbin.srcpad");
  start_time = GST_BUFFER_TIMESTAMP (buffer);

  if (GST_CLOCK_TIME_IS_VALID (start_time)) {
    GST_DEBUG_OBJECT (fcbin,
        "gst_fc_bin_chain_bypass : received start time %" GST_TIME_FORMAT,
        GST_TIME_ARGS (start_time));
    if (GST_BUFFER_DURATION_IS_VALID (buffer))
      GST_DEBUG_OBJECT (fcbin,
          "gst_fc_bin_chain_bypass : received end time %" GST_TIME_FORMAT,
          GST_TIME_ARGS (start_time + GST_BUFFER_DURATION (buffer)));
  }

  GST_FC_BIN_BROADCAST (fcbin);
  GST_FC_BIN_UNLOCK (fcbin);

  if (srcpad) {
    /* forward */
    GST_DEBUG_OBJECT (fcbin,
        "gst_fc_bin_chain_bypass : Forwarding buffer %p with timestamp %"
        GST_TIME_FORMAT, buffer, GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

    ret = gst_pad_push (srcpad, gst_buffer_ref (buffer));
  }

done:
  return ret;

flushing:
  {
    GST_DEBUG_OBJECT (fcbin,
        "gst_fc_bin_chain_bypass : We are flushing, discard buffer %p", buffer);
    gst_buffer_unref (buffer);
    ret = GST_FLOW_FLUSHING;
    goto done;
  }
}

static GstIterator *
gst_fc_bin_pad_iterate_linked_pads (GstPad * pad, GstObject * parent)
{
  GstFCBin *fcbin;
  GstPad *opad;
  GstIterator *it = NULL;

  fcbin = GST_FC_BIN (parent);
  GST_DEBUG_OBJECT (fcbin, "gst_fc_bin_pad_iterate_linked_pads : pad = %s",
      gst_pad_get_name (pad));

  //GST_FC_BIN_LOCK (fcbin);

  opad = gst_object_ref (g_object_get_data (G_OBJECT (pad), "fcbin.srcpad"));

  if (opad) {
    GValue val = { 0, };
    g_value_init (&val, GST_TYPE_PAD);
    g_value_set_object (&val, opad);
    it = gst_iterator_new_single (GST_TYPE_PAD, &val);
    g_value_unset (&val);

    gst_object_unref (opad);
  }
  //GST_FC_BIN_UNLOCK (fcbin);

  return it;
}

GstCaps *
get_caps_by_type (GstFCBin * fcbin, gchar * type)
{
  GstCaps *caps = NULL;
  GList *walk = NULL;
  GST_DEBUG_OBJECT (fcbin, "get_caps_by_type : type = %s", type);
  if (g_str_has_prefix (type, "audio/"))
    walk = fcbin->audio_caps_list;
  else if (g_str_has_prefix (type, "video/"))
    walk = fcbin->video_caps_list;

  for (; walk; walk = g_list_next (walk)) {
    GstFCCaps *caps_prop = NULL;
    caps_prop = (GstFCCaps *) walk->data;
    if (caps_prop->used == FALSE) {
      caps = gst_caps_ref (caps_prop->caps);
      caps_prop->used = TRUE;
      goto done;
    }
  }

done:
  GST_DEBUG_OBJECT (fcbin, "get_caps_by_type : caps = %s",
      gst_caps_to_string (caps));
  return caps;
}

void
setup_bypass (GstFCBin * fcbin, GstCaps * caps)
{
  GstStructure *s;

  s = gst_caps_get_structure (caps, 0);
  GST_DEBUG_OBJECT (fcbin, "setup_bypass : caps = %s",
      gst_caps_to_string (caps));
  if (g_str_has_prefix (gst_structure_get_name (s), "video/")) {
    if (gst_structure_has_field (s, "typeof3D")) {
      GST_DEBUG_OBJECT (fcbin, "setup_bypass : has typeof3D");
      fcbin->video_bypass = TRUE;
    }
  }
}

void
setup_caps (GstFCBin * fcbin)
{
  GstStructure *structure = NULL;
  GValue value = { 0, };
  GstIterator *iter = NULL;
  gchar *name = NULL;
  gboolean done = FALSE;
  GstElement *element = NULL;
  GstElement *multiqueue = NULL;
  gboolean src_pad_done = FALSE;
  guint i = 0;
  GstElement *lpbin;

  lpbin = (GstElement *) gst_element_get_parent (GST_ELEMENT (fcbin));
  iter = gst_bin_iterate_recurse (GST_BIN (lpbin));

  while (!done) {
    switch (gst_iterator_next (iter, &value)) {
      case GST_ITERATOR_OK:
        element = GST_ELEMENT (g_value_get_object (&value));
        name = gst_object_get_name (GST_OBJECT (element));

        if (!strcmp (name, "multiqueue0")) {
          while (src_pad_done == FALSE) {
            GstPad *pad = NULL;
            GstCaps *caps = NULL;
            gchar *caps_str;
            gchar *pad_name;

            pad_name = g_strdup_printf ("src_%d", i++);
            pad = gst_element_get_static_pad (element, pad_name);

            if (pad != NULL) {
              if ((caps = gst_pad_query_caps (pad, 0)) != NULL) {
                structure = gst_caps_get_structure (caps, 0);
                if (structure != NULL) {
                  caps_str = gst_caps_to_string (caps);
                  if (g_str_has_prefix (caps_str, "audio/")) {
                    GstFCCaps *audio_caps = NULL;
                    setup_bypass (fcbin, caps);
                    audio_caps = g_slice_alloc0 (sizeof (GstFCCaps));
                    audio_caps->caps = gst_caps_ref (caps);
                    audio_caps->used = FALSE;
                    fcbin->audio_caps_list =
                        g_list_append (fcbin->audio_caps_list,
                        (GstFCCaps *) audio_caps);
                  } else if (g_str_has_prefix (caps_str, "video/")
                      || g_str_has_prefix (caps_str, "image/")) {
                    GstFCCaps *video_caps = NULL;
                    setup_bypass (fcbin, caps);
                    video_caps = g_slice_alloc0 (sizeof (GstFCCaps));
                    video_caps->caps = gst_caps_ref (caps);
                    video_caps->used = FALSE;
                    fcbin->video_caps_list =
                        g_list_append (fcbin->video_caps_list,
                        (GstFCCaps *) video_caps);
                  }
                  g_free (caps_str);
                  gst_caps_unref (caps);
                }
                g_free (pad_name);
                gst_object_unref (pad);
              } else {
                src_pad_done = TRUE;
                break;
              }
            } else {
              src_pad_done = TRUE;
              break;
            }
          }
        }

        g_free (name);
        element = NULL;
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iter);
        g_value_unset (&value);
        break;
      case GST_ITERATOR_ERROR:
        g_value_unset (&value);
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        g_value_unset (&value);
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (iter);

}

static GstPad *
gst_fc_bin_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name, const GstCaps * caps)
{
  GstFCBin *fcbin;
  const GstStructure *s;
  const gchar *in_name;
  gint i, j;
  GstFCSelect *select;
  GstPad *sinkpad, *ghost_sinkpad;
  GstCaps *in_caps;

  fcbin = GST_FC_BIN (element);

  if (fcbin->setup_caps == FALSE) {
    setup_caps (fcbin);
    fcbin->setup_caps = TRUE;
  }

  s = gst_caps_get_structure (caps, 0);
  in_name = gst_structure_get_name (s);

  GST_DEBUG_OBJECT (fcbin, "gst_fc_bin_request_new_pad : in_name = %s",
      in_name);

  if ((g_str_has_prefix (in_name, "video/") && fcbin->video_bypass)
      || (g_str_has_prefix (in_name, "audio/") && fcbin->audio_bypass)
      || (g_str_has_prefix (in_name, "text/") && fcbin->text_bypass)) {
    GstPadTemplate *pad_tmpl = NULL;
    GstPad *srcpad = NULL;
    guint idx = get_next_sinkpad_idx (fcbin);
    guint flushing = 0;

    gchar *sinkpad_name = g_strdup_printf ("sink%u", idx);
    gchar *srcpad_name = g_strdup_printf ("src%u", idx);

    pad_tmpl = gst_static_pad_template_get (&gst_fc_bin_sink_pad_template);
    sinkpad = gst_pad_new_from_template (pad_tmpl, sinkpad_name);
    gst_object_unref (pad_tmpl);
    g_free (sinkpad_name);

    pad_tmpl = gst_static_pad_template_get (&gst_fc_bin_src_pad_template);
    srcpad = gst_pad_new_from_template (pad_tmpl, srcpad_name);
    gst_object_unref (pad_tmpl);
    g_free (srcpad_name);

    gst_pad_set_iterate_internal_links_function (sinkpad,
        GST_DEBUG_FUNCPTR (gst_fc_bin_pad_iterate_linked_pads));

    gst_pad_set_chain_function (sinkpad,
        GST_DEBUG_FUNCPTR (gst_fc_bin_chain_bypass));

    gst_pad_set_event_function (sinkpad,
        GST_DEBUG_FUNCPTR (gst_fc_bin_pad_event));

    gst_pad_set_active (srcpad, TRUE);
    gst_pad_set_active (sinkpad, TRUE);

    gst_element_add_pad (GST_ELEMENT_CAST (fcbin), srcpad);
    gst_element_add_pad (GST_ELEMENT_CAST (fcbin), sinkpad);
    g_object_set_data (G_OBJECT (sinkpad), "fcbin.srcpad", srcpad);
    g_object_set_data (G_OBJECT (sinkpad), "fcbin.caps",
        get_caps_by_type (fcbin, in_name));
    g_object_set_data (G_OBJECT (sinkpad), "flushing",
        GINT_TO_POINTER (flushing));

    gst_pad_link (srcpad, sinkpad);

    return sinkpad;
  }

  for (i = 0; i < GST_FC_BIN_STREAM_LAST; i++) {
    for (j = 0; fcbin->select[i].media_list[j]; j++) {
      if (array_has_value (fcbin->select[i].media_list, in_name)) {
        select = &fcbin->select[i];
        break;
      }
    }
  }

  if (select == NULL)
    goto unknown_type;

  if (select->selector == NULL) {
    select->selector = gst_element_factory_make ("input-selector", NULL);

    if (select->selector == NULL) {
      /* gst_element_post_message (GST_ELEMENT_CAST (fcbin), 
         gst_missing_element_message_new (GST_ELEMENT_CAST (fcbin),
         "input-selector")); */
    } else {

      g_object_set (select->selector, "sync-streams", TRUE, NULL);

      g_signal_connect (select->selector, "notify::active-pad",
          G_CALLBACK (selector_active_pad_changed), fcbin);

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
      gst_element_add_pad (element, select->srcpad);
    }
  }

  if (select->selector) {
    gchar *pad_name = g_strdup_printf ("sink_%d", select->channels->len);
    if ((sinkpad = gst_element_get_request_pad (select->selector, pad_name))) {
      g_object_set_data (G_OBJECT (sinkpad), "fcbin.select", select);

      ghost_sinkpad = gst_ghost_pad_new (NULL, sinkpad);
      gst_pad_set_active (ghost_sinkpad, TRUE);
      gst_element_add_pad (element, ghost_sinkpad);
      g_object_set_data (G_OBJECT (ghost_sinkpad), "fcbin.srcpad",
          select->srcpad);
      g_object_set_data (G_OBJECT (ghost_sinkpad), "fcbin.caps",
          get_caps_by_type (fcbin, in_name));

      g_ptr_array_add (select->channels, sinkpad);
    }
  }

unknown_type:
  GST_ERROR_OBJECT (fcbin, "unknown type for pad %s", name);

done:
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
      GST_FC_BIN_LOCK (fcbin);
      fcbin->blocked = FALSE;
      fcbin->flushing = FALSE;
      GST_FC_BIN_UNLOCK (fcbin);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_FC_BIN_LOCK (fcbin);
      fcbin->blocked = FALSE;
      fcbin->flushing = TRUE;
      GST_FC_BIN_BROADCAST (fcbin);
      GST_FC_BIN_UNLOCK (fcbin);
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
/*
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
*/
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
