dnl ---------------------------------------------------------------------------
dnl Author:          wxWidgets development team,
dnl                  Francesco Montorsi,
dnl                  Bob McCown (Mac-testing)
dnl Creation date:   24/11/2001
dnl RCS-ID:          $Id$
dnl ---------------------------------------------------------------------------

dnl ===========================================================================
dnl Table of Contents of this macro file:
dnl -------------------------------------
dnl
dnl SECTION A: wxWidgets main macros
dnl  - WX_CONFIG_OPTIONS
dnl  - WX_CONFIG_CHECK
dnl  - WXRC_CHECK
dnl  - WX_STANDARD_OPTIONS
dnl  - WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS
dnl  - WX_DETECT_STANDARD_OPTION_VALUES
dnl
dnl SECTION B: wxWidgets-related utilities
dnl  - WX_LIKE_LIBNAME
dnl  - WX_ARG_ENABLE_YESNOAUTO
dnl  - WX_ARG_WITH_YESNOAUTO
dnl
dnl SECTION C: messages to the user
dnl  - WX_STANDARD_OPTIONS_SUMMARY_MSG
dnl  - WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN
dnl  - WX_STANDARD_OPTIONS_SUMMARY_MSG_END
dnl  - WX_BOOLOPT_SUMMARY
dnl
dnl The special "WX_DEBUG_CONFIGURE" variable can be set to 1 to enable extra
dnl debug output on stdout from these macros.
dnl ===========================================================================


dnl ---------------------------------------------------------------------------
dnl Macros for wxWidgets detection. Typically used in configure.in as:
dnl
dnl     AC_ARG_ENABLE(...)
dnl     AC_ARG_WITH(...)
dnl        ...
dnl     WX_CONFIG_OPTIONS
dnl        ...
dnl        ...
dnl     WX_CONFIG_CHECK([2.6.0], [wxWin=1])
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl                wxWidgets must be installed on your system
dnl                but wx-config script couldn't be found.
dnl
dnl                Please check that wx-config is in path, the directory
dnl                where wxWidgets libraries are installed (returned by
dnl                'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl                equivalent variable and wxWidgets version is 2.3.4 or above.
dnl        ])
dnl     fi
dnl     CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
dnl
dnl     LIBS="$LIBS $WX_LIBS"
dnl
dnl If you want to support standard --enable-debug/unicode/shared options, you
dnl may do the following:
dnl
dnl     ...
dnl     AC_CANONICAL_SYSTEM
dnl
dnl     # define configure options
dnl     WX_CONFIG_OPTIONS
dnl     WX_STANDARD_OPTIONS([debug,unicode,shared,toolkit,wxshared])
dnl
dnl     # basic configure checks
dnl     ...
dnl
dnl     # we want to always have DEBUG==WX_DEBUG and UNICODE==WX_UNICODE
dnl     WX_DEBUG=$DEBUG
dnl     WX_UNICODE=$UNICODE
dnl
dnl     WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS
dnl     WX_CONFIG_CHECK([2.8.0], [wxWin=1],,[html,core,net,base],[$WXCONFIG_FLAGS])
dnl     WX_DETECT_STANDARD_OPTION_VALUES
dnl
dnl     # write the output files
dnl     AC_CONFIG_FILES([Makefile ...])
dnl     AC_OUTPUT
dnl
dnl     # optional: just to show a message to the user
dnl     WX_STANDARD_OPTIONS_SUMMARY_MSG
dnl
dnl ---------------------------------------------------------------------------


dnl ---------------------------------------------------------------------------
dnl WX_CONFIG_OPTIONS
dnl
dnl adds support for --wx-prefix, --wx-exec-prefix, --with-wxdir and
dnl --wx-config command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([WX_CONFIG_OPTIONS],
[
    AC_ARG_WITH(wxdir,
                [  --with-wxdir=PATH       Use uninstalled version of wxWidgets in PATH],
                [ wx_config_name="$withval/wx-config"
                  wx_config_args="--inplace"])
    AC_ARG_WITH(wx-config,
                [  --with-wx-config=CONFIG wx-config script to use (optional)],
                wx_config_name="$withval" )
    AC_ARG_WITH(wx-prefix,
                [  --with-wx-prefix=PREFIX Prefix where wxWidgets is installed (optional)],
                wx_config_prefix="$withval", wx_config_prefix="")
    AC_ARG_WITH(wx-exec-prefix,
                [  --with-wx-exec-prefix=PREFIX
                          Exec prefix where wxWidgets is installed (optional)],
                wx_config_exec_prefix="$withval", wx_config_exec_prefix="")
])

dnl Helper macro for checking if wx version is at least $1.$2.$3, set's
dnl wx_ver_ok=yes if it is:
AC_DEFUN([_WX_PRIVATE_CHECK_VERSION],
[
    wx_ver_ok=""
    if test "x$WX_VERSION" != x ; then
      if test $wx_config_major_version -gt $1; then
        wx_ver_ok=yes
      else
        if test $wx_config_major_version -eq $1; then
           if test $wx_config_minor_version -gt $2; then
              wx_ver_ok=yes
           else
              if test $wx_config_minor_version -eq $2; then
                 if test $wx_config_micro_version -ge $3; then
                    wx_ver_ok=yes
                 fi
              fi
           fi
        fi
      fi
    fi
])

dnl ---------------------------------------------------------------------------
dnl WX_CONFIG_CHECK(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl                  [, WX-LIBS [, ADDITIONAL-WX-CONFIG-FLAGS]]]])
dnl
dnl Test for wxWidgets, and define WX_C*FLAGS, WX_LIBS and WX_LIBS_STATIC
dnl (the latter is for static linking against wxWidgets). Set WX_CONFIG_NAME
dnl environment variable to override the default name of the wx-config script
dnl to use. Set WX_CONFIG_PATH to specify the full path to wx-config - in this
dnl case the macro won't even waste time on tests for its existence.
dnl
dnl Optional WX-LIBS argument contains comma- or space-separated list of
dnl wxWidgets libraries to link against. If it is not specified then WX_LIBS
dnl and WX_LIBS_STATIC will contain flags to link with all of the core
dnl wxWidgets libraries.
dnl
dnl Optional ADDITIONAL-WX-CONFIG-FLAGS argument is appended to wx-config
dnl invocation command in present. It can be used to fine-tune lookup of
dnl best wxWidgets build available.
dnl
dnl Example use:
dnl   WX_CONFIG_CHECK([2.6.0], [wxWin=1], [wxWin=0], [html,core,net]
dnl                    [--unicode --debug])
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the wx-config script
dnl
AC_DEFUN([WX_CONFIG_CHECK],
[
  dnl do we have wx-config name: it can be wx-config or wxd-config or ...
  if test x${WX_CONFIG_NAME+set} != xset ; then
     WX_CONFIG_NAME=wx-config
  fi

  if test "x$wx_config_name" != x ; then
     WX_CONFIG_NAME="$wx_config_name"
  fi

  dnl deal with optional prefixes
  if test x$wx_config_exec_prefix != x ; then
     wx_config_args="$wx_config_args --exec-prefix=$wx_config_exec_prefix"
     WX_LOOKUP_PATH="$wx_config_exec_prefix/bin"
  fi
  if test x$wx_config_prefix != x ; then
     wx_config_args="$wx_config_args --prefix=$wx_config_prefix"
     WX_LOOKUP_PATH="$WX_LOOKUP_PATH:$wx_config_prefix/bin"
  fi
  if test "$cross_compiling" = "yes"; then
     wx_config_args="$wx_config_args --host=$host_alias"
  fi

  dnl don't search the PATH if WX_CONFIG_NAME is absolute filename
  if test -x "$WX_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for wx-config)
     WX_CONFIG_PATH="$WX_CONFIG_NAME"
     AC_MSG_RESULT($WX_CONFIG_PATH)
  else
     AC_PATH_PROG(WX_CONFIG_PATH, $WX_CONFIG_NAME, no, "$WX_LOOKUP_PATH:$PATH")
  fi

  if test "$WX_CONFIG_PATH" != "no" ; then
    WX_VERSION=""

    min_wx_version=ifelse([$1], ,2.2.1,$1)
    if test -z "$5" ; then
      AC_MSG_CHECKING([for wxWidgets version >= $min_wx_version])
    else
      AC_MSG_CHECKING([for wxWidgets version >= $min_wx_version ($5)])
    fi

    dnl don't add the libraries ($4) to this variable as this would result in
    dnl an error when it's used with --version below
    WX_CONFIG_WITH_ARGS="$WX_CONFIG_PATH $wx_config_args $5"

    WX_VERSION=`$WX_CONFIG_WITH_ARGS --version 2>/dev/null`
    wx_config_major_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_config_minor_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_config_micro_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wx_requested_major_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_requested_minor_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_requested_micro_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    _WX_PRIVATE_CHECK_VERSION([$wx_requested_major_version],
                              [$wx_requested_minor_version],
                              [$wx_requested_micro_version])

    if test -n "$wx_ver_ok"; then
      AC_MSG_RESULT(yes (version $WX_VERSION))
      WX_LIBS=`$WX_CONFIG_WITH_ARGS --libs $4`

      dnl is this even still appropriate?  --static is a real option now
      dnl and WX_CONFIG_WITH_ARGS is likely to contain it if that is
      dnl what the user actually wants, making this redundant at best.
      dnl For now keep it in case anyone actually used it in the past.
      AC_MSG_CHECKING([for wxWidgets static library])
      WX_LIBS_STATIC=`$WX_CONFIG_WITH_ARGS --static --libs $4 2>/dev/null`
      if test "x$WX_LIBS_STATIC" = "x"; then
        AC_MSG_RESULT(no)
      else
        AC_MSG_RESULT(yes)
      fi

      dnl starting with version 2.2.6 wx-config has --cppflags argument
      wx_has_cppflags=""
      if test $wx_config_major_version -gt 2; then
        wx_has_cppflags=yes
      else
        if test $wx_config_major_version -eq 2; then
           if test $wx_config_minor_version -gt 2; then
              wx_has_cppflags=yes
           else
              if test $wx_config_minor_version -eq 2; then
                 if test $wx_config_micro_version -ge 6; then
                    wx_has_cppflags=yes
                 fi
              fi
           fi
        fi
      fi

      dnl starting with version 2.7.0 wx-config has --rescomp option
      wx_has_rescomp=""
      if test $wx_config_major_version -gt 2; then
        wx_has_rescomp=yes
      else
        if test $wx_config_major_version -eq 2; then
           if test $wx_config_minor_version -ge 7; then
              wx_has_rescomp=yes
           fi
        fi
      fi
      if test "x$wx_has_rescomp" = x ; then
         dnl cannot give any useful info for resource compiler
         WX_RESCOMP=
      else
         WX_RESCOMP=`$WX_CONFIG_WITH_ARGS --rescomp`
      fi

      if test "x$wx_has_cppflags" = x ; then
         dnl no choice but to define all flags like CFLAGS
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags $4`
         WX_CPPFLAGS=$WX_CFLAGS
         WX_CXXFLAGS=$WX_CFLAGS

         WX_CFLAGS_ONLY=$WX_CFLAGS
         WX_CXXFLAGS_ONLY=$WX_CFLAGS
      else
         dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
         WX_CPPFLAGS=`$WX_CONFIG_WITH_ARGS --cppflags $4`
         WX_CXXFLAGS=`$WX_CONFIG_WITH_ARGS --cxxflags $4`
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags $4`

         WX_CFLAGS_ONLY=`echo $WX_CFLAGS | sed "s@^$WX_CPPFLAGS *@@"`
         WX_CXXFLAGS_ONLY=`echo $WX_CXXFLAGS | sed "s@^$WX_CFLAGS *@@"`
      fi

      ifelse([$2], , :, [$2])

    else

       if test "x$WX_VERSION" = x; then
          dnl no wx-config at all
          AC_MSG_RESULT(no)
       else
          AC_MSG_RESULT(no (version $WX_VERSION is not new enough))
       fi

       WX_CFLAGS=""
       WX_CPPFLAGS=""
       WX_CXXFLAGS=""
       WX_LIBS=""
       WX_LIBS_STATIC=""
       WX_RESCOMP=""

       if test ! -z "$5"; then

          wx_error_message="
    The configuration you asked for $PACKAGE_NAME requires a wxWidgets
    build with the following settings:
        $5
    but such build is not available.

    To see the wxWidgets builds available on this system, please use
    'wx-config --list' command. To use the default build, returned by
    'wx-config --selected-config', use the options with their 'auto'
    default values."

       fi

       wx_error_message="
    The requested wxWidgets build couldn't be found.
    $wx_error_message

    If you still get this error, then check that 'wx-config' is
    in path, the directory where wxWidgets libraries are installed
    (returned by 'wx-config --libs' command) is in LD_LIBRARY_PATH
    or equivalent variable and wxWidgets version is $1 or above."

       ifelse([$3], , AC_MSG_ERROR([$wx_error_message]), [$3])

    fi
  else

    WX_CFLAGS=""
    WX_CPPFLAGS=""
    WX_CXXFLAGS=""
    WX_LIBS=""
    WX_LIBS_STATIC=""
    WX_RESCOMP=""

    ifelse([$3], , :, [$3])

  fi

  AC_SUBST(WX_CPPFLAGS)
  AC_SUBST(WX_CFLAGS)
  AC_SUBST(WX_CXXFLAGS)
  AC_SUBST(WX_CFLAGS_ONLY)
  AC_SUBST(WX_CXXFLAGS_ONLY)
  AC_SUBST(WX_LIBS)
  AC_SUBST(WX_LIBS_STATIC)
  AC_SUBST(WX_VERSION)
  AC_SUBST(WX_RESCOMP)

  dnl need to export also WX_VERSION_MINOR and WX_VERSION_MAJOR symbols
  dnl to support wxpresets bakefiles (we export also WX_VERSION_MICRO for completeness):
  WX_VERSION_MAJOR="$wx_config_major_version"
  WX_VERSION_MINOR="$wx_config_minor_version"
  WX_VERSION_MICRO="$wx_config_micro_version"
  AC_SUBST(WX_VERSION_MAJOR)
  AC_SUBST(WX_VERSION_MINOR)
  AC_SUBST(WX_VERSION_MICRO)
])

dnl ---------------------------------------------------------------------------
dnl Get information on the wxrc program for making C++, Python and xrs
dnl resource files.
dnl
dnl     AC_ARG_ENABLE(...)
dnl     AC_ARG_WITH(...)
dnl        ...
dnl     WX_CONFIG_OPTIONS
dnl        ...
dnl     WX_CONFIG_CHECK(2.6.0, wxWin=1)
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl                wxWidgets must be installed on your system
dnl                but wx-config script couldn't be found.
dnl
dnl                Please check that wx-config is in path, the directory
dnl                where wxWidgets libraries are installed (returned by
dnl                'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl                equivalent variable and wxWidgets version is 2.6.0 or above.
dnl        ])
dnl     fi
dnl
dnl     WXRC_CHECK([HAVE_WXRC=1], [HAVE_WXRC=0])
dnl     if test "x$HAVE_WXRC" != x1; then
dnl         AC_MSG_ERROR([
dnl                The wxrc program was not installed or not found.
dnl
dnl                Please check the wxWidgets installation.
dnl         ])
dnl     fi
dnl
dnl     CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
dnl
dnl     LDFLAGS="$LDFLAGS $WX_LIBS"
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl WXRC_CHECK([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for wxWidgets' wxrc program for creating either C++, Python or XRS
dnl resources.  The variable WXRC will be set and substituted in the configure
dnl script and Makefiles.
dnl
dnl Example use:
dnl   WXRC_CHECK([wxrc=1], [wxrc=0])
dnl ---------------------------------------------------------------------------

dnl
dnl wxrc program from the wx-config script
dnl
AC_DEFUN([WXRC_CHECK],
[
  AC_ARG_VAR([WXRC], [Path to wxWidget's wxrc resource compiler])

  if test "x$WX_CONFIG_NAME" = x; then
    AC_MSG_ERROR([The wxrc tests must run after wxWidgets test.])
  else

    AC_MSG_CHECKING([for wxrc])

    if test "x$WXRC" = x ; then
      dnl wx-config --utility is a new addition to wxWidgets:
      _WX_PRIVATE_CHECK_VERSION(2,5,3)
      if test -n "$wx_ver_ok"; then
        WXRC=`$WX_CONFIG_WITH_ARGS --utility=wxrc`
      fi
    fi

    if test "x$WXRC" = x ; then
      AC_MSG_RESULT([not found])
      ifelse([$2], , :, [$2])
    else
      AC_MSG_RESULT([$WXRC])
      ifelse([$1], , :, [$1])
    fi

    AC_SUBST(WXRC)
  fi
])

dnl ---------------------------------------------------------------------------
dnl WX_LIKE_LIBNAME([output-var] [prefix], [name])
dnl
dnl Sets the "output-var" variable to the name of a library named with same
dnl wxWidgets rule.
dnl E.g. for output-var=='lib', name=='test', prefix='mine', sets
dnl      the $lib variable to:
dnl          'mine_gtk2ud_test-2.8'
dnl      if WX_PORT=gtk2, WX_UNICODE=1, WX_DEBUG=1 and WX_RELEASE=28
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_LIKE_LIBNAME],
    [
        wx_temp="$2""_""$WX_PORT"

        dnl add the [u][d] string
        if test "$WX_UNICODE" = "1"; then
            wx_temp="$wx_temp""u"
        fi
        if test "$WX_DEBUG" = "1"; then
            wx_temp="$wx_temp""d"
        fi

        dnl complete the name of the lib
        wx_temp="$wx_temp""_""$3""-$WX_VERSION_MAJOR.$WX_VERSION_MINOR"

        dnl save it in the user's variable
        $1=$wx_temp
    ])

dnl ---------------------------------------------------------------------------
dnl WX_ARG_ENABLE_YESNOAUTO/WX_ARG_WITH_YESNOAUTO
dnl
dnl Two little custom macros which define the ENABLE/WITH configure arguments.
dnl Macro arguments:
dnl $1 = the name of the --enable / --with  feature
dnl $2 = the name of the variable associated
dnl $3 = the description of that feature
dnl $4 = the default value for that feature
dnl $5 = additional action to do in case option is given with "yes" value
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_ARG_ENABLE_YESNOAUTO],
         [AC_ARG_ENABLE($1,
            AC_HELP_STRING([--enable-$1], [$3 (default is $4)]),
            [], [enableval="$4"])

            dnl Show a message to the user about this option
            AC_MSG_CHECKING([for the --enable-$1 option])
            if test "$enableval" = "yes" ; then
                AC_MSG_RESULT([yes])
                $2=1
                $5
            elif test "$enableval" = "no" ; then
                AC_MSG_RESULT([no])
                $2=0
            elif test "$enableval" = "auto" ; then
                AC_MSG_RESULT([will be automatically detected])
                $2="auto"
            else
                AC_MSG_ERROR([
    Unrecognized option value (allowed values: yes, no, auto)
                ])
            fi
         ])

AC_DEFUN([WX_ARG_WITH_YESNOAUTO],
         [AC_ARG_WITH($1,
            AC_HELP_STRING([--with-$1], [$3 (default is $4)]),
            [], [withval="$4"])

            dnl Show a message to the user about this option
            AC_MSG_CHECKING([for the --with-$1 option])
            if test "$withval" = "yes" ; then
                AC_MSG_RESULT([yes])
                $2=1
                $5
            dnl NB: by default we don't allow --with-$1=no option
            dnl     since it does not make much sense !
            elif test "$6" = "1" -a "$withval" = "no" ; then
                AC_MSG_RESULT([no])
                $2=0
            elif test "$withval" = "auto" ; then
                AC_MSG_RESULT([will be automatically detected])
                $2="auto"
            else
                AC_MSG_ERROR([
    Unrecognized option value (allowed values: yes, auto)
                ])
            fi
         ])


dnl ---------------------------------------------------------------------------
dnl WX_STANDARD_OPTIONS([options-to-add])
dnl
dnl Adds to the configure script one or more of the following options:
dnl   --enable-[debug|unicode|shared|wxshared|wxdebug]
dnl   --with-[gtk|msw|motif|x11|mac|mgl|dfb]
dnl   --with-wxversion
dnl Then checks for their presence and eventually set the DEBUG, UNICODE, SHARED,
dnl PORT, WX_SHARED, WX_DEBUG, variables to one of the "yes", "no", "auto" values.
dnl
dnl Note that e.g. UNICODE != WX_UNICODE; the first is the value of the
dnl --enable-unicode option (in boolean format) while the second indicates
dnl if wxWidgets was built in Unicode mode (and still is in boolean format).
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_STANDARD_OPTIONS],
        [

        dnl the following lines will expand to WX_ARG_ENABLE_YESNOAUTO calls if and only if
        dnl the $1 argument contains respectively the debug,unicode or shared options.

        dnl be careful here not to set debug flag if only "wxdebug" was specified
        ifelse(regexp([$1], [\bdebug]), [-1],,
               [WX_ARG_ENABLE_YESNOAUTO([debug], [DEBUG], [Build in debug mode], [auto])])

        ifelse(index([$1], [unicode]), [-1],,
               [WX_ARG_ENABLE_YESNOAUTO([unicode], [UNICODE], [Build in Unicode mode], [auto])])

        ifelse(regexp([$1], [\bshared]), [-1],,
               [WX_ARG_ENABLE_YESNOAUTO([shared], [SHARED], [Build as shared library], [auto])])

        dnl WX_ARG_WITH_YESNOAUTO cannot be used for --with-toolkit since it's an option
        dnl which must be able to accept the auto|gtk1|gtk2|msw|... values
        ifelse(index([$1], [toolkit]), [-1],,
               [
                AC_ARG_WITH([toolkit],
                            AC_HELP_STRING([--with-toolkit],
                                           [Build against a specific wxWidgets toolkit (default is auto)]),
                            [], [withval="auto"])

                dnl Show a message to the user about this option
                AC_MSG_CHECKING([for the --with-toolkit option])
                if test "$withval" = "auto" ; then
                    AC_MSG_RESULT([will be automatically detected])
                    TOOLKIT="auto"
                else
                    TOOLKIT="$withval"

                    dnl PORT must be one of the allowed values
                    if test "$TOOLKIT" != "gtk1" -a "$TOOLKIT" != "gtk2" -a \
                            "$TOOLKIT" != "msw" -a "$TOOLKIT" != "motif" -a \
                            "$TOOLKIT" != "x11" -a "$TOOLKIT" != "mac" -a \
                            "$TOOLKIT" != "mgl" -a "$TOOLKIT" != "dfb" ; then
                        AC_MSG_ERROR([
    Unrecognized option value (allowed values: auto, gtk1, gtk2, msw, motif, x11, mac, mgl, dfb)
                        ])
                    fi

                    AC_MSG_RESULT([$TOOLKIT])
                fi
               ])

        dnl ****** IMPORTANT *******
        dnl   Unlike for the UNICODE setting, you can build your program in
        dnl   shared mode against a static build of wxWidgets. Thus we have the
        dnl   following option which allows these mixtures. E.g.
        dnl
        dnl      ./configure --disable-shared --with-wxshared
        dnl
        dnl   will build your library in static mode against the first available
        dnl   shared build of wxWidgets.
        dnl
        dnl   Note that's not possible to do the viceversa:
        dnl
        dnl      ./configure --enable-shared --without-wxshared
        dnl
        dnl   Doing so you would try to build your library in shared mode against a static
        dnl   build of wxWidgets. This is not possible (you would mix PIC and non PIC code) !
        dnl   A check for this combination of options is in WX_DETECT_STANDARD_OPTION_VALUES
        dnl   (where we know what 'auto' should be expanded to).
        dnl
        dnl   If you try to build something in ANSI mode against a UNICODE build
        dnl   of wxWidgets or in RELEASE mode against a DEBUG build of wxWidgets,
        dnl   then at best you'll get ton of linking errors !
        dnl ************************

        ifelse(index([$1], [wxshared]), [-1],,
               [
                WX_ARG_WITH_YESNOAUTO(
                    [wxshared], [WX_SHARED],
                    [Force building against a shared build of wxWidgets, even if --disable-shared is given],
                    [auto], [], [1])
               ])

        dnl Just like for SHARED and WX_SHARED it may happen that some adventurous
        dnl peoples will want to mix a wxWidgets release build with a debug build of
        dnl his app/lib. So, we have both DEBUG and WX_DEBUG variables.
        ifelse(index([$1], [wxdebug]), [-1],,
               [
                WX_ARG_WITH_YESNOAUTO(
                    [wxdebug], [WX_DEBUG],
                    [Force building against a debug build of wxWidgets, even if --disable-debug is given],
                    [auto], [], [1])
               ])

        dnl WX_ARG_WITH_YESNOAUTO cannot be used for --with-wxversion since it's an option
        dnl which accepts the "auto|2.6|2.7|2.8|2.9|3.0" etc etc values
        ifelse(index([$1], [wxversion]), [-1],,
               [
                AC_ARG_WITH([wxversion],
                            AC_HELP_STRING([--with-wxversion],
                                           [Build against a specific version of wxWidgets (default is auto)]),
                            [], [withval="auto"])

                dnl Show a message to the user about this option
                AC_MSG_CHECKING([for the --with-wxversion option])
                if test "$withval" = "auto" ; then
                    AC_MSG_RESULT([will be automatically detected])
                    WX_RELEASE="auto"
                else

                    wx_requested_major_version=`echo $withval | \
                        sed 's/\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
                    wx_requested_minor_version=`echo $withval | \
                        sed 's/\([[0-9]]*\).\([[0-9]]*\).*/\2/'`

                    dnl both vars above must be exactly 1 digit
                    if test "${#wx_requested_major_version}" != "1" -o \
                            "${#wx_requested_minor_version}" != "1" ; then
                        AC_MSG_ERROR([
    Unrecognized option value (allowed values: auto, 2.6, 2.7, 2.8, 2.9, 3.0)
                        ])
                    fi

                    WX_RELEASE="$wx_requested_major_version"".""$wx_requested_minor_version"
                    AC_MSG_RESULT([$WX_RELEASE])
                fi
               ])

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] DEBUG: $DEBUG, WX_DEBUG: $WX_DEBUG"
            echo "[[dbg]] UNICODE: $UNICODE, WX_UNICODE: $WX_UNICODE"
            echo "[[dbg]] SHARED: $SHARED, WX_SHARED: $WX_SHARED"
            echo "[[dbg]] TOOLKIT: $TOOLKIT, WX_TOOLKIT: $WX_TOOLKIT"
            echo "[[dbg]] VERSION: $VERSION, WX_RELEASE: $WX_RELEASE"
        fi
    ])


dnl ---------------------------------------------------------------------------
dnl WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS
dnl
dnl Sets the WXCONFIG_FLAGS string using the SHARED,DEBUG,UNICODE variable values
dnl which are different from "auto".
dnl Thus this macro needs to be called only once all options have been set.
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS],
        [
        if test "$WX_SHARED" = "1" ; then
            WXCONFIG_FLAGS="--static=no "
        elif test "$WX_SHARED" = "0" ; then
            WXCONFIG_FLAGS="--static=yes "
        fi

        if test "$WX_DEBUG" = "1" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--debug=yes "
        elif test "$WX_DEBUG" = "0" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--debug=no "
        fi

        dnl The user should have set WX_UNICODE=UNICODE
        if test "$WX_UNICODE" = "1" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--unicode=yes "
        elif test "$WX_UNICODE" = "0" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--unicode=no "
        fi

        if test "$TOOLKIT" != "auto" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--toolkit=$TOOLKIT "
        fi

        if test "$WX_RELEASE" != "auto" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--version=$WX_RELEASE "
        fi

        dnl strip out the last space of the string
        WXCONFIG_FLAGS=${WXCONFIG_FLAGS% }

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] WXCONFIG_FLAGS: $WXCONFIG_FLAGS"
        fi
    ])


dnl ---------------------------------------------------------------------------
dnl _WX_SELECTEDCONFIG_CHECKFOR([RESULTVAR], [STRING], [MSG]
dnl                             [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Outputs the given MSG. Then searches the given STRING in the wxWidgets
dnl additional CPP flags and put the result of the search in WX_$RESULTVAR
dnl also adding the "yes" or "no" message result to MSG.
dnl ---------------------------------------------------------------------------
AC_DEFUN([_WX_SELECTEDCONFIG_CHECKFOR],
        [
        if test "$$1" = "auto" ; then

            dnl The user does not have particular preferences for this option;
            dnl so we will detect the wxWidgets relative build setting and use it
            AC_MSG_CHECKING([$3])

            dnl set WX_$1 variable to 1 if the $WX_SELECTEDCONFIG contains the $2
            dnl string or to 0 otherwise.
            dnl NOTE: 'expr match STRING REGEXP' cannot be used since on Mac it
            dnl       doesn't work; we use 'expr STRING : REGEXP' instead
            WX_$1=$(expr "$WX_SELECTEDCONFIG" : ".*$2.*")

            if test "$WX_$1" != "0"; then
                WX_$1=1
                AC_MSG_RESULT([yes])
                ifelse([$4], , :, [$4])
            else
                WX_$1=0
                AC_MSG_RESULT([no])
                ifelse([$5], , :, [$5])
            fi
        else

            dnl Use the setting given by the user
            WX_$1=$$1
        fi
    ])

dnl ---------------------------------------------------------------------------
dnl WX_DETECT_STANDARD_OPTION_VALUES
dnl
dnl Detects the values of the following variables:
dnl 1) WX_RELEASE
dnl 2) WX_UNICODE
dnl 3) WX_DEBUG
dnl 4) WX_SHARED    (and also WX_STATIC)
dnl 5) WX_PORT
dnl from the previously selected wxWidgets build; this macro in fact must be
dnl called *after* calling the WX_CONFIG_CHECK macro.
dnl
dnl Note that the WX_VERSION_MAJOR, WX_VERSION_MINOR symbols are already set
dnl by WX_CONFIG_CHECK macro
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_DETECT_STANDARD_OPTION_VALUES],
        [
        dnl IMPORTANT: WX_VERSION contains all three major.minor.micro digits,
        dnl            while WX_RELEASE only the major.minor ones.
        WX_RELEASE="$WX_VERSION_MAJOR""$WX_VERSION_MINOR"
        if test $WX_RELEASE -lt 26 ; then

            AC_MSG_ERROR([
    Cannot detect the wxWidgets configuration for the selected wxWidgets build
    since its version is $WX_VERSION < 2.6.0; please install a newer
    version of wxWidgets.
                         ])
        fi

        dnl The wx-config we are using understands the "--selected_config"
        dnl option which returns an easy-parseable string !
        WX_SELECTEDCONFIG=$($WX_CONFIG_WITH_ARGS --selected_config)

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] Using wx-config --selected-config"
            echo "[[dbg]] WX_SELECTEDCONFIG: $WX_SELECTEDCONFIG"
        fi


        dnl we could test directly for WX_SHARED with a line like:
        dnl    _WX_SELECTEDCONFIG_CHECKFOR([SHARED], [shared],
        dnl                                [if wxWidgets was built in SHARED mode])
        dnl but wx-config --selected-config DOES NOT outputs the 'shared'
        dnl word when wx was built in shared mode; it rather outputs the
        dnl 'static' word when built in static mode.
        if test $WX_SHARED = "1"; then
            STATIC=0
        elif test $WX_SHARED = "0"; then
            STATIC=1
        elif test $WX_SHARED = "auto"; then
            STATIC="auto"
        fi

        dnl Now set the WX_UNICODE, WX_DEBUG, WX_STATIC variables
        _WX_SELECTEDCONFIG_CHECKFOR([UNICODE], [unicode],
                                    [if wxWidgets was built with UNICODE enabled])
        _WX_SELECTEDCONFIG_CHECKFOR([DEBUG], [debug],
                                    [if wxWidgets was built in DEBUG mode])
        _WX_SELECTEDCONFIG_CHECKFOR([STATIC], [static],
                                    [if wxWidgets was built in STATIC mode])

        dnl init WX_SHARED from WX_STATIC
        if test "$WX_STATIC" != "0"; then
            WX_SHARED=0
        else
            WX_SHARED=1
        fi

        AC_SUBST(WX_UNICODE)
        AC_SUBST(WX_DEBUG)
        AC_SUBST(WX_SHARED)

        dnl detect the WX_PORT to use
        if test "$TOOLKIT" = "auto" ; then

            dnl The user does not have particular preferences for this option;
            dnl so we will detect the wxWidgets relative build setting and use it
            AC_MSG_CHECKING([which wxWidgets toolkit was selected])

            WX_GTKPORT1=$(expr "$WX_SELECTEDCONFIG" : ".*gtk1.*")
            WX_GTKPORT2=$(expr "$WX_SELECTEDCONFIG" : ".*gtk2.*")
            WX_MSWPORT=$(expr "$WX_SELECTEDCONFIG" : ".*msw.*")
            WX_MOTIFPORT=$(expr "$WX_SELECTEDCONFIG" : ".*motif.*")
            WX_OSXCOCOAPORT=$(expr "$WX_SELECTEDCONFIG" : ".*osx_cocoa.*")
            WX_OSXCARBONPORT=$(expr "$WX_SELECTEDCONFIG" : ".*osx_carbon.*")
            WX_X11PORT=$(expr "$WX_SELECTEDCONFIG" : ".*x11.*")
            WX_MGLPORT=$(expr "$WX_SELECTEDCONFIG" : ".*mgl.*")
            WX_DFBPORT=$(expr "$WX_SELECTEDCONFIG" : ".*dfb.*")

            WX_PORT="unknown"
            if test "$WX_GTKPORT1" != "0"; then WX_PORT="gtk1"; fi
            if test "$WX_GTKPORT2" != "0"; then WX_PORT="gtk2"; fi
            if test "$WX_MSWPORT" != "0"; then WX_PORT="msw"; fi
            if test "$WX_MOTIFPORT" != "0"; then WX_PORT="motif"; fi
            if test "$WX_OSXCOCOAPORT" != "0"; then WX_PORT="osx_cocoa"; fi
            if test "$WX_OSXCARBONPORT" != "0"; then WX_PORT="osx_carbon"; fi
            if test "$WX_X11PORT" != "0"; then WX_PORT="x11"; fi
            if test "$WX_MGLPORT" != "0"; then WX_PORT="mgl"; fi
            if test "$WX_DFBPORT" != "0"; then WX_PORT="dfb"; fi

            dnl NOTE: backward-compatible check for wx2.8; in wx2.9 the mac
            dnl       ports are called 'osx_cocoa' and 'osx_carbon' (see above)
            WX_MACPORT=$(expr "$WX_SELECTEDCONFIG" : ".*mac.*")
            if test "$WX_MACPORT" != "0"; then WX_PORT="mac"; fi

            dnl check at least one of the WX_*PORT has been set !

            if test "$WX_PORT" = "unknown" ; then
                AC_MSG_ERROR([
        Cannot detect the currently installed wxWidgets port !
        Please check your 'wx-config --cxxflags'...
                            ])
            fi

            AC_MSG_RESULT([$WX_PORT])
        else

            dnl Use the setting given by the user
            if test -z "$TOOLKIT" ; then
                WX_PORT=$TOOLKIT
            else
                dnl try with PORT
                WX_PORT=$PORT
            fi
        fi

        AC_SUBST(WX_PORT)

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] Values of all WX_* options after final detection:"
            echo "[[dbg]] WX_DEBUG: $WX_DEBUG"
            echo "[[dbg]] WX_UNICODE: $WX_UNICODE"
            echo "[[dbg]] WX_SHARED: $WX_SHARED"
            echo "[[dbg]] WX_RELEASE: $WX_RELEASE"
            echo "[[dbg]] WX_PORT: $WX_PORT"
        fi

        dnl Avoid problem described in the WX_STANDARD_OPTIONS which happens when
        dnl the user gives the options:
        dnl      ./configure --enable-shared --without-wxshared
        dnl or just do
        dnl      ./configure --enable-shared
        dnl but there is only a static build of wxWidgets available.
        if test "$WX_SHARED" = "0" -a "$SHARED" = "1"; then
            AC_MSG_ERROR([
    Cannot build shared library against a static build of wxWidgets !
    This error happens because the wxWidgets build which was selected
    has been detected as static while you asked to build $PACKAGE_NAME
    as shared library and this is not possible.
    Use the '--disable-shared' option to build $PACKAGE_NAME
    as static library or '--with-wxshared' to use wxWidgets as shared library.
                         ])
        fi

        dnl now we can finally update the DEBUG,UNICODE,SHARED options
        dnl to their final values if they were set to 'auto'
        if test "$DEBUG" = "auto"; then
            DEBUG=$WX_DEBUG
        fi
        if test "$UNICODE" = "auto"; then
            UNICODE=$WX_UNICODE
        fi
        if test "$SHARED" = "auto"; then
            SHARED=$WX_SHARED
        fi
        if test "$TOOLKIT" = "auto"; then
            TOOLKIT=$WX_PORT
        fi

        dnl in case the user needs a BUILD=debug/release var...
        if test "$DEBUG" = "1"; then
            BUILD="debug"
        elif test "$DEBUG" = "0" -o "$DEBUG" = ""; then
            BUILD="release"
        fi

        dnl respect the DEBUG variable adding the optimize/debug flags
        dnl NOTE: the CXXFLAGS are merged together with the CPPFLAGS so we
        dnl       don't need to set them, too
        if test "$DEBUG" = "1"; then
            CXXFLAGS="$CXXFLAGS -g -O0"
            CFLAGS="$CFLAGS -g -O0"
        else
            CXXFLAGS="$CXXFLAGS -O2"
            CFLAGS="$CFLAGS -O2"
        fi
    ])

dnl ---------------------------------------------------------------------------
dnl WX_BOOLOPT_SUMMARY([name of the boolean variable to show summary for],
dnl                   [what to print when var is 1],
dnl                   [what to print when var is 0])
dnl
dnl Prints $2 when variable $1 == 1 and prints $3 when variable $1 == 0.
dnl This macro mainly exists just to make configure.ac scripts more readable.
dnl
dnl NOTE: you need to use the [" my message"] syntax for 2nd and 3rd arguments
dnl       if you want that m4 avoid to throw away the spaces prefixed to the
dnl       argument value.
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_BOOLOPT_SUMMARY],
        [
        if test "x$$1" = "x1" ; then
            echo $2
        elif test "x$$1" = "x0" ; then
            echo $3
        else
            echo "$1 is $$1"
        fi
    ])

dnl ---------------------------------------------------------------------------
dnl WX_STANDARD_OPTIONS_SUMMARY_MSG
dnl
dnl Shows a summary message to the user about the WX_* variable contents.
dnl This macro is used typically at the end of the configure script.
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_STANDARD_OPTIONS_SUMMARY_MSG],
        [
        echo
        echo "  The wxWidgets build which will be used by $PACKAGE_NAME $PACKAGE_VERSION"
        echo "  has the following settings:"
        WX_BOOLOPT_SUMMARY([WX_DEBUG],   ["  - DEBUG build"],  ["  - RELEASE build"])
        WX_BOOLOPT_SUMMARY([WX_UNICODE], ["  - UNICODE mode"], ["  - ANSI mode"])
        WX_BOOLOPT_SUMMARY([WX_SHARED],  ["  - SHARED mode"],  ["  - STATIC mode"])
        echo "  - VERSION: $WX_VERSION"
        echo "  - PORT: $WX_PORT"
    ])


dnl ---------------------------------------------------------------------------
dnl WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN, WX_STANDARD_OPTIONS_SUMMARY_MSG_END
dnl
dnl Like WX_STANDARD_OPTIONS_SUMMARY_MSG macro but these two macros also gives info
dnl about the configuration of the package which used the wxpresets.
dnl
dnl Typical usage:
dnl    WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN
dnl    echo "   - Package setting 1: $SETTING1"
dnl    echo "   - Package setting 2: $SETTING1"
dnl    ...
dnl    WX_STANDARD_OPTIONS_SUMMARY_MSG_END
dnl
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN],
        [
        echo
        echo " ----------------------------------------------------------------"
        echo "  Configuration for $PACKAGE_NAME $PACKAGE_VERSION successfully completed."
        echo "  Summary of main configuration settings for $PACKAGE_NAME:"
        WX_BOOLOPT_SUMMARY([DEBUG], ["  - DEBUG build"], ["  - RELEASE build"])
        WX_BOOLOPT_SUMMARY([UNICODE], ["  - UNICODE mode"], ["  - ANSI mode"])
        WX_BOOLOPT_SUMMARY([SHARED], ["  - SHARED mode"], ["  - STATIC mode"])
    ])

AC_DEFUN([WX_STANDARD_OPTIONS_SUMMARY_MSG_END],
        [
        WX_STANDARD_OPTIONS_SUMMARY_MSG
        echo
        echo "  Now, just run make."
        echo " ----------------------------------------------------------------"
        echo
    ])


dnl ---------------------------------------------------------------------------
dnl Deprecated macro wrappers
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_WXCONFIG], [WX_CONFIG_OPTIONS])
AC_DEFUN([AM_PATH_WXCONFIG], [
    WX_CONFIG_CHECK([$1],[$2],[$3],[$4],[$5])
])
AC_DEFUN([AM_PATH_WXRC], [WXRC_CHECK([$1],[$2])])
