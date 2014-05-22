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

#define NUM_REPEAT 5
#define NUM_APPSRC 10

typedef struct _App App;

struct _App
{
  GstElement *pipeline;
  GstElement *dynappsrc;
  GstElement *appsrc[NUM_APPSRC];
  GstElement *fakesink[NUM_APPSRC];
  gint nb_received_event;
  GMainLoop *loop;
};

App s_app;

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

static GstPadProbeReturn
_appsrc_event_probe (GstPad * pad, GstPadProbeInfo * info, gpointer udata)
{
  GstEvent *event = GST_PAD_PROBE_INFO_DATA (info);
  GstElement *appsrc = udata;
  const GstStructure *s;
  gchar *sstr;
  App *app = &s_app;

  GST_DEBUG ("element:%s got an event:%s", GST_ELEMENT_NAME (appsrc),
      GST_EVENT_TYPE_NAME (event));

  if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_UPSTREAM) {
    s = gst_event_get_structure (event);
    sstr = gst_structure_to_string (s);
    if (g_str_equal (sstr, "application/x-custom;"))
      app->nb_received_event++;
    g_free (sstr);
  } else if (GST_EVENT_TYPE (event) == GST_EVENT_SEEK) {
    app->nb_received_event++;
  }

  return GST_PAD_PROBE_OK;
}

static void
pad_added_cb (GstElement * element, GstPad * pad, gint * n_added)
{
  App *app = &s_app;
  GstPad *sinkpad = NULL;
  GstPad *target = NULL;

  if (app->fakesink[*n_added] != NULL) {
    sinkpad = gst_element_get_static_pad (app->fakesink[*n_added], "sink");
    fail_unless (GST_PAD_LINK_SUCCESSFUL (gst_pad_link (pad, sinkpad)));
    gst_object_unref (sinkpad);

    target = gst_ghost_pad_get_target (GST_GHOST_PAD (pad));
    fail_unless (target != NULL);

    gst_pad_add_probe (target, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM,
        _appsrc_event_probe, GST_PAD_PARENT (target), NULL);
    gst_object_unref (target);
  }

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

GST_START_TEST (test_repeat_state_change)
{
  GstElement *dynappsrc;
  guint pad_added_id = 0;
  gint n_added = 0;
  GstStateChangeReturn ret;
  GstElement *appsrc[NUM_REPEAT];
  gint n_source = 0;
  gint count = 0;
  gint nb_appsrc = 0;

  dynappsrc =
      gst_element_make_from_uri (GST_URI_SRC, "dynappsrc://", "source", NULL);

  pad_added_id =
      g_signal_connect (dynappsrc, "pad-added",
      G_CALLBACK (pad_added_cb), &n_added);

  for (count = 1; count <= NUM_REPEAT; count++) {
    for (nb_appsrc = 0; nb_appsrc < count; nb_appsrc++) {
      g_signal_emit_by_name (dynappsrc, "new-appsrc", NULL, &appsrc[nb_appsrc]);
      gst_object_ref (appsrc[nb_appsrc]);
      fail_unless (appsrc[nb_appsrc], "failed to create appsrc element");
    }
    g_object_get (dynappsrc, "n-source", &n_source, NULL);
    fail_unless (n_source == count,
        "the number of source element is not matched");

    // change state to paused
    ret = gst_element_set_state (dynappsrc, GST_STATE_PAUSED);
    fail_unless (ret == GST_STATE_CHANGE_SUCCESS,
        "fail to state change to PAUSED");

    ret = gst_element_get_state (dynappsrc, NULL, NULL, -1);
    fail_unless (ret == GST_STATE_CHANGE_SUCCESS,
        "fail to get state after PAUSED");

    fail_unless (n_added == count, "srcpad of dynappsrc does not added");

    // change state to ready
    ret = gst_element_set_state (dynappsrc, GST_STATE_READY);
    fail_unless (ret == GST_STATE_CHANGE_SUCCESS,
        "fail to state change to READY");

    ret = gst_element_get_state (dynappsrc, NULL, NULL, -1);
    fail_unless (ret == GST_STATE_CHANGE_SUCCESS,
        "fail to get state after READY");

    n_added = 0;
    for (nb_appsrc = 0; nb_appsrc < count; nb_appsrc++) {
      gst_object_unref (appsrc[nb_appsrc]);
    }
  }

  gst_element_set_state (dynappsrc, GST_STATE_NULL);
  g_signal_handler_disconnect (dynappsrc, pad_added_id);
  gst_object_unref (dynappsrc);
}

GST_END_TEST;

GST_START_TEST (test_appsrc_upstream_event)
{
  App *app = &s_app;
  guint pad_added_id = 0;
  gint n_added = 0;
  GstStateChangeReturn ret;
  gint n_source = 0;
  gint count = 0;
  GstEvent *event;

  GST_DEBUG ("Creating pipeline");
  app->pipeline = gst_pipeline_new ("pipeline");
  fail_if (app->pipeline == NULL);

  GST_DEBUG ("Creating dynappsrc");
  app->dynappsrc =
      gst_element_make_from_uri (GST_URI_SRC, "dynappsrc://", "source", NULL);
  fail_unless (app->dynappsrc != NULL,
      "fail to create dynappsrc element by uri");
  gst_bin_add (GST_BIN (app->pipeline), app->dynappsrc);

  GST_DEBUG ("Creating fakesink");
  for (count = 0; count < NUM_APPSRC; count++) {
    app->fakesink[count] = gst_element_factory_make ("fakesink", NULL);
    fail_if (app->fakesink[count] == NULL);
    g_object_set (G_OBJECT (app->fakesink[count]), "sync", TRUE, NULL);
    gst_bin_add (GST_BIN (app->pipeline), app->fakesink[count]);
  }

  pad_added_id =
      g_signal_connect (app->dynappsrc, "pad-added",
      G_CALLBACK (pad_added_cb), &n_added);

  GST_DEBUG ("Creating appsrc");
  for (count = 0; count < NUM_APPSRC; count++) {
    gchar *appsrc_name;
    appsrc_name = g_strdup_printf ("foot%d", count);
    g_signal_emit_by_name (app->dynappsrc, "new-appsrc", appsrc_name,
        &app->appsrc[count]);
    /* user should do ref appsrc elements before using it */
    gst_object_ref (app->appsrc[count]);

    fail_unless (app->appsrc[count] != NULL, "failed to create appsrc element");
    g_free (appsrc_name);
  }

  g_object_get (app->dynappsrc, "n-source", &n_source, NULL);
  fail_unless (n_source == NUM_APPSRC,
      "the number of source element is not matched");

  ret = gst_element_set_state (app->pipeline, GST_STATE_PAUSED);
  fail_unless (ret == GST_STATE_CHANGE_ASYNC);

  fail_unless (n_added == NUM_APPSRC, "srcpad of dynappsrc does not added");

  ret = gst_element_set_state (app->pipeline, GST_STATE_PLAYING);
  fail_unless (ret == GST_STATE_CHANGE_ASYNC);

  GST_DEBUG ("Sending upstream custom event");
  app->nb_received_event = 0;
  event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
      gst_structure_new_empty ("application/x-custom"));

  gst_element_send_event (app->pipeline, event);
  fail_unless (app->nb_received_event == NUM_APPSRC,
      "the number of received events are not matched");

  /*
   * First of all, dynappsrc handle a seek-event that it send all of appsrc elements.
   * Try to send seek-event in a fakesink.
   * Received seek-events in all of appsrc elements should be 10.
   */
  GST_DEBUG ("Sending flush seek event to fakesink");
  for (count = 0; count < NUM_APPSRC; count++) {
    app->nb_received_event = 0;
    gst_element_seek (app->fakesink[count], 1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, -1);

    fail_unless (app->nb_received_event == NUM_APPSRC,
        "the number of received events are not matched");
  }

  /*
   * Try to send seek-event in pipeline.
   * In this case, received seek-event in all of appsrc elements should be 100.
   */
  GST_DEBUG ("Sending flush seek event to pipeline");
  app->nb_received_event = 0;
  gst_element_seek (app->pipeline, 1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, -1);

  fail_unless (app->nb_received_event == (NUM_APPSRC * 10),
      "the number of received events are not matched");

  GST_DEBUG ("Release pipeline");
  /* user should do unref appsrc elements before destroy pipeline */
  for (count = 0; count < NUM_APPSRC; count++)
    gst_object_unref (app->appsrc[count]);

  gst_element_set_state (app->pipeline, GST_STATE_NULL);
  g_signal_handler_disconnect (app->dynappsrc, pad_added_id);
  gst_object_unref (app->pipeline);
}

GST_END_TEST;

static gboolean
bus_message (GstBus * bus, GstMessage * message, App * app)
{
  GST_DEBUG ("got message %s \n",
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (app->loop);
      break;
    default:
      break;
  }
  return TRUE;
}

static gboolean
do_send_eos (App * app)
{
  GstFlowReturn flow_ret = GST_FLOW_OK;

  /*
   * First of all, dynappsrc handle an EOS event that it send all of appsrc elements.
   * Try to send an EOS event via emmit signal.
   * Returned valude should be GST_FLOW_OK.
   */
  GST_DEBUG ("Sending EOS event");
  g_signal_emit_by_name (app->dynappsrc, "end-of-stream", &flow_ret);
  fail_unless (flow_ret == GST_FLOW_OK,
      "failed to send EOS event to dynappsrc bin");

  return FALSE;
}


GST_START_TEST (test_appsrc_eos)
{
  App *app = &s_app;
  guint pad_added_id = 0;
  gint n_added = 0;
  GstStateChangeReturn ret;
  gint n_source = 0;
  gint count = 0;
  GstBus *bus;

  GST_DEBUG ("Creating pipeline");
  app->pipeline = gst_pipeline_new ("pipeline");
  fail_if (app->pipeline == NULL);

  app->loop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (app->pipeline));

  /* add watch for messages */
  gst_bus_add_watch (bus, (GstBusFunc) bus_message, app);
  gst_object_unref (bus);

  GST_DEBUG ("Creating dynappsrc");
  app->dynappsrc =
      gst_element_make_from_uri (GST_URI_SRC, "dynappsrc://", "source", NULL);
  fail_unless (app->dynappsrc != NULL,
      "fail to create dynappsrc element by uri");
  gst_bin_add (GST_BIN (app->pipeline), app->dynappsrc);

  GST_DEBUG ("Creating fakesink");
  for (count = 0; count < NUM_APPSRC; count++) {
    app->fakesink[count] = gst_element_factory_make ("fakesink", NULL);
    fail_if (app->fakesink[count] == NULL);
    g_object_set (G_OBJECT (app->fakesink[count]), "sync", TRUE, NULL);
    gst_bin_add (GST_BIN (app->pipeline), app->fakesink[count]);
  }

  pad_added_id =
      g_signal_connect (app->dynappsrc, "pad-added",
      G_CALLBACK (pad_added_cb), &n_added);

  GST_DEBUG ("Creating appsrc");
  for (count = 0; count < NUM_APPSRC; count++) {
    gchar *appsrc_name;
    appsrc_name = g_strdup_printf ("foot%d", count);
    g_signal_emit_by_name (app->dynappsrc, "new-appsrc", appsrc_name,
        &app->appsrc[count]);
    /* user should do ref appsrc elements before using it */
    gst_object_ref (app->appsrc[count]);

    fail_unless (app->appsrc[count] != NULL, "failed to create appsrc element");
    g_free (appsrc_name);
  }

  g_object_get (app->dynappsrc, "n-source", &n_source, NULL);
  fail_unless (n_source == NUM_APPSRC,
      "the number of source element is not matched");

  ret = gst_element_set_state (app->pipeline, GST_STATE_PAUSED);
  fail_unless (ret == GST_STATE_CHANGE_ASYNC);

  fail_unless (n_added == NUM_APPSRC, "srcpad of dynappsrc does not added");

  ret = gst_element_set_state (app->pipeline, GST_STATE_PLAYING);
  fail_unless (ret == GST_STATE_CHANGE_ASYNC);

  /* add a timeout to send an EOS event to dynappsrc */
  g_timeout_add_seconds (1, (GSourceFunc) do_send_eos, app);

  /* now run */
  g_main_loop_run (app->loop);

  GST_DEBUG ("Release pipeline");
  /* user should do unref appsrc elements before destroy pipeline */
  for (count = 0; count < NUM_APPSRC; count++)
    gst_object_unref (app->appsrc[count]);

  gst_element_set_state (app->pipeline, GST_STATE_NULL);

  g_signal_handler_disconnect (app->dynappsrc, pad_added_id);
  gst_object_unref (app->pipeline);
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
  tcase_add_test (tc_chain, test_repeat_state_change);
  tcase_add_test (tc_chain, test_appsrc_upstream_event);
  tcase_add_test (tc_chain, test_appsrc_eos);

  return s;
}

GST_CHECK_MAIN (dynappsrc);
