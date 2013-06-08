/* GStreamer Lightweight Playback Plugins
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


#ifndef __GST_FC_BIN_H__
#define __GST_FC_BIN_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_FC_BIN (gst_fc_bin_get_type())
#define GST_FC_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_FC_BIN,GstFCBin))
#define GST_FC_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FC_BIN,GstFCBinClass))
#define GST_IS_FC_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FC_BIN))
#define GST_IS_FC_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FC_BIN))
typedef struct _GstFCSelect GstFCSelect;
typedef struct _GstFCBin GstFCBin;
typedef struct _GstFCBinClass GstFCBinClass;

enum
{
  GST_FC_BIN_STREAM_AUDIO = 0,
  GST_FC_BIN_STREAM_VIDEO,
  GST_FC_BIN_STREAM_TEXT,
  GST_FC_BIN_STREAM_LAST
};

struct _GstFCSelect
{
  const gchar *media_list[8];   /* the media types for the selector */
  GstElement *selector;         /* the selector */

  GPtrArray *channels;
  GstPad *srcpad;
  //GstPad *sinkpad;
};

struct _GstFCBin
{
  GstBin parent;

  GstFlowReturn ret;

  GstFCSelect select[GST_FC_BIN_STREAM_LAST];
};

struct _GstFCBinClass
{
  GstBinClass parent_class;
};

GType gst_fc_bin_get_type (void);

G_END_DECLS
#endif // __GST_FC_BIN_H__
