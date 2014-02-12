/* GStreamer httpextbin element
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *  Author : HoonHee Lee <hoonhee.lee@lge.com>
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
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gsthttpextbin.h"

GST_DEBUG_CATEGORY_STATIC (http_ext_bin_debug);
#define GST_CAT_DEFAULT http_ext_bin_debug

#define parent_class gst_http_ext_bin_parent_class

#define DEFAULT_PROP_URI NULL
enum
{
  PROP_0,
  PROP_SOURCE,
  PROP_URI,
  PROP_LAST
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_http_ext_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_http_ext_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_http_ext_bin_finalize (GObject * self);
static GstStateChangeReturn gst_http_ext_bin_change_state (GstElement *
    element, GstStateChange transition);
static void gst_http_ext_bin_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static gboolean connect_filter_element (GstHttpExtBin * bin);
G_DEFINE_TYPE_WITH_CODE (GstHttpExtBin, gst_http_ext_bin, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_http_ext_bin_uri_handler_init));

static void
gst_http_ext_bin_class_init (GstHttpExtBinClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_http_ext_bin_set_property;
  gobject_class->get_property = gst_http_ext_bin_get_property;
  gobject_class->finalize = gst_http_ext_bin_finalize;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));

  g_object_class_install_property (gobject_class, PROP_SOURCE,
      g_param_spec_object ("source", "HTTP client source",
          "the http client source element to use",
          GST_TYPE_ELEMENT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("uri", "URI",
          "URI to be set source element",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_http_ext_bin_change_state);

  gst_element_class_set_static_metadata (gstelement_class,
      "Extended Http Source Bin", "Source/Bin/Network/Protocol",
      "HTTP Source with unified data processing element",
      "HoonHee Lee <hoonhee.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (http_ext_bin_debug, "httpextbin", 0,
      "Extended Http Source Bin");
}

static void
gst_http_ext_bin_init (GstHttpExtBin * bin)
{
  GstPadTemplate *pad_tmpl;

  /* get src pad template */
  pad_tmpl = gst_static_pad_template_get (&src_template);
  bin->srcpad = gst_ghost_pad_new_no_target_from_template ("src", pad_tmpl);
  gst_pad_set_active (bin->srcpad, TRUE);
  /* add src ghost pad */
  gst_element_add_pad (GST_ELEMENT (bin), bin->srcpad);
  gst_object_unref (pad_tmpl);

  /* init member variable */
  bin->uri = g_strdup (DEFAULT_PROP_URI);

  GST_OBJECT_FLAG_SET (bin, GST_ELEMENT_FLAG_SOURCE);
}

static void
gst_http_ext_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstHttpExtBin *bin = GST_HTTP_EXT_BIN (object);
  switch (prop_id) {
    case PROP_URI:
    {
      GST_OBJECT_LOCK (bin);
      GstURIHandler *handler;
      GstURIHandlerInterface *iface;

      handler = GST_URI_HANDLER (bin);
      iface = GST_URI_HANDLER_GET_INTERFACE (handler);
      iface->set_uri (handler, g_value_get_string (value), NULL);
      GST_OBJECT_UNLOCK (bin);
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_http_ext_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstHttpExtBin *bin = GST_HTTP_EXT_BIN (object);

  switch (prop_id) {
    case PROP_SOURCE:
      GST_OBJECT_LOCK (bin);
      g_value_set_object (value, bin->source_elem);
      GST_OBJECT_UNLOCK (bin);
      break;
    case PROP_URI:
      g_value_set_string (value, bin->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_http_ext_bin_finalize (GObject * self)
{
  GstHttpExtBin *bin = GST_HTTP_EXT_BIN (self);

  if (bin->uri)
    g_free (bin->uri);

  if (bin->caps)
    gst_caps_unref (bin->caps);
  bin->caps = NULL;

  if (bin->list)
    gst_plugin_feature_list_free (bin->list);
  bin->list = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (self);
}

static gboolean
set_uri_to_source (GstHttpExtBin * bin, const gchar * new_uri)
{
  GstURIHandler *src_handler;
  GstURIHandlerInterface *src_iface;

  src_handler = GST_URI_HANDLER (bin->source_elem);
  if (!src_handler)
    return FALSE;
  src_iface = GST_URI_HANDLER_GET_INTERFACE (src_handler);
  if (!src_iface)
    return FALSE;

  return src_iface->set_uri (src_handler, new_uri, NULL);
}

gint
_http_ext_bin_compare_factories_func (gconstpointer p1, gconstpointer p2)
{
  /* And if it's a both a parser we first sort by rank
   * and then by factory name */
  return gst_plugin_feature_rank_compare_func (p1, p2);
}

static GList *
gst_http_ext_bin_update_factories_list (GstHttpExtBin * bin)
{
  GList *factories;

  /* list up elements with given caps */
  factories =
      gst_registry_get_feature_list (gst_registry_get (),
      GST_TYPE_ELEMENT_FACTORY);

  if (!factories) {
    GST_WARNING_OBJECT (bin, "Couldn't list up any of elements");
    return NULL;
  }

  if (bin->list)
    gst_plugin_feature_list_free (bin->list);
  bin->list =
      gst_element_factory_list_filter (factories, bin->caps, GST_PAD_SINK,
      gst_caps_is_fixed (bin->caps));
  gst_plugin_feature_list_free (factories);

  if (!bin->list) {
    GST_WARNING_OBJECT (bin,
        "Couldn't list up any of element which handles caps:%s",
        gst_caps_to_string (bin->caps));
    return NULL;
  }

  return g_list_sort (bin->list, _http_ext_bin_compare_factories_func);
}

static gboolean
connect_filter_element (GstHttpExtBin * bin)
{
  GstPad *srcpad, *sinkpad;
  gboolean ret = TRUE;

  /* add filter element to bin */
  if (!(gst_bin_add (GST_BIN_CAST (bin), bin->filter_elem))) {
    GST_WARNING_OBJECT (bin, "Couldn't add %s to the bin",
        GST_ELEMENT_NAME (bin->filter_elem));
    return FALSE;
  }

  /* try to get src pad from source element */
  if (!(srcpad = gst_element_get_static_pad (bin->source_elem, "src"))) {
    GST_WARNING_OBJECT (bin, "Element %s doesn't have a src pad",
        GST_ELEMENT_NAME (bin->source_elem));
    return FALSE;
  }

  /* try to get sink pad from filter element */
  if (!(sinkpad = gst_element_get_static_pad (bin->filter_elem, "sink"))) {
    GST_WARNING_OBJECT (bin, "Element %s doesn't have a sink pad",
        GST_ELEMENT_NAME (bin->filter_elem));
    return FALSE;
  }

  /* try to link srcpad and sinkpad */
  if ((gst_pad_link (srcpad, sinkpad)) != GST_PAD_LINK_OK) {
    GST_WARNING_OBJECT (bin, "Link failed on pad %s:%s",
        GST_DEBUG_PAD_NAME (sinkpad));
    return FALSE;
  }

  g_object_unref (srcpad);
  g_object_unref (sinkpad);

  /* try to target from ghostpad to srcpad */
  if (!(srcpad = gst_element_get_static_pad (bin->filter_elem, "src"))) {
    GST_WARNING_OBJECT (bin, "Element %s doesn't have a src pad",
        GST_ELEMENT_NAME (bin->filter_elem));
    return FALSE;
  }
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (bin->srcpad), srcpad);

  g_object_unref (srcpad);

  return ret;
}

/* remove source and all related elements */
static void
remove_source (GstHttpExtBin * bin)
{
  if (bin->source_elem) {
    GST_DEBUG_OBJECT (bin, "removing old src element");
    gst_element_set_state (bin->source_elem, GST_STATE_NULL);

    gst_bin_remove (GST_BIN_CAST (bin), bin->source_elem);
    bin->source_elem = NULL;
  }

  if (bin->filter_elem) {
    GST_DEBUG_OBJECT (bin, "removing old filter element");
    gst_element_set_state (bin->filter_elem, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (bin), bin->filter_elem);
    bin->filter_elem = NULL;
  }

  if (bin->caps)
    gst_caps_unref (bin->caps);
  bin->caps = NULL;

  if (bin->list)
    gst_plugin_feature_list_free (bin->list);
  bin->list = NULL;

  /* Don't loose the SOURCE flag */
  GST_OBJECT_FLAG_SET (bin, GST_ELEMENT_FLAG_SOURCE);
}

static gboolean
setup_source (GstHttpExtBin * bin)
{
  gboolean ret = TRUE;
  gchar *protocol, *location;
  gchar **protocols = NULL;
  gchar *real_protocol;
  gchar *new_uri;
  GList *tmp;
  GstElementFactory *factory = NULL;
  gboolean skip = FALSE;
  gchar *caps_str;

  GST_DEBUG_OBJECT (bin, "setup source");

  /* delete old src */
  remove_source (bin);

  protocol = gst_uri_get_protocol (bin->uri);
  location = gst_uri_get_location (bin->uri);

  GST_INFO_OBJECT (bin, "protocol:%s, location:%s", protocol, location);

  protocols = g_strsplit_set (protocol, "+", -1);
  g_assert (protocols != NULL);
  g_assert (g_strv_length (protocols) == 2);

  real_protocol = protocols[0];
  bin->caps =
      gst_caps_from_string (g_strdup_printf ("application/%s", protocols[1]));

  GST_INFO_OBJECT (bin, "real_protocol:%s, caps:%s", real_protocol,
      gst_caps_to_string (bin->caps));

  /* list up elements with given caps */
  bin->list = gst_http_ext_bin_update_factories_list (bin);

  for (tmp = bin->list; tmp; tmp = tmp->next) {
    const GList *templs;
    GstElementFactory *fac = GST_ELEMENT_FACTORY_CAST (tmp->data);
    templs = gst_element_factory_get_static_pad_templates (fac);

    while (templs) {
      GstStaticPadTemplate *templ = (GstStaticPadTemplate *) templs->data;

      if (templ->direction == GST_PAD_SINK) {
        GstCaps *templcaps = gst_static_caps_get (&templ->static_caps);
        if (!gst_caps_is_any (templcaps)
            && gst_caps_is_subset (bin->caps, templcaps)) {
          GST_INFO_OBJECT (bin,
              "caps %" GST_PTR_FORMAT " subset of %" GST_PTR_FORMAT, bin->caps,
              templcaps);
          factory = fac;
          gst_caps_unref (templcaps);
          skip = TRUE;
          break;
        }
        gst_caps_unref (templcaps);
      }
      templs = g_list_next (templs);
    }
    if (skip)
      break;
  }

  if (!factory) {
    caps_str = gst_caps_to_string (bin->caps);
    GST_ELEMENT_ERROR (bin, CORE, MISSING_PLUGIN, (NULL),
        ("No filter element which can handle given caps:%s, check your installation",
            caps_str));
    g_free (caps_str);
    return FALSE;
  }

  /* generate souphttpsrc element */
  if (!(bin->source_elem = gst_element_factory_make ("souphttpsrc", NULL))) {
    GST_WARNING_OBJECT (bin, "Could not create a souphttpsrc element ");
    return FALSE;
  }

  new_uri = g_strdup_printf ("%s://%s", real_protocol, location);
  GST_INFO_OBJECT (bin, "new_uri:%s", new_uri);

  /* set converted uri to souphttpsrc element */
  ret = set_uri_to_source (bin, new_uri);
  if (!ret) {
    GST_WARNING_OBJECT (bin, "Failed to set uri:%s to souphttpsrc element",
        new_uri);
    return FALSE;
  }
  g_free (protocol);
  g_free (location);
  g_free (real_protocol);
  g_free (new_uri);

  if (!(gst_bin_add (GST_BIN_CAST (bin), bin->source_elem))) {
    GST_WARNING_OBJECT (bin, "Couldn't add souphttp element to bin");
    return FALSE;
  }

  /* generate filter element */
  GST_INFO_OBJECT (bin, "filtered factory:%s", GST_OBJECT_NAME (factory));
  if ((bin->filter_elem = gst_element_factory_create (factory, NULL)) == NULL) {
    GST_WARNING_OBJECT (bin, "Could not create an element from %s",
        gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)));
    return FALSE;
  }

  /* connent filter element */
  ret = connect_filter_element (bin);

done:
  GST_INFO_OBJECT (bin, "ret:%d", ret);
  return ret;
}

static GstStateChangeReturn
gst_http_ext_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstHttpExtBin *bin;

  bin = GST_HTTP_EXT_BIN (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (!setup_source (bin))
        goto source_failed;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_DEBUG ("ready to paused");
      if (ret == GST_STATE_CHANGE_FAILURE)
        goto setup_failed;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG ("paused to ready");
      remove_source (bin);
      break;
    default:
      break;
  }
  return ret;

  /* ERRORS */
source_failed:
  {
    GST_WARNING_OBJECT (bin, "Failed to configure source and filter elements");
    return GST_STATE_CHANGE_FAILURE;
  }
setup_failed:
  {
    GST_DEBUG_OBJECT (bin,
        "element failed to change states -- activation problem?");
    return GST_STATE_CHANGE_FAILURE;
  }
}

static GstURIType
gst_http_ext_bin_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_http_ext_bin_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http+bbts", NULL };

  return protocols;
}

static gchar *
gst_http_ext_bin_uri_get_uri (GstURIHandler * handler)
{
  GstHttpExtBin *bin = GST_HTTP_EXT_BIN (handler);

  return bin->uri ? g_strdup (bin->uri) : NULL;
}

static gboolean
gst_http_ext_bin_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  gboolean ret = TRUE;
  GstHttpExtBin *bin;
  GstState current_state;

  if (!uri) {
    GST_WARNING_OBJECT (bin, "uri to set is empty");
    return FALSE;
  }

  bin = GST_HTTP_EXT_BIN (handler);
  current_state = GST_STATE (bin);

  /* check bin's state */
  if (current_state == GST_STATE_PAUSED || current_state == GST_STATE_PLAYING) {
    GST_WARNING_OBJECT (bin, "state of bin is %s",
        gst_element_state_get_name (current_state));
    return FALSE;
  }

  /* set original uri */
  g_free (bin->uri);
  bin->uri = g_strdup (uri);
  return TRUE;
}

static void
gst_http_ext_bin_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_http_ext_bin_uri_get_type;
  iface->get_protocols = gst_http_ext_bin_uri_get_protocols;
  iface->get_uri = gst_http_ext_bin_uri_get_uri;
  iface->set_uri = gst_http_ext_bin_uri_set_uri;
}
