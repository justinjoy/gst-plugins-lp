/* GStreamer Lightweight Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Justin Joy <justin.joy.9to5@gmail.com> 
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

#include <stdlib.h>
#include <string.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

GST_DEBUG_CATEGORY_STATIC (fakevdec_debug);
#define GST_CAT_DEFAULT fakevdec_debug

static gboolean gst_fakevdec_reset (GstVideoDecoder * decoder, gboolean hard);
static gboolean gst_fakevdec_start (GstVideoDecoder * decoder);
static gboolean gst_fakevdec_stop (GstVideoDecoder * decoder);
static gboolean gst_fakevdec_set_format (GstVideoDecoder * Decoder,
    GstVideoCodecState * state);
static GstFlowReturn gst_fakevdec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame);
static gboolean gst_fakevdec_decide_allocation (GstVideoDecoder * decoder,
    GstQuery * query);

#define parent_class gst_fakevdec_parent_class
G_DEFINE_TYPE (GstFakeVdec, gst_fakevdec, GST_TYPE_VIDEO_DECODER);

static GstStaticPadTemplate gst_fakevdec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264")
    );

static GstStaticPadTemplate gst_fakevdec_src_pad_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void
gst_fakevdec_class_init (GstFakeVdecClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GstVideoDecoderClass *vdec_class = (GstVideoDecoderClass *) klass;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_sink_pad_template));
  gst_element_class_set_static_metadata (element_class, "Fake Video decoder",
      "Codec/Decoder/Video",
      "Pass data to backend decoder",
      "Justin Joy <justin.joy.9to5@gmail.com>");

  vdec_class->start = gst_fakevdec_start;
  vdec_class->stop = gst_fakevdec_stop;
  vdec_class->reset = gst_fakevdec_reset;
  vdec_class->set_format = gst_fakevdec_set_format;
  vdec_class->handle_frame = gst_fakevdec_handle_frame;
  vdec_class->decide_allocation = gst_fakevdec_decide_allocation;

  GST_DEBUG_CATEGORY_INIT (fakevdec_debug, "fakevdec", 0, "Fake video decoder");
}

static void
gst_fakevdec_init (GstFakeVdec * fakevdec)
{
  GST_DEBUG_OBJECT (fakevdec, "initializing");
}

static gboolean
gst_fakevdec_start (GstVideoDecoder * decoder)
{
  GstFakeVdec *fakevdec = (GstFakeVdec *) decoder;

  GST_DEBUG_OBJECT (fakevdec, "starting");
  return TRUE;
}

static gboolean
gst_fakevdec_stop (GstVideoDecoder * decoder)
{
  GstFakeVdec *fakevdec = (GstFakeVdec *) decoder;

  GST_DEBUG_OBJECT (fakevdec, "stopping");
  return TRUE;
}

static gboolean
gst_fakevdec_reset (GstVideoDecoder * decoder, gboolean hard)
{
  return TRUE;
}

static gboolean
gst_fakevdec_set_format (GstVideoDecoder * decoder, GstVideoCodecState * state)
{
  GstFakeVdec *fakevdec = (GstFakeVdec *) decoder;

  GST_DEBUG_OBJECT (fakevdec, "setcaps called");

  GST_DEBUG_OBJECT (state->caps, "getting state caps");
  gst_pad_set_caps(decoder->srcpad, state->caps);


  return TRUE;
}

static GstFlowReturn
gst_fakevdec_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  GstFakeVdec *fakevdec = (GstFakeVdec *) decoder;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_LOG_OBJECT (fakevdec,
      "Received new data of size %u, dts %" GST_TIME_FORMAT ", pts:%"
      GST_TIME_FORMAT ", dur:%" GST_TIME_FORMAT,
      gst_buffer_get_size (frame->input_buffer),
      GST_TIME_ARGS (frame->dts),
      GST_TIME_ARGS (frame->pts), GST_TIME_ARGS (frame->duration));


  return ret;
}

static gboolean
gst_fakevdec_decide_allocation (GstVideoDecoder * bdec, GstQuery * query)
{
  GstBufferPool *pool = NULL;
  GstStructure *config;

  if (!GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (bdec, query))
    return FALSE;

  if (gst_query_get_n_allocation_pools (query) > 0)
    gst_query_parse_nth_allocation_pool (query, 0, &pool, NULL, NULL, NULL);

  if (pool == NULL)
    return FALSE;

  config = gst_buffer_pool_get_config (pool);
  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }
  gst_buffer_pool_set_config (pool, config);
  gst_object_unref (pool);

  return TRUE;
}


