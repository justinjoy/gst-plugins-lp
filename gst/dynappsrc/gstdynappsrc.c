/* GStreamer Dynamic App Source element
 * Copyright (C) 2014 LG Electronics, Inc.
 *  Author : Wonchul Lee <wonchul86.lee@lge.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


/**
 * SECTION:element-dynappsrc
 *
 * Dynappsrc provides multiple appsrc elements inside a single source element(bin)
 * for separated audio, video and text stream.
 *
 * <refsect2>
 * <title>Usage</title>
 * <para>
 * A dynappsrc element can be created by pipeline using URI included dynappsrc://
 * protocal.
 * The URI to play should be set via the #GstLpBin:uri property.
 *
 * Dynappsrc is a #GstBin. It will notified to application when it is created by
 * source-setup signal of pipeline.
 * A appsrc element can be created by new-appsrc signal action to dynappsrc. It
 * should be created before changing state READY to PAUSED. Therefore application
 * need to create appsrc element as soon as receiving source-setup signal.
 * Then appsrc can be configured by setting the element to PAUSED state.
 *
 * When playback has finished (an EOS message has been received on the bus)
 * or an error has occured (an ERROR message has been received on the bus) or
 * the user wants to play a different track, dynappsrc should be set back to
 * READY or NULL state, then appsrc elements in dynappsrc should be set to the
 * NULL state and removed from it.
 *
 * </para>
 * </refsect2>
 * <refsect2>
 * <title>Rule for reference appsrc</title>
 * <para>
 * Here is a rule for application when using the appsrc element.
 * When application created appsrc element, ref-count of appsrc is one by
 * default. If it was not intended to decrease ref-count during using by
 * application, pipeline may broke.
 * Therefore it should increase ref-count explicitly by using gst_object_ref.
 * It should be decreased ref-count either by using gst_object_unref when playback
 * has finished.
 * </para>
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gstdynappsrc.h"

GST_DEBUG_CATEGORY_STATIC (dyn_appsrc_debug);
#define GST_CAT_DEFAULT dyn_appsrc_debug

#define parent_class gst_dyn_appsrc_parent_class

#define DEFAULT_PROP_URI NULL

enum
{
  PROP_0,
  PROP_URI,
  PROP_N_SRC,
  PROP_LAST
};

enum
{
  SIGNAL_NEW_APPSRC,
  LAST_SIGNAL
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static guint gst_dyn_appsrc_signals[LAST_SIGNAL] = { 0 };

static void gst_dyn_appsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dyn_appsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_dyn_appsrc_finalize (GObject * self);
static GstStateChangeReturn gst_dyn_appsrc_change_state (GstElement * element,
    GstStateChange transition);
static GstElement *gst_dyn_appsrc_new_appsrc (GstDynAppSrc * bin,
    const gchar * name);
static void gst_dyn_appsrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (GstDynAppSrc, gst_dyn_appsrc, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_dyn_appsrc_uri_handler_init));


static void
gst_dyn_appsrc_class_init (GstDynAppSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_dyn_appsrc_set_property;
  gobject_class->get_property = gst_dyn_appsrc_get_property;
  gobject_class->finalize = gst_dyn_appsrc_finalize;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));

  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("uri", "URI",
          "URI to get protected content",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstDynAppSrc:n-src
   *
   * Get the total number of available streams.
   */
  g_object_class_install_property (gobject_class, PROP_N_SRC,
      g_param_spec_int ("n-source", "Number Source",
          "Total number of source streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * GstDynAppSrc::new-appsrc
   * @dynappsrc: a #GstDynAppSrc
   * @name : name of appsrc element
   *
   * Action signal to create a appsrc element.
   * This signal should be emitted before changing state READY to PAUSED.
   * The application emit this signal as soon as receiving source-setup signal from pipeline.
   *
   * Returns: a GstElement of appsrc element or NULL when element creation failed.
   */
  gst_dyn_appsrc_signals[SIGNAL_NEW_APPSRC] =
      g_signal_new ("new-appsrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstDynAppSrcClass, new_appsrc), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_ELEMENT, 1, G_TYPE_STRING);

  klass->new_appsrc = gst_dyn_appsrc_new_appsrc;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_dyn_appsrc_change_state);

  gst_element_class_set_static_metadata (gstelement_class,
      "Dynappsrc", "Source/Bin",
      "Dynamic App Source", "Wonchul Lee <wonchul86.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (dyn_appsrc_debug, "dynappsrc", 0,
      "Dynamic App Source");
}

static void
gst_dyn_appsrc_init (GstDynAppSrc * bin)
{
  /* init member variable */
  bin->uri = g_strdup (DEFAULT_PROP_URI);
  bin->appsrc_list = NULL;
  bin->n_source = 0;

  GST_OBJECT_FLAG_SET (bin, GST_ELEMENT_FLAG_SOURCE);
}

static void
gst_dyn_appsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (object);

  switch (prop_id) {
    case PROP_URI:
    {
      GstURIHandler *handler;
      GstURIHandlerInterface *iface;

      handler = GST_URI_HANDLER (bin);
      iface = GST_URI_HANDLER_GET_INTERFACE (handler);
      iface->set_uri (handler, g_value_get_string (value), NULL);
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dyn_appsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (object);

  switch (prop_id) {
    case PROP_URI:
      g_value_set_string (value, bin->uri);
      break;
    case PROP_N_SRC:
    {
      GST_OBJECT_LOCK (bin);
      g_value_set_int (value, bin->n_source);
      GST_OBJECT_UNLOCK (bin);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dyn_appsrc_finalize (GObject * self)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (self);

  g_free (bin->uri);

  G_OBJECT_CLASS (parent_class)->finalize (self);
}

static gboolean
gst_dyn_appsrc_handle_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstPad *target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad));
  gboolean res = FALSE;

  /* forward the query to the proxy target pad */
  if (target) {
    res = gst_pad_query (target, query);
    gst_object_unref (target);
  }

  return res;
}

static gboolean
gst_dyn_appsrc_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res = TRUE;
  GstPad *target;

  if ((target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad)))) {
    res = gst_pad_send_event (target, event);
    gst_object_unref (target);
  } else
    gst_event_unref (event);

  return res;
}

static gboolean
setup_source (GstDynAppSrc * bin)
{
  GList *item;
  GstPadTemplate *pad_tmpl;
  gchar *padname;
  gboolean ret = FALSE;

  pad_tmpl = gst_static_pad_template_get (&src_template);

  for (item = bin->appsrc_list; item; item = g_list_next (item)) {
    GstAppSourceGroup *appsrc_group = (GstAppSourceGroup *) item->data;
    GstPad *srcpad = NULL;

    gst_bin_add (GST_BIN_CAST (bin), appsrc_group->appsrc);

    srcpad = gst_element_get_static_pad (appsrc_group->appsrc, "src");
    padname =
        g_strdup_printf ("src_%u", g_list_position (bin->appsrc_list, item));
    appsrc_group->srcpad =
        gst_ghost_pad_new_from_template (padname, srcpad, pad_tmpl);
    gst_pad_set_event_function (appsrc_group->srcpad,
        gst_dyn_appsrc_handle_src_event);
    gst_pad_set_query_function (appsrc_group->srcpad,
        gst_dyn_appsrc_handle_src_query);

    gst_pad_set_active (appsrc_group->srcpad, TRUE);
    gst_element_add_pad (GST_ELEMENT_CAST (bin), appsrc_group->srcpad);

    g_free (padname);

    ret = TRUE;
  }
  gst_object_unref (pad_tmpl);

  if (ret) {
    GST_DEBUG_OBJECT (bin, "all appsrc elements are added");
    gst_element_no_more_pads (GST_ELEMENT_CAST (bin));
  }

  return ret;
}

static void
remove_source (GstDynAppSrc * bin)
{
  GList *item;

  for (item = bin->appsrc_list; item; item = g_list_next (item)) {
    GstAppSourceGroup *appsrc_group = (GstAppSourceGroup *) item->data;

    GST_DEBUG_OBJECT (bin, "removing appsrc element and ghostpad");

    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (appsrc_group->srcpad), NULL);
    gst_element_remove_pad (GST_ELEMENT_CAST (bin), appsrc_group->srcpad);
    appsrc_group->srcpad = NULL;

    gst_element_set_state (appsrc_group->appsrc, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (bin), appsrc_group->appsrc);
    appsrc_group->appsrc = NULL;
  }

  g_list_free_full (bin->appsrc_list, g_free);
  bin->appsrc_list = NULL;
}

static GstElement *
gst_dyn_appsrc_new_appsrc (GstDynAppSrc * bin, const gchar * name)
{
  GstAppSourceGroup *appsrc_group;

  GST_OBJECT_LOCK (bin);

  if (GST_STATE (bin) >= GST_STATE_PAUSED) {
    GST_WARNING_OBJECT (bin,
        "deny to create appsrc when state is in PAUSED or PLAYING state");
    GST_OBJECT_UNLOCK (bin);
    return NULL;
  }
  appsrc_group = g_malloc0 (sizeof (GstAppSourceGroup));
  appsrc_group->appsrc = gst_element_factory_make ("appsrc", name);
  bin->appsrc_list = g_list_append (bin->appsrc_list, appsrc_group);
  bin->n_source++;

  GST_INFO_OBJECT (bin, "appsrc %p is appended to a list",
      appsrc_group->appsrc);
  GST_INFO_OBJECT (bin, "source number = %d", bin->n_source);

  GST_OBJECT_UNLOCK (bin);

  return appsrc_group->appsrc;
}

static GstStateChangeReturn
gst_dyn_appsrc_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstDynAppSrc *bin = GST_DYN_APPSRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (!setup_source (bin))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_DEBUG_OBJECT (bin, "ready to paused");
      if (ret == GST_STATE_CHANGE_FAILURE)
        goto setup_failed;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG_OBJECT (bin, "paused to ready");
      remove_source (bin);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      remove_source (bin);
      break;
    default:
      break;
  }

  return ret;

setup_failed:
  {
    /* clean up leftover groups */
    return GST_STATE_CHANGE_FAILURE;
  }
}

static GstURIType
gst_dyn_appsrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_dyn_appsrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "dynappsrc", NULL };

  return protocols;
}

static gchar *
gst_dyn_appsrc_uri_get_uri (GstURIHandler * handler)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (handler);

  return bin->uri ? g_strdup (bin->uri) : NULL;
}

static gboolean
gst_dyn_appsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  return TRUE;
}

static void
gst_dyn_appsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_dyn_appsrc_uri_get_type;
  iface->get_protocols = gst_dyn_appsrc_uri_get_protocols;
  iface->get_uri = gst_dyn_appsrc_uri_get_uri;
  iface->set_uri = gst_dyn_appsrc_uri_set_uri;
}
