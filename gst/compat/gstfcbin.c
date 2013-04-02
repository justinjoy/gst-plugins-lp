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

#include "gstfcbin.h"

#include <stdlib.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (fc_bin_debug);
#define GST_CAT_DEFAULT fc_bin_debug

#define parent_class gst_fc_bin_parent_class
G_DEFINE_TYPE (GstFCBin, gst_fc_bin, GST_TYPE_BIN);

static void gst_fc_bin_finalize (GObject * obj);

static GstStaticPadTemplate gst_fc_bin_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static void
gst_fc_bin_class_init (GstFCBinClass * klass)
{
  GObjectClass *gobject_klass;
  GstBinClass *gstbin_klass;
  GstElementClass *element_class = (GstElementClass *) klass;

  gobject_klass = (GObjectClass *) klass;
  gstbin_klass = (GstBinClass *) klass;

  gobject_klass->finalize = gst_fc_bin_finalize;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_sink_pad_template));
  gst_element_class_set_static_metadata (element_class, "Flow Controller",
      "Lightweight/Controller/Flow",
      "data flow controller behind Fake Decoder",
      "Justin Joy <justin.joy.9to5@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (fc_bin_debug, "fcbin", 0, "Flow Controller");
}

static void
gst_fc_bin_init (GstFCBin * fcbin)
{
  GST_DEBUG_OBJECT (fcbin, "initializing");
}

static void
gst_fc_bin_finalize (GObject * obj)
{
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}
