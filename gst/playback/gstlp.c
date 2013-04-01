#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>

#include "gstlpbin.h"
#include "gstlpsink.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "lpbin", GST_RANK_PRIMARY,
          GST_TYPE_LP_BIN))
    return FALSE;

  if (!gst_element_register (plugin, "lpsink", GST_RANK_PRIMARY,
          GST_TYPE_LP_SINK))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    lp,
    "Lightweight Playback Plugin Library",
    plugin_init, PACKAGE_VERSION, "LGPL", PACKAGE_NAME, PACKAGE_BUGREPORT)
