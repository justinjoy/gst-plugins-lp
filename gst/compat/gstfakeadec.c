/* GStreamer Lightweight Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Wonchul86 Lee <wonchul86.lee@lge.com> 
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

#include <stdlib.h>
#include <string.h>
#include <gst/audio/audio.h>

GST_DEBUG_CATEGORY_STATIC (fakeadec_debug);
#define GST_CAT_DEFAULT fakeadec_debug

static gboolean gst_fakeadec_reset (GstBaseParse * decoder, gboolean hard);
static gboolean gst_fakeadec_start (GstBaseParse * decoder);
static gboolean gst_fakeadec_stop (GstBaseParse * decoder);
static GstFlowReturn gst_fakeadec_handle_frame (GstBaseParse * decoder,
GstBaseParseFrame * frame, gint * skipsize);

#define parent_class gst_fakeadec_parent_class
G_DEFINE_TYPE (GstFakeAdec, gst_fakeadec, GST_TYPE_BASE_PARSE);

static GstStaticPadTemplate gst_fakeadec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (FD_AUDIO_CAPS)
    );

static GstStaticPadTemplate gst_fakeadec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-fd")
    );

static void
gst_fakeadec_class_init (GstFakeAdecClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseParseClass *adec_class = (GstBaseParseClass *) klass;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_sink_pad_template));
  gst_element_class_set_static_metadata (element_class, "Fake Audio decoder",
      "Codec/Decoder/Audio",
      "Pass data to backend decoder",
      "Wonchul86 Lee <wonchul86.lee@lge.com>,Justin Joy <justin.joy.9to5@gmail.com>");

  //adec_class->start = gst_fakeadec_start;
  //adec_class->stop = gst_fakeadec_stop;
  //adec_class->reset = gst_fakeadec_reset;
  adec_class->handle_frame = gst_fakeadec_handle_frame;

  GST_DEBUG_CATEGORY_INIT (fakeadec_debug, "fakeadec", 0, "Fake audio decoder");
}

static void
gst_fakeadec_init (GstFakeAdec * fakeadec)
{
  GST_DEBUG_OBJECT (fakeadec, "initializing");
}

static gboolean
gst_fakeadec_start (GstBaseParse * decoder)
{
  GstFakeAdec *fakeadec = (GstFakeAdec *) decoder;

  GST_DEBUG_OBJECT (fakeadec, "starting");
  return TRUE;
}

static gboolean
gst_fakeadec_stop (GstBaseParse * decoder)
{
  GstFakeAdec *fakeadec = (GstFakeAdec *) decoder;

  GST_DEBUG_OBJECT (fakeadec, "stopping");
  return TRUE;
}

static gboolean
gst_fakeadec_reset (GstBaseParse * decoder, gboolean hard)
{
  return TRUE;
}

static GstFlowReturn
gst_fakeadec_handle_frame (GstBaseParse * decoder, GstBaseParseFrame * frame, gint * skipsize)
{
  GstFakeAdec *fakeadec = (GstFakeAdec *) decoder;
  GstFlowReturn ret = GST_FLOW_OK;
	gsize size;
	GstMapInfo map;
	GstCaps *sink_caps;
	GstCaps *target_caps;

	sink_caps = gst_pad_get_current_caps (GST_BASE_PARSE_SINK_PAD(decoder));
	target_caps = gst_caps_copy (sink_caps);
	gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD(decoder), target_caps);

	gst_buffer_map (frame->buffer, &map, GST_MAP_READ);
	size = map.size;
	gst_buffer_unmap (frame->buffer, &map);
	frame->out_buffer = gst_buffer_copy (frame->buffer);
	ret = gst_base_parse_finish_frame (decoder, frame, size);
  return ret;
}
