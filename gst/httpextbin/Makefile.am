plugin_LTLIBRARIES = libgsthttpextbin.la

# sources used to compile this plug-in
libgsthttpextbin_la_SOURCES = gsthttpextbin.c plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgsthttpextbin_la_CFLAGS = $(GST_CFLAGS)
libgsthttpextbin_la_LIBADD = $(GST_LIBS) -lgstvideo-@GST_API_VERSION@ -lgstaudio-@GST_API_VERSION@
libgsthttpextbin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgsthttpextbin_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gsthttpextbin.h
