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
    GST_STATIC_CAPS ("justin; jeongseok; hoonhee; wonchul; yongjin")
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
httpextbin_pad_added_cb (GstElement * bin, GstPad * pad, gboolean * p_flag)
{
  GstPad *target_pad;
  fail_unless (GST_IS_GHOST_PAD (pad));

  target_pad = gst_ghost_pad_get_target (GST_GHOST_PAD (pad));
  fail_unless (target_pad != NULL);

  gst_object_unref (target_pad);
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
      GST_DEBUG ("Supported URI protocols : %s", *uri_protocols);
  } else {
    fail ("No supported URI protocols");
  }

  gst_object_unref (httpextbin);

}

GST_END_TEST;

GST_START_TEST (test_set_uri)
{
  GstElement *httpextbin;
  gchar *uri;
  gchar **item;

  const gchar *uris[] =
      { "http+justin://", "http+hoonhee://", "https+wonchul://", NULL };

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  for (item = uris; *item != NULL; *item++) {
    GST_DEBUG ("Try to set uri : %s", *item);
    g_object_set (httpextbin, "uri", *item, NULL);
    g_object_get (httpextbin, "uri", &uri, NULL);
    fail_unless_equals_string (uri, *item);
    g_free (uri);
  }

  gst_object_unref (httpextbin);
}

GST_END_TEST;

GST_START_TEST (test_missing_plugin)
{
  GstElement *httpextbin;
  GstBus *bus;
  GstMessage *msg;
  GError *err = NULL;
  GstStructure *s = NULL;

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  // set uri as "http+unsupported://"
  g_object_set (httpextbin, "uri", "http+unsupported://", NULL);

  fail_unless_equals_int (gst_element_set_state (httpextbin, GST_STATE_READY),
      GST_STATE_CHANGE_SUCCESS);
  fail_unless_equals_int (gst_element_set_state (httpextbin, GST_STATE_PAUSED),
      GST_STATE_CHANGE_FAILURE);

  /* there should be at least a missing-plugin message on the bus now and an
   * error message; the missing-plugin message should be first */
  bus = gst_element_get_bus (httpextbin);

  msg = gst_bus_poll (bus, GST_MESSAGE_ELEMENT | GST_MESSAGE_ERROR, -1);
  fail_unless_equals_int (GST_MESSAGE_TYPE (msg), GST_MESSAGE_ELEMENT);
  s = (GstStructure *) gst_message_get_structure (msg);
  fail_unless (s != NULL);
  fail_unless (gst_structure_has_name (s, "missing-plugin"));
  fail_unless (gst_structure_has_field_typed (s, "detail", G_TYPE_STRING));
  fail_unless_equals_string (gst_structure_get_string (s, "detail"),
      "http+unsupported");
  fail_unless (gst_structure_has_field_typed (s, "type", G_TYPE_STRING));
  fail_unless_equals_string (gst_structure_get_string (s, "type"), "urisource");
  gst_message_unref (msg);

  msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, -1);
  fail_unless_equals_int (GST_MESSAGE_TYPE (msg), GST_MESSAGE_ERROR);

  /* make sure the error is a CORE MISSING_PLUGIN one */
  gst_message_parse_error (msg, &err, NULL);
  fail_unless (err != NULL);
  fail_unless (err->domain == GST_CORE_ERROR, "error has wrong error domain "
      "%s instead of core-error-quark", g_quark_to_string (err->domain));
  fail_unless (err->code == GST_CORE_ERROR_MISSING_PLUGIN, "error has wrong "
      "code %u instead of GST_CORE_ERROR_MISSING_PLUGIN", err->code);
  g_error_free (err);
  gst_message_unref (msg);
  gst_object_unref (bus);

  gst_element_set_state (httpextbin, GST_STATE_NULL);

  gst_object_unref (httpextbin);
}

GST_END_TEST;

GST_START_TEST (test_set_state_paused)
{
  GstElement *httpextbin;
  GstElement *souphttpsrc;

  fail_unless (gst_element_register (NULL, "httpfilter",
          GST_RANK_PRIMARY + 100, gst_http_filter_get_type ()));

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  g_signal_connect (httpextbin, "pad-added",
      G_CALLBACK (httpextbin_pad_added_cb), NULL);

  // set uri as "http+justin://"
  g_object_set (httpextbin, "uri", "http+justin://", NULL);

  // change state to paused
  fail_unless_equals_int (gst_element_set_state (httpextbin, GST_STATE_PAUSED),
      GST_STATE_CHANGE_SUCCESS);

  g_object_get (httpextbin, "source", &souphttpsrc, NULL);
  fail_unless (souphttpsrc != NULL, "Could not create souphttpsrc element");

  gst_element_set_state (httpextbin, GST_STATE_NULL);

  gst_object_unref (souphttpsrc);
  gst_object_unref (httpextbin);
}

GST_END_TEST;

GST_START_TEST (test_repeat_state_change)
{
  GstElement *httpextbin;
  gchar **item;

  const gchar *uris[] =
      { "http+justin://", "http+jeongseok://", "http+hoonhee://",
    "http+wonchul://", "http+yongjin://", NULL
  };

  fail_unless (gst_element_register (NULL, "httpfilter",
          GST_RANK_PRIMARY + 100, gst_http_filter_get_type ()));

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  g_signal_connect (httpextbin, "pad-added",
      G_CALLBACK (httpextbin_pad_added_cb), NULL);

  for (item = uris; *item != NULL; *item++) {
    GST_DEBUG ("Try to set uri : %s", *item);
    g_object_set (httpextbin, "uri", *item, NULL);

    // change state to paused
    fail_unless_equals_int (gst_element_set_state (httpextbin,
            GST_STATE_PAUSED), GST_STATE_CHANGE_SUCCESS);

    fail_unless_equals_int (gst_element_get_state (httpextbin, NULL, NULL, -1),
        GST_STATE_CHANGE_SUCCESS);

    // change state to ready
    fail_unless_equals_int (gst_element_set_state (httpextbin, GST_STATE_READY),
        GST_STATE_CHANGE_SUCCESS);

    fail_unless_equals_int (gst_element_get_state (httpextbin, NULL, NULL, -1),
        GST_STATE_CHANGE_SUCCESS);
  }

  gst_element_set_state (httpextbin, GST_STATE_NULL);

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
  tcase_add_test (tc_chain, test_missing_plugin);
  tcase_add_test (tc_chain, test_set_state_paused);
  tcase_add_test (tc_chain, test_repeat_state_change);

  suite_add_tcase (s, tc_chain);

  return s;
}

GST_CHECK_MAIN (httpextbin);
