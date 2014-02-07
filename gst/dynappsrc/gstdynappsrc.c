/* GStreamer Dynamic App Source element
 * Copyright (C) 2014 LG Electronics, Inc.
 *  Author : Wonchul Lee <wonchul86.lee@lge.com>
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
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gstdynappsrc.h"

GST_DEBUG_CATEGORY_STATIC (dyn_appsrc_debug);
#define GST_CAT_DEFAULT dyn_appsrc_debug

#define parent_class gst_dyn_appsrc_parent_class

#define DEFAULT_PROP_URI NULL

enum
{
  PROP_0,
  PROP_URI,
  PROP_LAST
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static void gst_dyn_appsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dyn_appsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_dyn_appsrc_finalize (GObject * self);
static void gst_dyn_appsrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);


G_DEFINE_TYPE_WITH_CODE (GstDynAppSrc, gst_dyn_appsrc, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_dyn_appsrc_uri_handler_init));


static void
gst_dyn_appsrc_class_init (GstDynAppSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_dyn_appsrc_set_property;
  gobject_class->get_property = gst_dyn_appsrc_get_property;
  gobject_class->finalize = gst_dyn_appsrc_finalize;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));

  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("uri", "URI",
          "URI to get protected content",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "Dynappsrc", "Source/Bin",
      "Dynamic App Source", "Wonchul Lee <wonchul86.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (dyn_appsrc_debug, "dynappsrc", 0,
      "Dynamic App Source");
}

static void
gst_dyn_appsrc_init (GstDynAppSrc * bin)
{
  /* init member variable */
  bin->uri = g_strdup (DEFAULT_PROP_URI);

  GST_OBJECT_FLAG_SET (bin, GST_ELEMENT_FLAG_SOURCE);
}

static void
gst_dyn_appsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (object);

  switch (prop_id) {
    case PROP_URI:
    {
      GstURIHandler *handler;
      GstURIHandlerInterface *iface;

      handler = GST_URI_HANDLER (bin);
      iface = GST_URI_HANDLER_GET_INTERFACE (handler);
      iface->set_uri (handler, g_value_get_string (value), NULL);
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dyn_appsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (object);

  switch (prop_id) {
    case PROP_URI:
      g_value_set_string (value, bin->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dyn_appsrc_finalize (GObject * self)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (self);

  g_free (bin->uri);

  G_OBJECT_CLASS (parent_class)->finalize (self);
}

static GstURIType
gst_dyn_appsrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_dyn_appsrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "dynappsrc", NULL };

  return protocols;
}

static gchar *
gst_dyn_appsrc_uri_get_uri (GstURIHandler * handler)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (handler);

  return bin->uri ? g_strdup (bin->uri) : NULL;
}

static gboolean
gst_dyn_appsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  gboolean ret = FALSE;

  return ret;
}

static void
gst_dyn_appsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_dyn_appsrc_uri_get_type;
  iface->get_protocols = gst_dyn_appsrc_uri_get_protocols;
  iface->get_uri = gst_dyn_appsrc_uri_get_uri;
  iface->set_uri = gst_dyn_appsrc_uri_set_uri;
}
