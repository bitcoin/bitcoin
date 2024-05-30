dnl Copyright (c) 2013-2016 The Bitcoin Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

dnl Helper for cases where a qt dependency is not met.
dnl Output: If qt version is auto, set bitcoin_enable_qt to false. Else, exit.
AC_DEFUN([BITCOIN_QT_FAIL],[
  if test "$bitcoin_qt_want_version" = "auto" && test "$bitcoin_qt_force" != "yes"; then
    if test "$bitcoin_enable_qt" != "no"; then
      AC_MSG_WARN([$1; bitcoin-qt frontend will not be built])
    fi
    bitcoin_enable_qt=no
    bitcoin_enable_qt_test=no
  else
    AC_MSG_ERROR([$1])
  fi
])

AC_DEFUN([BITCOIN_QT_CHECK],[
  if test "$bitcoin_enable_qt" != "no" && test "$bitcoin_qt_want_version" != "no"; then
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
    if test "$3" != ""; then
      AC_PATH_PROGS([$1], [$2], [], [$3])
    else
      AC_PATH_PROGS([$1], [$2])
    fi
    if test "$$1" = "" && test "$4" != "yes"; then
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
    [AS_HELP_STRING([--with-gui@<:@=no|qt5|auto@:>@],
    [build bitcoin-qt GUI (default=auto)])],
    [
     bitcoin_qt_want_version=$withval
     if test "$bitcoin_qt_want_version" = "yes"; then
       bitcoin_qt_force=yes
       bitcoin_qt_want_version=auto
     fi
    ],
    [bitcoin_qt_want_version=auto])

  AS_IF([test "$with_gui" = "qt5_debug"],
        [AS_CASE([$host],
                 [*darwin*], [qt_lib_suffix=_debug],
                 [qt_lib_suffix= ]); bitcoin_qt_want_version=qt5],
        [qt_lib_suffix= ])

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

dnl Find Qt libraries and includes.
dnl
dnl   BITCOIN_QT_CONFIGURE([MINIMUM-VERSION])
dnl
dnl Outputs: See _BITCOIN_QT_FIND_LIBS
dnl Outputs: Sets variables for all qt-related tools.
dnl Outputs: bitcoin_enable_qt, bitcoin_enable_qt_dbus, bitcoin_enable_qt_test
AC_DEFUN([BITCOIN_QT_CONFIGURE],[
  qt_version=">= $1"
  qt_lib_prefix="Qt5"
  BITCOIN_QT_CHECK([_BITCOIN_QT_FIND_LIBS])

  dnl This is ugly and complicated. Yuck. Works as follows:
  dnl We check a header to find out whether Qt is built statically.
  dnl When Qt is built statically, some plugins must be linked into
  dnl the final binary as well. _BITCOIN_QT_CHECK_STATIC_PLUGIN does
  dnl a quick link-check and appends the results to QT_LIBS.
  BITCOIN_QT_CHECK([
  TEMP_CPPFLAGS=$CPPFLAGS
  TEMP_CXXFLAGS=$CXXFLAGS
  CPPFLAGS="$QT_INCLUDES $CORE_CPPFLAGS $CPPFLAGS"
  CXXFLAGS="$PIC_FLAGS $CORE_CXXFLAGS $CXXFLAGS"
  _BITCOIN_QT_IS_STATIC
  if test "$bitcoin_cv_static_qt" = "yes"; then
    _BITCOIN_QT_CHECK_STATIC_LIBS

    if test "$qt_plugin_path" != ""; then
      if test -d "$qt_plugin_path/platforms"; then
        QT_LIBS="$QT_LIBS -L$qt_plugin_path/platforms"
      fi
      if test -d "$qt_plugin_path/styles"; then
        QT_LIBS="$QT_LIBS -L$qt_plugin_path/styles"
      fi
      if test -d "$qt_plugin_path/accessible"; then
        QT_LIBS="$QT_LIBS -L$qt_plugin_path/accessible"
      fi
    fi

    AC_DEFINE([QT_STATICPLUGIN], [1], [Define this symbol if qt plugins are static])
    _BITCOIN_QT_CHECK_STATIC_PLUGIN([QMinimalIntegrationPlugin], [-lqminimal])
    AC_DEFINE([QT_QPA_PLATFORM_MINIMAL], [1], [Define this symbol if the minimal qt platform exists])
    if test "$TARGET_OS" = "windows"; then
      dnl Linking against wtsapi32 is required. See #17749 and
      dnl https://bugreports.qt.io/browse/QTBUG-27097.
      AX_CHECK_LINK_FLAG([-lwtsapi32], [QT_LIBS="$QT_LIBS -lwtsapi32"], [AC_MSG_ERROR([could not link against -lwtsapi32])])
      _BITCOIN_QT_CHECK_STATIC_PLUGIN([QWindowsIntegrationPlugin], [-lqwindows])
      _BITCOIN_QT_CHECK_STATIC_PLUGIN([QWindowsVistaStylePlugin], [-lqwindowsvistastyle])
      AC_DEFINE([QT_QPA_PLATFORM_WINDOWS], [1], [Define this symbol if the qt platform is windows])
    elif test "$TARGET_OS" = "linux"; then
      _BITCOIN_QT_CHECK_STATIC_PLUGIN([QXcbIntegrationPlugin], [-lqxcb])
      AC_DEFINE([QT_QPA_PLATFORM_XCB], [1], [Define this symbol if the qt platform is xcb])
    elif test "$TARGET_OS" = "darwin"; then
      AX_CHECK_LINK_FLAG([-framework Carbon], [QT_LIBS="$QT_LIBS -framework Carbon"], [AC_MSG_ERROR(could not link against Carbon framework)])
      AX_CHECK_LINK_FLAG([-framework IOSurface], [QT_LIBS="$QT_LIBS -framework IOSurface"], [AC_MSG_ERROR(could not link against IOSurface framework)])
      AX_CHECK_LINK_FLAG([-framework Metal], [QT_LIBS="$QT_LIBS -framework Metal"], [AC_MSG_ERROR(could not link against Metal framework)])
      AX_CHECK_LINK_FLAG([-framework QuartzCore], [QT_LIBS="$QT_LIBS -framework QuartzCore"], [AC_MSG_ERROR(could not link against QuartzCore framework)])
      _BITCOIN_QT_CHECK_STATIC_PLUGIN([QCocoaIntegrationPlugin], [-lqcocoa])
      _BITCOIN_QT_CHECK_STATIC_PLUGIN([QMacStylePlugin], [-lqmacstyle])
      AC_DEFINE([QT_QPA_PLATFORM_COCOA], [1], [Define this symbol if the qt platform is cocoa])
    fi
  fi
  CPPFLAGS=$TEMP_CPPFLAGS
  CXXFLAGS=$TEMP_CXXFLAGS
  ])

  if test "$qt_bin_path" = ""; then
    qt_bin_path="`$PKG_CONFIG --variable=host_bins ${qt_lib_prefix}Core 2>/dev/null`"
  fi

  if test "$use_hardening" != "no"; then
    BITCOIN_QT_CHECK([
    AC_MSG_CHECKING([whether -fPIE can be used with this Qt config])
    TEMP_CPPFLAGS=$CPPFLAGS
    TEMP_CXXFLAGS=$CXXFLAGS
    CPPFLAGS="$QT_INCLUDES $CORE_CPPFLAGS $CPPFLAGS"
    CXXFLAGS="$PIE_FLAGS $CORE_CXXFLAGS $CXXFLAGS"
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
      [ AC_MSG_RESULT([yes]); QT_PIE_FLAGS=$PIE_FLAGS ],
      [ AC_MSG_RESULT([no]); QT_PIE_FLAGS=$PIC_FLAGS]
    )
    CPPFLAGS=$TEMP_CPPFLAGS
    CXXFLAGS=$TEMP_CXXFLAGS
    ])
  else
    BITCOIN_QT_CHECK([
    AC_MSG_CHECKING([whether -fPIC is needed with this Qt config])
    TEMP_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$QT_INCLUDES $CORE_CPPFLAGS $CPPFLAGS"
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
      [ AC_MSG_RESULT([no])],
      [ AC_MSG_RESULT([yes]); QT_PIE_FLAGS=$PIC_FLAGS]
    )
    CPPFLAGS=$TEMP_CPPFLAGS
    ])
  fi

  BITCOIN_QT_PATH_PROGS([MOC], [moc-qt5 moc5 moc], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([UIC], [uic-qt5 uic5 uic], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([RCC], [rcc-qt5 rcc5 rcc], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([LRELEASE], [lrelease-qt5 lrelease5 lrelease], $qt_bin_path)
  BITCOIN_QT_PATH_PROGS([LUPDATE], [lupdate-qt5 lupdate5 lupdate],$qt_bin_path, yes)
  BITCOIN_QT_PATH_PROGS([LCONVERT], [lconvert-qt5 lconvert5 lconvert], $qt_bin_path, yes)

  MOC_DEFS='-I$(srcdir)'
  case $host in
    *darwin*)
     BITCOIN_QT_CHECK([
       MOC_DEFS="${MOC_DEFS} -DQ_OS_MAC"
       base_frameworks="-framework Foundation -framework AppKit"
       AX_CHECK_LINK_FLAG([$base_frameworks], [QT_LIBS="$QT_LIBS $base_frameworks"], [AC_MSG_ERROR(could not find base frameworks)])
     ])
    ;;
    *mingw*)
       BITCOIN_QT_CHECK([
         AX_CHECK_LINK_FLAG([-mwindows], [QT_LDFLAGS="$QT_LDFLAGS -mwindows"], [AC_MSG_WARN([-mwindows linker support not detected])])
       ])
  esac


  dnl enable qt support
  AC_MSG_CHECKING([whether to build ]AC_PACKAGE_NAME[ GUI])
  BITCOIN_QT_CHECK([
    bitcoin_enable_qt=yes
    bitcoin_enable_qt_test=yes
    if test "$have_qt_test" = "no"; then
      bitcoin_enable_qt_test=no
    fi
    bitcoin_enable_qt_dbus=no
    if test "$use_dbus" != "no" && test "$have_qt_dbus" = "yes"; then
      bitcoin_enable_qt_dbus=yes
    fi
    if test "$use_dbus" = "yes" && test "$have_qt_dbus" = "no"; then
      AC_MSG_ERROR([libQtDBus not found. Install libQtDBus or remove --with-qtdbus.])
    fi
    if test "$LUPDATE" = ""; then
      AC_MSG_WARN([lupdate tool is required to update Qt translations.])
    fi
    if test "$LCONVERT" = ""; then
      AC_MSG_WARN([lconvert tool is required to update Qt translations.])
    fi
  ],[
    bitcoin_enable_qt=no
  ])
  if test $bitcoin_enable_qt = "yes"; then
    AC_MSG_RESULT([$bitcoin_enable_qt ($qt_lib_prefix)])
  else
    AC_MSG_RESULT([$bitcoin_enable_qt])
  fi

  AC_SUBST(QT_PIE_FLAGS)
  AC_SUBST(QT_INCLUDES)
  AC_SUBST(QT_LIBS)
  AC_SUBST(QT_LDFLAGS)
  AC_SUBST(QT_DBUS_INCLUDES)
  AC_SUBST(QT_TEST_INCLUDES)
  AC_SUBST(QT_SELECT, qt5)
  AC_SUBST(MOC_DEFS)
])

dnl All macros below are internal and should _not_ be used from configure.ac.

dnl Internal. Check if the linked version of Qt was built statically.
dnl
dnl _BITCOIN_QT_IS_STATIC
dnl ---------------------
dnl
dnl Requires: INCLUDES and LIBS must be populated as necessary.
dnl Output: bitcoin_cv_static_qt=yes|no
AC_DEFUN([_BITCOIN_QT_IS_STATIC],[
  AC_CACHE_CHECK(for static Qt, bitcoin_cv_static_qt,[
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
      [bitcoin_cv_static_qt=yes],
      [bitcoin_cv_static_qt=no])
    ])
])

dnl Internal. Check if the link-requirements for a static plugin are met.
dnl
dnl _BITCOIN_QT_CHECK_STATIC_PLUGIN(PLUGIN, LIBRARIES)
dnl --------------------------------------------------
dnl
dnl Requires: INCLUDES and LIBS must be populated as necessary.
dnl Inputs: $1: A static plugin name.
dnl Inputs: $2: The libraries that resolve $1.
dnl Output: QT_LIBS is prepended or configure exits.
AC_DEFUN([_BITCOIN_QT_CHECK_STATIC_PLUGIN], [
  AC_MSG_CHECKING([for $1 ($2)])
  CHECK_STATIC_PLUGINS_TEMP_LIBS="$LIBS"
  LIBS="$2${qt_lib_suffix} $QT_LIBS $LIBS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
      #include <QtPlugin>
      Q_IMPORT_PLUGIN($1)
    ]])],
    [AC_MSG_RESULT([yes]); QT_LIBS="$2${qt_lib_suffix} $QT_LIBS"],
    [AC_MSG_RESULT([no]); BITCOIN_QT_FAIL([$1 not found.])])
  LIBS="$CHECK_STATIC_PLUGINS_TEMP_LIBS"
])

dnl Internal. Check Qt static libs with PKG_CHECK_MODULES.
dnl
dnl _BITCOIN_QT_CHECK_STATIC_LIBS
dnl -----------------------------
dnl
dnl Outputs: QT_LIBS is prepended.
AC_DEFUN([_BITCOIN_QT_CHECK_STATIC_LIBS], [
  PKG_CHECK_MODULES([QT_ACCESSIBILITY], [${qt_lib_prefix}AccessibilitySupport${qt_lib_suffix}], [QT_LIBS="$QT_ACCESSIBILITY_LIBS $QT_LIBS"])
  PKG_CHECK_MODULES([QT_DEVICEDISCOVERY], [${qt_lib_prefix}DeviceDiscoverySupport${qt_lib_suffix}], [QT_LIBS="$QT_DEVICEDISCOVERY_LIBS $QT_LIBS"])
  PKG_CHECK_MODULES([QT_EDID], [${qt_lib_prefix}EdidSupport${qt_lib_suffix}], [QT_LIBS="$QT_EDID_LIBS $QT_LIBS"])
  PKG_CHECK_MODULES([QT_EVENTDISPATCHER], [${qt_lib_prefix}EventDispatcherSupport${qt_lib_suffix}], [QT_LIBS="$QT_EVENTDISPATCHER_LIBS $QT_LIBS"])
  PKG_CHECK_MODULES([QT_FB], [${qt_lib_prefix}FbSupport${qt_lib_suffix}], [QT_LIBS="$QT_FB_LIBS $QT_LIBS"])
  PKG_CHECK_MODULES([QT_FONTDATABASE], [${qt_lib_prefix}FontDatabaseSupport${qt_lib_suffix}], [QT_LIBS="$QT_FONTDATABASE_LIBS $QT_LIBS"])
  PKG_CHECK_MODULES([QT_THEME], [${qt_lib_prefix}ThemeSupport${qt_lib_suffix}], [QT_LIBS="$QT_THEME_LIBS $QT_LIBS"])
  if test "$TARGET_OS" = "linux"; then
    PKG_CHECK_MODULES([QT_INPUT], [${qt_lib_prefix}InputSupport], [QT_LIBS="$QT_INPUT_LIBS $QT_LIBS"])
    PKG_CHECK_MODULES([QT_SERVICE], [${qt_lib_prefix}ServiceSupport], [QT_LIBS="$QT_SERVICE_LIBS $QT_LIBS"])
    PKG_CHECK_MODULES([QT_XCBQPA], [${qt_lib_prefix}XcbQpa], [QT_LIBS="$QT_XCBQPA_LIBS $QT_LIBS"])
    PKG_CHECK_MODULES([QT_XKBCOMMON], [${qt_lib_prefix}XkbCommonSupport], [QT_LIBS="$QT_XKBCOMMON_LIBS $QT_LIBS"])
  elif test "$TARGET_OS" = "darwin"; then
    PKG_CHECK_MODULES([QT_CLIPBOARD], [${qt_lib_prefix}ClipboardSupport${qt_lib_suffix}], [QT_LIBS="$QT_CLIPBOARD_LIBS $QT_LIBS"])
    PKG_CHECK_MODULES([QT_GRAPHICS], [${qt_lib_prefix}GraphicsSupport${qt_lib_suffix}], [QT_LIBS="$QT_GRAPHICS_LIBS $QT_LIBS"])
    PKG_CHECK_MODULES([QT_SERVICE], [${qt_lib_prefix}ServiceSupport${qt_lib_suffix}], [QT_LIBS="$QT_SERVICE_LIBS $QT_LIBS"])
  elif test "$TARGET_OS" = "windows"; then
    PKG_CHECK_MODULES([QT_WINDOWSUIAUTOMATION], [${qt_lib_prefix}WindowsUIAutomationSupport${qt_lib_suffix}], [QT_LIBS="$QT_WINDOWSUIAUTOMATION_LIBS $QT_LIBS"])
  fi
])

dnl Internal. Find Qt libraries using pkg-config.
dnl
dnl _BITCOIN_QT_FIND_LIBS
dnl ---------------------
dnl
dnl Outputs: All necessary QT_* variables are set.
dnl Outputs: have_qt_test and have_qt_dbus are set (if applicable) to yes|no.
AC_DEFUN([_BITCOIN_QT_FIND_LIBS],[
  BITCOIN_QT_CHECK([
    PKG_CHECK_MODULES([QT_CORE], [${qt_lib_prefix}Core${qt_lib_suffix} $qt_version], [QT_INCLUDES="$QT_CORE_CFLAGS $QT_INCLUDES" QT_LIBS="$QT_CORE_LIBS $QT_LIBS"],
                      [BITCOIN_QT_FAIL([${qt_lib_prefix}Core${qt_lib_suffix} $qt_version not found])])
  ])
  BITCOIN_QT_CHECK([
    PKG_CHECK_MODULES([QT_GUI], [${qt_lib_prefix}Gui${qt_lib_suffix} $qt_version], [QT_INCLUDES="$QT_GUI_CFLAGS $QT_INCLUDES" QT_LIBS="$QT_GUI_LIBS $QT_LIBS"],
                      [BITCOIN_QT_FAIL([${qt_lib_prefix}Gui${qt_lib_suffix} $qt_version not found])])
  ])
  BITCOIN_QT_CHECK([
    PKG_CHECK_MODULES([QT_WIDGETS], [${qt_lib_prefix}Widgets${qt_lib_suffix} $qt_version], [QT_INCLUDES="$QT_WIDGETS_CFLAGS $QT_INCLUDES" QT_LIBS="$QT_WIDGETS_LIBS $QT_LIBS"],
                      [BITCOIN_QT_FAIL([${qt_lib_prefix}Widgets${qt_lib_suffix} $qt_version not found])])
  ])
  BITCOIN_QT_CHECK([
    PKG_CHECK_MODULES([QT_NETWORK], [${qt_lib_prefix}Network${qt_lib_suffix} $qt_version], [QT_INCLUDES="$QT_NETWORK_CFLAGS $QT_INCLUDES" QT_LIBS="$QT_NETWORK_LIBS $QT_LIBS"],
                      [BITCOIN_QT_FAIL([${qt_lib_prefix}Network${qt_lib_suffix} $qt_version not found])])
  ])

  BITCOIN_QT_CHECK([
    PKG_CHECK_MODULES([QT_TEST], [${qt_lib_prefix}Test${qt_lib_suffix} $qt_version], [QT_TEST_INCLUDES="$QT_TEST_CFLAGS"; have_qt_test=yes], [have_qt_test=no])
    if test "$use_dbus" != "no"; then
      PKG_CHECK_MODULES([QT_DBUS], [${qt_lib_prefix}DBus $qt_version], [QT_DBUS_INCLUDES="$QT_DBUS_CFLAGS"; have_qt_dbus=yes], [have_qt_dbus=no])
    fi
  ])
])
