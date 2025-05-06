// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_COMPAT_H
#define BITCOIN_COMPAT_COMPAT_H

// Windows defines FD_SETSIZE to 64 (see _fd_types.h in mingw-w64),
// which is too small for our usage, but allows us to redefine it safely.
// We redefine it to be 1024, to match glibc, see typesizes.h.
#ifdef WIN32
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>
#else
#include <arpa/inet.h>   // IWYU pragma: export
#include <fcntl.h>       // IWYU pragma: export
#include <net/if.h>      // IWYU pragma: export
#include <netdb.h>       // IWYU pragma: export
#include <netinet/in.h>  // IWYU pragma: export
#include <netinet/tcp.h> // IWYU pragma: export
#include <sys/mman.h>    // IWYU pragma: export
#include <sys/select.h>  // IWYU pragma: export
#include <sys/socket.h>  // IWYU pragma: export
#include <sys/types.h>   // IWYU pragma: export
#include <unistd.h>      // IWYU pragma: export
#endif

// Windows does not have `sa_family_t` - it defines `sockaddr::sa_family` as `u_short`.
// Thus define `sa_family_t` on Windows too so that the rest of the code can use `sa_family_t`.
// See https://learn.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-sockaddr#syntax
#ifdef WIN32
typedef u_short sa_family_t;
#endif

// We map Linux / BSD error functions and codes, to the equivalent
// Windows definitions, and use the WSA* names throughout our code.
// Note that glibc defines EWOULDBLOCK as EAGAIN (see errno.h).
#ifndef WIN32
typedef unsigned int SOCKET;
#include <cerrno>
#define WSAGetLastError()   errno
#define WSAEINVAL           EINVAL
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define WSAEAGAIN           EAGAIN
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEINTR            EINTR
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEADDRINUSE       EADDRINUSE
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1
#else
// WSAEAGAIN doesn't exist on Windows
#ifdef EAGAIN
#define WSAEAGAIN EAGAIN
#else
#define WSAEAGAIN WSAEWOULDBLOCK
#endif
#endif

// Windows defines MAX_PATH as it's maximum path length.
// We define MAX_PATH for use on non-Windows systems.
#ifndef WIN32
#define MAX_PATH            1024
#endif

// ssize_t is POSIX, and not present when using MSVC.
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

// The type of the option value passed to getsockopt & setsockopt
// differs between Windows and non-Windows.
#ifndef WIN32
typedef void* sockopt_arg_type;
#else
typedef char* sockopt_arg_type;
#endif

#ifdef WIN32
// Export main() and ensure working ASLR when using mingw-w64.
// Exporting a symbol will prevent the linker from stripping
// the .reloc section from the binary, which is a requirement
// for ASLR. While release builds are not affected, anyone
// building with a binutils < 2.36 is subject to this ld bug.
#define MAIN_FUNCTION __declspec(dllexport) int main(int argc, char* argv[])
#else
#define MAIN_FUNCTION int main(int argc, char* argv[])
#endif

// Note these both should work with the current usage of poll, but best to be safe
// WIN32 poll is broken https://daniel.haxx.se/blog/2012/10/10/wsapoll-is-broken/
// __APPLE__ poll is broke https://github.com/bitcoin/bitcoin/pull/14336#issuecomment-437384408
#if defined(__linux__)
#define USE_POLL
#endif

// MSG_NOSIGNAL is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

// MSG_DONTWAIT is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT 0
#endif

#endif // BITCOIN_COMPAT_COMPAT_H
