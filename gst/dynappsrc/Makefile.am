plugin_LTLIBRARIES = libgstdynappsrc.la

# sources used to compile this plug-in
libgstdynappsrc_la_SOURCES = gstdynamic.c gstdynappsrc.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstdynappsrc_la_CFLAGS = $(GST_CFLAGS)
libgstdynappsrc_la_LIBADD = $(GST_LIBS) -lgstvideo-@GST_API_VERSION@ -lgstaudio-@GST_API_VERSION@
libgstdynappsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstdynappsrc_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstdynappsrc.h
