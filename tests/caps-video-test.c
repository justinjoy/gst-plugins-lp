#include <gst/gst.h>
#include "gst/playback/gstlpbin.h"

gint
main (gint argc, gchar * argv[])
{
  GstElement *lpbin;
  gchar *uri;
  GstStateChangeReturn ret;

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

  gst_element_set_state (lpbin, GST_STATE_PLAYING);

  ret = gst_element_get_state (lpbin, NULL, NULL, GST_CLOCK_TIME_NONE);

  if (ret == GST_STATE_CHANGE_SUCCESS) {
    GstStructure *structure;
    g_signal_emit_by_name (G_OBJECT (lpbin), "caps-video", &structure);

    GST_INFO_OBJECT (lpbin, "struture is %" GST_PTR_FORMAT, structure);
  } else
    GST_INFO_OBJECT (lpbin, "state change failed - %d", ret);

  gst_element_set_state (lpbin, GST_STATE_NULL);
  gst_object_unref (lpbin);

  return 0;
}
