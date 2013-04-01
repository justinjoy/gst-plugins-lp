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


#ifndef __GST_LP_BIN_H__
#define __GST_LP_BIN_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_LP_BIN (gst_lp_bin_get_type())
#define GST_LP_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_LP_BIN,GstLpBin))
#define GST_LP_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_LP_BIN,GstLpBinClass))
#define GST_IS_LP_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_LP_BIN))
#define GST_IS_LP_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_LP_BIN))

#define GST_LP_BIN_GET_LOCK(bin) (&((GstLpBin*)(bin))->lock)
#define GST_LP_BIN_LOCK(bin) (g_rec_mutex_lock (GST_LP_BIN_GET_LOCK(bin)))
#define GST_LP_BIN_UNLOCK(bin) (g_rec_mutex_unlock (GST_LP_BIN_GET_LOCK(bin)))

typedef struct _GstLpBin GstLpBin;
typedef struct _GstLpBinClass GstLpBinClass;

struct _GstLpBin
{
  GstPipeline parent;

  GRecMutex lock;               /* to protect group switching */

  GstFlowReturn ret;
};

struct _GstLpBinClass
{
  GstPipelineClass parent_class;
};

GType gst_lp_bin_get_type (void);

G_END_DECLS
#endif // __GST_LP_BIN_H__
