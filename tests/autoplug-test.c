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

static gboolean
lpbin_notify_autoplug_continue_cb (GstElement * element, GstPad * pad,
    GstCaps * caps, gpointer u_data)
{
  GST_DEBUG_OBJECT (element, "lpbin_notify_autoplug_continue_cb");
  return TRUE;
}

static gint
compare_factories_func (gconstpointer p1, gconstpointer p2)
{
  GstPluginFeature *f1, *f2;
  gint diff;
  gboolean is_sink1, is_sink2;
  gboolean is_parser1, is_parser2;

  f1 = (GstPluginFeature *) p1;
  f2 = (GstPluginFeature *) p2;

  is_sink1 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f1),
      GST_ELEMENT_FACTORY_TYPE_SINK);
  is_sink2 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f2),
      GST_ELEMENT_FACTORY_TYPE_SINK);
  is_parser1 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f1),
      GST_ELEMENT_FACTORY_TYPE_PARSER);
  is_parser2 = gst_element_factory_list_is_type (GST_ELEMENT_FACTORY_CAST (f2),
      GST_ELEMENT_FACTORY_TYPE_PARSER);

  /* First we want all sinks as we prefer a sink if it directly
   * supports the current caps */
  if (is_sink1 && !is_sink2)
    return -1;
  else if (!is_sink1 && is_sink2)
    return 1;

  /* Then we want all parsers as we always want to plug parsers
   * before decoders */
  if (is_parser1 && !is_parser2)
    return -1;
  else if (!is_parser1 && is_parser2)
    return 1;

  /* And if it's a both a parser or sink we first sort by rank
   * and then by factory name */
  diff = gst_plugin_feature_get_rank (f2) - gst_plugin_feature_get_rank (f1);
  if (diff != 0)
    return diff;

  diff = strcmp (GST_OBJECT_NAME (f2), GST_OBJECT_NAME (f1));

  return diff;
}

static void
update_elements_list (GstLpBin * lpbin)
{
  GList *res, *tmp;
  guint cookie;

  cookie = gst_registry_get_feature_list_cookie (gst_registry_get ());
  if (!lpbin->elements || lpbin->elements_cookie != cookie) {
    if (lpbin->elements)
      gst_plugin_feature_list_free (lpbin->elements);
    res =
        gst_element_factory_list_get_elements
        (GST_ELEMENT_FACTORY_TYPE_DECODABLE, GST_RANK_MARGINAL);
    tmp =
        gst_element_factory_list_get_elements
        (GST_ELEMENT_FACTORY_TYPE_AUDIOVIDEO_SINKS, GST_RANK_MARGINAL);
    lpbin->elements = g_list_concat (res, tmp);
    lpbin->elements = g_list_sort (lpbin->elements, compare_factories_func);
    lpbin->elements_cookie = cookie;
  }
}

static GValueArray *
lpbin_notify_autoplug_factories_cb (GstElement * element, GstPad * pad,
    GstCaps * caps, gpointer u_data)
{
  GList *mylist, *tmp;
  GValueArray *result;
  GstLpBin *lpbin = GST_LP_BIN_CAST (element);
  GST_DEBUG_OBJECT (lpbin, "lpbin_notify_autoplug_factories_cb");

  /* filter out the elements based on the caps. */
  g_mutex_lock (&lpbin->elements_lock);
  update_elements_list (lpbin);
  mylist =
      gst_element_factory_list_filter (lpbin->elements, caps, GST_PAD_SINK,
      FALSE);
  g_mutex_unlock (&lpbin->elements_lock);

  GST_PLUGIN_FEATURE_LIST_DEBUG (mylist);

  result = g_value_array_new (g_list_length (mylist));

  for (tmp = mylist; tmp; tmp = tmp->next) {
    GstElementFactory *factory = GST_ELEMENT_FACTORY_CAST (tmp->data);
    GValue val = { 0, };

    if (gst_element_factory_list_is_type (factory,
            GST_ELEMENT_FACTORY_TYPE_SINK |
            GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)) {
      continue;
    }
    if (gst_element_factory_list_is_type (factory,
            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO
            | GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE)) {
      continue;
    }

    g_value_init (&val, G_TYPE_OBJECT);
    g_value_set_object (&val, factory);
    g_value_array_append (result, &val);
    g_value_unset (&val);
  }
  gst_plugin_feature_list_free (mylist);

  return result;
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

  g_signal_connect (lpbin, "autoplug-continue",
      G_CALLBACK (lpbin_notify_autoplug_continue_cb), loop);

  g_signal_connect (lpbin, "autoplug-factories",
      G_CALLBACK (lpbin_notify_autoplug_factories_cb), loop);

  gst_element_set_state (lpbin, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  gst_element_set_state (lpbin, GST_STATE_NULL);
  g_object_unref (lpbin);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
