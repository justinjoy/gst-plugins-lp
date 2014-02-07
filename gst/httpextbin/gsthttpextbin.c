/* GStreamer httpextbin element
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *  Author : HoonHee Lee <hoonhee.lee@lge.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gsthttpextbin.h"

GST_DEBUG_CATEGORY_STATIC (http_ext_bin_debug);
#define GST_CAT_DEFAULT http_ext_bin_debug

#define parent_class gst_http_ext_bin_parent_class

static void gst_http_ext_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_http_ext_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_http_ext_bin_finalize (GObject * self);
static void gst_http_ext_bin_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (GstHttpExtBin, gst_http_ext_bin, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_http_ext_bin_uri_handler_init));

static void
gst_http_ext_bin_class_init (GstHttpExtBinClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_http_ext_bin_set_property;
  gobject_class->get_property = gst_http_ext_bin_get_property;
  gobject_class->finalize = gst_http_ext_bin_finalize;

  gst_element_class_set_static_metadata (gstelement_class,
      "Extended Http Source Bin", "Source/Bin/Network/Protocol",
      "HTTP Source with unified data processing element",
      "HoonHee Lee <hoonhee.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (http_ext_bin_debug, "httpextbin", 0,
      "Extended Http Source Bin");
}

static void
gst_http_ext_bin_init (GstHttpExtBin * bin)
{
  GST_OBJECT_FLAG_SET (bin, GST_ELEMENT_FLAG_SOURCE);
}

static void
gst_http_ext_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_http_ext_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_http_ext_bin_finalize (GObject * self)
{
  G_OBJECT_CLASS (parent_class)->finalize (self);
}

static GstURIType
gst_http_ext_bin_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_http_ext_bin_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http+bbts", NULL };

  return protocols;
}

static gchar *
gst_http_ext_bin_uri_get_uri (GstURIHandler * handler)
{
  return NULL;
}

static gboolean
gst_http_ext_bin_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  return TRUE;
}

static void
gst_http_ext_bin_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_http_ext_bin_uri_get_type;
  iface->get_protocols = gst_http_ext_bin_uri_get_protocols;
  iface->get_uri = gst_http_ext_bin_uri_get_uri;
  iface->set_uri = gst_http_ext_bin_uri_set_uri;
}
