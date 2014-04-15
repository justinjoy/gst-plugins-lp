#include <gst/gst.h>

static GstElement *demuxer = NULL;

static void
playbin_element_added_cb (GstBin * playbin, GstElement * element,
    gpointer u_data)
{
  GstElementFactory *factory = NULL;
  const gchar *klass = NULL;
  gchar *elem_name = NULL;

  factory = gst_element_get_factory (element);
  klass = gst_element_factory_get_klass (factory);
  elem_name = gst_element_get_name (element);

  if (g_strrstr (klass, "Bin"))
    g_signal_connect (element, "element-added",
        G_CALLBACK (playbin_element_added_cb), playbin);

  if (g_strrstr (klass, "Demuxer"))
    demuxer = element;

  g_free (elem_name);
}

static void
source_setup (GstElement * element, GstElement * source, gpointer data)
{
  GstStructure *s;
  GstStructure *s2;

  GST_WARNING ("source_setup");
  s = gst_structure_new ("smart-properties",
      "thumbnail-mode", G_TYPE_BOOLEAN, TRUE, "dlna-opval", G_TYPE_INT, 0x99,
      NULL);

  g_object_set (source, "smart-properties", s, NULL);
  g_object_get (source, "smart-properties", &s2, NULL);
  GST_WARNING ("%s", gst_structure_to_string (s2));

  gst_structure_free (s);
}

static gboolean
bus_message (GstBus * bus, GstMessage * message, GMainLoop * loop)
{
  GST_DEBUG ("got message %s",
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      g_error ("received error");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ASYNC_DONE:
    {
      GstStructure *s;
      GstQuery *query;
      GstPad *srcpad = NULL;

      GST_WARNING ("in async done");
      srcpad = gst_element_get_static_pad (demuxer, "src");

      s = gst_structure_new ("smart-properties",
          "thumbnail-mode", G_TYPE_BOOLEAN, NULL,
          "dlna-opval", G_TYPE_INT, 0, NULL);
      query = gst_query_new_custom (GST_QUERY_CUSTOM, s);

      if (gst_pad_query (srcpad, query)) {
        const GstStructure *structure = gst_query_get_structure (query);
        gchar *structure_detail = gst_structure_to_string (structure);

        GST_WARNING ("%s", structure_detail);
        g_free (structure_detail);
      }

      gst_object_unref (srcpad);
    }
      break;
    default:
      break;
  }
  return TRUE;
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *playbin;
  GMainLoop *loop;
  GstBus *bus;
  gchar *uri;

  gst_init (&argc, &argv);

  if (argc < 2) {
    GST_ERROR ("usage: %s <media file or uri>\n", argv[0]);
    return 1;
  }

  playbin = gst_element_factory_make ("playbin", NULL);
  if (!playbin) {
    GST_ERROR ("playbin plugin missing\n");
    return 1;
  }

  if (gst_uri_is_valid (argv[1]))
    uri = g_strdup (argv[1]);
  else
    uri = gst_filename_to_uri (argv[1], NULL);

  g_object_set (playbin, "uri", uri, NULL);
  g_free (uri);

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (playbin);

  gst_bus_add_watch (bus, (GstBusFunc) bus_message, loop);
  g_object_unref (bus);

  g_signal_connect (playbin, "element-added",
      G_CALLBACK (playbin_element_added_cb), loop);

  g_signal_connect (playbin, "source-setup", G_CALLBACK (source_setup), NULL);

  gst_element_set_state (playbin, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  gst_element_set_state (playbin, GST_STATE_NULL);
  g_object_unref (playbin);
  g_main_loop_unref (loop);

  return 0;
}
