AC_PREREQ(2.60)

# Set version from contents of "VERSION_CURRENT" file (minus newlines).
AC_INIT([libbson],
        m4_esyscmd(cat VERSION_CURRENT | tr -d '\n'),
        [https://jira.mongodb.org/browse/CDRIVER])

m4_include([build/autotools/Versions.m4])

AC_CONFIG_MACRO_DIR([build/autotools/m4])
AC_CONFIG_AUX_DIR([build/autotools])
AC_SUBST(ACLOCAL_AMFLAGS, "-I build/autotools")

BSON_API_VERSION=1.0
AC_SUBST(BSON_API_VERSION)

m4_include([build/autotools/CheckCompiler.m4])
m4_include([build/autotools/CheckHeaders.m4])
m4_include([build/autotools/CheckProgs.m4])
m4_include([build/autotools/CheckHost.m4])
m4_include([build/autotools/CheckTarget.m4])
m4_include([build/autotools/Endian.m4])

m4_include([build/autotools/SetupLibtool.m4])
LT_INIT

m4_include([build/autotools/ReadCommandLineArguments.m4])
m4_include([build/autotools/FindDependencies.m4])
m4_include([build/autotools/CheckAtomics.m4])
m4_include([build/autotools/MaintainerFlags.m4])
m4_include([build/autotools/Optimizations.m4])
m4_include([build/autotools/Coverage.m4])

# We would put AM_INIT_AUTOMAKE into SetupAutomake.m4, but seems to cause autoconf errors.
AM_INIT_AUTOMAKE([foreign subdir-objects tar-ustar])
m4_include([build/autotools/SetupAutomake.m4])

# Substitute our dependencies into the pkg-config files.
LIBBSON_LIBS=""
if test "$os_win32" = "yes"; then
    LIBBSON_LIBS="${LIBBSON_LIBS} -lws2_32"
fi
# It is apparently legitimate to pass cflags to the linker
LIBS="${LIBS} ${LIBBSON_LIBS} ${PTHREAD_LIBS} ${PTHREAD_CFLAGS}"
LIBBSON_LIBS="${LIBS}"
AC_SUBST(LIBBSON_LIBS)

AC_CONFIG_FILES([
    Makefile
    src/libbson-1.0.pc
    src/libbson-static-1.0.pc
    build/cmake/libbson-1.0-config.cmake
    build/cmake/libbson-1.0-config-version.cmake
    build/cmake/libbson-static-1.0-config.cmake
    build/cmake/libbson-static-1.0-config-version.cmake
    src/bson/bson-config.h
    src/bson/bson-version.h
])

m4_include([build/autotools/PrintBuildConfiguration.m4])
