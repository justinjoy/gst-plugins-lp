/* GStreamer Dynamic App Source element
 * Copyright (C) 2014 LG Electronics, Inc.
 *  Author : Wonchul Lee <wonchul86.lee@lge.com>
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

#ifndef __GST_DYN_APPSRC_H__
#define __GST_DYN_APPSRC_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DYN_APPSRC (gst_dyn_appsrc_get_type())
#define GST_DYN_APPSRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DYN_APPSRC,GstDynAppSrc))
#define GST_DYN_APPSRC_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj),GST_TYPE_DYN_APPSRC,GstDynAppSrcClass))
#define GST_IS_DYN_APPSRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DYN_APPSRC))
#define GST_IS_DYN_APPSRC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((obj),GST_TYPE_DYN_APPSRC))

typedef struct _GstDynAppSrc      GstDynAppSrc;
typedef struct _GstDynAppSrcClass GstDynAppSrcClass;
typedef struct _GstAppSourceGroup    GstAppSourceGroup;

/**
 * GstDynAppSrc:
 *
 * dynappsrc element data structure
 */
struct _GstDynAppSrc
{
  GstBin parent;

  GstPad* srcpad;
  gchar* uri;

  GList *appsrc_list;

  gint n_source;
};

struct _GstDynAppSrcClass
{
  GstBinClass parent_class;

  /* create a appsrc element */
  GstElement *(*new_appsrc) (GstDynAppSrc * dynappsrc, const gchar * name);
};

struct _GstAppSourceGroup
{
  GstElement *appsrc;
  GstPad *srcpad;
};

GType gst_dyn_appsrc_get_type (void);

G_END_DECLS
#endif /* __GST_DYN_APPSRC_H__ */
