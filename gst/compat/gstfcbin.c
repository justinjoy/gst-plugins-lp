/* GStreamer Lightweight Plugins
 * Copyright (C) 2013 LG Electronics.
 *	Author : Justin Joy <justin.joy.9to5@gmail.com> 
 *	         Wonchul Lee <wonchul86.lee@lge.com> 
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

#include <gst/pbutils/missing-plugins.h>
#include <stdlib.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (fc_bin_debug);
#define GST_CAT_DEFAULT fc_bin_debug

#define parent_class gst_fc_bin_parent_class
G_DEFINE_TYPE (GstFCBin, gst_fc_bin, GST_TYPE_BIN);

static void gst_fc_bin_finalize (GObject * obj);
static GstPad *gst_fc_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static gboolean array_has_value (const gchar * values[], const gchar * value);

static GstStaticPadTemplate gst_fc_bin_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_vsink_pad_template =
GST_STATIC_PAD_TEMPLATE ("video_sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_vsrc_pad_template =
GST_STATIC_PAD_TEMPLATE ("video_src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_asink_pad_template =
GST_STATIC_PAD_TEMPLATE ("audio_sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_asrc_pad_template =
GST_STATIC_PAD_TEMPLATE ("audio_src%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_tsink_pad_template =
GST_STATIC_PAD_TEMPLATE ("text_sink%u", GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_fc_bin_tsrc_pad_template =
GST_STATIC_PAD_TEMPLATE ("text_src%u",
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
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_asrc_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_asink_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_vsrc_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_vsink_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_tsrc_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fc_bin_tsink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Flow Controller",
      "Lightweight/Controller/Flow",
      "data flow controller behind Fake Decoder",
      "Justin Joy <justin.joy.9to5@gmail.com, Wonchul Lee <wonchul86.lee@lge.com>");

  //element_class->change_state = GST_DEBUG_FUNCPTR (gst_fc_bin_change_state);
  element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_fc_bin_request_new_pad);

  GST_DEBUG_CATEGORY_INIT (fc_bin_debug, "fcbin", 0, "Flow Controller");
}

static void
gst_fc_bin_init (GstFCBin * fcbin)
{
  GST_DEBUG_OBJECT (fcbin, "initializing");

  fcbin->select[GST_FC_BIN_STREAM_AUDIO].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_AUDIO].media_list[0] = "audio/";
  fcbin->select[GST_FC_BIN_STREAM_VIDEO].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_VIDEO].media_list[0] = "video/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].channels = g_ptr_array_new ();
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[0] = "text/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[1] =
      "application/x-subtitle";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[2] = "application/x-ssa";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[3] = "application/x-ass";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[4] = "subpicture/x-dvd";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[5] = "subpicture/";
  fcbin->select[GST_FC_BIN_STREAM_TEXT].media_list[6] = "subtitle/";
}



static void
gst_fc_bin_finalize (GObject * obj)
{
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static GstPad *
gst_fc_bin_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name, const GstCaps * caps)
{
  GstFCBin *fcbin;
  const GstStructure *s;
  const gchar *in_name;
  gint i, j;
  GstFCSelect *selected;
  GstPad *sinkpad, *ghost_sinkpad;
  GstCaps *in_caps;


  fcbin = GST_FC_BIN (element);

  s = gst_caps_get_structure (caps, 0);
  in_name = gst_structure_get_name (s);

  for (i = 0; i < GST_FC_BIN_STREAM_LAST; i++) {
    for (j = 0; fcbin->select[i].media_list[j]; j++) {
      if (array_has_value (fcbin->select[i].media_list, in_name)) {
        selected = &fcbin->select[i];
        break;
      }
    }
  }

  if (selected == NULL)
    goto unknown_type;

  if (selected->selector == NULL) {
    selected->selector = gst_element_factory_make ("input-selector", NULL);

    if (selected->selector == NULL) {
      /* gst_element_post_message (GST_ELEMENT_CAST (fcbin), 
         gst_missing_element_message_new (GST_ELEMENT_CAST (fcbin),
         "input-selector")); */
    } else {

      g_object_set (selected->selector, "sync-streams", TRUE, NULL);

      gst_bin_add (GST_BIN_CAST (fcbin), selected->selector);
      gst_element_set_state (selected->selector, GST_STATE_PAUSED);
    }
  }

  if (selected->srcpad == NULL) {
    if (selected->selector) {
      GstPad *sel_srcpad;

      sel_srcpad = gst_element_get_static_pad (selected->selector, "src");
      selected->srcpad = gst_ghost_pad_new (NULL, sel_srcpad);
      gst_pad_set_active (selected->srcpad, TRUE);
      gst_element_add_pad (element, selected->srcpad);
    }
  }

  if (selected->selector) {
    gchar *pad_name = g_strdup_printf ("sink_%d", selected->channels->len);
    if ((sinkpad = gst_element_get_request_pad (selected->selector, pad_name))) {
      g_object_set_data (G_OBJECT (sinkpad), "fcbin.select", selected);

      ghost_sinkpad = gst_ghost_pad_new (NULL, sinkpad);
      gst_pad_set_active (ghost_sinkpad, TRUE);
      gst_element_add_pad (element, ghost_sinkpad);
      g_object_set_data (G_OBJECT (ghost_sinkpad), "fcbin.srcpad",
          selected->srcpad);

      g_ptr_array_add (selected->channels, sinkpad);
    }
  }

unknown_type:
  GST_ERROR_OBJECT (fcbin, "unknown type for pad %s", name);

done:
  return ghost_sinkpad;
}

//FIXME need to implement resource deallocation code.
/*
static void
gst_fc_bin_release_pad (GstElement * element, GstPad *pad)
{
	GstFCBin * fcbin;
	GstFCSelect *selected;
	GstPad *sinkpad;
	GstFCSelect *select;

	fcbin = GST_FC_BIN (element);

	sinkpad = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad));
	select = g_object_get_data (G_OBJECT(sinkpad), "fcbin.select");
	
	//gst_element_release_request_pad (selected->selector, ); //FIXME 
	
}
*/
static gboolean
array_has_value (const gchar * values[], const gchar * value)
{
  gint i;

  for (i = 0; values[i]; i++) {
    if (g_str_has_prefix (value, values[i]))
      return TRUE;
  }
  return FALSE;
}
