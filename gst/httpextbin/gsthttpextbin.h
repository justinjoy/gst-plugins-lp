/* GStreamer httpextbin element
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *  Author : HoonHee Lee <hoonhee.lee@lge.com>
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

#ifndef __GST_HTTP_EXT_BIN_H__
#define __GST_HTTP_EXT_BIN_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_HTTP_EXT_BIN (gst_http_ext_bin_get_type())
#define GST_HTTP_EXT_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_HTTP_EXT_BIN,GstHttpExtBin))
#define GST_HTTP_EXT_BIN_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj),GST_TYPE_HTTP_EXT_BIN,GstHttpExtBinClass))
#define GST_IS_HTTP_EXT_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_HTTP_EXT_BIN))
#define GST_IS_HTTP_EXT_BIN_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((obj),GST_TYPE_HTTP_EXT_BIN))
typedef struct _GstHttpExtBin GstHttpExtBin;
typedef struct _GstHttpExtBinClass GstHttpExtBinClass;

struct _GstHttpExtBin
{
  GstBin parent;

  GstPad *srcpad;

  GstElement *source_elem;
  GstElement *filter_elem;

  gchar *uri;
  GstCaps *caps;

  GList *list;                  /* list we can use for selecting elements */
};

struct _GstHttpExtBinClass
{
  GstBinClass parent_class;
};

GType gst_http_ext_bin_get_type (void);

G_END_DECLS
#endif /* __GST_HTTP_EXT_BIN_H__ */
