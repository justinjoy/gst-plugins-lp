/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013 LG Electronics.
 * Author : HoonHee Lee <hoonhee.lee@lge.com>
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

#ifndef __GST_REVERSE_FUNNEL_H__
#define __GST_REVERSE_FUNNEL_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_REVERSE_FUNNEL \
  (gst_reverse_funnel_get_type())
#define GST_REVERSE_FUNNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_REVERSE_FUNNEL, GstReverseFunnel))
#define GST_REVERSE_FUNNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_REVERSE_FUNNEL, GstReverseFunnelClass))
#define GST_IS_REVERSE_FUNNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_REVERSE_FUNNEL))
#define GST_IS_REVERSE_FUNNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_REVERSE_FUNNEL))
typedef struct _GstReverseFunnel GstReverseFunnel;
typedef struct _GstReverseFunnelClass GstReverseFunnelClass;

struct _GstReverseFunnel
{
  GstElement element;

  GstPad *sinkpad;

  guint nb_srcpads;

  /* This table contains srcpad and stream-id */
  GHashTable *stream_id_pairs;
};

struct _GstReverseFunnelClass
{
  GstElementClass parent_class;

  void (*src_pad_added) (GstElement * element, GstPad * pad);
};

G_GNUC_INTERNAL GType gst_reverse_funnel_get_type (void);

G_END_DECLS
#endif /* __GST_REVERSE_FUNNEL_H__ */
