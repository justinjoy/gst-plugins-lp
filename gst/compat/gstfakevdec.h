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


#ifndef __GST_FAKEVDEC_H__
#define __GST_FAKEVDEC_H__

#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>

G_BEGIN_DECLS

#define GST_TYPE_FAKEVDEC (gst_fakevdec_get_type())
#define GST_FAKEVDEC(obj) (G_TYPE_CHECL_INSTANCE_CAST((obj),GST_TYPE_FAKEVDEC,GstFakeVdec))
#define GST_FAKEVDEC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FAKEVDEC,GstFakeVdecClass))
#define GST_IS_FAKEVDEC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FAKEVDEC))
#define GST_IS_FAKEVDEC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FAKEVDEC))

typedef struct _GstFakeVdec GstFakeVdec;
typedef struct _GstFakeVdecClass GstFakeVdecClass;

struct _GstFakeVdec
{
  GstVideoDecoder parent;

  GstVideoCodecState *input_state;
  GstVideoCodecState *output_state;
  GstMapInfo current_frame_map;
  GstVideoCodecFrame *current_frame;

  GstFlowReturn ret;
};

struct _GstFakeVdecClass
{
  GstVideoDecoderClass parent_class;
};

GType gst_fakevdec_get_type (void);

G_END_DECLS
#endif // __GST_FAKEVDEC_H__
