include $(top_srcdir)/common/check.mak

CHECK_REGISTRY = $(top_builddir)/tests/check/test-registry.reg
TEST_FILES_DIRECTORY = $(top_srcdir)/tests/files

REGISTRY_ENVIRONMENT = \
	GST_REGISTRY_1_0=$(CHECK_REGISTRY)

TESTS_ENVIRONMENT = \
        CK_DEFAULT_TIMEOUT=120                                  \
        GST_STATE_IGNORE_ELEMENTS="$(STATE_IGNORE_ELEMENTS)"	\
	$(REGISTRY_ENVIRONMENT)					\
	GST_PLUGIN_SYSTEM_PATH_1_0=				\
	GST_PLUGIN_PATH_1_0=$(top_builddir)/gst:$(top_builddir)/sys:$(top_builddir)/ext:$(GST_PLUGINS_GOOD_DIR):$(GST_PLUGINS_BASE_DIR):$(GST_PLUGINS_DIR) \
	GST_PLUGIN_LOADING_WHITELIST="gstreamer@$(GST_PLUGINS_DIR):gst-plugins-base@$(GSTPB_PLUGINS_DIR):gst-plugins-good:gst-plugins-lp@$(top_builddir)" \
	GST_TAG_LICENSE_TRANSLATIONS_DICT="$(top_srcdir)/gst-libs/gst/tag/license-translations.dict"

# ths core dumps of some machines have PIDs appended
CLEANFILES = core.* test-registry.*

clean-local: clean-local-check

$(CHECK_REGISTRY):
	$(TESTS_ENVIRONMENT)

# elements to ignore for the state tests
STATE_IGNORE_ELEMENTS =

TESTS = $(check_PROGRAMS)

check_PROGRAMS = \
	elements/dynappsrc \
	elements/httpextbin \
	elements/streamiddemux \
	elements/lpbin

# these tests don't even pass
noinst_PROGRAMS =

noinst_HEADERS =

AM_CFLAGS = -I$(top_srcdir)/gst-libs -I$(top_builddir)/gst-libs \
	$(GST_CFLAGS) $(GST_CHECK_CFLAGS) \
	-DGST_TEST_FILES_PATH="\"$(TEST_FILES_DIRECTORY)\"" \
	-UG_DISABLE_ASSERT -UG_DISABLE_CAST_CHECKS

AM_CXXFLAGS = -I$(top_srcdir)/gst-libs -I$(top_builddir)/gst-libs \
	$(GST_CXXFLAGS) $(GST_CHECK_CFLAGS) \
	-DGST_TEST_FILES_PATH="\"$(TEST_FILES_DIRECTORY)\"" \
	-UG_DISABLE_ASSERT -UG_DISABLE_CAST_CHECKS

LDADD = $(GST_LIBS) $(GST_CHECK_LIBS)

# valgrind testing
VALGRIND_TESTS_DISABLE = $(VALGRIND_TO_FIX)

elements_dynappsrc_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(AM_CFLAGS)

elements_dynappsrc_LDADD = \
	$(LDADD)

elements_streamiddemux_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(AM_CFLAGS)

elements_streamiddemux_LDADD = \
	$(LDADD)

elements_lpbin_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(AM_CFLAGS)

elements_lpbin_LDADD = \
	$(LDADD)

elements_httpextbin_CFLAGS = \
        $(GST_PLUGINS_BASE_CFLAGS) \
        $(AM_CFLAGS)

elements_httpextbin_LDADD = \
        $(LDADD)

