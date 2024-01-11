// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_H
#define BITCOIN_COMPAT_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef FD_SETSIZE
#undef FD_SETSIZE // prevent redefinition compiler warning
#endif
#define FD_SETSIZE 1024 // max number of fds in fd_set
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>
#endif

#ifndef WIN32
typedef unsigned int SOCKET;
#include <errno.h>
#define WSAGetLastError()   errno
#define WSAEINVAL           EINVAL
#define WSAEALREADY         EALREADY
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define WSAEAGAIN           EAGAIN
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEINTR            EINTR
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEADDRINUSE       EADDRINUSE
#define WSAENOTSOCK         EBADF
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1
#define SD_SEND             SHUT_WR
#else
#ifndef WSAEAGAIN
#ifdef EAGAIN
#define WSAEAGAIN EAGAIN
#else
#define WSAEAGAIN WSAEWOULDBLOCK
#endif
#endif
#endif

#ifdef WIN32
#ifndef S_IRUSR
#define S_IRUSR             0400
#define S_IWUSR             0200
#endif
#else
#define MAX_PATH            1024
#endif
#ifdef _MSC_VER
#if !defined(ssize_t)
#ifdef _WIN64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif
#endif
#endif

#if HAVE_DECL_STRNLEN == 0
size_t strnlen( const char *start, size_t max_len);
#endif // HAVE_DECL_STRNLEN

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
#if defined(__linux__) || defined(__FreeBSD__)
#define USE_POLL
#endif

#if defined(__linux__)
#define USE_EPOLL
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#define USE_KQUEUE
#endif

bool static inline IsSelectableSocket(const SOCKET& s) {
#if defined(USE_POLL) || defined(WIN32)
    return true;
#else
    return (s < FD_SETSIZE);
#endif
}

// MSG_NOSIGNAL is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

// MSG_DONTWAIT is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT 0
#endif

#endif // BITCOIN_COMPAT_H
