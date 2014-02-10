bin_PROGRAMS = appsrc_stream
appsrc_stream_SOURCES = appsrc-stream.c
appsrc_stream_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(GST_CFLAGS)
appsrc_stream_LDFLAGS = \
  -L$(top_builddir)/gst/playback/.libs/ -lgstlp   \
  -L$(top_builddir)/gst/compat/.libs/ -lgstcompat \
  $(GST_LIBS)
