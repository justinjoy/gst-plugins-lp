DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
SUBDIRS = gst m4 common tests

DIST_SUBDIRS = gst m4 common tests

include $(top_srcdir)/common/win32.mak

EXTRA_DIST = autogen.sh

DISTCLEANFILES = _stdint.h

ACLOCAL_AMFLAGS = -I m4 -I common/m4

include $(top_srcdir)/common/release.mak
include $(top_srcdir)/common/po.mak

include $(top_srcdir)/common/coverage/lcov.mak
