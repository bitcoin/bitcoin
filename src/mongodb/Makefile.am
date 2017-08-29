EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

bin_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
noinst_PROGRAMS =
noinst_HEADERS =

SUBDIRS =

if WITH_LIBBSON
SUBDIRS += src/libbson
endif

AM_CTAGSFLAGS = --fields=+l --languages=c

include doc/Makefile.am
if ENABLE_MAN_PAGES
include doc/man/Makefile.am
endif
if ENABLE_HTML_DOCS
include doc/html/Makefile.am
endif
include src/Makefile.am


# for EXTRA_DIST
include build/Makefile.am

ACLOCAL_AMFLAGS = -I build/autotools/m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-silent-rules --enable-man-pages --enable-html-docs --enable-sasl --enable-ssl --enable-maintainer-flags --enable-debug --with-libbson=bundled --with-snappy=bundled --with-zlib=bundled

mongocdocdir = ${docdir}
mongocdoc_DATA = \
	COPYING \
	NEWS \
	README.rst \
	THIRD_PARTY_NOTICES

EXTRA_DIST += $(mongocdoc_DATA)

EXTRA_DIST += \
	CMakeLists.txt \
	CONTRIBUTING.md \
	VERSION_CURRENT \
	VERSION_RELEASED

EXTRA_DIST += $(wildcard orchestration_configs/*/*)

dist-hook:
	-rm -rf $(distdir)/doc/html/.buildinfo $(distdir)/doc/html/.doctrees
	-rm -rf $(distdir)/doc/man/.buildinfo $(distdir)/doc/man/.doctrees

	@if test -d "$(srcdir)/.git"; then                                              \
		(cd "$(srcdir)" &&                                                           \
      $(top_srcdir)/build/autotools/missing --run git log --stat ) > ChangeLog.tmp \
		&& mv -f ChangeLog.tmp $(top_distdir)/ChangeLog                              \
		|| (rm -f ChangeLog.tmp;                                                     \
		echo Failed to generate ChangeLog >&2);                                      \
	else                                                                            \
	  echo A git checkout is required to generate a ChangeLog >&2;                  \
	fi

# Make libmongoc-static-1.0.a symlink to libmongoc-1.0.a. If libmongoc is built with
# CMake instead of the Autotools, the static archive is named
# libmongoc-static-1.0.a by default. For the sake of projects that want to
# statically link to libmongoc, ensure the static archive is present with the
# name no matter how it was built.

# Also, copy CMake config-file packages to lib/cmake/ for use with CMake's
# "find_package" command.

if ENABLE_STATIC
enable_static=1
endif

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(libdir)/cmake/libmongoc-1.0
	cp -f build/cmake/libmongoc-1.0-config.cmake $(DESTDIR)$(libdir)/cmake/libmongoc-1.0
	cp -f build/cmake/libmongoc-1.0-config-version.cmake $(DESTDIR)$(libdir)/cmake/libmongoc-1.0

	if [ ! -z "$(enable_static)" ]; then                                                                      \
	  rm -rf $(DESTDIR)$(libdir)/cmake/libmongoc-static-1.0;                                                  \
	  $(MKDIR_P) $(DESTDIR)$(libdir)/cmake/libmongoc-static-1.0;                                              \
	  cd $(DESTDIR)$(libdir);                                                                                 \
	  rm -f libmongoc-static-1.0.a;                                                                           \
	  $(LN_S) libmongoc-1.0.a libmongoc-static-1.0.a;                                                         \
	  cp -f $(abs_builddir)/build/cmake/libmongoc-static-1.0-config.cmake cmake/libmongoc-static-1.0;         \
	  cp -f $(abs_builddir)/build/cmake/libmongoc-static-1.0-config-version.cmake cmake/libmongoc-static-1.0; \
	fi

clean-local:
	-rm -rf $(CLEANFILES)

uninstall-local:
	-rm -r $(mongocdocdir)
	-rm -rf $(DESTDIR)$(libdir)/libmongoc-static-1.0.a
	-rm -rf $(DESTDIR)$(libdir)/cmake/libmongoc-1.0
	-rm -rf $(DESTDIR)$(libdir)/cmake/libmongoc-static-1.0
