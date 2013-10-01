/* GStreamer streamiddemux element
 *
 * Copyright 2013 LGE Corporation.
 *  @author: HoonHee Lee <hoonhee.lee@lge.com>
 *  @author: Jeongseok Kim <jeongseok.kim@lge.com>
 *  @author: Wonchul Lee <wonchul86.lee@lge.com>
 *
 * gststreamiddemux.c: Simple stream-id-demultiplexer element
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/**
 * SECTION:element-streamid-demux
 * @see_also: #GstStreamidDemux, #GstFunnel
 *
 * Direct input stream to one out of N output pads by stream-id.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gststreamiddemux.h"

GST_DEBUG_CATEGORY_STATIC (streamid_demux_debug);
#define GST_CAT_DEFAULT streamid_demux_debug

enum
{
  PROP_0,
  PROP_ACTIVE_SRC_PAD,
  PROP_LAST
};

static GstStaticPadTemplate gst_streamid_demux_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_streamid_demux_src_factory =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

#define _do_init \
GST_DEBUG_CATEGORY_INIT (streamid_demux_debug, \
        "streamiddemux", 0, "Streamid demuxer");
#define gst_streamid_demux_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstStreamidDemux, gst_streamid_demux,
    GST_TYPE_ELEMENT, _do_init);

static void gst_streamid_demux_dispose (GObject * object);
static void gst_streamid_demux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_streamid_demux_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);
static gboolean gst_streamid_demux_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstStateChangeReturn gst_streamid_demux_change_state (GstElement *
    element, GstStateChange transition);
static GstPad *gst_streamid_demux_get_srcpad_by_stream_id (GstStreamidDemux *
    demux, const gchar * stream_id);
static void gst_streamid_demux_srcpad_create (GstStreamidDemux * demux,
    GstPad * pad, const gchar * stream_id);

static void
gst_streamid_demux_class_init (GstStreamidDemuxClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->get_property = gst_streamid_demux_get_property;
  gobject_class->dispose = gst_streamid_demux_dispose;

  g_object_class_install_property (gobject_class, PROP_ACTIVE_SRC_PAD,
      g_param_spec_object ("active-src-pad", "Active src pad",
          "The currently active src pad", GST_TYPE_PAD,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "Streamid Demux",
      "Generic", "1-to-N output stream by stream-id",
      "HoonHee Lee <hoonhee.lee@lge.com>");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_streamid_demux_sink_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_streamid_demux_src_factory));

  gstelement_class->change_state = gst_streamid_demux_change_state;
}

static void
gst_streamid_demux_init (GstStreamidDemux * demux)
{
  demux->sinkpad =
      gst_pad_new_from_static_template (&gst_streamid_demux_sink_factory,
      "sink");
  gst_pad_set_chain_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_streamid_demux_chain));
  gst_pad_set_event_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_streamid_demux_event));

  gst_element_add_pad (GST_ELEMENT (demux), demux->sinkpad);

  g_rw_lock_init (&demux->lock);

  /* srcpad management */
  demux->active_srcpad = NULL;
  demux->nb_srcpads = 0;

  demux->stream_id_pairs = NULL;
}

static void
gst_streamid_demux_dispose (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_streamid_demux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstStreamidDemux *demux = GST_STREAMID_DEMUX (object);

  switch (prop_id) {
    case PROP_ACTIVE_SRC_PAD:
      GST_OBJECT_LOCK (demux);
      g_value_set_object (value, demux->active_srcpad);
      GST_OBJECT_UNLOCK (demux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
forward_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *srcpad = GST_PAD_CAST (user_data);

  gst_pad_push_event (srcpad, gst_event_ref (*event));

  return TRUE;
}

static void
gst_streamid_demux_srcpad_create (GstStreamidDemux * demux, GstPad * pad,
    const gchar * stream_id)
{
  gchar *padname = NULL;
  GstPad *srcpad = NULL;

  if (demux->stream_id_pairs == NULL)
    demux->stream_id_pairs = g_hash_table_new (g_str_hash, g_str_equal);

  padname = g_strdup_printf ("src_%u", demux->nb_srcpads++);
  srcpad =
      gst_pad_new_from_template (gst_static_pad_template_get
      (&gst_streamid_demux_src_factory), padname);

  gst_object_ref (srcpad);
  demux->active_srcpad = srcpad;

  g_rw_lock_writer_lock (&demux->lock);
  g_hash_table_insert (demux->stream_id_pairs, g_strdup (stream_id),
      (gpointer) srcpad);
  g_rw_lock_writer_unlock (&demux->lock);

  gst_pad_set_active (srcpad, TRUE);

  /* Forward sticky events to the new srcpad */
  gst_pad_sticky_events_foreach (demux->sinkpad, forward_sticky_events, srcpad);

  gst_element_add_pad (GST_ELEMENT_CAST (demux), srcpad);

  if (padname)
    g_free (padname);
}

static GstFlowReturn
gst_streamid_demux_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstFlowReturn res = GST_FLOW_OK;
  GstStreamidDemux *demux;

  demux = GST_STREAMID_DEMUX (parent);

  GST_LOG_OBJECT (demux, "pushing buffer to %" GST_PTR_FORMAT,
      demux->active_srcpad);

  if (demux->active_srcpad)
    res = gst_pad_push (demux->active_srcpad, buf);
  else
    gst_buffer_unref (buf);

  GST_LOG_OBJECT (demux, "handled buffer %s", gst_flow_get_name (res));
  return res;
}

static GstPad *
gst_streamid_demux_get_srcpad_by_stream_id (GstStreamidDemux * demux,
    const gchar * stream_id)
{
  GstPad *srcpad = NULL;

  GST_DEBUG_OBJECT (demux, "stream_id = %s", stream_id);
  if (demux->stream_id_pairs == NULL || stream_id == NULL) {
    goto done;
  }

  g_rw_lock_reader_lock (&demux->lock);
  srcpad = g_hash_table_lookup (demux->stream_id_pairs, stream_id);
  if (srcpad)
    gst_object_ref (srcpad);
  g_rw_lock_reader_unlock (&demux->lock);
  if (srcpad) {
    GST_DEBUG_OBJECT (demux, "srcpad = %s matched", gst_pad_get_name (srcpad));
  }

done:
  return srcpad;
}

static gboolean
gst_streamid_demux_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = TRUE;
  GstStreamidDemux *demux;
  const gchar *stream_id = NULL;
  GstPad *active_srcpad = NULL;

  demux = GST_STREAMID_DEMUX (parent);

  GST_DEBUG_OBJECT (demux, "event = %s, sticky = %d",
      GST_EVENT_TYPE_NAME (event), GST_EVENT_IS_STICKY (event));

  if (GST_EVENT_IS_STICKY (event)) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_STREAM_START) {
      gst_event_parse_stream_start (event, &stream_id);
      if (stream_id != NULL) {
        active_srcpad =
            gst_streamid_demux_get_srcpad_by_stream_id (demux, stream_id);
        if (!active_srcpad) {
          gst_streamid_demux_srcpad_create (demux, pad, stream_id);
        } else if (demux->active_srcpad != active_srcpad) {
          demux->active_srcpad = active_srcpad;
        }
      } else {
        goto no_stream_id;
      }
    }
  }

  if (GST_EVENT_TYPE (event) == GST_EVENT_FLUSH_START
      || GST_EVENT_TYPE (event) == GST_EVENT_FLUSH_STOP
      || GST_EVENT_TYPE (event) == GST_EVENT_EOS)
    res = gst_pad_event_default (pad, parent, event);
  else if (demux->active_srcpad)
    res = gst_pad_push_event (demux->active_srcpad, event);
  else
    gst_event_unref (event);

  return res;

  /* ERRORS */
no_stream_id:
  {
    GST_WARNING_OBJECT (demux, "no stream_id found");
    gst_event_unref (event);
    return FALSE;
  }
}

static void
gst_streamid_demux_free_stream_id_pairs (gpointer key, gpointer value,
    gpointer user_data)
{
  gchar *stream_id = (gchar *) key;
  GstPad *srcpad = (GstPad *) value;

  if (srcpad) {
    gst_object_unref (srcpad);
  }

  if (stream_id) {
    g_free (stream_id);
  }
}

static void
gst_streamid_demux_reset (GstStreamidDemux * demux)
{
  if (demux->stream_id_pairs) {
    g_hash_table_foreach (demux->stream_id_pairs,
        gst_streamid_demux_free_stream_id_pairs, NULL);
    g_hash_table_destroy (demux->stream_id_pairs);
    demux->stream_id_pairs = NULL;
  }
}

static GstStateChangeReturn
gst_streamid_demux_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStreamidDemux *demux;
  GstStateChangeReturn result;

  demux = GST_STREAMID_DEMUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    default:
      break;
  }

  result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_streamid_demux_reset (demux);
      break;
    default:
      break;
  }

  return result;
}
