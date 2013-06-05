#include <gst/gst.h>
#include "gst/playback/gstlpbin.h"

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:{
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_ERROR:{
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

static void
lpbin_notify_source_cb (GstElement * lpbin, GParamSpec * pParam,
    gpointer u_data)
{
  GST_DEBUG_OBJECT (lpbin, "notify source cb");
}

static void
lpbin_notify_aboutToFinish_cb (GstElement * lpbin, gpointer u_data)
{
  GST_DEBUG_OBJECT (lpbin, "notify about-to-finish cb");
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *lpbin;
  GstElement *vsink;
  GstElement *asink;
  GMainLoop *loop;
  GstBus *bus;
  guint bus_watch_id;
  gchar *uri;

  gst_init (&argc, &argv);

  if (argc < 2) {
    GST_ERROR ("usage: %s <media file or uri>\n", argv[0]);
    return 1;
  }

  lpbin = gst_element_factory_make ("lpbin", NULL);
  if (!lpbin) {
    GST_ERROR ("lpbin plugin missing\n");
    return 1;
  }

  vsink = gst_element_factory_make ("fakesink", NULL);
  asink = gst_element_factory_make ("fakesink", NULL);

  if (gst_uri_is_valid (argv[1]))
    uri = g_strdup (argv[1]);
  else
    uri = gst_filename_to_uri (argv[1], NULL);

  g_object_set (lpbin, "uri", uri, NULL);
  g_object_set (lpbin, "video-sink", vsink, NULL);
  g_object_set (lpbin, "audio-sink", asink, NULL);
  g_free (uri);

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (lpbin);
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  g_object_unref (bus);

  g_signal_connect (lpbin, "notify::source",
      G_CALLBACK (lpbin_notify_source_cb), loop);

  g_signal_connect (lpbin, "about-to-finish",
      G_CALLBACK (lpbin_notify_aboutToFinish_cb), loop);

  gst_element_set_state (lpbin, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  gst_element_set_state (lpbin, GST_STATE_NULL);
  g_object_unref (lpbin);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
