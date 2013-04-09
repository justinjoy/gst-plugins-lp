#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>

#include "gstfcbin.h"
#include "gstfakevdec.h"
#include "gstfakeadec.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "fakevdec", GST_RANK_PRIMARY,
          GST_TYPE_FAKEVDEC))
    return FALSE;

  if (!gst_element_register (plugin, "fakeadec", GST_RANK_PRIMARY,
          GST_TYPE_FAKEADEC))
    return FALSE;

  if (!gst_element_register (plugin, "fcbin", GST_RANK_PRIMARY,
          GST_TYPE_FC_BIN))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    lpcompat,
    "Lightweight Compatiblity Plugin Library",
    plugin_init, PACKAGE_VERSION, "LGPL", PACKAGE_NAME, PACKAGE_BUGREPORT)
