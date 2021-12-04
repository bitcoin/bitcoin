dnl Copyright (c) 2013-2015 The Bitcoin Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

AC_DEFUN([BITCOIN_FIND_BDB48],[
  AC_ARG_VAR([BDB_CFLAGS], [C compiler flags for BerkeleyDB, bypasses autodetection])
  AC_ARG_VAR([BDB_LIBS], [Linker flags for BerkeleyDB, bypasses autodetection])

  if test "x$use_bdb" = "xno"; then
    use_bdb=no
  elif test "x$BDB_CFLAGS" = "x"; then
    AC_MSG_CHECKING([for Berkeley DB C++ headers])
    BDB_CPPFLAGS=
    bdbpath=X
    bdb48path=X
    bdbdirlist=
    for _vn in 4.8 48 4 5 5.3 ''; do
      for _pfx in b lib ''; do
        bdbdirlist="$bdbdirlist ${_pfx}db${_vn}"
      done
    done
    for searchpath in $bdbdirlist ''; do
      test -n "${searchpath}" && searchpath="${searchpath}/"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <${searchpath}db_cxx.h>
      ]],[[
        #if !((DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8) || DB_VERSION_MAJOR > 4)
          #error "failed to find bdb 4.8+"
        #endif
      ]])],[
        if test "x$bdbpath" = "xX"; then
          bdbpath="${searchpath}"
        fi
      ],[
        continue
      ])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <${searchpath}db_cxx.h>
      ]],[[
        #if !(DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR == 8)
          #error "failed to find bdb 4.8"
        #endif
      ]])],[
        bdb48path="${searchpath}"
        break
      ],[])
    done
    if test "x$bdbpath" = "xX"; then
      use_bdb=no
      AC_MSG_RESULT([no])
      AC_MSG_WARN([libdb_cxx headers missing])
      AC_MSG_WARN(AC_PACKAGE_NAME[ requires this library for BDB (legacy) wallet support])
      AC_MSG_WARN([Passing --without-bdb will suppress this warning])
    elif test "x$bdb48path" = "xX"; then
      BITCOIN_SUBDIR_TO_INCLUDE(BDB_CPPFLAGS,[${bdbpath}],db_cxx)
      AC_ARG_WITH([incompatible-bdb],[AS_HELP_STRING([--with-incompatible-bdb], [allow using a bdb version other than 4.8])],[
        AC_MSG_WARN([Found Berkeley DB other than 4.8])
        AC_MSG_WARN([BDB (legacy) wallets opened by this build will not be portable!])
        use_bdb=yes
      ],[
        AC_MSG_WARN([Found Berkeley DB other than 4.8])
        AC_MSG_WARN([BDB (legacy) wallets opened by this build would not be portable!])
        AC_MSG_WARN([If this is intended, pass --with-incompatible-bdb])
        AC_MSG_WARN([Passing --without-bdb will suppress this warning])
        use_bdb=no
      ])
    else
      BITCOIN_SUBDIR_TO_INCLUDE(BDB_CPPFLAGS,[${bdb48path}],db_cxx)
      bdbpath="${bdb48path}"
      use_bdb=yes
    fi
  else
    BDB_CPPFLAGS=${BDB_CFLAGS}
  fi
  AC_SUBST(BDB_CPPFLAGS)

  if test "x$use_bdb" = "xno"; then
    use_bdb=no
  elif test "x$BDB_LIBS" = "x"; then
    # TODO: Ideally this could find the library version and make sure it matches the headers being used
    for searchlib in db_cxx-4.8 db_cxx db4_cxx; do
      AC_CHECK_LIB([$searchlib],[main],[
        BDB_LIBS="-l${searchlib}"
        break
      ])
    done
    if test "x$BDB_LIBS" = "x"; then
        AC_MSG_WARN([libdb_cxx headers missing])
        AC_MSG_WARN(AC_PACKAGE_NAME[ requires this library for BDB (legacy) wallet support])
        AC_MSG_WARN([Passing --without-bdb will suppress this warning])
    fi
  fi
  if test "x$use_bdb" != "xno"; then
    AC_SUBST(BDB_LIBS)
    AC_DEFINE([USE_BDB], [1], [Define if BDB support should be compiled in])
    use_bdb=yes
  fi
])
