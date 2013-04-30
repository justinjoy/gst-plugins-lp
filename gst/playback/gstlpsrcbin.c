/* GStreamer Lightweight Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Wonchul Lee <Wonchul86.lee@lge.com>
 *           Justin Joy <justin.joy.9to5@gmail.com> 
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

#include <gst/gst.h>
#include "gstlpsrcbin.h"


GST_DEBUG_CATEGORY_STATIC (gst_lp_src_bin_debug);
#define GST_CAT_DEFAULT gst_lp_src_bin_debug

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_URI,
  PROP_INKA_DRM
};

static void
gst_lp_src_bin_uri_handler_init (gpointer g_iface, gpointer iface_data);

static void
_do_init (GType type)
{
  GType g_define_type_id = type;

  G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, gst_lp_src_bin_uri_handler_init);

  GST_DEBUG_CATEGORY_INIT (gst_lp_src_bin_debug,
      "lpsrcbin", 0, "Lightweight Playback Source Bin");
}

G_DEFINE_TYPE_WITH_CODE (LPSrcBin, gst_lp_src_bin, GST_TYPE_BIN,
    _do_init (g_define_type_id));

#define parent_class gst_lp_src_bin_parent_class


static void
gst_lp_src_bin_finalize (GObject * self)
{
  LPSrcBin *lpsrcbin = GST_LP_SRC_BIN (self);

  /* call our parent method (always do this!) */

  if (lpsrcbin->uri)
    g_free (lpsrcbin->uri);

  G_OBJECT_CLASS (parent_class)->finalize (self);

}

static void
gst_lp_src_bin_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  LPSrcBin *lpsrcbin = GST_LP_SRC_BIN (object);
  GstURIHandler *uri_handler = GST_URI_HANDLER (lpsrcbin);
  GstURIHandlerInterface *uri_iface =
      GST_URI_HANDLER_GET_INTERFACE (uri_handler);

  gchar *uri;

  switch (property_id) {
    case PROP_URI:
      GST_OBJECT_LOCK (lpsrcbin);
      uri = g_value_dup_string (value);
      uri_iface->set_uri (uri_handler, uri, NULL);
      g_free (uri);

      GST_INFO_OBJECT (lpsrcbin, "setting up uri property [%s]", lpsrcbin->uri);
      GST_OBJECT_UNLOCK (lpsrcbin);

      break;
    case PROP_INKA_DRM:
      lpsrcbin->inka_drm = g_value_get_boolean (value);
      break;
    default:
      break;
  }
}

static void
gst_lp_src_bin_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  LPSrcBin *lpsrcbin = GST_LP_SRC_BIN (object);

  switch (property_id) {
    case PROP_URI:
      g_value_set_string (value, lpsrcbin->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

}

static void
gst_lp_src_bin_class_init (LPSrcBinClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) (klass);
  gstelement_class = (GstElementClass *) (klass);

  gobject_class->set_property = gst_lp_src_bin_set_property;
  gobject_class->get_property = gst_lp_src_bin_get_property;
  gobject_class->finalize = gst_lp_src_bin_finalize;

  g_object_class_install_property (gobject_class,
      PROP_URI,
      g_param_spec_string ("uri",
          "URI",
          "URI to decode", NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
gst_lp_src_bin_init (LPSrcBin * lpsrcbin)
{
  GST_INFO_OBJECT (lpsrcbin, "initializing lpsrc bin instance.");

  lpsrcbin->inka_drm = FALSE;
}

static const gchar *const *
gst_lp_src_bin_uri_get_protocols (GType type)
{
  static const gchar *const protocols[] = { "file", NULL };

  return protocols;
}

static gchar *
gst_lp_src_bin_uri_get_uri (GstURIHandler * handler)
{
  LPSrcBin *lpsrcbin = GST_LP_SRC_BIN (handler);
  return lpsrcbin->uri;
}

static gboolean
gst_lp_src_bin_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  gboolean ret = FALSE;

  GstURIHandlerInterface *iface;
  LPSrcBin *lpsrcbin = GST_LP_SRC_BIN (handler);
  GstPad *srcpad;
  GstURIHandler *internal_handler;

  if (lpsrcbin->inka_drm)
    lpsrcbin->source = gst_element_factory_make ("inka", NULL);
  else
    lpsrcbin->source = gst_element_factory_make ("filesrc", NULL);

  gst_bin_add (GST_BIN_CAST (lpsrcbin), GST_ELEMENT_CAST (lpsrcbin->source));

  srcpad = gst_element_get_static_pad (lpsrcbin->source, "src");
  lpsrcbin->srcpad = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (lpsrcbin->srcpad, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (lpsrcbin), lpsrcbin->srcpad);

  internal_handler = GST_URI_HANDLER (lpsrcbin->source);

  if (!internal_handler) {
    GST_ERROR_OBJECT (lpsrcbin, "cannot find internal uri handler");
    return FALSE;
  }

  iface = GST_URI_HANDLER_GET_INTERFACE (internal_handler);
  if (!iface) {
    GST_ERROR_OBJECT (lpsrcbin, "cannot find internal uri interface");
    return FALSE;
  }

  g_free (lpsrcbin->uri);
  lpsrcbin->uri = g_strdup (uri);


  ret = iface->set_uri (internal_handler, uri, NULL);

  return ret;
}

static GstURIType
gst_lp_src_bin_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static void
gst_lp_src_bin_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  GST_DEBUG_OBJECT (iface, "initializing uri handler");
  iface->get_type = gst_lp_src_bin_uri_get_type;

  iface->get_protocols = gst_lp_src_bin_uri_get_protocols;
  iface->get_uri = gst_lp_src_bin_uri_get_uri;
  iface->set_uri = gst_lp_src_bin_uri_set_uri;
}
