/* GStreamer Lightweight Playback Plugins
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com>
 *	         Jeongseok Kim <jeongseok.kim@lge.com>
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

#ifndef __GST_FAKEADEC_H__
#define __GST_FAKEADEC_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>

G_BEGIN_DECLS
#define GST_TYPE_FAKEADEC \
  (gst_fakeadec_get_type())
#define GST_FAKEADEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_FAKEADEC,GstFakeAdec))
#define GST_FAKEADEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FAKEADEC,GstFakeAdecClass))
#define GST_IS_FAKEADEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FAKEADEC))
#define GST_IS_FAKEADEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FAKEADEC))
typedef struct _GstFakeAdec GstFakeAdec;
typedef struct _GstFakeAdecClass GstFakeAdecClass;

struct _GstFakeAdec
{
  GstElement element;

  GstPad *sinkpad, *srcpad;
  gboolean src_caps_set;
};

struct _GstFakeAdecClass
{
  GstElementClass parent_class;
};

GType gst_fakeadec_get_type (void);

G_END_DECLS
#endif /* __GST_FAKEADEC_H__ */
