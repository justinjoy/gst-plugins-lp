bin_PROGRAMS = dynappsrc_stream
dynappsrc_stream_SOURCES = dynappsrc-stream.c
dynappsrc_stream_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(GST_CFLAGS)
dynappsrc_stream_LDFLAGS = \
  -L$(top_builddir)/gst/playback/.libs/ -lgstlp   \
  -L$(top_builddir)/gst/compat/.libs/ -lgstcompat \
  $(GST_LIBS)
