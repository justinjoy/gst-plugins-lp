bin_PROGRAMS = lpbin_test autoplug_test thumbnail_mode_test caps_video_test
lpbin_test_SOURCES = lpbin-test.c
lpbin_test_CPPFLAGS = -I$(top_srcdir)/gst/playback -I$(top_builddir)/gst/playback/.libs -I$(top_srcdir)/gst/compat/ -I$(top_builddir)/gst/compat/.libs
lpbin_test_CFLAGS = $(GST_CFLAGS)
lpbin_test_LDADD = 					\
	-L$(top_builddir)/gst/playback/.libs/ -lgstlp	\
	-L$(top_builddir)/gst/compat/.libs/ -lgstcompat	\
	$(GST_LIBS)

autoplug_test_SOURCES = autoplug-test.c
autoplug_test_CPPFLAGS = -I$(top_srcdir)/gst/playback -I$(top_builddir)/gst/playback/.libs -I$(top_srcdir)/gst/compat/ -I$(top_builddir)/gst/compat/.libs
autoplug_test_CFLAGS = $(GST_CFLAGS)
autoplug_test_LDADD =                                   \
        -L$(top_builddir)/gst/playback/.libs/ -lgstlp   \
        -L$(top_builddir)/gst/compat/.libs/ -lgstcompat \
        $(GST_LIBS)

thumbnail_mode_test_SOURCES = thumbnail-mode-test.c
thumbnail_mode_test_CPPFLAGS = -I$(top_srcdir)/gst/playback -I$(top_builddir)/gst/playback/.libs -I$(top_srcdir)/gst/compat/ -I$(top_builddir)/gst/compat/.libs
thumbnail_mode_test_CFLAGS = $(GST_CFLAGS)
thumbnail_mode_test_LDADD =                             \
        -L$(top_builddir)/gst/playback/.libs/ -lgstlp   \
        -L$(top_builddir)/gst/compat/.libs/ -lgstcompat \
        $(GST_LIBS)

caps_video_test_SOURCES = caps-video-test.c
caps_video_test_CPPFLAGS = -I$(top_srcdir)/gst/playback -I$(top_builddir)/gst/playback/.libs -I$(top_srcdir)/gst/compat/ -I$(top_builddir)/gst/compat/.libs
caps_video_test_CFLAGS = $(GST_CFLAGS)
caps_video_test_LDADD =                             \
        -L$(top_builddir)/gst/playback/.libs/ -lgstlp   \
        -L$(top_builddir)/gst/compat/.libs/ -lgstcompat \
        $(GST_LIBS)
