/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013 LG Electronics.
 * Author : HoonHee Lee <hoonhee.lee@lge.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstreversefunnel.h"

GST_DEBUG_CATEGORY_STATIC (reverse_funnel_debug);
#define GST_CAT_DEFAULT reverse_funnel_debug

static GstStaticPadTemplate gst_reverse_funnel_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_reverse_funnel_src_factory =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

/* signals */
enum
{
  SIGNAL_SRC_PAD_ADDED,
  LAST_SIGNAL
};

static guint gst_reverse_funnel_signals[LAST_SIGNAL] = { 0 };

static GRWLock lock;

#define _do_init \
GST_DEBUG_CATEGORY_INIT (reverse_funnel_debug, \
        "reversefunnel", 0, "Reverse funnel");
#define gst_reverse_funnel_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstReverseFunnel, gst_reverse_funnel,
    GST_TYPE_ELEMENT, _do_init);

static void gst_reverse_funnel_dispose (GObject * object);
static GstFlowReturn gst_reverse_funnel_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);
static GstStateChangeReturn gst_reverse_funnel_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_reverse_funnel_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_reverse_funnel_query (GstPad * pad, GstObject * parent,
    GstQuery * query);
const static GstPad
    *gst_reverse_funnel_get_srcpad_by_stream_id (GstReverseFunnel * sel,
    const gchar * stream_id);
const static gboolean gst_reverse_funnel_srcpad_has_stream_id (GstReverseFunnel
    * sel, const gchar * stream_id);
static void gst_reverse_funnel_srcpad_create (GstReverseFunnel * osel,
    GstPad * pad, GstCaps * caps);

static void
gst_reverse_funnel_class_init (GstReverseFunnelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->dispose = gst_reverse_funnel_dispose;

  gst_reverse_funnel_signals[SIGNAL_SRC_PAD_ADDED] =
      g_signal_new ("src-pad-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstReverseFunnelClass,
          src_pad_added), NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE,
      1, GST_TYPE_PAD);

  gst_element_class_set_static_metadata (gstelement_class, "Reverse funnel",
      "Generic", "1-to-N output stream for funnel",
      "HoonHee Lee <hoonhee.lee@lge.com>");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_reverse_funnel_sink_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_reverse_funnel_src_factory));

  gstelement_class->change_state = gst_reverse_funnel_change_state;
}

static void
gst_reverse_funnel_init (GstReverseFunnel * rfunnel)
{
  rfunnel->sinkpad =
      gst_pad_new_from_static_template (&gst_reverse_funnel_sink_factory,
      "sink");
  gst_pad_set_chain_function (rfunnel->sinkpad,
      GST_DEBUG_FUNCPTR (gst_reverse_funnel_chain));
  gst_pad_set_event_function (rfunnel->sinkpad,
      GST_DEBUG_FUNCPTR (gst_reverse_funnel_event));
  gst_pad_set_query_function (rfunnel->sinkpad,
      GST_DEBUG_FUNCPTR (gst_reverse_funnel_query));

  gst_element_add_pad (GST_ELEMENT (rfunnel), rfunnel->sinkpad);

  g_rw_lock_init (&lock);

  /* srcpad management */
  rfunnel->nb_srcpads = 0;

  rfunnel->stream_id_pairs = NULL;
}

static void
gst_reverse_funnel_dispose (GObject * object)
{
  GstReverseFunnel *rfunnel = GST_REVERSE_FUNNEL (object);

  if (rfunnel->stream_id_pairs) {
    g_hash_table_remove_all (rfunnel->stream_id_pairs);
    g_hash_table_destroy (rfunnel->stream_id_pairs);
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
forward_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *srcpad = GST_PAD_CAST (user_data);
  gst_pad_push_event (srcpad, gst_event_ref (*event));

  return TRUE;
}

static void
gst_reverse_funnel_srcpad_create (GstReverseFunnel * rfunnel, GstPad * pad,
    GstCaps * caps)
{
  gchar *padname = NULL;
  GstPad *srcpad = NULL;
  GstCaps *outcaps = NULL;
  const gchar *stream_id = NULL;
  gchar *pad_name = NULL;

  stream_id = gst_pad_get_stream_id (pad);

  if (rfunnel->stream_id_pairs == NULL)
    rfunnel->stream_id_pairs = g_hash_table_new (g_str_hash, g_str_equal);

  if (gst_reverse_funnel_srcpad_has_stream_id (rfunnel, stream_id)) {
    GST_DEBUG_OBJECT (rfunnel,
        "gst_reverse_funnel_srcpad_create : already created src pad");
    return;
  }

  GST_DEBUG_OBJECT (rfunnel, "gst_reverse_funnel_srcpad_create");

  GST_OBJECT_LOCK (rfunnel);
  padname = g_strdup_printf ("src_%u", rfunnel->nb_srcpads++);
  srcpad =
      gst_pad_new_from_template (gst_static_pad_template_get
      (&gst_reverse_funnel_src_factory), padname);
  GST_OBJECT_UNLOCK (rfunnel);

  gst_pad_set_active (srcpad, TRUE);

  /* Forward sticky events to the new srcpad */
  gst_pad_sticky_events_foreach (rfunnel->sinkpad, forward_sticky_events,
      srcpad);

  gst_element_add_pad (GST_ELEMENT (rfunnel), srcpad);

  g_rw_lock_writer_lock (&lock);
  gst_object_ref (srcpad);
  g_hash_table_insert (rfunnel->stream_id_pairs, g_strdup (stream_id),
      (gpointer) srcpad);
  g_rw_lock_writer_unlock (&lock);

  GST_DEBUG_OBJECT (rfunnel, "gst_reverse_funnel_srcpad_create");

  outcaps = gst_caps_copy (caps);
  gst_pad_set_caps (srcpad, outcaps);

  g_signal_emit (G_OBJECT (rfunnel),
      gst_reverse_funnel_signals[SIGNAL_SRC_PAD_ADDED], 0, srcpad);

  if (outcaps)
    gst_caps_unref (outcaps);
  if (padname)
    g_free (padname);
  if (stream_id)
    g_free (stream_id);
}

static GstFlowReturn
gst_reverse_funnel_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstFlowReturn res;
  GstReverseFunnel *rfunnel;
  GstPad *active_srcpad;

  rfunnel = GST_REVERSE_FUNNEL (parent);

  GST_OBJECT_LOCK (rfunnel);

  active_srcpad =
      gst_reverse_funnel_get_srcpad_by_stream_id (rfunnel,
      gst_pad_get_stream_id (pad));
  GST_LOG_OBJECT (rfunnel, "pushing buffer to %" GST_PTR_FORMAT, active_srcpad);

  GST_OBJECT_UNLOCK (rfunnel);
  if (active_srcpad)
    res = gst_pad_push (active_srcpad, buf);
  return res;
}

static GstStateChangeReturn
gst_reverse_funnel_change_state (GstElement * element,
    GstStateChange transition)
{
  GstReverseFunnel *rfunnel;
  GstStateChangeReturn result;

  rfunnel = GST_REVERSE_FUNNEL (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    default:
      break;
  }

  result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    default:
      break;
  }

  return result;
}

const static GstPad *
gst_reverse_funnel_get_srcpad_by_stream_id (GstReverseFunnel * rfunnel,
    const gchar * stream_id)
{
  GST_DEBUG_OBJECT (rfunnel,
      "gst_reverse_funnel_get_srcpad_by_stream_id : stream_id = %s", stream_id);
  GstPad *srcpad = NULL;

  if (rfunnel->stream_id_pairs == NULL || stream_id == NULL) {
    goto done;
  }

  g_rw_lock_writer_lock (&lock);
  srcpad = g_hash_table_lookup (rfunnel->stream_id_pairs, stream_id);
  g_rw_lock_writer_unlock (&lock);
  if (srcpad) {
    GST_DEBUG_OBJECT (rfunnel,
        "gst_reverse_funnel_get_srcpad_by_stream_id : srcpad = %s matched",
        gst_pad_get_name (srcpad));
  }

done:
  return srcpad;
}

const static gboolean
gst_reverse_funnel_srcpad_has_stream_id (GstReverseFunnel * rfunnel,
    const gchar * stream_id)
{
  GST_DEBUG_OBJECT (rfunnel,
      "gst_reverse_funnel_srcpad_has_stream_id : stream_id = %s", stream_id);
  gboolean ret = FALSE;
  GstPad *srcpad = NULL;
  g_rw_lock_writer_lock (&lock);
  srcpad = g_hash_table_lookup (rfunnel->stream_id_pairs, stream_id);
  g_rw_lock_writer_unlock (&lock);
  if (srcpad) {
    GST_DEBUG_OBJECT (rfunnel,
        "gst_reverse_funnel_srcpad_has_stream_id : already added");
    ret = TRUE;
  }

  return ret;
}

static gboolean
gst_reverse_funnel_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = TRUE;
  GstReverseFunnel *rfunnel;
  GstPad *active = NULL;
  GstCaps *caps = NULL;

  rfunnel = GST_REVERSE_FUNNEL (parent);
  GST_DEBUG_OBJECT (rfunnel,
      "gst_reverse_funnel_event : event = %s, stream-id = %s",
      GST_EVENT_TYPE_NAME (event), gst_pad_get_stream_id (pad));
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_STREAM_START:
    {
      if (gst_pad_get_stream_id (pad) != NULL) {
        caps = gst_pad_get_current_caps (pad);
        gst_reverse_funnel_srcpad_create (rfunnel, pad, caps);
      }
      break;
    }
    case GST_EVENT_CAPS:
    {
      /* Send caps to all src pads */
      res = gst_pad_event_default (pad, parent, event);
      break;
    }
    case GST_EVENT_SEGMENT:
    {
      /* Send newsegment to all src pads */
      res = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
    {
      if (GST_EVENT_IS_STICKY (event)) {
        res = gst_pad_event_default (pad, parent, event);
      } else {
        active = gst_reverse_funnel_get_srcpad_by_stream_id (rfunnel,
            gst_pad_get_stream_id (pad));
        if (active == NULL) {
          gst_event_unref (event);
        } else {
          /* Send other events to active src pad */
          res = gst_pad_push_event (active, event);
        }
      }
      break;
    }
  }

  return res;
}

static gboolean
gst_reverse_funnel_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE;
  GstReverseFunnel *rfunnel;
  GstPad *active = NULL;
  rfunnel = GST_REVERSE_FUNNEL (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      active = gst_reverse_funnel_get_srcpad_by_stream_id (rfunnel,
          gst_pad_get_stream_id (pad));
      if (active == NULL) {
        res = FALSE;
      } else {
        res = gst_pad_peer_query (active, query);
      }
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }
  return res;
}
