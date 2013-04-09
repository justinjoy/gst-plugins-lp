/* GStreamer Lightweight Plugins
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

GST_DEBUG_CATEGORY_STATIC (gst_lp_bin_debug);
#define GST_CAT_DEFAULT gst_lp_bin_debug

enum
{
  PROP_0,
  PROP_URI,
  PROP_LAST
};
enum
{
  SIGNAL_SOURCE_SETUP,
  LAST_SIGNAL
};

static void gst_lp_bin_class_init (GstLpBinClass * klass);
static void gst_lp_bin_init (GstLpBin * lpbin);
static void gst_lp_bin_finalize (GObject * object);

static void gst_lp_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_lp_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);

static void no_more_pads_cb (GstElement * decodebin, GstPad * pad,
    GstLpBin * lpbin);
static void pad_added_cb (GstElement * decodebin, GstPad * pad,
    GstLpBin * lpbin);
static gboolean gst_lp_bin_setup_element (GstLpBin * lpbin);
static GstStateChangeReturn gst_lp_bin_change_state (GstElement * element,
    GstStateChange transition);


static void gst_lp_bin_handle_message (GstBin * bin, GstMessage * message);
static gboolean gst_lp_bin_query (GstElement * element, GstQuery * query);

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

  gstelement_klass->change_state = GST_DEBUG_FUNCPTR (gst_lp_bin_change_state);
  gstelement_klass->query = GST_DEBUG_FUNCPTR (gst_lp_bin_query);

  gstbin_klass->handle_message = GST_DEBUG_FUNCPTR (gst_lp_bin_handle_message);
}

static void
gst_lp_bin_init (GstLpBin * lpbin)
{
  GST_DEBUG_CATEGORY_INIT (gst_lp_bin_debug, "lpbin", 0,
      "Lightweight Play Bin");
  g_rec_mutex_init (&lpbin->lock);
  lpbin->uridecodebin = NULL;
  lpbin->lpsink = NULL;
}

static void
gst_lp_bin_finalize (GObject * obj)
{
  GstLpBin *lpbin;

  lpbin = GST_LP_BIN (obj);

  g_rec_mutex_clear (&lpbin->lock);

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

static void
gst_lp_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstLpBin *lpbin = GST_LP_BIN (object);

  switch (prop_id) {
    case PROP_URI:
      lpbin->uri = g_strdup (g_value_get_string (value));
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
  const gchar *sink_name;
  GstPad *lpsink_sinkpad;
  gint ret;

  caps = gst_pad_query_caps (pad, NULL);
  s = gst_caps_get_structure (caps, 0);
  name = gst_structure_get_name (s);

  GST_DEBUG_OBJECT (lpbin,
      "pad %s:%s with caps %" GST_PTR_FORMAT " added",
      GST_DEBUG_PAD_NAME (pad), caps);

  if (g_str_has_prefix (name, "video/")) {
    sink_name = "video_sink";
  } else if (g_str_has_prefix (name, "audio/")) {
    sink_name = "audio_sink";
  }
  lpsink_sinkpad = gst_element_get_request_pad (lpbin->lpsink, sink_name);
  ret = gst_pad_link (pad, lpsink_sinkpad);
  g_printf ("%d\n", ret);
}

static void
no_more_pads_cb (GstElement * decodebin, GstPad * pad, GstLpBin * lpbin)
{
}

static gboolean
gst_lp_bin_setup_element (GstLpBin * lpbin)
{
  GstCaps *fd_caps;

  fd_caps = gst_caps_from_string ("video/x-fd; audio/x-fd");

  lpbin->uridecodebin = gst_element_factory_make ("uridecodebin", NULL);
  g_object_set (lpbin->uridecodebin, "caps", fd_caps, "uri", lpbin->uri, NULL);
  lpbin->pad_added_id = g_signal_connect (lpbin->uridecodebin, "pad-added",
      G_CALLBACK (pad_added_cb), lpbin);
  lpbin->no_more_pads_id =
      g_signal_connect (lpbin->uridecodebin, "no-more-pads",
      G_CALLBACK (no_more_pads_cb), lpbin);

  gst_bin_add (GST_BIN_CAST (lpbin), lpbin->uridecodebin);

  lpbin->lpsink = gst_element_factory_make ("lpsink", NULL);
  gst_bin_add (GST_BIN_CAST (lpbin), lpbin->lpsink);

  g_object_unref (fd_caps);

  return TRUE;
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
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto failure;

  return ret;

  /* ERRORS */
failure:
  {
    return ret;
  }

}
