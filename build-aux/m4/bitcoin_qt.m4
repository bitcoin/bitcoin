dnl Copyright (c) 2013-2016 The Bitcoin Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

dnl Helper for cases where a qt dependency is not met.
dnl Output: If qt version is auto, set bitcoin_enable_qt to false. Else, exit.
AC_DEFUN([BITCOIN_QT_FAIL],[
  if test "x$bitcoin_qt_want_version" = "xauto" && test x$bitcoin_qt_force != xyes; then
    if test x$bitcoin_enable_qt != xno; then
      AC_MSG_WARN([$1; bitcoin-qt frontend will not be built])
    fi
    bitcoin_enable_qt=no
    bitcoin_enable_qt_test=no
  else
    AC_MSG_ERROR([$1])
  fi
])

AC_DEFUN([BITCOIN_QT_CHECK],[
  if test "x$bitcoin_enable_qt" != "xno" && test x$bitcoin_qt_want_version != xno; then
    true
    $1
  else
    true
    $2
  fi
])

dnl BITCOIN_QT_PATH_PROGS([FOO], [foo foo2], [/path/to/search/first], [continue if missing])
dnl Helper for finding the path of programs needed for Qt.
dnl Inputs: $1: Variable to be set
dnl Inputs: $2: List of programs to search for
dnl Inputs: $3: Look for $2 here before $PATH
dnl Inputs: $4: If "yes", don't fail if $2 is not found.
dnl Output: $1 is set to the path of $2 if found. $2 are searched in order.
AC_DEFUN([BITCOIN_QT_PATH_PROGS],[
  BITCOIN_QT_CHECK([
    if test "x$3" != "x"; then
      AC_PATH_PROGS($1,$2,,$3)
    else
      AC_PATH_PROGS($1,$2)
    fi
    if test "x$$1" = "x" && test "x$4" != "xyes"; then
      BITCOIN_QT_FAIL([$1 not found])
    fi
  ])
])

dnl Initialize qt input.
dnl This must be called before any other BITCOIN_QT* macros to ensure that
dnl input variables are set correctly.
dnl CAUTION: Do not use this inside of a conditional.
AC_DEFUN([BITCOIN_QT_INIT],[
  dnl enable qt support
  AC_ARG_WITH([gui],
    [AS_HELP_STRING([--with-gui@<:@=no|qt4|qt5|auto@:>@],
    [build bitcoin-qt GUI (default=auto, qt5 tried first)])],
    [
     bitcoin_qt_want_version=$withval
     if test x$bitcoin_qt_want_version = xyes; then
       bitcoin_qt_force=yes
       bitcoin_qt_want_version=auto
     fi
    ],
    [bitcoin_qt_want_version=auto])

  AC_ARG_WITH([qt-incdir],[AS_HELP_STRING([--with-qt-incdir=INC_DIR],[specify qt include path (overridden by pkgconfig)])], [qt_include_path=$withval], [])
  AC_ARG_WITH([qt-libdir],[AS_HELP_STRING([--with-qt-libdir=LIB_DIR],[specify qt lib path (overridden by pkgconfig)])], [qt_lib_path=$withval], [])
  AC_ARG_WITH([qt-plugindir],[AS_HELP_STRING([--with-qt-plugindir=PLUGIN_DIR],[specify qt plugin path (overridden by pkgconfig)])], [qt_plugin_path=$withval], [])
  AC_ARG_WITH([qt-translationdir],[AS_HELP_STRING([--with-qt-translationdir=PLUGIN_DIR],[specify qt translation path (overridden by pkgconfig)])], [qt_translation_path=$withval], [])
  AC_ARG_WITH([qt-bindir],[AS_HELP_STRING([--with-qt-bindir=BIN_DIR],[specify qt bin path])], [qt_bin_path=$withval], [])

  AC_ARG_WITH([qtdbus],
    [AS_HELP_STRING([--with-qtdbus],
    [enable DBus support (default is yes if qt is enabled and QtDBus is found)])],
    [use_dbus=$withval],
    [use_dbus=auto])

  AC_SUBST(QT_TRANSLATION_DIR,$qt_translation_path)
])

dnl Find the appropriate version of Qt libraries and includes.
dnl Inputs: $1: Whether or not pkg-config should be used. yes|no. Default: yes.
dnl Inputs: $2: If $1 is "yes" and --with-gui=auto, which qt version should be
dnl         tried first.
dnl Outputs: See _BITCOIN_QT_FIND_LIBS_*
dnl Outputs: Sets variables for all qt-related tools.
dnl Outputs: bitcoin_enable_qt, bitcoin_enable_qt_dbus, bitcoin_enable_qt_test
AC_DEFUN([BITCOIN_QT_CONFIGURE],[
  use_pkgconfig=$1

  if test x$use_pkgconfig = x; then
    use_pkgconfig=yes
  fi

  if test x$use_pkgconfig = xyes; then
    BITCOIN_QT_CHECK([_BITCOIN_QT_FIND_LIBS_WITH_PKGCONFIG([$2])])
  else
    BITCOIN_QT_CHECK([_BITCOIN_QT_FIND_LIBS_WITHOUT_PKGCONFIG])
  fi

  dnl This is ugly and complicated. Yuck. Works as follows:
  dnl We can't discern whether Qt4 builds are static or not. For Qt5, we can
  dnl check a header to find out. When Qt is built statically, some plugins must
  dnl be linked into the final binary as well. These plugins have changed between
  dnl Qt4 and Qt5. With Qt5, languages moved into core and the WindowsIntegration
  dnl plugin was added. Since we can't tell if Qt4 is static or not, it is
  dnl assumed for windows builds.
  dnl _BITCOIN_QT_CHECK_STATIC_PLUGINS does a quick link-check and appends the
  dnl results to QT_LIBS.
  BITCOIN_QT_CHECK([
  TEMP_CPPFLAGS=$CPPFLAGS
  TEMP_CXXFLAGS=$CXXFLAGS
  CPPFLAGS="$QT_INCLUDES $CPPFLAGS"
  CXXFLAGS="$PIC_FLAGS $CXXFLAGS"
  if test x$bitcoin_qt_got_major_vers = x5; then
    _BITCOIN_QT_IS_STATIC
    if test x$bitcoin_cv_static_qt = xyes; then
      _BITCOIN_QT_FIND_STATIC_PLUGINS
      AC_DEFINE(QT_STATICPLUGIN, 1, [Define this symbol if qt plugins are static])
      AC_CACHE_CHECK(for Qt < 5.4, bitcoin_cv_need_acc_widget,[AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
          [[#include <QtCore>]],[[
          #if QT_VERSION >= 0x050400
          choke;
          #endif
          ]])],
        [bitcoin_cv_need_acc_widget=yes],
        [bitcoin_cv_need_acc_widget=no])
      ])
      if test "x$bitcoin_cv_need_acc_widget" = "xyes"; then
        _BITCOIN_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(AccessibleFactory)], [-lqtaccessiblewidgets])
      fi
      _BITCOIN_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QMinimalIntegrationPlugin)],[-lqminimal])
      AC_DEFINE(QT_QPA_PLATFORM_MINIMAL, 1, [Define this symbol if the minimal qt platform exists])
      if test x$TARGET_OS = xwindows; then
        _BITCOIN_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)],[-lqwindows])
        AC_DEFINE(QT_QPA_PLATFORM_WINDOWS, 1, [Define this symbol if the qt platform is windows])
      elif test x$TARGET_OS = xlinux; then
        _BITCOIN_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)],[-lqxcb -lxcb-static])
        AC_DEFINE(QT_QPA_PLATFORM_XCB, 1, [Define this symbol if the qt platform is xcb])
      elif test x$TARGET_OS = xdarwin; then
        AX_CHECK_LINK_FLAG([[-framework IOKit]],[QT_LIBS="$QT_LIBS -framework IOKit"],[AC_MSG_ERROR(could not iokit framework)])
        _BITCOIN_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)],[-lqcocoa])
        AC_DEFINE(QT_QPA_PLATFORM_COCOA, 1, [Define this symbol if the qt platform is cocoa])
      fi
    fi
  else
    if test x$TARGET_OS = xwindows; then
      AC_DEFINE(QT_STATICPLUGIN, 1, [Define this symbol if qt plugins are static])
      _BITCOIN_QT_CHECK_STATIC_PLUGINS([
         Q_IMPORT_PLUGIN(qcncodecs)
         Q_IMPORT_PLUGIN(qjpcodecs)
         Q_IMPORT_PLUGIN(qtwcodecs)
         Q_IMPORT_PLUGIN(qkrcodecs)
         Q_IMPORT_PLUGIN(AccessibleFactory)],
         [-lqcncodecs -lqjpcodecs -lqtwcodecs -lqkrcodecs -lqtaccessiblewidgets])
    fi
  fi
  CPPFLAGS=$TEMP_CPPFLAGS
  CXXFLAGS=$TEMP_CXXFLAGS
  ])

  if test x$use_pkgconfig$qt_bin_path = xyes; then
    if test x$bitcoin_qt_got_major_vers = x5; then
      qt_bin_path="`$PKG_CONFIG --variable=host_bins Qt5Core 2>/dev/null`"
    fi
  fi

  if test x$use_hardening != xno; then
    BITCOIN_QT_CHECK([
    AC_MSG_CHECKING(whether -fPIE can be used with this Qt config)
    TEMP_CPPFLAGS=$CPPFLAGS
    TEMP_CXXFLAGS=$CXXFLAGS
    CPPFLAGS="$QT_INCLUDES $CPPFLAGS"
    CXXFLAGS="$PIE_FLAGS $CXXFLAGS"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <QtCore/qconfig.h>]],
      [[
          #if defined(QT_REDUCE_RELOCATIONS)
              choke;
          #endif
      ]])],
      [ AC_MSG_RESULT(yes); QT_PIE_FLAGS=$PIE_FLAGS ],
      [ AC_MSG_RESULT(no); QT_PIE_FLAGS=$PIC_FLAGS]
    )
    CPPFLAGS=$TEMP_CPPFLAGS
    CXXFLAGS=$TEMP_CXXFLAGS
    ])
  else
    BITCOIN_QT_CHECK([
    AC_MSG_CHECKING(whether -fPIC is needed with this Qt config)
    TEMP_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$QT_INCLUDES $CPPFLAGS"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <QtCore/qconfig.h>]],
      [[
          #if defined(QT_REDUCE_RELOCATIONS)
              choke;
          #endif
      ]])],
      [ AC_MSG_RESULT(no)],
      [ AC_MSG_RESULT(yes); QT_PIE_FLAGS=$PIC_FLAGS]
    )
    CPPFLAGS=$TEMP_CPPFLAGS
    ])
  fi

  BITCOIN_QT_PATH_PROGS([MOC], [moc-qt${bitcoin_qt_got_major_vers} moc${bitcoin_qt_got_major_vers} moc], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([UIC], [uic-qt${bitcoin_qt_got_major_vers} uic${bitcoin_qt_got_major_vers} uic], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([RCC], [rcc-qt${bitcoin_qt_got_major_vers} rcc${bitcoin_qt_got_major_vers} rcc], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([LRELEASE], [lrelease-qt${bitcoin_qt_got_major_vers} lrelease${bitcoin_qt_got_major_vers} lrelease], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([LUPDATE], [lupdate-qt${bitcoin_qt_got_major_vers} lupdate${bitcoin_qt_got_major_vers} lupdate],$qt_bin_path, yes)

  MOC_DEFS='-DHAVE_CONFIG_H -I$(srcdir)'
  case $host in
    *darwin*)
     BITCOIN_QT_CHECK([
       MOC_DEFS="${MOC_DEFS} -DQ_OS_MAC"
       base_frameworks="-framework Foundation -framework ApplicationServices -framework AppKit"
       AX_CHECK_LINK_FLAG([[$base_frameworks]],[QT_LIBS="$QT_LIBS $base_frameworks"],[AC_MSG_ERROR(could not find base frameworks)])
     ])
    ;;
    *mingw*)
       BITCOIN_QT_CHECK([
         AX_CHECK_LINK_FLAG([[-mwindows]],[QT_LDFLAGS="$QT_LDFLAGS -mwindows"],[AC_MSG_WARN(-mwindows linker support not detected)])
       ])
  esac


  dnl enable qt support
  AC_MSG_CHECKING(whether to build ]AC_PACKAGE_NAME[ GUI)
  BITCOIN_QT_CHECK([
    bitcoin_enable_qt=yes
    bitcoin_enable_qt_test=yes
    if test x$have_qt_test = xno; then
      bitcoin_enable_qt_test=no
    fi
    bitcoin_enable_qt_dbus=no
    if test x$use_dbus != xno && test x$have_qt_dbus = xyes; then
      bitcoin_enable_qt_dbus=yes
    fi
    if test x$use_dbus = xyes && test x$have_qt_dbus = xno; then
      AC_MSG_ERROR("libQtDBus not found. Install libQtDBus or remove --with-qtdbus.")
    fi
    if test x$LUPDATE = x; then
      AC_MSG_WARN("lupdate is required to update qt translations")
    fi
  ],[
    bitcoin_enable_qt=no
  ])
  AC_MSG_RESULT([$bitcoin_enable_qt (Qt${bitcoin_qt_got_major_vers})])

  AC_SUBST(QT_PIE_FLAGS)
  AC_SUBST(QT_INCLUDES)
  AC_SUBST(QT_LIBS)
  AC_SUBST(QT_LDFLAGS)
  AC_SUBST(QT_DBUS_INCLUDES)
  AC_SUBST(QT_DBUS_LIBS)
  AC_SUBST(QT_TEST_INCLUDES)
  AC_SUBST(QT_TEST_LIBS)
  AC_SUBST(QT_SELECT, qt${bitcoin_qt_got_major_vers})
  AC_SUBST(MOC_DEFS)
])

dnl All macros below are internal and should _not_ be used from the main
dnl configure.ac.
dnl ----

dnl Internal. Check if the included version of Qt is Qt5.
dnl Requires: INCLUDES must be populated as necessary.
dnl Output: bitcoin_cv_qt5=yes|no
AC_DEFUN([_BITCOIN_QT_CHECK_QT5],[
  AC_CACHE_CHECK(for Qt 5, bitcoin_cv_qt5,[
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <QtCore>]],
    [[
      #if QT_VERSION < 0x050000
      choke me
      #else
      return 0;
      #endif
    ]])],
    [bitcoin_cv_qt5=yes],
    [bitcoin_cv_qt5=no])
])])

dnl Internal. Check if the linked version of Qt was built as static libs.
dnl Requires: Qt5. This check cannot determine if Qt4 is static.
dnl Requires: INCLUDES and LIBS must be populated as necessary.
dnl Output: bitcoin_cv_static_qt=yes|no
dnl Output: Defines QT_STATICPLUGIN if plugins are static.
AC_DEFUN([_BITCOIN_QT_IS_STATIC],[
  AC_CACHE_CHECK(for static Qt, bitcoin_cv_static_qt,[
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <QtCore>]],
    [[
      #if defined(QT_STATIC)
      return 0;
      #else
      choke me
      #endif
    ]])],
    [bitcoin_cv_static_qt=yes],
    [bitcoin_cv_static_qt=no])
  ])
  if test xbitcoin_cv_static_qt = xyes; then
    AC_DEFINE(QT_STATICPLUGIN, 1, [Define this symbol for static Qt plugins])
  fi
])

dnl Internal. Check if the link-requirements for static plugins are met.
dnl Requires: INCLUDES and LIBS must be populated as necessary.
dnl Inputs: $1: A series of Q_IMPORT_PLUGIN().
dnl Inputs: $2: The libraries that resolve $1.
dnl Output: QT_LIBS is prepended or configure exits.
AC_DEFUN([_BITCOIN_QT_CHECK_STATIC_PLUGINS],[
  AC_MSG_CHECKING(for static Qt plugins: $2)
  CHECK_STATIC_PLUGINS_TEMP_LIBS="$LIBS"
  LIBS="$2 $QT_LIBS $LIBS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #define QT_STATICPLUGIN
    #include <QtPlugin>
    $1]],
    [[return 0;]])],
    [AC_MSG_RESULT(yes); QT_LIBS="$2 $QT_LIBS"],
    [AC_MSG_RESULT(no); BITCOIN_QT_FAIL(Could not resolve: $2)])
  LIBS="$CHECK_STATIC_PLUGINS_TEMP_LIBS"
])

dnl Internal. Find paths necessary for linking qt static plugins
dnl Inputs: bitcoin_qt_got_major_vers. 4 or 5.
dnl Inputs: qt_plugin_path. optional.
dnl Outputs: QT_LIBS is appended
AC_DEFUN([_BITCOIN_QT_FIND_STATIC_PLUGINS],[
  if test x$bitcoin_qt_got_major_vers = x5; then
      if test x$qt_plugin_path != x; then
        QT_LIBS="$QT_LIBS -L$qt_plugin_path/platforms"
        if test -d "$qt_plugin_path/accessible"; then
          QT_LIBS="$QT_LIBS -L$qt_plugin_path/accessible"
        fi
      fi
     if test x$use_pkgconfig = xyes; then
     : dnl
     m4_ifdef([PKG_CHECK_MODULES],[
       PKG_CHECK_MODULES([QTPLATFORM], [Qt5PlatformSupport], [QT_LIBS="$QTPLATFORM_LIBS $QT_LIBS"])
       if test x$TARGET_OS = xlinux; then
         PKG_CHECK_MODULES([X11XCB], [x11-xcb], [QT_LIBS="$X11XCB_LIBS $QT_LIBS"])
         if ${PKG_CONFIG} --exists "Qt5Core >= 5.5" 2>/dev/null; then
           PKG_CHECK_MODULES([QTXCBQPA], [Qt5XcbQpa], [QT_LIBS="$QTXCBQPA_LIBS $QT_LIBS"])
         fi
       elif test x$TARGET_OS = xdarwin; then
         PKG_CHECK_MODULES([QTPRINT], [Qt5PrintSupport], [QT_LIBS="$QTPRINT_LIBS $QT_LIBS"])
       fi
     ])
     else
       if test x$TARGET_OS = xwindows; then
         AC_CACHE_CHECK(for Qt >= 5.6, bitcoin_cv_need_platformsupport,[AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
             [[#include <QtCore>]],[[
             #if QT_VERSION < 0x050600
             choke;
             #endif
             ]])],
           [bitcoin_cv_need_platformsupport=yes],
           [bitcoin_cv_need_platformsupport=no])
         ])
         if test x$bitcoin_cv_need_platformsupport = xyes; then
           BITCOIN_QT_CHECK(AC_CHECK_LIB([${QT_LIB_PREFIX}PlatformSupport],[main],,BITCOIN_QT_FAIL(lib$QT_LIB_PREFIXPlatformSupport not found)))
         fi
       fi
     fi
  else
    if test x$qt_plugin_path != x; then
      QT_LIBS="$QT_LIBS -L$qt_plugin_path/accessible"
      QT_LIBS="$QT_LIBS -L$qt_plugin_path/codecs"
    fi
  fi
])

dnl Internal. Find Qt libraries using pkg-config.
dnl Inputs: bitcoin_qt_want_version (from --with-gui=). The version to check
dnl         first.
dnl Inputs: $1: If bitcoin_qt_want_version is "auto", check for this version
dnl         first.
dnl Outputs: All necessary QT_* variables are set.
dnl Outputs: bitcoin_qt_got_major_vers is set to "4" or "5".
dnl Outputs: have_qt_test and have_qt_dbus are set (if applicable) to yes|no.
AC_DEFUN([_BITCOIN_QT_FIND_LIBS_WITH_PKGCONFIG],[
  m4_ifdef([PKG_CHECK_MODULES],[
  auto_priority_version=$1
  if test x$auto_priority_version = x; then
    auto_priority_version=qt5
  fi
    if test x$bitcoin_qt_want_version = xqt5 ||  ( test x$bitcoin_qt_want_version = xauto && test x$auto_priority_version = xqt5 ); then
      QT_LIB_PREFIX=Qt5
      bitcoin_qt_got_major_vers=5
    else
      QT_LIB_PREFIX=Qt
      bitcoin_qt_got_major_vers=4
    fi
    qt5_modules="Qt5Core Qt5Gui Qt5Network Qt5Widgets"
    qt4_modules="QtCore QtGui QtNetwork"
    BITCOIN_QT_CHECK([
      if test x$bitcoin_qt_want_version = xqt5 || ( test x$bitcoin_qt_want_version = xauto && test x$auto_priority_version = xqt5 ); then
        PKG_CHECK_MODULES([QT5], [$qt5_modules], [QT_INCLUDES="$QT5_CFLAGS"; QT_LIBS="$QT5_LIBS" have_qt=yes],[have_qt=no])
      elif test x$bitcoin_qt_want_version = xqt4 || ( test x$bitcoin_qt_want_version = xauto && test x$auto_priority_version = xqt4 ); then
        PKG_CHECK_MODULES([QT4], [$qt4_modules], [QT_INCLUDES="$QT4_CFLAGS"; QT_LIBS="$QT4_LIBS" ; have_qt=yes], [have_qt=no])
      fi

      dnl qt version is set to 'auto' and the preferred version wasn't found. Now try the other.
      if test x$have_qt = xno && test x$bitcoin_qt_want_version = xauto; then
        if test x$auto_priority_version = xqt5; then
          PKG_CHECK_MODULES([QT4], [$qt4_modules], [QT_INCLUDES="$QT4_CFLAGS"; QT_LIBS="$QT4_LIBS" ; have_qt=yes; QT_LIB_PREFIX=Qt; bitcoin_qt_got_major_vers=4], [have_qt=no])
        else
          PKG_CHECK_MODULES([QT5], [$qt5_modules], [QT_INCLUDES="$QT5_CFLAGS"; QT_LIBS="$QT5_LIBS" ; have_qt=yes; QT_LIB_PREFIX=Qt5; bitcoin_qt_got_major_vers=5], [have_qt=no])
        fi
      fi
      if test x$have_qt != xyes; then
        have_qt=no
        BITCOIN_QT_FAIL([Qt dependencies not found])
      fi
    ])
    BITCOIN_QT_CHECK([
      PKG_CHECK_MODULES([QT_TEST], [${QT_LIB_PREFIX}Test], [QT_TEST_INCLUDES="$QT_TEST_CFLAGS"; have_qt_test=yes], [have_qt_test=no])
      if test x$use_dbus != xno; then
        PKG_CHECK_MODULES([QT_DBUS], [${QT_LIB_PREFIX}DBus], [QT_DBUS_INCLUDES="$QT_DBUS_CFLAGS"; have_qt_dbus=yes], [have_qt_dbus=no])
      fi
    ])
  ])
  true; dnl
])

dnl Internal. Find Qt libraries without using pkg-config. Version is deduced
dnl from the discovered headers.
dnl Inputs: bitcoin_qt_want_version (from --with-gui=). The version to use.
dnl         If "auto", the version will be discovered by _BITCOIN_QT_CHECK_QT5.
dnl Outputs: All necessary QT_* variables are set.
dnl Outputs: bitcoin_qt_got_major_vers is set to "4" or "5".
dnl Outputs: have_qt_test and have_qt_dbus are set (if applicable) to yes|no.
AC_DEFUN([_BITCOIN_QT_FIND_LIBS_WITHOUT_PKGCONFIG],[
  TEMP_CPPFLAGS="$CPPFLAGS"
  TEMP_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$PIC_FLAGS $CXXFLAGS"
  TEMP_LIBS="$LIBS"
  BITCOIN_QT_CHECK([
    if test x$qt_include_path != x; then
      QT_INCLUDES="-I$qt_include_path -I$qt_include_path/QtCore -I$qt_include_path/QtGui -I$qt_include_path/QtWidgets -I$qt_include_path/QtNetwork -I$qt_include_path/QtTest -I$qt_include_path/QtDBus"
      CPPFLAGS="$QT_INCLUDES $CPPFLAGS"
    fi
  ])

  BITCOIN_QT_CHECK([AC_CHECK_HEADER([QtPlugin],,BITCOIN_QT_FAIL(QtCore headers missing))])
  BITCOIN_QT_CHECK([AC_CHECK_HEADER([QApplication],, BITCOIN_QT_FAIL(QtGui headers missing))])
  BITCOIN_QT_CHECK([AC_CHECK_HEADER([QLocalSocket],, BITCOIN_QT_FAIL(QtNetwork headers missing))])

  BITCOIN_QT_CHECK([
    if test x$bitcoin_qt_want_version = xauto; then
      _BITCOIN_QT_CHECK_QT5
    fi
    if test x$bitcoin_cv_qt5 = xyes || test x$bitcoin_qt_want_version = xqt5; then
      QT_LIB_PREFIX=Qt5
      bitcoin_qt_got_major_vers=5
    else
      QT_LIB_PREFIX=Qt
      bitcoin_qt_got_major_vers=4
    fi
  ])

  BITCOIN_QT_CHECK([
    LIBS=
    if test x$qt_lib_path != x; then
      LIBS="$LIBS -L$qt_lib_path"
    fi

    if test x$TARGET_OS = xwindows; then
      AC_CHECK_LIB([imm32],      [main],, BITCOIN_QT_FAIL(libimm32 not found))
    fi
  ])

  BITCOIN_QT_CHECK(AC_CHECK_LIB([z] ,[main],,AC_MSG_WARN([zlib not found. Assuming qt has it built-in])))
  BITCOIN_QT_CHECK(AC_SEARCH_LIBS([png_error] ,[qtpng png],,AC_MSG_WARN([libpng not found. Assuming qt has it built-in])))
  BITCOIN_QT_CHECK(AC_SEARCH_LIBS([jpeg_create_decompress] ,[qtjpeg jpeg],,AC_MSG_WARN([libjpeg not found. Assuming qt has it built-in])))
  BITCOIN_QT_CHECK(AC_SEARCH_LIBS([pcre16_exec], [qtpcre pcre16],,AC_MSG_WARN([libpcre16 not found. Assuming qt has it built-in])))
  BITCOIN_QT_CHECK(AC_SEARCH_LIBS([hb_ot_tags_from_script] ,[qtharfbuzzng harfbuzz],,AC_MSG_WARN([libharfbuzz not found. Assuming qt has it built-in or support is disabled])))
  BITCOIN_QT_CHECK(AC_CHECK_LIB([${QT_LIB_PREFIX}Core]   ,[main],,BITCOIN_QT_FAIL(lib$QT_LIB_PREFIXCore not found)))
  BITCOIN_QT_CHECK(AC_CHECK_LIB([${QT_LIB_PREFIX}Gui]    ,[main],,BITCOIN_QT_FAIL(lib$QT_LIB_PREFIXGui not found)))
  BITCOIN_QT_CHECK(AC_CHECK_LIB([${QT_LIB_PREFIX}Network],[main],,BITCOIN_QT_FAIL(lib$QT_LIB_PREFIXNetwork not found)))
  if test x$bitcoin_qt_got_major_vers = x5; then
    BITCOIN_QT_CHECK(AC_CHECK_LIB([${QT_LIB_PREFIX}Widgets],[main],,BITCOIN_QT_FAIL(lib$QT_LIB_PREFIXWidgets not found)))
  fi
  QT_LIBS="$LIBS"
  LIBS="$TEMP_LIBS"

  BITCOIN_QT_CHECK([
    LIBS=
    if test x$qt_lib_path != x; then
      LIBS="-L$qt_lib_path"
    fi
    AC_CHECK_LIB([${QT_LIB_PREFIX}Test],      [main],, have_qt_test=no)
    AC_CHECK_HEADER([QTest],, have_qt_test=no)
    QT_TEST_LIBS="$LIBS"
    if test x$use_dbus != xno; then
      LIBS=
      if test x$qt_lib_path != x; then
        LIBS="-L$qt_lib_path"
      fi
      AC_CHECK_LIB([${QT_LIB_PREFIX}DBus],      [main],, have_qt_dbus=no)
      AC_CHECK_HEADER([QtDBus],, have_qt_dbus=no)
      QT_DBUS_LIBS="$LIBS"
    fi
  ])
  CPPFLAGS="$TEMP_CPPFLAGS"
  CXXFLAGS="$TEMP_CXXFLAGS"
  LIBS="$TEMP_LIBS"
])

