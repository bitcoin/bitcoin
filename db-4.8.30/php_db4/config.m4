#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# http://www.apache.org/licenses/LICENSE-2.0.txt
#

dnl $Id$
dnl config.m4 for extension db4

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

PHP_ARG_WITH(db4, whether to enable db4 support,
[  --with-db4           Enable db4 support])

PHP_ARG_WITH(mod_db4, whether to link against mod_db4,
[  --with-mod_db4         Enable mod_db4 support])

if test "$PHP_DB4" != "no"; then
  if test "$PHP_DB4" != "no"; then
    for i in $PHP_DB4 /usr/local/BerkeleyDB.4.7 /usr/local /usr; do
      if test -f "$i/db4/db.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/db4
        THIS_INCLUDE=$i/db4/db.h
        break
      elif test -f "$i/include/db4/db.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include/db4
        THIS_INCLUDE=$i/include/db4/db.h
        break
      elif test -f "$i/include/db/db4.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include/db4
        THIS_INCLUDE=$i/include/db/db4.h
        break
      elif test -f "$i/include/db4.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include
        THIS_INCLUDE=$i/include/db4.h
        break
      elif test -f "$i/include/db.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include
        THIS_INCLUDE=$i/include/db.h
        break
      fi
    done
    PHP_ADD_INCLUDE($INC_DIR)
    PHP_ADD_LIBRARY_WITH_PATH(db_cxx, $THIS_PREFIX/lib, DB4_SHARED_LIBADD)
  fi 
  AC_MSG_CHECKING([if we really need to link against mod_db4])

  # crazy hard-coding to yes in shared extension builds is really annoying
  # user must specify a path for PHP_MOD_DB4
  if test "$PHP_MOD_DB4" = "yes" ; then
    PHP_MOD_DB4="no"
  fi

  if test "$PHP_MOD_DB4" != "no" && test "$PHP_MOD_DB4" != "yes"; then
    PHP_ADD_INCLUDE("$PHP_MOD_DB4")
    AC_DEFINE(HAVE_MOD_DB4, 1, [Whether you have mod_db4])
    AC_MSG_RESULT(yes)
  elif test "$PHP_MOD_DB4" = "no"; then
    PHP_ADD_LIBRARY(db_cxx, $THIS_PREFIX/lib, DB4_SHARED_LIBADD)
    AC_MSG_RESULT(no)
  else 
    AC_MSG_RESULT([err, don't really know] $PHP_MOD_DB4)
  fi
  EXTRA_CXXFLAGS="-g -DHAVE_CONFIG_H -O2 -Wall"
  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY(stdc++, 1, DB4_SHARED_LIBADD)
  PHP_NEW_EXTENSION(db4, db4.cpp, $ext_shared)
  PHP_ADD_MAKEFILE_FRAGMENT
  PHP_SUBST(DB4_SHARED_LIBADD)
  AC_MSG_WARN([*** A note about pthreads ***
  The db4 c++ library by default tries to link against libpthread on some
  systems (notably Linux).  If your PHP install is not linked against
  libpthread, you will need to disable pthread support in db4.  This can
  be done by compiling db4 with the flag  --with-mutex=x86/gcc-assembly.
  PHP can itself be forced to link against libpthread either by manually editing
  its build files (which some distributions do), or by building it with
  --with-experimental-zts.])


fi
