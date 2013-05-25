plugin_LTLIBRARIES = libgstlp.la

# sources used to compile this plug-in
libgstlp_la_SOURCES = gstlp.c gstlpbin.c gstlpsink.c gstlpsrcbin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstlp_la_CFLAGS = $(GST_CFLAGS)
libgstlp_la_LIBADD = $(GST_LIBS) 
libgstlp_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstlp_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS =  gstlpbin.h gstlpsink.h gstlpsrcbin.h
