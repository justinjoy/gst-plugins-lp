/* GStreamer
 *
 * Copyright (C) 2014 Jeongseok Kim <jeongseok.kim@lge.com>
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

GST_START_TEST (test_uri_interface)
{
  GstElement *dynappsrc;
  const gchar *const *uri_protocols;

  dynappsrc = gst_element_factory_make ("dynappsrc", "dynappsrc");
  fail_unless (dynappsrc != NULL, "Failed to create dynappsrc element");

  fail_unless (GST_IS_URI_HANDLER (dynappsrc),
      "Not implemented as URI handler");

  uri_protocols = gst_uri_handler_get_protocols (GST_URI_HANDLER (dynappsrc));

  fail_unless (uri_protocols && *uri_protocols,
      "Cannot get supported protocol information");

  fail_unless (!g_strcmp0 (*uri_protocols, "dynappsrc"),
      "Not supported 'dynappsrc' protocol");

  gst_object_unref (dynappsrc);
}

GST_END_TEST;

static Suite *
dynappsrc_suite (void)
{
  Suite *s = suite_create ("dynappsrc");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_uri_interface);

  return s;
}

GST_CHECK_MAIN (dynappsrc);
