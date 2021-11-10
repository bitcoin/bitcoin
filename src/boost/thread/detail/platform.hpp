// Copyright 2006 Roland Schwarz.
// (C) Copyright 2007 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06040501_HPP
#define BOOST_THREAD_RS06040501_HPP

// fetch compiler and platform configuration
#include <boost/config.hpp>

// insist on threading support being available:
#include <boost/config/requires_threads.hpp>

// choose platform
#if defined(linux) || defined(__linux) || defined(__linux__)
#  define BOOST_THREAD_LINUX
//#  define BOOST_THREAD_WAIT_BUG boost::posix_time::microseconds(100000)
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#  define BOOST_THREAD_BSD
#elif defined(sun) || defined(__sun)
#  define BOOST_THREAD_SOLARIS
#elif defined(__sgi)
#  define BOOST_THREAD_IRIX
#elif defined(__hpux)
#  define BOOST_THREAD_HPUX
#elif defined(__CYGWIN__)
#  define BOOST_THREAD_CYGWIN
#elif (defined(_WIN32) || defined(__WIN32__) || defined(WIN32)) && !defined(BOOST_DISABLE_WIN32)
#if ! defined BOOST_THREAD_WIN32
#  define BOOST_THREAD_WIN32
#endif
#elif defined(__BEOS__)
#  define BOOST_THREAD_BEOS
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#  define BOOST_THREAD_MACOS
//#  define BOOST_THREAD_WAIT_BUG boost::posix_time::microseconds(1000)
#elif defined(__IBMCPP__) || defined(_AIX)
#  define BOOST_THREAD_AIX
#elif defined(__amigaos__)
#  define BOOST_THREAD_AMIGAOS
#elif defined(__QNXNTO__)
#  define BOOST_THREAD_QNXNTO
#elif defined(unix) || defined(__unix) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
#       if defined(BOOST_HAS_PTHREADS) && !defined(BOOST_THREAD_POSIX)
#               define BOOST_THREAD_POSIX
#       endif
#endif

// For every supported platform add a new entry into the dispatch table below.
// BOOST_THREAD_POSIX is tested first, so on platforms where posix and native
// threading is available, the user may choose, by defining BOOST_THREAD_POSIX
// in her source. If a platform is known to support pthreads and no native
// port of boost_thread is available just specify "pthread" in the
// dispatcher table. If there is no entry for a platform but pthreads is
// available on the platform, pthread is choosen as default. If nothing is
// available the preprocessor will fail with a diagnostic message.

#if defined(BOOST_THREAD_POSIX)
#  define BOOST_THREAD_PLATFORM_PTHREAD
#else
#  if defined(BOOST_THREAD_WIN32)
#       define BOOST_THREAD_PLATFORM_WIN32
#  elif defined(BOOST_HAS_PTHREADS)
#       define BOOST_THREAD_PLATFORM_PTHREAD
#  else
#       error "Sorry, no boost threads are available for this platform."
#  endif
#endif

#endif // BOOST_THREAD_RS06040501_HPP
