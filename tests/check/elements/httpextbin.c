/* GStreamer unit tests for the httpextbin
 *
 * Copyright 2014 LGE Corporation.
 *  @author: Hoonhee Lee <hoonhee.lee@lge.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <gst/check/gstcheck.h>
#include <stdlib.h>

static GType gst_http_filter_get_type (void);

typedef struct _GstHttpFilter GstHttpFilter;
typedef GstElementClass GstHttpFilterClass;

struct _GstHttpFilter
{
  GstElement parent;

  GstPad *sinkpad;
  GstPad *srcpad;
};

static GstStaticPadTemplate filter_sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("caps")
    );

static GstStaticPadTemplate filter_src_templ = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

G_DEFINE_TYPE (GstHttpFilter, gst_http_filter, GST_TYPE_ELEMENT);

static void
gst_http_filter_finalize (GObject * object)
{
  GstHttpFilter *demux = (GstHttpFilter *) object;

  G_OBJECT_CLASS (gst_http_filter_parent_class)->finalize (object);
}

static void
gst_http_filter_class_init (GstHttpFilterClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gobject_class->finalize = gst_http_filter_finalize;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&filter_sink_templ));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&filter_src_templ));

  gst_element_class_set_metadata (element_class,
      "HttpFilter", "Generic", "Use in unit tests",
      "Hoonhee Lee <hoonhee.lee@lge.com>");
}

static void
gst_http_filter_init (GstHttpFilter * filter)
{
  filter->sinkpad =
      gst_pad_new_from_static_template (&filter_sink_templ, "sink");
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&filter_src_templ, "src");
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);
}

static void
on_bin_element_added (GstBin * bin, GstElement * element, gpointer user_data)
{
  gchar *name;

  name = gst_element_get_name (element);
  GST_DEBUG ("Added element in bin : %s", name);

  if (!strstr (name, "souphttpsrc") && !strstr (name, "httpfilter"))
    fail ("Generated unexpected element");

  g_free (name);
}

GST_START_TEST (test_uri_interface)
{
  GstElement *httpextbin;
  const gchar *const *uri_protocols;

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  fail_unless (GST_IS_URI_HANDLER (httpextbin), "httpextbin not a URI handler");

  uri_protocols = gst_uri_handler_get_protocols (GST_URI_HANDLER (httpextbin));

  if (uri_protocols && *uri_protocols) {
    for (; *uri_protocols != NULL; uri_protocols++)
      GST_DEBUG ("Suppiorted URI protocols : %s", *uri_protocols);
  } else {
    fail ("No supported URI protocols");
  }

  gst_object_unref (httpextbin);

}

GST_END_TEST;

GST_START_TEST (test_set_uri)
{
  GstElement *httpextbin;
  gchar *uri, *current_uri;

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  // set uri as "http+caps://"
  g_object_set (httpextbin, "uri", "http+caps://", NULL);

  g_object_get (httpextbin, "uri", &uri, NULL);
  fail_unless_equals_string (uri, "http+caps://");
  g_free (uri);

  g_object_get (httpextbin, "current-uri", &current_uri, NULL);
  fail_unless_equals_string (uri, "http://");
  g_free (current_uri);

  // set uri as "http+application://"
  g_object_set (httpextbin, "uri", "http+application://", NULL);

  g_object_get (httpextbin, "uri", &uri, NULL);
  fail_unless_equals_string (uri, "http+application://");
  g_free (uri);

  g_object_get (httpextbin, "current-uri", &current_uri, NULL);
  fail_unless_equals_string (uri, "http://");
  g_free (current_uri);

  gst_object_unref (httpextbin);
}

GST_START_TEST (test_configure_child_element)
{
  GstElement *httpextbin;
  GstElement *souphttpsrc;
  gchar *uri;
  gchar *location;

  fail_unless (gst_element_register (NULL, "httpfilter",
          GST_RANK_PRIMARY + 100, gst_http_filter_get_type ()));

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  g_signal_connect (G_OBJECT (httpextbin), "element-added",
      G_CALLBACK (on_bin_element_added), NULL);

  // set uri as "http+caps://"
  g_object_set (httpextbin, "uri", "http+caps://", NULL);

  // change state to paused
  fail_unless_equals_int (gst_element_set_state (httpextbin, GST_STATE_PAUSED),
      GST_STATE_CHANGE_SUCCESS);

  g_object_get (httpextbin, "source", &souphttpsrc, NULL);
  fail_unless (souphttpsrc != NULL, "Could not create souphttpsrc element");

  // set uri as "http+application://"
  g_object_set (httpextbin, "uri", "http+application://", NULL);

  g_object_get (httpextbin, "uri", &uri, NULL);
  fail_unless_equals_string (uri, "http+caps://");
  g_free (uri);

  g_object_get (souphttpsrc, "location", &location, NULL);
  fail_unless_equals_string (location, "http://");
  g_free (location);

  gst_element_set_state (httpextbin, GST_STATE_NULL);

  gst_object_unref (souphttpsrc);
  gst_object_unref (httpextbin);

}

GST_END_TEST;

static Suite *
httpextbin_suite (void)
{
  Suite *s = suite_create ("httpextbin");
  TCase *tc_chain;

  tc_chain = tcase_create ("general");
  tcase_add_test (tc_chain, test_uri_interface);
  tcase_add_test (tc_chain, test_set_uri);
  tcase_add_test (tc_chain, test_configure_child_element);
  suite_add_tcase (s, tc_chain);

  return s;
}

GST_CHECK_MAIN (httpextbin);
