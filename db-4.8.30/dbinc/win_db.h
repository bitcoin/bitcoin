/*-
 * $Id$
 *
 * The following provides the information necessary to build Berkeley
 * DB on native Windows, and other Windows environments such as MinGW.
 */

/*
 * Windows NT 4.0 and later required for the replication manager.
 */
#ifdef HAVE_REPLICATION_THREADS
#define _WIN32_WINNT 0x0400
#endif

#ifndef DB_WINCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#endif /* DB_WINCE */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>

/*
 * To build Tcl interface libraries, the include path must be configured to
 * use the directory containing <tcl.h>, usually the include directory in
 * the Tcl distribution.
 */
#ifdef DB_TCL_SUPPORT
#include <tcl.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#ifdef HAVE_GETADDRINFO
/*
 * Need explicit includes for IPv6 support on Windows.  Both are necessary to
 * ensure that pre WinXP versions have an implementation of the getaddrinfo API.
 */
#include <ws2tcpip.h>
#include <wspiapi.h>
#endif

/*
 * Microsoft's C runtime library has fsync, getcwd, getpid, snprintf and
 * vsnprintf, but under different names.
 */
#define fsync           _commit

#ifndef DB_WINCE
#define getcwd(buf, size)   _getcwd(buf, size)
#endif
#define getpid          GetCurrentProcessId
#define snprintf        _snprintf
#define strcasecmp      _stricmp
#define strncasecmp     _strnicmp
#define vsnprintf       _vsnprintf

#define h_errno         WSAGetLastError()

/*
 * Win32 does not have getopt.
 *
 * The externs are here, instead of using db_config.h and clib_port.h, because
 * that approach changes function names to BDB specific names, and the example
 * programs use getopt and can't use BDB specific names.
 */
#if defined(__cplusplus)
extern "C" {
#endif
extern int getopt(int, char * const *, const char *);
#if defined(__cplusplus)
}
#endif

/*
 * Microsoft's compiler _doesn't_ define __STDC__ unless you invoke it with
 * arguments turning OFF all vendor extensions.  Even more unfortunately, if
 * we do that, it fails to parse windows.h!!!!!  So, we define __STDC__ here,
 * after windows.h comes in.  Note: the compiler knows we've defined it, and
 * starts enforcing strict ANSI compliance from this point on.
 */
#ifndef __STDC__
#define __STDC__ 1
#endif

#ifdef _UNICODE
#define TO_TSTRING(dbenv, s, ts, ret) do {              \
        int __len = (int)strlen(s) + 1;             \
        ts = NULL;                      \
        if ((ret = __os_malloc((dbenv),             \
            __len * sizeof(_TCHAR), &(ts))) == 0 &&     \
            MultiByteToWideChar(CP_UTF8, 0,         \
            (s), -1, (ts), __len) == 0)             \
            ret = __os_posix_err(__os_get_syserr());    \
    } while (0)

#define FROM_TSTRING(dbenv, ts, s, ret) {               \
        int __len = WideCharToMultiByte(CP_UTF8, 0, ts, -1, \
            NULL, 0, NULL, NULL);               \
        s = NULL;                       \
        if ((ret = __os_malloc((dbenv), __len, &(s))) == 0 &&   \
            WideCharToMultiByte(CP_UTF8, 0,         \
            (ts), -1, (s), __len, NULL, NULL) == 0)     \
            ret = __os_posix_err(__os_get_syserr());    \
    } while (0)

#define FREE_STRING(dbenv, s) do {                  \
        if ((s) != NULL) {                  \
            __os_free((dbenv), (s));            \
            (s) = NULL;                 \
        }                           \
    } while (0)

#else
#define TO_TSTRING(dbenv, s, ts, ret) (ret) = 0, (ts) = (_TCHAR *)(s)
#define FROM_TSTRING(dbenv, ts, s, ret) (ret) = 0, (s) = (char *)(ts)
#define FREE_STRING(dbenv, ts)
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif
