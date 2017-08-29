EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

bin_PROGRAMS = 
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
noinst_PROGRAMS =

include src/Makefile.am
include doc/Makefile.am
if ENABLE_MAN_PAGES
include doc/man/Makefile.am
endif
if ENABLE_HTML_DOCS
include doc/html/Makefile.am
endif
if ENABLE_TESTS
include tests/Makefile.am
endif

if ENABLE_EXAMPLES
include examples/Makefile.am
endif

# for various extra dist
include build/cmake/Makefile.am

ACLOCAL_AMFLAGS = -I build/autotools/m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-flags --enable-silent-rules --enable-optimizations --enable-html-docs --enable-man-pages

bsondocdir = ${docdir}
bsondoc_DATA = \
	COPYING \
	NEWS \
	README \
	THIRD_PARTY_NOTICES

EXTRA_DIST += $(bsondoc_DATA)

EXTRA_DIST += CMakeLists.txt \
    VERSION_CURRENT \
    VERSION_RELEASED

dist-hook:
	@if test -d "$(srcdir)/.git"; then                                   \
	       (cd "$(srcdir)" &&                                            \
	        $(top_srcdir)/build/autotools/missing --run git log --stat ) > ChangeLog.tmp \
	        && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog              \
	        || (rm -f ChangeLog.tmp;                                     \
	            echo Failed to generate ChangeLog >&2);                  \
	else                                                                 \
	  echo A git checkout is required to generate a ChangeLog >&2;       \
	fi

# Make libbson-static-1.0.a symlink to libbson-1.0.a. If libbson is built with
# CMake instead of the Autotools, the static archive is named
# libbson-static-1.0.a by default. For the sake of projects that want to
# statically link to libbson, ensure the static archive is present with the
# name no matter how it was built.

# Also, copy CMake config-file packages to lib/cmake/ for use with CMake's
# "find_package" command.

if ENABLE_STATIC
enable_static=1
endif

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(libdir)/cmake/libbson-1.0
	cp -f build/cmake/libbson-1.0-config.cmake $(DESTDIR)$(libdir)/cmake/libbson-1.0
	cp -f build/cmake/libbson-1.0-config-version.cmake $(DESTDIR)$(libdir)/cmake/libbson-1.0

	if [ ! -z "$(enable_static)" ]; then                                                                  \
	  rm -rf $(DESTDIR)$(libdir)/cmake/libbson-static-1.0;                                                \
	  $(MKDIR_P) $(DESTDIR)$(libdir)/cmake/libbson-static-1.0;                                            \
	  cd $(DESTDIR)$(libdir);                                                                             \
	  rm -f libbson-static-1.0.a;                                                                         \
	  $(LN_S) libbson-1.0.a libbson-static-1.0.a;                                                         \
	  cp -f $(abs_builddir)/build/cmake/libbson-static-1.0-config.cmake cmake/libbson-static-1.0;         \
	  cp -f $(abs_builddir)/build/cmake/libbson-static-1.0-config-version.cmake cmake/libbson-static-1.0; \
	fi

clean-local:
	-rm -rf $(CLEANFILES)

uninstall-local:
	-rm -r $(bsondocdir)
	-rm -rf $(DESTDIR)$(libdir)/libbson-static-1.0.a
	-rm -rf $(DESTDIR)$(libdir)/cmake/libbson-1.0
	-rm -rf $(DESTDIR)$(libdir)/cmake/libbson-static-1.0

