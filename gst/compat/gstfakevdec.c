/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com> 
 *	         Justin Joy <justin.joy.9to5@gmail.com> 
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

#include "gstfakevdec.h"
#include "gstfdcaps.h"

static GstStaticPadTemplate gst_fakevdec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (FD_VIDEO_CAPS)
    );

static GstStaticPadTemplate gst_fakevdec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-fd")
    );

GST_DEBUG_CATEGORY_STATIC (fakevdec_debug);
#define GST_CAT_DEFAULT fakevdec_debug

static GstStateChangeReturn
gst_fakevdec_change_state (GstElement * element, GstStateChange transition);

static gboolean gst_fakevdec_set_caps (GstFakeVdec * fakevdec, GstCaps * caps);
static gboolean gst_fakevdec_query (GstPad * pad, GstObject * parent,
    GstQuery * query);
static gboolean gst_fakevdec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_fakevdec_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

#define gst_fakevdec_parent_class parent_class
G_DEFINE_TYPE (GstFakeVdec, gst_fakevdec, GST_TYPE_ELEMENT);


static gboolean
gst_fakevdec_set_caps (GstFakeVdec * fakevdec, GstCaps * caps)
{
  //GstStructure *structure;
  gboolean ret;
  GstCaps *outcaps;

  //structure = gst_caps_get_structure (caps, 0); 

  //ret = gst_structure_get_int (structure, "rate", &rate);
  //ret &= gst_structure_get_int (structure, "channels", &channels);

  outcaps = gst_caps_copy (caps);
  ret = gst_pad_set_caps (fakevdec->srcpad, outcaps);   //outcaps);
  gst_caps_unref (outcaps);

  return ret;
}

static GstCaps *
gst_fakevdec_get_caps (GstPad * pad, GstCaps * filter)
{
  GstFakeVdec *fakevdec;
  GstPad *otherpad;
  GstCaps *othercaps, *result;
  GstCaps *templ;

  fakevdec = GST_FAKEVDEC (GST_PAD_PARENT (pad));

  /* figure out the name of the caps we are going to return */
  if (pad == fakevdec->srcpad) {
    otherpad = fakevdec->sinkpad;
  } else {
    otherpad = fakevdec->srcpad;
  }
  /* get caps from the peer, this can return NULL when there is no peer */
  othercaps = gst_pad_peer_query_caps (otherpad, NULL);

  /* get the template caps to make sure we return something acceptable */
  templ = gst_pad_get_pad_template_caps (pad);

  if (othercaps) {
    /* filter against the allowed caps of the pad to return our result */
    result = gst_caps_intersect (othercaps, templ);
    gst_caps_unref (othercaps);
    gst_caps_unref (templ);
  } else {
    /* there was no peer, return the template caps */
    result = templ;
  }
  if (filter && result) {
    GstCaps *temp;

    temp = gst_caps_intersect (result, filter);
    gst_caps_unref (result);
    result = temp;
  }
  return result;
}

static gboolean
gst_fakevdec_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_fakevdec_get_caps (pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);

      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }
  return res;
}

static void
gst_fakevdec_class_init (GstFakeVdecClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_sink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Fake Video decoder",
      "Codec/Decoder/Video",
      "Pass data to backend decoder",
      "Wonchul Lee <wonchul86.lee@lge.com>,Justin Joy <justin.joy.9to5@gmail.com>");

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_fakevdec_change_state);

  GST_DEBUG_CATEGORY_INIT (fakevdec_debug, "fakevdec", 0, "Fake video decoder");
}

static void
gst_fakevdec_init (GstFakeVdec * fakevdec)
{
  fakevdec->sinkpad =
      gst_pad_new_from_static_template (&gst_fakevdec_sink_pad_template,
      "sink");
  gst_pad_set_event_function (fakevdec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakevdec_sink_event));
  gst_pad_set_chain_function (fakevdec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakevdec_chain));
  //gst_pad_set_query_function (fakevdec->sinkpad,
  //              GST_DEBUG_FUNCPTR (gst_fakevdec_query));
  gst_element_add_pad (GST_ELEMENT (fakevdec), fakevdec->sinkpad);

  fakevdec->srcpad =
      gst_pad_new_from_static_template (&gst_fakevdec_src_pad_template, "src");
  //gst_pad_set_query_function (fakevdec->srcpad,
  //              GST_DEBUG_FUNCPTR (gst_fakevdec_query));
  //gst_pad_use_fixed_caps (fakevdec->srcpad);
  gst_element_add_pad (GST_ELEMENT (fakevdec), fakevdec->srcpad);
}

static gboolean
gst_fakevdec_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstFakeVdec *fakevdec;
  gboolean res;

  fakevdec = GST_FAKEVDEC (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      gst_event_parse_caps (event, &caps);
      gst_fakevdec_set_caps (fakevdec, caps);
      gst_event_unref (event);
    }
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}


static GstFlowReturn
gst_fakevdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFakeVdec *fakevdec;
  GstMapInfo inmap;
  GstBuffer *outbuf;
  GstFlowReturn ret;

  fakevdec = GST_FAKEVDEC (parent);

  GST_LOG_OBJECT (fakevdec, "buffer with ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

  gst_buffer_map (buffer, &inmap, GST_MAP_READ);

  outbuf = gst_buffer_copy (buffer);

  gst_buffer_unmap (buffer, &inmap);
  gst_buffer_unref (buffer);

  ret = gst_pad_push (fakevdec->srcpad, outbuf);

  return ret;

}

static GstStateChangeReturn
gst_fakevdec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  //GstFakeVdec *fakevdec = GST_FAKEVDEC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;

  switch (transition) {
    default:
      break;
  }

  return ret;
}
