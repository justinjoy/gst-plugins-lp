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

#include "gstfakeadec.h"
#include "gstfdcaps.h"

static GstStaticPadTemplate gst_fakeadec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (FD_AUDIO_CAPS)
    );

static GstStaticPadTemplate gst_fakeadec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-fd")
    );

GST_DEBUG_CATEGORY_STATIC (fakeadec_debug);
#define GST_CAT_DEFAULT fakeadec_debug

static gboolean gst_fakeadec_set_caps (GstFakeAdec * fakeadec, GstCaps * caps);
static gboolean gst_fakeadec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstStateChangeReturn gst_fakeadec_change_state (GstElement * element,
    GstStateChange transition);

static GstFlowReturn gst_fakeadec_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

#define gst_fakeadec_parent_class parent_class
G_DEFINE_TYPE (GstFakeAdec, gst_fakeadec, GST_TYPE_ELEMENT);


static gboolean
gst_fakeadec_set_caps (GstFakeAdec * fakeadec, GstCaps * caps)
{
  //GstStructure *structure;
  gboolean ret;
  GstCaps *outcaps;

  //structure = gst_caps_get_structure (caps, 0); 

  //ret = gst_structure_get_int (structure, "rate", &rate);
  //ret &= gst_structure_get_int (structure, "channels", &channels);

  outcaps = gst_caps_copy (caps);
  ret = gst_pad_set_caps (fakeadec->srcpad, outcaps);   //outcaps);
  gst_caps_unref (outcaps);

  return ret;
}

/*
static gboolean
gst_fakeadec_src_query (GstPad * pad, GstObject * parent, GstQuery *query)
{
	gboolean res;
	
	GstFakeAdec *fakeadec = GST_FAKEADEC (gst_pad_get_parent (pad));

	switch (GST_QUERY_TYPE (query)) {
		case GST_QUERY_CAPS:
		{
			GstCaps *sink_caps;
		
			sink_caps = gst_pad_get_current_caps (fakeadec->sinkpad);
			gst_query_set_caps_result (query, sink_caps);
			//FIXME gst_pad_get_current_caps increase reference count, need to unref code.

			res = TRUE;
			break;
		}
		default :
			res = gst_pad_query_default (pad, parent, query);
			break;
	}

	g_object_unref (fakeadec);
	return res;
}
*/
static void
gst_fakeadec_class_init (GstFakeAdecClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_sink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Fake Audio decoder",
      "Codec/Decoder/Audio",
      "Pass data to backend decoder",
      "Wonchul Lee <wonchul86.lee@lge.com>,Justin Joy <justin.joy.9to5@gmail.com>");

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_fakeadec_change_state);

  GST_DEBUG_CATEGORY_INIT (fakeadec_debug, "fakeadec", 0, "Fake audio decoder");
}

static void
gst_fakeadec_init (GstFakeAdec * fakeadec)
{
  fakeadec->sinkpad =
      gst_pad_new_from_static_template (&gst_fakeadec_sink_pad_template,
      "sink");
  gst_pad_set_chain_function (fakeadec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakeadec_chain));
  gst_pad_set_event_function (fakeadec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakeadec_sink_event));
  gst_element_add_pad (GST_ELEMENT (fakeadec), fakeadec->sinkpad);

  fakeadec->srcpad =
      gst_pad_new_from_static_template (&gst_fakeadec_src_pad_template, "src");
  /*gst_pad_set_query_function (fakeadec->srcpad,
     GST_DEBUG_FUNCPTR (gst_fakeadec_src_query)); */
  gst_element_add_pad (GST_ELEMENT (fakeadec), fakeadec->srcpad);

  fakeadec->src_caps_set = FALSE;
}

static gboolean
gst_fakeadec_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstFakeAdec *fakeadec;
  gboolean res;

  fakeadec = GST_FAKEADEC (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      gst_fakeadec_set_caps (fakeadec, caps);
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
gst_fakeadec_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFakeAdec *fakeadec;
  GstMapInfo inmap;
  GstBuffer *outbuf;
  GstFlowReturn ret;

  fakeadec = GST_FAKEADEC (parent);

  GST_LOG_OBJECT (fakeadec, "buffer with ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

  gst_buffer_map (buffer, &inmap, GST_MAP_READ);

  outbuf = gst_buffer_copy (buffer);

  gst_buffer_unmap (buffer, &inmap);
  gst_buffer_unref (buffer);

  ret = gst_pad_push (fakeadec->srcpad, outbuf);

  return ret;

}

static GstStateChangeReturn
gst_fakeadec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  //GstFakeAdec *fakeadec = GST_FAKEADEC (element);

  switch (transition) {
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
