dnl Copyright (c) 2018 The Bitcoin Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

dnl BITCOIN_SYSTEM_INCLUDE([DESTINATION-VARIABLE-NAME], [INCLUDES-STRING])
dnl Populates the variable DESTINATION with the value from INCLUDES but
dnl with -I includes switched to -isystem, with the exception of -I/usr/include,
dnl which is left unmodified to avoid order issues with /usr/include. See:
dnl https://stackoverflow.com/questions/37218953/isystem-on-a-system-include-directory-causes-errors
AC_DEFUN([BITCOIN_SYSTEM_INCLUDE],[
  if test "x$use_isystem" = "xyes" && test "x$2" != "x"; then
    dnl Including /usr/include with -isystem is known to cause problems, e.g.
    dnl breaking include_next due to, the directive changing inclusion order.
    dnl https://bugreports.qt.io/browse/QTBUG-53367
    $1=$(echo $2 | sed -e 's| -I| -isystem|g' -e 's|^-I|-isystem|g' -e 's|-isystem/usr/include|-I/usr/include|g')
  else
    $1=$2
  fi
])
