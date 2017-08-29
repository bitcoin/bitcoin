lib_LTLIBRARIES += libmongoc-1.0.la

libmongoc_1_0_la_SOURCES =
libmongoc_1_0_la_LIBADD = \
        libmongoc.la

libmongoc_1_0_la_LDFLAGS = \
	$(OPTIMIZE_LDFLAGS) \
	$(COVERAGE_LDFLAGS) \
        -fvisibility=hidden \
        -no-undefined \
        -rpath $(libdir)


include src/mongoc/Makefile.am
include src/tools/Makefile.am

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/src/libmongoc-1.0.pc
if ENABLE_SSL
  # Deprecated alias for libmongoc-1.0.pc, see CDRIVER-2086
  pkgconfig_DATA += $(top_builddir)/src/libmongoc-ssl-1.0.pc
endif
if ENABLE_STATIC
  pkgconfig_DATA += $(top_builddir)/src/libmongoc-static-1.0.pc
endif


EXTRA_DIST += \
	src/libmongoc-1.0.pc.in \
	src/libmongoc-ssl-1.0.pc.in \
	src/libmongoc-static-1.0.pc.in
