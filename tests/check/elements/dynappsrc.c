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

static void
pad_added_cb (GstElement * element, GstPad * pad, gint * n_added)
{
  *n_added = *n_added + 1;
}

GST_START_TEST (test_appsrc_creation)
{
  GstElement *dynappsrc;
  guint pad_added_id = 0;
  gint n_added = 0;
  GstStateChangeReturn ret;
  GstElement *appsrc1 = NULL;
  GstElement *appsrc2 = NULL;
  GstIterator *iter;
  GValue item = { 0, };
  gboolean done = FALSE;
  gboolean exist_srcpad = FALSE;
  gint n_source = 0;

  dynappsrc =
      gst_element_make_from_uri (GST_URI_SRC, "dynappsrc://", "source", NULL);

  fail_unless (dynappsrc != NULL, "fail to create dynappsrc element by uri");

  pad_added_id =
      g_signal_connect (dynappsrc, "pad-added",
      G_CALLBACK (pad_added_cb), &n_added);

  g_signal_emit_by_name (dynappsrc, "new-appsrc", NULL, &appsrc1);
  g_signal_emit_by_name (dynappsrc, "new-appsrc", "thisisappsrc", &appsrc2);

  /* user should do ref appsrc elements before using it */
  gst_object_ref (appsrc1);
  gst_object_ref (appsrc2);

  fail_unless (appsrc1 && appsrc2, "fail to create appsrc element");
  fail_unless (GST_OBJECT_NAME (appsrc2)
      && "thisisappsrc", "fail to set user-defined name");

  iter = gst_element_iterate_src_pads (dynappsrc);
  while (!done) {
    switch (gst_iterator_next (iter, &item)) {
      case GST_ITERATOR_OK:
        exist_srcpad = TRUE;
        done = TRUE;
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iter);
        break;
      case GST_ITERATOR_ERROR:
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }

  fail_unless (!exist_srcpad,
      "srcpad is added too earlier, it should be added during state change");

  g_value_unset (&item);
  gst_iterator_free (iter);

  g_object_get (dynappsrc, "n-source", &n_source, NULL);
  fail_unless (n_source == 2, "the number of source element is not matched");

  ret = gst_element_set_state (dynappsrc, GST_STATE_PAUSED);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS,
      "fail to state change to PAUSED");
  fail_unless (n_added == 2, "srcpad of dynappsrc does not added");

  /* user should do unref appsrc elements before destroy pipeline */
  gst_object_unref (appsrc1);
  gst_object_unref (appsrc2);

  /*
     In general, we don't need to call set_state(NULL) before unref element
     because if ref_count is zero by gst_object_unref, the state of the element
     automatically becomes NULL.
     However, in here, set_state(NULL) is called in order to clarify
     how to handle appsrc from dynappsrc. Without internal appsrcs de-referencing,
     the appsrcs are remained as detached elements from dynappsrc.
     This means that the upper layer cannot re-use appsrcs after calling set_state(NULL).
   */
  gst_element_set_state (dynappsrc, GST_STATE_NULL);
  g_signal_handler_disconnect (dynappsrc, pad_added_id);
  gst_object_unref (dynappsrc);
}

GST_END_TEST;

GST_START_TEST (test_appsrc_create_when_paused)
{
  GstElement *dynappsrc;
  guint pad_added_id = 0;
  gint n_added = 0;
  GstStateChangeReturn ret;
  GstElement *appsrc1 = NULL;
  GstElement *appsrc2 = NULL;
  gint n_source = 0;

  dynappsrc =
      gst_element_make_from_uri (GST_URI_SRC, "dynappsrc://", "source", NULL);

  pad_added_id =
      g_signal_connect (dynappsrc, "pad-added",
      G_CALLBACK (pad_added_cb), &n_added);

  g_signal_emit_by_name (dynappsrc, "new-appsrc", "justin", &appsrc1);

  /* user should do ref appsrc elements before using it */
  gst_object_ref (appsrc1);

  fail_unless (appsrc1, "failed to create appsrc element");

  g_object_get (dynappsrc, "n-source", &n_source, NULL);
  fail_unless (n_source == 1, "the number of source element is not matched");

  ret = gst_element_set_state (dynappsrc, GST_STATE_PAUSED);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS,
      "fail to state change to PAUSED");

  /* Try to generate a appsrc element when state is paused.
   * If it is genetated, it is not make sence.
   * Because, in this time, configuratoin of all appsrc elements in dynappsrc bin is already done.
   * Thus, you should call "new-appsrc" singal action before paused state.
   */
  g_signal_emit_by_name (dynappsrc, "new-appsrc", "wonchul", &appsrc2);
  fail_unless (!appsrc2,
      "appsrc element is generated even though when state is paused");

  g_object_get (dynappsrc, "n-source", &n_source, NULL);
  fail_unless (n_source == 1, "the number of source element is not matched");

  fail_unless (n_added == 1, "srcpad of dynappsrc does not added");

  /* user should do unref appsrc elements before destroy pipeline */
  gst_object_unref (appsrc1);

  gst_element_set_state (dynappsrc, GST_STATE_NULL);
  g_signal_handler_disconnect (dynappsrc, pad_added_id);
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
  tcase_add_test (tc_chain, test_appsrc_creation);
  tcase_add_test (tc_chain, test_appsrc_create_when_paused);

  return s;
}

GST_CHECK_MAIN (dynappsrc);
