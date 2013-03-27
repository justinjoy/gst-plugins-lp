#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>

#include "gstfakevdec.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "fakevdec", GST_RANK_PRIMARY,
          GST_TYPE_FAKEVDEC))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    lpcompat,
    "Lightweight Compatiblity Plugin Library",
    plugin_init, PACKAGE_VERSION, "LGPL", PACKAGE_NAME, PACKAGE_BUGREPORT)
