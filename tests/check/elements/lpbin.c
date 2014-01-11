/* GStreamer unit tests for lpbin
 *
 * Copyright (C) 2014 Hoonhee Lee <hoonhee.lee@lge.com>
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
# include <config.h>
#endif

#include <gst/gst.h>
#include <gst/check/gstcheck.h>
#include <gst/base/gstpushsrc.h>
#include <unistd.h>

static GType gst_red_video_src_get_type (void);

GST_START_TEST (test_uri)
{
  GstElement *lpbin, *fakesink;
  gchar *uri;

  fail_unless (gst_element_register (NULL, "redvideosrc", GST_RANK_PRIMARY,
          gst_red_video_src_get_type ()));

  lpbin = gst_element_factory_make ("lpbin", "lpbin");
  fail_unless (lpbin != NULL, "Failed to create lpbin element");

  fakesink = gst_element_factory_make ("fakesink", "fakesink");
  fail_unless (fakesink != NULL, "Failed to create fakesink element");

  g_object_set (lpbin, "video-sink", fakesink, NULL);

  g_object_set (lpbin, "uri", "redvideo://", NULL);
  g_object_get (lpbin, "uri", &uri, NULL);

  fail_unless_equals_string (uri, "redvideo://");
  g_free (uri);
  gst_object_unref (lpbin);
}

GST_END_TEST;

/*** redvideo:// source ***/

static GstURIType
gst_red_video_src_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_red_video_src_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "redvideo", NULL };

  return protocols;
}

static gchar *
gst_red_video_src_uri_get_uri (GstURIHandler * handler)
{
  return g_strdup ("redvideo://");
}

static gboolean
gst_red_video_src_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  return (uri != NULL && g_str_has_prefix (uri, "redvideo:"));
}

static void
gst_red_video_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_red_video_src_uri_get_type;
  iface->get_protocols = gst_red_video_src_uri_get_protocols;
  iface->get_uri = gst_red_video_src_uri_get_uri;
  iface->set_uri = gst_red_video_src_uri_set_uri;
}

static void
gst_red_video_src_init_type (GType type)
{
  static const GInterfaceInfo uri_hdlr_info = {
    gst_red_video_src_uri_handler_init, NULL, NULL
  };

  g_type_add_interface_static (type, GST_TYPE_URI_HANDLER, &uri_hdlr_info);
}

typedef GstPushSrc GstRedVideoSrc;
typedef GstPushSrcClass GstRedVideoSrcClass;

G_DEFINE_TYPE_WITH_CODE (GstRedVideoSrc, gst_red_video_src,
    GST_TYPE_PUSH_SRC, gst_red_video_src_init_type (g_define_type_id));

static GstFlowReturn
gst_red_video_src_create (GstPushSrc * src, GstBuffer ** p_buf)
{
  GstBuffer *buf;
  GstMapInfo map;
  guint w = 64, h = 64;
  guint size;

  size = w * h * 3 / 2;
  buf = gst_buffer_new_and_alloc (size);
  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  memset (map.data, 76, w * h);
  memset (map.data + (w * h), 85, (w * h) / 4);
  memset (map.data + (w * h) + ((w * h) / 4), 255, (w * h) / 4);
  gst_buffer_unmap (buf, &map);

  *p_buf = buf;
  return GST_FLOW_OK;
}

static GstCaps *
gst_red_video_src_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  guint w = 64, h = 64;
  return gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING,
      "I420", "width", G_TYPE_INT, w, "height",
      G_TYPE_INT, h, "framerate", GST_TYPE_FRACTION, 1, 1, NULL);
}

static void
gst_red_video_src_class_init (GstRedVideoSrcClass * klass)
{
  GstPushSrcClass *pushsrc_class = GST_PUSH_SRC_CLASS (klass);
  GstBaseSrcClass *basesrc_class = GST_BASE_SRC_CLASS (klass);
  static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("video/x-raw, format=(string)I420")
      );
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_templ));
  gst_element_class_set_metadata (element_class,
      "Red Video Src", "Source/Video", "yep", "me");

  pushsrc_class->create = gst_red_video_src_create;
  basesrc_class->get_caps = gst_red_video_src_get_caps;
}

static void
gst_red_video_src_init (GstRedVideoSrc * src)
{
}

static Suite *
lpbin_suite (void)
{
  Suite *s = suite_create ("lpbin");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_uri);

  return s;
}

GST_CHECK_MAIN (lpbin);
