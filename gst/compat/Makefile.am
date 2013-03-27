plugin_LTLIBRARIES = libgstcompat.la

# sources used to compile this plug-in
libgstplugin_la_SOURCES = gstfakevdec.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstplugin_la_CFLAGS = $(GST_CFLAGS)
libgstplugin_la_LIBADD = $(GST_LIBS)
libgstplugin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstplugin_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstfakevdec.h
