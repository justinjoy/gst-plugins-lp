bin_PROGRAMS = lpbin_test
lpbin_test_SOURCES = lpbin-test.c
lpbin_test_CPPFLAGS = -I$(top_srcdir)/gst/playback -I$(top_builddir)/gst/playback/.libs -I$(top_srcdir)/gst/compat/ -I$(top_builddir)/gst/compat/.libs
lpbin_test_CFLAGS = $(GST_CFLAGS)
lpbin_test_LDADD = 					\
	-L$(top_builddir)/gst/playback/.libs/ -lgstlp	\
	-L$(top_builddir)/gst/compat/.libs/ -lgstcompat	\
	$(GST_LIBS)
