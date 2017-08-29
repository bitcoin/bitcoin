lib_LTLIBRARIES += libbson-1.0.la

libbson_1_0_la_SOURCES =
libbson_1_0_la_CPPFLAGS = "-DJSONSL_PARSE_NAN -DBSON_STATIC"
libbson_1_0_la_LIBADD = \
	libbson.la
libbson_1_0_la_LDFLAGS = \
	$(OPTIMIZE_LDFLAGS) \
	$(COVERAGE_LDFLAGS) \
	-no-undefined \
	-rpath $(libdir)

include src/bson/Makefile.am
include src/jsonsl/Makefile.am

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/src/libbson-1.0.pc

if ENABLE_STATIC
pkgconfig_DATA += $(top_builddir)/src/libbson-static-1.0.pc
endif

EXTRA_DIST += \
	src/libbson-1.0.pc.in \
	src/libbson-static-1.0.pc.in
