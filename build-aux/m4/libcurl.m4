# LIBCURL_CHECK_CONFIG ([DEFAULT-ACTION], [MINIMUM-VERSION],
#                       [ACTION-IF-YES], [ACTION-IF-NO])
# ----------------------------------------------------------
#      David Shaw <dshaw@jabberwocky.com>   May-09-2006
#
# Checks for libcurl.  DEFAULT-ACTION is the string yes or no to
# specify whether to default to --with-libcurl or --without-libcurl.
# If not supplied, DEFAULT-ACTION is yes.  MINIMUM-VERSION is the
# minimum version of libcurl to accept.  Pass the version as a regular
# version number like 7.10.1. If not supplied, any version is
# accepted.  ACTION-IF-YES is a list of shell commands to run if
# libcurl was successfully found and passed the various tests.
# ACTION-IF-NO is a list of shell commands that are run otherwise.
# Note that using --without-libcurl does run ACTION-IF-NO.
#
# This macro #defines HAVE_LIBCURL if a working libcurl setup is
# found, and sets @LIBCURL_LIBS@ and @LIBCURL_CFLAGS@ to the necessary
# values.  Other useful defines are LIBCURL_FEATURE_xxx where xxx are
# the various features supported by libcurl, and LIBCURL_PROTOCOL_yyy
# where yyy are the various protocols supported by libcurl.  Both xxx
# and yyy are capitalized.  See the list of AH_TEMPLATEs at the top of
# the macro for the complete list of possible defines.  Shell
# variables $libcurl_feature_xxx and $libcurl_protocol_yyy are also
# defined to 'yes' for those features and protocols that were found.
# Note that xxx and yyy keep the same capitalization as in the
# curl-config list (e.g. it's "HTTP" and not "http").
#
# Users may override the detected values by doing something like:
# LIBCURL_LIBS="-lcurl" LIBCURL_CFLAGS="-I/usr/myinclude" ./configure
#
# For the sake of sanity, this macro assumes that any libcurl that is
# found is after version 7.7.2, the first version that included the
# curl-config script.  Note that it is very important for people
# packaging binary versions of libcurl to include this script!
# Without curl-config, we can only guess what protocols are available,
# or use curl_version_info to figure it out at runtime.

AC_DEFUN([LIBCURL_CHECK_CONFIG],
[
  AH_TEMPLATE([LIBCURL_FEATURE_SSL],[Defined if libcurl supports SSL])
  AH_TEMPLATE([LIBCURL_FEATURE_KRB4],[Defined if libcurl supports KRB4])
  AH_TEMPLATE([LIBCURL_FEATURE_IPV6],[Defined if libcurl supports IPv6])
  AH_TEMPLATE([LIBCURL_FEATURE_LIBZ],[Defined if libcurl supports libz])
  AH_TEMPLATE([LIBCURL_FEATURE_ASYNCHDNS],[Defined if libcurl supports AsynchDNS])
  AH_TEMPLATE([LIBCURL_FEATURE_IDN],[Defined if libcurl supports IDN])
  AH_TEMPLATE([LIBCURL_FEATURE_SSPI],[Defined if libcurl supports SSPI])
  AH_TEMPLATE([LIBCURL_FEATURE_NTLM],[Defined if libcurl supports NTLM])

  AH_TEMPLATE([LIBCURL_PROTOCOL_HTTP],[Defined if libcurl supports HTTP])
  AH_TEMPLATE([LIBCURL_PROTOCOL_HTTPS],[Defined if libcurl supports HTTPS])
  AH_TEMPLATE([LIBCURL_PROTOCOL_FTP],[Defined if libcurl supports FTP])
  AH_TEMPLATE([LIBCURL_PROTOCOL_FTPS],[Defined if libcurl supports FTPS])
  AH_TEMPLATE([LIBCURL_PROTOCOL_FILE],[Defined if libcurl supports FILE])
  AH_TEMPLATE([LIBCURL_PROTOCOL_TELNET],[Defined if libcurl supports TELNET])
  AH_TEMPLATE([LIBCURL_PROTOCOL_LDAP],[Defined if libcurl supports LDAP])
  AH_TEMPLATE([LIBCURL_PROTOCOL_DICT],[Defined if libcurl supports DICT])
  AH_TEMPLATE([LIBCURL_PROTOCOL_TFTP],[Defined if libcurl supports TFTP])

  _libcurl_config="no"

  AC_ARG_WITH(libcurl,
     [
If you want to use cURL library:
AC_HELP_STRING([--with-libcurl@<:@=DIR@:>@],[use cURL package @<:@default=no@:>@, optionally specify path to curl-config])],
        [
        if test "x$withval" = "xno"; then
            want_curl="no"
        elif test "x$withval" = "xyes"; then
            want_curl="yes"
        else
            want_curl="yes"
            _libcurl_config=$withval
        fi
        ],
        [want_curl=ifelse([$1],,[no],[$1])])

  if test "x$want_curl" != "xno"; then

     AC_PROG_AWK

     _libcurl_version_parse="eval $AWK '{split(\$NF,A,\".\"); X=256*256*A[[1]]+256*A[[2]]+A[[3]]; print X;}'"

     _libcurl_try_link=no

     AC_PATH_PROG([_libcurl_config], [curl-config], [])

     if test -x "$_libcurl_config"; then
        AC_CACHE_CHECK([for the version of libcurl],
	   [libcurl_cv_lib_curl_version],
           [libcurl_cv_lib_curl_version=`$_libcurl_config --version | $AWK '{print $[]2}'`])

	_libcurl_version=`echo $libcurl_cv_lib_curl_version | $_libcurl_version_parse`
	_libcurl_wanted=`echo ifelse([$2],,[0],[$2]) | $_libcurl_version_parse`

        if test $_libcurl_wanted -gt 0; then
	   AC_CACHE_CHECK([for libcurl >= version $2],
	      [libcurl_cv_lib_version_ok],
              [
   	      if test $_libcurl_version -ge $_libcurl_wanted; then
	         libcurl_cv_lib_version_ok=yes
      	      else
	         libcurl_cv_lib_version_ok=no
  	      fi
              ])
        fi

	if test $_libcurl_wanted -eq 0 || test "x$libcurl_cv_lib_version_ok" = "xyes"; then
           if test "x$LIBCURL_CFLAGS" = "x"; then
              LIBCURL_CFLAGS=`$_libcurl_config --cflags`
           fi
           if test "x$LIBCURL_LIBS" = "x"; then
		_full_libcurl_libs=`$_libcurl_config --libs`
		for i in $_full_libcurl_libs; do
			case $i in
				-L*)
					LIBCURL_LDFLAGS="$LIBCURL_LDFLAGS $i"
			;;
				-R*)
					LIBCURL_LDFLAGS="$LIBCURL_LDFLAGS -Wl,$i"
			;;
				-l*)
					LIBCURL_LIBS="$LIBCURL_LIBS $i"
			;;
			esac
		done

		if test "x$enable_static" = "xyes"; then
			_full_libcurl_libs=`$_libcurl_config --static-libs`
			for i in $_full_libcurl_libs; do
				case $i in
					-lcurl)
				;;
					-l*)
						_lib_name=`echo "$i" | cut -b3-`
						AC_CHECK_LIB($_lib_name , main,[
								LIBCURL_LIBS="$LIBCURL_LIBS $i"
							],[
								AC_MSG_ERROR([static library $_lib_name required for linking libcurl not found])
							])

				;;
				esac
			done
		fi

		_save_curl_cflags="$CFLAGS"
		_save_curl_ldflags="$LDFLAGS"
		_save_curl_libs="$LIBS"
		CFLAGS="$CFLAGS $LIBCURL_CFLAGS"
		LDFLAGS="$LDFLAGS $LIBCURL_LDFLAGS"
		LIBS="$LIBS $LIBCURL_LIBS"

		AC_CHECK_LIB(curl, main, , [AC_MSG_ERROR([libcurl library not found])])

		CFLAGS="$_save_curl_cflags"
		LDFLAGS="$_save_curl_ldflags"
		LIBS="$_save_curl_libs"
		unset _save_curl_cflags
		unset _save_curl_ldflags
		unset _save_curl_libs

              # This is so silly, but Apple actually has a bug in their
	      # curl-config script.  Fixed in Tiger, but there are still
	      # lots of Panther installs around.
              case "${host}" in
                 powerpc-apple-darwin7*)
                    LIBCURL_LIBS=`echo $LIBCURL_LIBS | sed -e 's|-arch i386||g'`
                 ;;
              esac
           fi

	   # All curl-config scripts support --feature
	   _libcurl_features=`$_libcurl_config --feature`

           # Is it modern enough to have --protocols? (7.12.4)
	   if test $_libcurl_version -ge 461828; then
              _libcurl_protocols=`$_libcurl_config --protocols`
           fi

           _libcurl_try_link=yes
	fi

	unset _libcurl_wanted
     fi
     if test "x$_libcurl_try_link" = "xyes"; then
        # we didn't find curl-config, so let's see if the user-supplied
        # link line (or failing that, "-lcurl") is enough.
        LIBCURL_LIBS=${LIBCURL_LIBS-"$_libcurl_libs -lcurl"}

        AC_CACHE_CHECK([whether libcurl is usable],
           [libcurl_cv_lib_curl_usable],
           [
		_save_curl_libs="${LIBS}"
		_save_curl_ldflags="${LDFLAGS}"
		_save_curl_cflags="${CFLAGS}"
		LIBS="${LIBS} ${LIBCURL_LIBS}"
		LDFLAGS="${LDFLAGS} ${LIBCURL_LDFLAGS}"
		CFLAGS="${CFLAGS} ${LIBCURL_CFLAGS}"

           AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <curl/curl.h>
#ifndef NULL
#define NULL (void *)0
#endif],[
/* Try and use a few common options to force a failure if we are
   missing symbols or can't link. */
int x;
curl_easy_setopt(NULL,CURLOPT_URL,NULL);
x=CURL_ERROR_SIZE;
x=CURLOPT_WRITEFUNCTION;
x=CURLOPT_FILE;
x=CURLOPT_ERRORBUFFER;
x=CURLOPT_STDERR;
x=CURLOPT_VERBOSE;
])],libcurl_cv_lib_curl_usable=yes,libcurl_cv_lib_curl_usable=no)

		LIBS="${_save_curl_libs}"
		LDFLAGS="${_save_curl_ldflags}"
		CFLAGS="${_save_curl_cflags}"
		unset _save_curl_libs
		unset _save_curl_ldflags
		unset _save_curl_cflags
	])

	if test "x$libcurl_cv_lib_curl_usable" = "xno"; then
		link_mode="dynamic"
		if test "x$enable_static" = "xyes"; then
			link_mode="static"
		fi
		AC_MSG_ERROR([libcurl is not available for ${link_mode} linking])
	fi

	# Does curl_free() exist in this version of libcurl?
	# If not, fake it with free()

	_save_curl_libs="${LIBS}"
	_save_curl_ldflags="${LDFLAGS}"
	_save_curl_cflags="${CFLAGS}"
	LIBS="${LIBS} ${LIBCURL_LIBS}"
	LDFLAGS="${LDFLAGS} ${LIBCURL_LDFLAGS}"
	CFLAGS="${CFLAGS} ${LIBCURL_CFLAGS}"

	AC_CHECK_FUNC(curl_free,,
		AC_DEFINE(curl_free,free,
			[Define curl_free() as free() if our version of curl lacks curl_free.]))

	AC_CHECK_FUNC(curl_easy_escape,
		AC_DEFINE(HAVE_FUNCTION_CURL_EASY_ESCAPE,1,
			[Define to 1 if function 'curl_easy_escape' exists.]))

	LIBS="${_save_curl_libs}"
	LDFLAGS="${_save_curl_ldflags}"
	CFLAGS="${_save_curl_cflags}"
	unset _save_curl_libs
	unset _save_curl_ldflags
	unset _save_curl_cflags

	AC_DEFINE(HAVE_LIBCURL,1,
		[Define to 1 if you have a functional curl library.])
	AC_SUBST(LIBCURL_CFLAGS)
	AC_SUBST(LIBCURL_LDFLAGS)
	AC_SUBST(LIBCURL_LIBS)
	found_curl="yes"

	for _libcurl_feature in $_libcurl_features ; do
		AC_DEFINE_UNQUOTED(AS_TR_CPP(libcurl_feature_$_libcurl_feature),[1])
		eval AS_TR_SH(libcurl_feature_$_libcurl_feature)=yes
	done

	if test "x$_libcurl_protocols" = "x"; then
		# We don't have --protocols, so just assume that all
		# protocols are available
		_libcurl_protocols="HTTP FTP FILE TELNET LDAP DICT"

		if test "x$libcurl_feature_SSL" = "xyes"; then
			_libcurl_protocols="$_libcurl_protocols HTTPS"

			# FTPS wasn't standards-compliant until version
			# 7.11.0
			if test $_libcurl_version -ge 461568; then
				_libcurl_protocols="$_libcurl_protocols FTPS"
			fi
		fi
	fi

	for _libcurl_protocol in $_libcurl_protocols ; do
		AC_DEFINE_UNQUOTED(AS_TR_CPP(libcurl_protocol_$_libcurl_protocol),[1])
		eval AS_TR_SH(libcurl_protocol_$_libcurl_protocol)=yes
        done
     else
	   unset LIBCURL_LIBS
	   unset LIBCURL_CFLAGS
     fi

     unset _libcurl_try_link
     unset _libcurl_version_parse
     unset _libcurl_config
     unset _libcurl_feature
     unset _libcurl_features
     unset _libcurl_protocol
     unset _libcurl_protocols
     unset _libcurl_version
     unset _libcurl_libs
  fi

  if test "x$want_curl" = "xno" || test "x$libcurl_cv_lib_curl_usable" != "xyes"; then
     # This is the IF-NO path
     ifelse([$4],,:,[$4])
  else
     # This is the IF-YES path
     ifelse([$3],,:,[$3])
  fi

])dnl