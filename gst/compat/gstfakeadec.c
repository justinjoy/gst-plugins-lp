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
static GstAudioInfo *ainfo;

static gboolean gst_fakeadec_reset (GstAudioDecoder * decoder, gboolean hard);
static gboolean gst_fakeadec_start (GstAudioDecoder * decoder);
static gboolean gst_fakeadec_stop (GstAudioDecoder * decoder);
static gboolean gst_fakeadec_set_format (GstAudioDecoder * Decoder,
    GstCaps * caps);
static GstFlowReturn gst_fakeadec_handle_frame (GstAudioDecoder * decoder,
    GstBuffer * buffer);

#define parent_class gst_fakeadec_parent_class
G_DEFINE_TYPE (GstFakeAdec, gst_fakeadec, GST_TYPE_AUDIO_DECODER);

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
  GstAudioDecoderClass *adec_class = (GstAudioDecoderClass *) klass;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_sink_pad_template));
  gst_element_class_set_static_metadata (element_class, "Fake Audio decoder",
      "Codec/Decoder/Audio",
      "Pass data to backend decoder",
      "Wonchul86 Lee <wonchul86.lee@lge.com>,Justin Joy <justin.joy.9to5@gmail.com>");

//  adec_class->start = gst_fakeadec_start;
//  adec_class->stop = gst_fakeadec_stop;
  //adec_class->reset = gst_fakeadec_reset;
  adec_class->set_format = gst_fakeadec_set_format;
  adec_class->handle_frame = gst_fakeadec_handle_frame;

  GST_DEBUG_CATEGORY_INIT (fakeadec_debug, "fakeadec", 0, "Fake audio decoder");
}

static void
gst_fakeadec_init (GstFakeAdec * fakeadec)
{
  GST_DEBUG_OBJECT (fakeadec, "initializing");
}

static gboolean
gst_fakeadec_start (GstAudioDecoder * decoder)
{
  GstFakeAdec *fakeadec = (GstFakeAdec *) decoder;

  GST_DEBUG_OBJECT (fakeadec, "starting");
  return TRUE;
}

static gboolean
gst_fakeadec_stop (GstAudioDecoder * decoder)
{
  GstFakeAdec *fakeadec = (GstFakeAdec *) decoder;

  GST_DEBUG_OBJECT (fakeadec, "stopping");
  return TRUE;
}

static gboolean
gst_fakeadec_reset (GstAudioDecoder * decoder, gboolean hard)
{
  return TRUE;
}

static gboolean
gst_fakeadec_set_format (GstAudioDecoder * decoder, GstCaps * in_caps)
{
  GstFakeAdec *fakeadec = (GstFakeAdec *) decoder;
  GstCaps *target_caps;
  GstAudioInfo info;

  GST_DEBUG_OBJECT (in_caps, "getting state caps");
  target_caps = gst_caps_copy (in_caps); 

  gst_caps_make_writable (target_caps);
  gst_caps_set_simple (target_caps, "passed_fakedecoder", G_TYPE_BOOLEAN, TRUE, NULL); 
//  gst_caps_replace (decoder->srcpad, target_caps);
  gst_pad_set_caps(decoder->srcpad, target_caps);

  gst_audio_info_init (&info);                                        
  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, 44100, 2, NULL);
                                                                      
  gst_audio_decoder_set_output_format (decoder, &info);                  
  return TRUE;
}

static GstFlowReturn
gst_fakeadec_handle_frame (GstAudioDecoder * decoder, GstBuffer * buffer)
{
  GstFakeAdec *fakeadec = (GstFakeAdec *) decoder;
  GstFlowReturn ret = GST_FLOW_OK;
  GstBuffer *copied_buffer;

  GST_LOG_OBJECT (fakeadec,
      "Received new data of size %u", 
      gst_buffer_get_size (buffer));

//  copied_buffer = gst_buffer_copy(buffer);
//  ret = gst_audio_decoder_finish_frame (decoder, copied_buffer, 1);
  return ret;
}
