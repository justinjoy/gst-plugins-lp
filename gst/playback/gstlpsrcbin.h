/* GStreamer Lightweight PlayBack Plugins
 *
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com>
 *               Jeongseok Kim <jeongseok.kim@lge.com>
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

#ifndef LP_SRC_BIN_H_
#define LP_SRC_BIN_H_

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_LP_SRC_BIN \
  (gst_lp_src_bin_get_type())
#define GST_LP_SRC_BIN(obj) \
 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_LP_SRC_BIN, LPSrcBin))
#define GST_LP_SRC_BIN_CLASS(obj) \
 (G_TYPE_CHECK_CLASS_CAST ((obj), GST_TYPE_LP_SRC_BIN, LPSrcBinClass))
#define GST_IS_LP_SRC_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_LP_SRC_BIN))
#define GST_IS_LP_SRC_BIN_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE ((obj), GST_TYPE_LP_SRC_BIN))
#define GST_GET_LP_SRC_BIN_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_LP_SRC_BIN, LPSrcBinClass))
#define GST_LP_SRC_BIN_GET_LOCK(lpbin) (((LPSrcBin*)(lpbin))->lock)
#define GST_LP_SRC_BIN_LOCK(lpbin) (g_mutex_lock (GST_LP_SRC_BIN_GET_LOCK(lpbin)))
#define GST_LP_SRC_BIN_UNLOCK(lpbin) (g_mutex_unlock (GST_LP_SRC_BIN_GET_LOCK(lpbin)))
typedef struct _LPSrcBinClass LPSrcBinClass;
typedef struct _LPSrcBin LPSrcBin;

struct _LPSrcBin
{
  GstBin parent;

  GstPad *srcpad;
  GstElement *source;

  gboolean inka_drm;
  gchar *uri;
};

struct _LPSrcBinClass
{
  GstBinClass parent_class;
};

GType gst_lp_src_bin_get_type (void);

G_END_DECLS
#endif /* LP_SRC_BIN_H_ */
