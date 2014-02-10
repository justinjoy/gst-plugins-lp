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
  gchar *uri;

  const gchar *uris[] =
      { "http+justin://", "http+hoonhee://", "https+wonchul://", NULL };

  httpextbin = gst_element_factory_make ("httpextbin", NULL);
  fail_unless (httpextbin != NULL, "Could not create httpextbin element");

  for (; *uris != NULL; uris++) {
    GST_DEBUG ("Try to set uri : %s", *uris);
    g_object_set (httpextbin, "uri", *uris, NULL);
    g_object_get (httpextbin, "uri", &uri, NULL);
    fail_unless_equals_string (uri, *uris);
    g_free (uri);
  }

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
  suite_add_tcase (s, tc_chain);

  return s;
}

GST_CHECK_MAIN (httpextbin);
