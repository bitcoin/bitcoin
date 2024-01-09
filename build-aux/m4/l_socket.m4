# Illumos/SmartOS requires linking with -lsocket if
# using getifaddrs & freeifaddrs

m4_define([_CHECK_SOCKET_testbody], [[
  #include <sys/types.h>
  #include <ifaddrs.h>

  int main() {
    struct ifaddrs *ifaddr;
    getifaddrs(&ifaddr);
    freeifaddrs(ifaddr);
  }
]])

AC_DEFUN([CHECK_SOCKET], [

  AC_LANG_PUSH(C++)

  AC_MSG_CHECKING([whether ifaddrs funcs can be used without link library])

  AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_SOCKET_testbody])],[
      AC_MSG_RESULT([yes])
    ],[
      AC_MSG_RESULT([no])
      LIBS="$LIBS -lsocket"
      AC_MSG_CHECKING([whether getifaddrs needs -lsocket])
      AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_SOCKET_testbody])],[
          AC_MSG_RESULT([yes])
        ],[
          AC_MSG_RESULT([no])
          AC_MSG_FAILURE([cannot figure out how to use getifaddrs])
        ])
    ])

  AC_LANG_POP
])
