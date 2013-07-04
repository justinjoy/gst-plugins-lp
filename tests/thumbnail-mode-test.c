#include <gst/gst.h>
#include <stdio.h>
#include "gst/playback/gstlpbin.h"

static GMainLoop *loop = NULL;

static void
get_thumbnail (GstLpBin * lpbin)
{
  GstBuffer *buffer;

  FILE *fp;
  guint8 *data;
  guint size;
  gint width;
  gint height;
  const char *output_file_path;

  width = 128;
  height = 72;
  output_file_path = "/mnt/thumbnail.raw";

  g_signal_emit_by_name (G_OBJECT (lpbin), "retrieve-thumbnail", width, height,
      "RGB", &buffer);

  if (buffer == NULL) {
    GST_DEBUG_OBJECT (lpbin, "get_thumbnail : Failed to get thumbnail data.");
    return;
  } else
    GST_DEBUG_OBJECT (lpbin,
        "get_thumbnail : Success to get thumbnail data. buffer = %p", buffer);

  GstMapInfo map;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  data = map.data;
  size = map.size;

  fp = fopen (output_file_path, "wb");

  if (fp == NULL) {
    GST_DEBUG_OBJECT (lpbin, "get_thumbnail : Failed to open file");
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);
    return;
  }

  /* width(4) | height(4) | data(..) */
  fwrite (&width, sizeof (int), 1, fp);
  fwrite (&height, sizeof (int), 1, fp);
  fwrite (map.data, map.size, 1, fp);

  GST_DEBUG_OBJECT (lpbin,
      "get_thumbnail : wrote down a thumbnail buffer to a file");

  fclose (fp);
  gst_buffer_unmap (buffer, &map);
  gst_buffer_unref (buffer);
}

static gboolean
bus_call (GstBus * bus, GstMessage * message, gpointer data)
{
  GstLpBin *lpbin = (GstLpBin *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED:
      GST_DEBUG_OBJECT (lpbin,
          "thumbnail_mode_test bus_cb : GST_MESSAGE_STATE_CHANGED, element name = %s",
          GST_OBJECT_NAME (GST_MESSAGE_SRC (message)));
      GstState oldstate, newstate, pending;
      gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);

      GST_DEBUG_OBJECT (lpbin,
          "thumbnail_mode_test bus_cb : GST_MESSAGE_STATE_CHANGED, state-changed: %s -> %s, pending-state = %s",
          gst_element_state_get_name (oldstate),
          gst_element_state_get_name (newstate),
          gst_element_state_get_name (pending));

      if (newstate == GST_STATE_PAUSED) {
        GstElement *elem = NULL;
        GstElementFactory *factory = NULL;
        const gchar *klass = NULL;
        GstBuffer *buffer;
        GstCaps *caps;

        elem = GST_ELEMENT (GST_MESSAGE_SRC (message));
        factory = gst_element_get_factory (elem);
        klass = gst_element_factory_get_klass (factory);

        if (strstr (klass, "Lightweight/Bin/Player")) {
          get_thumbnail (lpbin);
        }
      }
      break;
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
  GstStateChangeReturn sret;

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

  if (gst_uri_is_valid (argv[1]))
    uri = g_strdup (argv[1]);
  else
    uri = gst_filename_to_uri (argv[1], NULL);

  g_object_set (lpbin, "uri", uri, NULL);
  g_free (uri);
  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (lpbin);
  if (bus) {
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_call), lpbin);
  }
  g_object_unref (bus);

  g_object_set (lpbin, "thumbnail-mode", TRUE, NULL);

  if (gst_element_set_state (lpbin,
          GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    GST_DEBUG_OBJECT (lpbin, "thumbnail-mode-test : Failed to paused state");
    goto done;
  }

  sret = gst_element_get_state (lpbin, NULL, NULL, -1);

  if (sret == GST_STATE_CHANGE_FAILURE) {
    GST_DEBUG_OBJECT (lpbin, "thumbnail-mode-test : Failed to get lpbin state");
    goto done;
  }

  if (gst_element_set_state (lpbin,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    GST_DEBUG_OBJECT (lpbin, "thumbnail-mode-test : Failed to playing state");
    goto done;
  }

  sret = gst_element_get_state (lpbin, NULL, NULL, -1);

  if (sret == GST_STATE_CHANGE_FAILURE) {
    GST_DEBUG_OBJECT (lpbin, "thumbnail-mode-test : Failed to get lpbin state");
    goto done;
  }
  g_main_loop_run (loop);

done:
  gst_element_set_state (lpbin, GST_STATE_NULL);
  g_object_unref (lpbin);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
