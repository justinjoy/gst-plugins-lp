#include <gst/gst.h>
#include "gst/playback/gstlpbin.h"

static GMainLoop *loop = NULL;

static gboolean
bus_call (GstBus * bus, GstMessage * message, gpointer data)
{
  GstLpBin *lpbin = (GstLpBin *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }
  return TRUE;
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *lpbin;
  GstBus *bus;
  guint bus_watch_id;
  gchar *uri;
  guint repeat_cnt;
  guint i;
  GstStateChangeReturn sret;

  gst_init (&argc, &argv);

  if (argc < 3) {
    GST_ERROR ("usage: %s <media file or uri>\n", argv[0]);
    return 1;
  }

  lpbin = gst_element_factory_make ("lpbin", NULL);
  if (!lpbin) {
    GST_ERROR ("lpbin plugin missing\n");
    return 1;
  }

  if (gst_uri_is_valid (argv[1]))
    uri = g_strdup (argv[1]);
  else
    uri = gst_filename_to_uri (argv[1], NULL);

  repeat_cnt = atoi (argv[2]);

  loop = g_main_loop_new (NULL, FALSE);
  bus = gst_element_get_bus (lpbin);
  if (bus) {
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_call), lpbin);
  }
  g_object_unref (bus);

  for (i = 0; i < repeat_cnt; i++) {
    g_object_set (lpbin, "uri", uri, NULL);

    if (gst_element_set_state (lpbin,
            GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      GST_DEBUG_OBJECT (lpbin, "thumbnail-mode-test : Failed to playing state");
      goto done;
    }

    sret = gst_element_get_state (lpbin, NULL, NULL, -1);

    if (sret == GST_STATE_CHANGE_FAILURE) {
      GST_DEBUG_OBJECT (lpbin,
          "thumbnail-mode-test : Failed to get lpbin state");
      goto done;
    }

    if (i == 0)
      g_main_loop_run (loop);

    gst_element_set_state (lpbin, GST_STATE_NULL);
    g_usleep (3000000);
  }

done:
  g_free (uri);
  gst_element_set_state (lpbin, GST_STATE_NULL);
  g_object_unref (lpbin);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
