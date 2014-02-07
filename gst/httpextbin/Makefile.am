plugin_LTLIBRARIES = libgsthttpsrc.la

# sources used to compile this plug-in
libgsthttpsrc_la_SOURCES = gsthttpextbin.c plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgsthttpsrc_la_CFLAGS = $(GST_CFLAGS)
libgsthttpsrc_la_LIBADD = $(GST_LIBS) -lgstvideo-@GST_API_VERSION@ -lgstaudio-@GST_API_VERSION@
libgsthttpsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgsthttpsrc_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gsthttpextbin.h
