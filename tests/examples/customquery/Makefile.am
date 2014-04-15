bin_PROGRAMS = custom_query
custom_query_SOURCES = custom-query.c
custom_query_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(GST_CFLAGS)
custom_query_LDFLAGS = \
  -L$(top_builddir)/gst/playback/.libs/ -lgstlp   \
  -L$(top_builddir)/gst/compat/.libs/ -lgstcompat \
  $(GST_LIBS)
