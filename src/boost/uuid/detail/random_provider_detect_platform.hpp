//
// Copyright (c) 2017 James E. King III
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//   https://www.boost.org/LICENSE_1_0.txt)
//
// Platform-specific random entropy provider platform detection
//

#ifndef BOOST_UUID_DETAIL_RANDOM_PROVIDER_PLATFORM_DETECTION_HPP
#define BOOST_UUID_DETAIL_RANDOM_PROVIDER_PLATFORM_DETECTION_HPP

#include <boost/predef/library/c/cloudabi.h>
#include <boost/predef/library/c/gnu.h>
#include <boost/predef/os/bsd/open.h>
#include <boost/predef/os/windows.h>

// Note: Don't use Boost.Predef to detect Linux and Android as it may give different results depending on header inclusion order.
// https://github.com/boostorg/predef/issues/81#issuecomment-413329061
#if (defined(__linux__) || defined(__linux) || defined(linux)) && (!defined(__ANDROID__) || __ANDROID_API__ >= 28)
#include <sys/syscall.h>
#if defined(SYS_getrandom)
#define BOOST_UUID_RANDOM_PROVIDER_HAS_GETRANDOM
#endif // defined(SYS_getrandom)
#endif

// On Linux, getentropy is implemented via getrandom. If we know that getrandom is not supported by the kernel, getentropy
// will certainly not work, even if libc provides a wrapper function for it. There is no reason, ever, to use getentropy on that platform.
#if !defined(BOOST_UUID_RANDOM_PROVIDER_DISABLE_GETENTROPY) && (defined(__linux__) || defined(__linux) || defined(linux) || defined(__ANDROID__))
#define BOOST_UUID_RANDOM_PROVIDER_DISABLE_GETENTROPY
#endif

//
// Platform Detection - will load in the correct header and
// will define the class <tt>random_provider_base</tt>.
//

#if BOOST_OS_BSD_OPEN >= BOOST_VERSION_NUMBER(2, 1, 0) || BOOST_LIB_C_CLOUDABI
# define BOOST_UUID_RANDOM_PROVIDER_ARC4RANDOM
# define BOOST_UUID_RANDOM_PROVIDER_NAME arc4random

#elif BOOST_OS_WINDOWS
# include <boost/winapi/config.hpp>
# if BOOST_WINAPI_PARTITION_APP_SYSTEM && \
     !defined(BOOST_UUID_RANDOM_PROVIDER_FORCE_WINCRYPT) && \
     !defined(_WIN32_WCE) && \
     (BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6)
#  define BOOST_UUID_RANDOM_PROVIDER_BCRYPT
#  define BOOST_UUID_RANDOM_PROVIDER_NAME bcrypt

# elif BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM
#  define BOOST_UUID_RANDOM_PROVIDER_WINCRYPT
#  define BOOST_UUID_RANDOM_PROVIDER_NAME wincrypt
# else
#  error Unable to find a suitable windows entropy provider
# endif

#elif defined(BOOST_UUID_RANDOM_PROVIDER_HAS_GETRANDOM) && !defined(BOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX) && !defined(BOOST_UUID_RANDOM_PROVIDER_DISABLE_GETRANDOM)
# define BOOST_UUID_RANDOM_PROVIDER_GETRANDOM
# define BOOST_UUID_RANDOM_PROVIDER_NAME getrandom

#elif BOOST_LIB_C_GNU >= BOOST_VERSION_NUMBER(2, 25, 0) && !defined(BOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX) && !defined(BOOST_UUID_RANDOM_PROVIDER_DISABLE_GETENTROPY)
# define BOOST_UUID_RANDOM_PROVIDER_GETENTROPY
# define BOOST_UUID_RANDOM_PROVIDER_NAME getentropy

#else
# define BOOST_UUID_RANDOM_PROVIDER_POSIX
# define BOOST_UUID_RANDOM_PROVIDER_NAME posix

#endif

#define BOOST_UUID_RANDOM_PROVIDER_STRINGIFY2(X) #X
#define BOOST_UUID_RANDOM_PROVIDER_STRINGIFY(X) BOOST_UUID_RANDOM_PROVIDER_STRINGIFY2(X)

#if defined(BOOST_UUID_RANDOM_PROVIDER_SHOW)
#pragma message("BOOST_UUID_RANDOM_PROVIDER_NAME " BOOST_UUID_RANDOM_PROVIDER_STRINGIFY(BOOST_UUID_RANDOM_PROVIDER_NAME))
#endif

#endif // BOOST_UUID_DETAIL_RANDOM_PROVIDER_PLATFORM_DETECTION_HPP
