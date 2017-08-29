AC_PREREQ(2.60)

# Set version from contents of "VERSION_CURRENT" file (and strip newlines).
AC_INIT([mongo-c-driver],
        m4_esyscmd(cat VERSION_CURRENT | tr -d '\n'),
        [https://jira.mongodb.org/browse/CDRIVER])

m4_include([build/autotools/Versions.m4])

AC_CONFIG_MACRO_DIR([build/autotools/m4])
AC_CONFIG_AUX_DIR([build/autotools])
AC_SUBST(ACLOCAL_AMFLAGS, "-I build/autotools")

MONGOC_API_VERSION=1.0
AC_SUBST(MONGOC_API_VERSION)

# AS_VAR_COPY is available in AC 2.64 and on, but we only require 2.60.
# If we're on an older version, we define it ourselves:
m4_ifndef([AS_VAR_COPY],
          [m4_define([AS_VAR_COPY],
          [AS_LITERAL_IF([$1[]$2], [$1=$$2], [eval $1=\$$2])])])

# Get "user-set cflags" here, before we've added the flags we use by default
AS_VAR_COPY(MONGOC_USER_SET_CFLAGS, [CFLAGS])
AC_SUBST(MONGOC_USER_SET_CFLAGS)

AS_VAR_COPY(MONGOC_USER_SET_LDFLAGS, [LDFLAGS])
AC_SUBST(MONGOC_USER_SET_LDFLAGS)

m4_include([build/autotools/CheckHost.m4])
m4_include([build/autotools/CheckCompiler.m4])
m4_include([build/autotools/CheckProgs.m4])
m4_include([build/autotools/CheckTarget.m4])

m4_include([build/autotools/SetupLibtool.m4])
LT_INIT

m4_include([build/autotools/ReadCommandLineArguments.m4])
m4_include([build/autotools/CheckSasl.m4])
#if test "x$MONGOC_36_EXPERIMENT" = "xyes"; then
# m4_include([build/autotools/feature.m4])
#else
# AC_SUBST(MONGOC_FEATURE, 0)
# enable_feature_1=no
#fi
m4_include([build/autotools/CheckSnappy.m4])
m4_include([build/autotools/CheckZlib.m4])

if test "x$with_zlib" != "xno" -o "x$with_snappy" != "xno"; then
   AC_SUBST(MONGOC_ENABLE_COMPRESSION, 1)
else
   AC_SUBST(MONGOC_ENABLE_COMPRESSION, 0)
fi
m4_include([build/autotools/CheckSSL.m4])
m4_include([build/autotools/FindDependencies.m4])
m4_include([build/autotools/MaintainerFlags.m4])
m4_include([build/autotools/PlatformFlags.m4])
m4_include([build/autotools/Optimizations.m4])
m4_include([build/autotools/Coverage.m4])
m4_include([build/autotools/WeakSymbols.m4])
m4_include([build/autotools/AutomaticInitAndCleanup.m4])

AS_VAR_COPY(MONGOC_CC, [CC])
AC_SUBST(MONGOC_CC)

# We would put AM_INIT_AUTOMAKE into SetupAutomake.m4, but seems to cause
# autoconf errors.
AM_INIT_AUTOMAKE([foreign subdir-objects tar-ustar])
m4_include([build/autotools/SetupAutomake.m4])

# Substitute our dependencies into the pkg-config files.
MONGOC_LIBS="${SASL_LIBS} ${SSL_LIBS} ${SHM_LIB}"
if test "x$with_zlib" != "xbundled"; then
   MONGOC_LIBS="${MONGOC_LIBS} ${ZLIB_LIBS}"
fi
if test "x$with_snappy" != "xbundled"; then
   MONGOC_LIBS="${MONGOC_LIBS} ${SNAPPY_LIBS}"
fi
AC_SUBST(MONGOC_LIBS)

AC_CONFIG_FILES([
    Makefile
    src/libmongoc-1.0.pc
    src/libmongoc-static-1.0.pc
    src/libmongoc-ssl-1.0.pc
    build/cmake/libmongoc-1.0-config-version.cmake
    build/cmake/libmongoc-static-1.0-config.cmake
    build/cmake/libmongoc-1.0-config.cmake
    build/cmake/libmongoc-static-1.0-config-version.cmake
    src/mongoc/mongoc-config.h
    src/mongoc/mongoc-version.h
    src/snappy-1.1.3/snappy-stubs-public.h
    src/zlib-1.2.11/zconf.h
])

m4_include([build/autotools/Libbson.m4])

m4_include([build/autotools/PrintBuildConfiguration.m4])
