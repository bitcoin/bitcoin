#ifndef BOOST_THREAD_RECURSIVE_MUTEX_HPP
#define BOOST_THREAD_RECURSIVE_MUTEX_HPP

//  recursive_mutex.hpp
//
//  (C) Copyright 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/platform.hpp>
#if defined(BOOST_THREAD_PLATFORM_WIN32)
#include <boost/thread/win32/recursive_mutex.hpp>
#elif defined(BOOST_THREAD_PLATFORM_PTHREAD)
#include <boost/thread/pthread/recursive_mutex.hpp>
#else
#error "Boost threads unavailable on this platform"
#endif

#include <boost/thread/lockable_traits.hpp>

namespace boost
{
  namespace sync
  {

#ifdef BOOST_THREAD_NO_AUTO_DETECT_MUTEX_TYPES
    template<>
    struct is_basic_lockable<recursive_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_lockable<recursive_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_basic_lockable<recursive_timed_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_lockable<recursive_timed_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };
#endif

    template<>
    struct is_recursive_mutex_sur_parolle<recursive_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_recursive_mutex_sur_parolle<recursive_timed_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };

  }
}
#endif
