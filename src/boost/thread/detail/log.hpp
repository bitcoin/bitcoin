// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_DETAIL_LOG_HPP
#define BOOST_THREAD_DETAIL_LOG_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_USES_LOG
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#if defined BOOST_THREAD_USES_LOG_THREAD_ID
#include <boost/thread/thread.hpp>
#endif
#include <iostream>

namespace boost
{
  namespace thread_detail
  {
    inline boost::recursive_mutex& terminal_mutex()
    {
      static boost::recursive_mutex mtx;
      return mtx;
    }

  }
}
#if defined BOOST_THREAD_USES_LOG_THREAD_ID

#define BOOST_THREAD_LOG \
  { \
    boost::lock_guard<boost::recursive_mutex> _lk_(boost::thread_detail::terminal_mutex()); \
    std::cout << boost::this_thread::get_id() << " - "<<__FILE__<<"["<<__LINE__<<"] " <<std::dec
#else

#define BOOST_THREAD_LOG \
{ \
  boost::lock_guard<boost::recursive_mutex> _lk_(boost::thread_detail::terminal_mutex()); \
  std::cout << __FILE__<<"["<<__LINE__<<"] " <<std::dec

#endif
#define BOOST_THREAD_END_LOG \
    std::dec << std::endl; \
  }

#else

namespace boost
{
  namespace thread_detail
  {
    struct dummy_stream_t
    {
    };

    template <typename T>
    inline dummy_stream_t const& operator<<(dummy_stream_t const& os, T)
    {
      return os;
    }

    inline dummy_stream_t const& operator<<(dummy_stream_t const& os, dummy_stream_t const&)
    {
      return os;
    }


    BOOST_CONSTEXPR_OR_CONST dummy_stream_t dummy_stream = {};

  }
}

#ifdef BOOST_MSVC
#define BOOST_THREAD_LOG \
        __pragma(warning(suppress:4127)) /* conditional expression is constant */ \
        if (true) {} else boost::thread_detail::dummy_stream
#else
#define BOOST_THREAD_LOG if (true) {} else boost::thread_detail::dummy_stream
#endif
#define BOOST_THREAD_END_LOG boost::thread_detail::dummy_stream

#endif

#define BOOST_THREAD_TRACE BOOST_THREAD_LOG << BOOST_THREAD_END_LOG


#ifdef BOOST_MSVC
#define BOOST_DETAIL_THREAD_LOG \
        __pragma(warning(suppress:4127)) /* conditional expression is constant */ \
        if (false) {} else std::cout << std::endl << __FILE__ << "[" << __LINE__ << "]"
#else
#define BOOST_DETAIL_THREAD_LOG \
        if (false) {} else std::cout << std::endl << __FILE__ << "[" << __LINE__ << "]"
#endif

#endif // header
