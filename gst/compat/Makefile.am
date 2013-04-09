plugin_LTLIBRARIES = libgstcompat.la

# sources used to compile this plug-in
libgstcompat_la_SOURCES = gstcompat.c gstfcbin.c gstfakevdec.c gstfakeadec.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstcompat_la_CFLAGS = $(GST_CFLAGS)
libgstcompat_la_LIBADD = $(GST_LIBS) -lgstvideo-@GST_API_VERSION@ -lgstaudio-@GST_API_VERSION@
libgstcompat_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstcompat_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstfakevdec.h
