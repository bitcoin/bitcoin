dnl Copyright (c) 2013-2016 The XBit Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

dnl Helper for cases where a qt dependency is not met.
dnl Output: If qt version is auto, set xbit_enable_qt to false. Else, exit.
AC_DEFUN([XBIT_QT_FAIL],[
  if test "x$xbit_qt_want_version" = xauto && test "x$xbit_qt_force" != xyes; then
    if test "x$xbit_enable_qt" != xno; then
      AC_MSG_WARN([$1; xbit-qt frontend will not be built])
    fi
    xbit_enable_qt=no
    xbit_enable_qt_test=no
  else
    AC_MSG_ERROR([$1])
  fi
])

AC_DEFUN([XBIT_QT_CHECK],[
  if test "x$xbit_enable_qt" != xno && test "x$xbit_qt_want_version" != xno; then
    true
    $1
  else
    true
    $2
  fi
])

dnl XBIT_QT_PATH_PROGS([FOO], [foo foo2], [/path/to/search/first], [continue if missing])
dnl Helper for finding the path of programs needed for Qt.
dnl Inputs: $1: Variable to be set
dnl Inputs: $2: List of programs to search for
dnl Inputs: $3: Look for $2 here before $PATH
dnl Inputs: $4: If "yes", don't fail if $2 is not found.
dnl Output: $1 is set to the path of $2 if found. $2 are searched in order.
AC_DEFUN([XBIT_QT_PATH_PROGS],[
  XBIT_QT_CHECK([
    if test "x$3" != x; then
      AC_PATH_PROGS($1,$2,,$3)
    else
      AC_PATH_PROGS($1,$2)
    fi
    if test "x$$1" = x && test "x$4" != xyes; then
      XBIT_QT_FAIL([$1 not found])
    fi
  ])
])

dnl Initialize qt input.
dnl This must be called before any other XBIT_QT* macros to ensure that
dnl input variables are set correctly.
dnl CAUTION: Do not use this inside of a conditional.
AC_DEFUN([XBIT_QT_INIT],[
  dnl enable qt support
  AC_ARG_WITH([gui],
    [AS_HELP_STRING([--with-gui@<:@=no|qt5|auto@:>@],
    [build xbit-qt GUI (default=auto)])],
    [
     xbit_qt_want_version=$withval
     if test "x$xbit_qt_want_version" = xyes; then
       xbit_qt_force=yes
       xbit_qt_want_version=auto
     fi
    ],
    [xbit_qt_want_version=auto])

  AC_ARG_WITH([qt-incdir],[AS_HELP_STRING([--with-qt-incdir=INC_DIR],[specify qt include path (overridden by pkgconfig)])], [qt_include_path=$withval], [])
  AC_ARG_WITH([qt-libdir],[AS_HELP_STRING([--with-qt-libdir=LIB_DIR],[specify qt lib path (overridden by pkgconfig)])], [qt_lib_path=$withval], [])
  AC_ARG_WITH([qt-plugindir],[AS_HELP_STRING([--with-qt-plugindir=PLUGIN_DIR],[specify qt plugin path (overridden by pkgconfig)])], [qt_plugin_path=$withval], [])
  AC_ARG_WITH([qt-translationdir],[AS_HELP_STRING([--with-qt-translationdir=PLUGIN_DIR],[specify qt translation path (overridden by pkgconfig)])], [qt_translation_path=$withval], [])
  AC_ARG_WITH([qt-bindir],[AS_HELP_STRING([--with-qt-bindir=BIN_DIR],[specify qt bin path])], [qt_bin_path=$withval], [])

  AC_ARG_WITH([qtdbus],
    [AS_HELP_STRING([--with-qtdbus],
    [enable DBus support (default is yes if qt is enabled and QtDBus is found, except on Android)])],
    [use_dbus=$withval],
    [use_dbus=auto])

  dnl Android doesn't support D-Bus and certainly doesn't use it for notifications
  case $host in
    *android*)
      if test "x$use_dbus" != xyes; then
        use_dbus=no
      fi
    ;;
  esac

  AC_SUBST(QT_TRANSLATION_DIR,$qt_translation_path)
])

dnl Find Qt libraries and includes.
dnl
dnl   XBIT_QT_CONFIGURE([MINIMUM-VERSION])
dnl
dnl Outputs: See _XBIT_QT_FIND_LIBS
dnl Outputs: Sets variables for all qt-related tools.
dnl Outputs: xbit_enable_qt, xbit_enable_qt_dbus, xbit_enable_qt_test
AC_DEFUN([XBIT_QT_CONFIGURE],[
  qt_version=">= $1"
  qt_lib_prefix="Qt5"
  XBIT_QT_CHECK([_XBIT_QT_FIND_LIBS])

  dnl This is ugly and complicated. Yuck. Works as follows:
  dnl For Qt5, we can check a header to find out whether Qt is build
  dnl statically. When Qt is built statically, some plugins must be linked into
  dnl the final binary as well.
  dnl With Qt5, languages moved into core and the WindowsIntegration plugin was
  dnl added.
  dnl _XBIT_QT_CHECK_STATIC_PLUGINS does a quick link-check and appends the
  dnl results to QT_LIBS.
  XBIT_QT_CHECK([
  TEMP_CPPFLAGS=$CPPFLAGS
  TEMP_CXXFLAGS=$CXXFLAGS
  CPPFLAGS="$QT_INCLUDES $CPPFLAGS"
  CXXFLAGS="$PIC_FLAGS $CXXFLAGS"
  _XBIT_QT_IS_STATIC
  if test "x$xbit_cv_static_qt" = xyes; then
    _XBIT_QT_FIND_STATIC_PLUGINS
    AC_DEFINE(QT_STATICPLUGIN, 1, [Define this symbol if qt plugins are static])
    if test "x$TARGET_OS" != xandroid; then
      _XBIT_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QMinimalIntegrationPlugin)],[-lqminimal])
      AC_DEFINE(QT_QPA_PLATFORM_MINIMAL, 1, [Define this symbol if the minimal qt platform exists])
    fi
    if test "x$TARGET_OS" = xwindows; then
      _XBIT_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)],[-lqwindows])
      AC_DEFINE(QT_QPA_PLATFORM_WINDOWS, 1, [Define this symbol if the qt platform is windows])
    elif test "x$TARGET_OS" = xlinux; then
      _XBIT_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)],[-lqxcb -lxcb-static])
      AC_DEFINE(QT_QPA_PLATFORM_XCB, 1, [Define this symbol if the qt platform is xcb])
    elif test "x$TARGET_OS" = xdarwin; then
      AX_CHECK_LINK_FLAG([[-framework IOKit]],[QT_LIBS="$QT_LIBS -framework IOKit"],[AC_MSG_ERROR(could not iokit framework)])
      _XBIT_QT_CHECK_STATIC_PLUGINS([Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)],[-lqcocoa])
      AC_DEFINE(QT_QPA_PLATFORM_COCOA, 1, [Define this symbol if the qt platform is cocoa])
    elif test "x$TARGET_OS" = xandroid; then
      QT_LIBS="-Wl,--export-dynamic,--undefined=JNI_OnLoad -lqtforandroid -ljnigraphics -landroid -lqtfreetype -lQt5EglSupport $QT_LIBS"
      AC_DEFINE(QT_QPA_PLATFORM_ANDROID, 1, [Define this symbol if the qt platform is android])
    fi
  fi
  CPPFLAGS=$TEMP_CPPFLAGS
  CXXFLAGS=$TEMP_CXXFLAGS
  ])

  if test "x$qt_bin_path" = x; then
    qt_bin_path="`$PKG_CONFIG --variable=host_bins Qt5Core 2>/dev/null`"
  fi

  if test "x$use_hardening" != xno; then
    XBIT_QT_CHECK([
    AC_MSG_CHECKING(whether -fPIE can be used with this Qt config)
    TEMP_CPPFLAGS=$CPPFLAGS
    TEMP_CXXFLAGS=$CXXFLAGS
    CPPFLAGS="$QT_INCLUDES $CPPFLAGS"
    CXXFLAGS="$PIE_FLAGS $CXXFLAGS"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <QtCore/qconfig.h>
        #ifndef QT_VERSION
        #  include <QtCore/qglobal.h>
        #endif
      ]],
      [[
        #if defined(QT_REDUCE_RELOCATIONS)
        choke
        #endif
      ]])],
      [ AC_MSG_RESULT(yes); QT_PIE_FLAGS=$PIE_FLAGS ],
      [ AC_MSG_RESULT(no); QT_PIE_FLAGS=$PIC_FLAGS]
    )
    CPPFLAGS=$TEMP_CPPFLAGS
    CXXFLAGS=$TEMP_CXXFLAGS
    ])
  else
    XBIT_QT_CHECK([
    AC_MSG_CHECKING(whether -fPIC is needed with this Qt config)
    TEMP_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$QT_INCLUDES $CPPFLAGS"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <QtCore/qconfig.h>
        #ifndef QT_VERSION
        #  include <QtCore/qglobal.h>
        #endif
      ]],
      [[
        #if defined(QT_REDUCE_RELOCATIONS)
        choke
        #endif
      ]])],
      [ AC_MSG_RESULT(no)],
      [ AC_MSG_RESULT(yes); QT_PIE_FLAGS=$PIC_FLAGS]
    )
    CPPFLAGS=$TEMP_CPPFLAGS
    ])
  fi

  XBIT_QT_PATH_PROGS([MOC], [moc-qt5 moc5 moc], $qt_bin_path)
  XBIT_QT_PATH_PROGS([UIC], [uic-qt5 uic5 uic], $qt_bin_path)
  XBIT_QT_PATH_PROGS([RCC], [rcc-qt5 rcc5 rcc], $qt_bin_path)
  XBIT_QT_PATH_PROGS([LRELEASE], [lrelease-qt5 lrelease5 lrelease], $qt_bin_path)
  XBIT_QT_PATH_PROGS([LUPDATE], [lupdate-qt5 lupdate5 lupdate],$qt_bin_path, yes)

  MOC_DEFS='-DHAVE_CONFIG_H -I$(srcdir)'
  case $host in
    *darwin*)
     XBIT_QT_CHECK([
       MOC_DEFS="${MOC_DEFS} -DQ_OS_MAC"
       base_frameworks="-framework Foundation -framework ApplicationServices -framework AppKit"
       AX_CHECK_LINK_FLAG([[$base_frameworks]],[QT_LIBS="$QT_LIBS $base_frameworks"],[AC_MSG_ERROR(could not find base frameworks)])
     ])
    ;;
    *mingw*)
       XBIT_QT_CHECK([
         AX_CHECK_LINK_FLAG([[-mwindows]],[QT_LDFLAGS="$QT_LDFLAGS -mwindows"],[AC_MSG_WARN(-mwindows linker support not detected)])
       ])
  esac


  dnl enable qt support
  AC_MSG_CHECKING([whether to build ]AC_PACKAGE_NAME[ GUI])
  XBIT_QT_CHECK([
    xbit_enable_qt=yes
    xbit_enable_qt_test=yes
    if test "x$have_qt_test" = xno; then
      xbit_enable_qt_test=no
    fi
    xbit_enable_qt_dbus=no
    if test "x$use_dbus" != xno && test "x$have_qt_dbus" = xyes; then
      xbit_enable_qt_dbus=yes
    fi
    if test "x$use_dbus" = xyes && test "x$have_qt_dbus" = xno; then
      AC_MSG_ERROR([libQtDBus not found. Install libQtDBus or remove --with-qtdbus.])
    fi
    if test "x$LUPDATE" = x; then
      AC_MSG_WARN([lupdate is required to update qt translations])
    fi
  ],[
    xbit_enable_qt=no
  ])
  if test x$xbit_enable_qt = xyes; then
    AC_MSG_RESULT([$xbit_enable_qt ($qt_lib_prefix)])
  else
    AC_MSG_RESULT([$xbit_enable_qt])
  fi

  AC_SUBST(QT_PIE_FLAGS)
  AC_SUBST(QT_INCLUDES)
  AC_SUBST(QT_LIBS)
  AC_SUBST(QT_LDFLAGS)
  AC_SUBST(QT_DBUS_INCLUDES)
  AC_SUBST(QT_DBUS_LIBS)
  AC_SUBST(QT_TEST_INCLUDES)
  AC_SUBST(QT_TEST_LIBS)
  AC_SUBST(QT_SELECT, qt5)
  AC_SUBST(MOC_DEFS)
])

dnl All macros below are internal and should _not_ be used from the main
dnl configure.ac.
dnl ----

dnl Internal. Check if the linked version of Qt was built as static libs.
dnl Requires: Qt5.
dnl Requires: INCLUDES and LIBS must be populated as necessary.
dnl Output: xbit_cv_static_qt=yes|no
AC_DEFUN([_XBIT_QT_IS_STATIC],[
  AC_CACHE_CHECK(for static Qt, xbit_cv_static_qt,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <QtCore/qconfig.h>
        #ifndef QT_VERSION
        #  include <QtCore/qglobal.h>
        #endif
      ]],
      [[
        #if !defined(QT_STATIC)
        choke
        #endif
      ]])],
      [xbit_cv_static_qt=yes],
      [xbit_cv_static_qt=no])
    ])
])

dnl Internal. Check if the link-requirements for static plugins are met.
dnl Requires: INCLUDES and LIBS must be populated as necessary.
dnl Inputs: $1: A series of Q_IMPORT_PLUGIN().
dnl Inputs: $2: The libraries that resolve $1.
dnl Output: QT_LIBS is prepended or configure exits.
AC_DEFUN([_XBIT_QT_CHECK_STATIC_PLUGINS],[
  AC_MSG_CHECKING(for static Qt plugins: $2)
  CHECK_STATIC_PLUGINS_TEMP_LIBS="$LIBS"
  LIBS="$2 $QT_LIBS $LIBS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #define QT_STATICPLUGIN
    #include <QtPlugin>
    $1]],
    [[return 0;]])],
    [AC_MSG_RESULT(yes); QT_LIBS="$2 $QT_LIBS"],
    [AC_MSG_RESULT(no); XBIT_QT_FAIL(Could not resolve: $2)])
  LIBS="$CHECK_STATIC_PLUGINS_TEMP_LIBS"
])

dnl Internal. Find paths necessary for linking qt static plugins
dnl Inputs: qt_plugin_path. optional.
dnl Outputs: QT_LIBS is appended
AC_DEFUN([_XBIT_QT_FIND_STATIC_PLUGINS],[
    if test "x$qt_plugin_path" != x; then
      QT_LIBS="$QT_LIBS -L$qt_plugin_path/platforms"
      if test -d "$qt_plugin_path/accessible"; then
        QT_LIBS="$QT_LIBS -L$qt_plugin_path/accessible"
      fi
      if test -d "$qt_plugin_path/platforms/android"; then
        QT_LIBS="$QT_LIBS -L$qt_plugin_path/platforms/android -lqtfreetype -lEGL"
      fi
      PKG_CHECK_MODULES([QTFONTDATABASE], [Qt5FontDatabaseSupport], [QT_LIBS="-lQt5FontDatabaseSupport $QT_LIBS"])
      PKG_CHECK_MODULES([QTEVENTDISPATCHER], [Qt5EventDispatcherSupport], [QT_LIBS="-lQt5EventDispatcherSupport $QT_LIBS"])
      PKG_CHECK_MODULES([QTTHEME], [Qt5ThemeSupport], [QT_LIBS="-lQt5ThemeSupport $QT_LIBS"])
      PKG_CHECK_MODULES([QTDEVICEDISCOVERY], [Qt5DeviceDiscoverySupport], [QT_LIBS="-lQt5DeviceDiscoverySupport $QT_LIBS"])
      PKG_CHECK_MODULES([QTACCESSIBILITY], [Qt5AccessibilitySupport], [QT_LIBS="-lQt5AccessibilitySupport $QT_LIBS"])
      PKG_CHECK_MODULES([QTFB], [Qt5FbSupport], [QT_LIBS="-lQt5FbSupport $QT_LIBS"])
      if test "x$TARGET_OS" = xlinux; then
        PKG_CHECK_MODULES([QTXCBQPA], [Qt5XcbQpa], [QT_LIBS="$QTXCBQPA_LIBS $QT_LIBS"])
      elif test "x$TARGET_OS" = xdarwin; then
        PKG_CHECK_MODULES([QTCLIPBOARD], [Qt5ClipboardSupport], [QT_LIBS="-lQt5ClipboardSupport $QT_LIBS"])
        PKG_CHECK_MODULES([QTGRAPHICS], [Qt5GraphicsSupport], [QT_LIBS="-lQt5GraphicsSupport $QT_LIBS"])
        PKG_CHECK_MODULES([QTCGL], [Qt5CglSupport], [QT_LIBS="-lQt5CglSupport $QT_LIBS"])
      fi
    fi
])

dnl Internal. Find Qt libraries using pkg-config.
dnl Outputs: All necessary QT_* variables are set.
dnl Outputs: have_qt_test and have_qt_dbus are set (if applicable) to yes|no.
AC_DEFUN([_XBIT_QT_FIND_LIBS],[
  XBIT_QT_CHECK([
    PKG_CHECK_MODULES([QT_CORE], [${qt_lib_prefix}Core $qt_version], [],
                      [XBIT_QT_FAIL([${qt_lib_prefix}Core $qt_version not found])])
  ])
  XBIT_QT_CHECK([
    PKG_CHECK_MODULES([QT_GUI], [${qt_lib_prefix}Gui $qt_version], [],
                      [XBIT_QT_FAIL([${qt_lib_prefix}Gui $qt_version not found])])
  ])
  XBIT_QT_CHECK([
    PKG_CHECK_MODULES([QT_WIDGETS], [${qt_lib_prefix}Widgets $qt_version], [],
                      [XBIT_QT_FAIL([${qt_lib_prefix}Widgets $qt_version not found])])
  ])
  XBIT_QT_CHECK([
    PKG_CHECK_MODULES([QT_NETWORK], [${qt_lib_prefix}Network $qt_version], [],
                      [XBIT_QT_FAIL([${qt_lib_prefix}Network $qt_version not found])])
  ])
  QT_INCLUDES="$QT_CORE_CFLAGS $QT_GUI_CFLAGS $QT_WIDGETS_CFLAGS $QT_NETWORK_CFLAGS"
  QT_LIBS="$QT_CORE_LIBS $QT_GUI_LIBS $QT_WIDGETS_LIBS $QT_NETWORK_LIBS"

  XBIT_QT_CHECK([
    PKG_CHECK_MODULES([QT_TEST], [${qt_lib_prefix}Test $qt_version], [QT_TEST_INCLUDES="$QT_TEST_CFLAGS"; have_qt_test=yes], [have_qt_test=no])
    if test "x$use_dbus" != xno; then
      PKG_CHECK_MODULES([QT_DBUS], [${qt_lib_prefix}DBus $qt_version], [QT_DBUS_INCLUDES="$QT_DBUS_CFLAGS"; have_qt_dbus=yes], [have_qt_dbus=no])
    fi
  ])
])
