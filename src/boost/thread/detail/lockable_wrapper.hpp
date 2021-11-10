// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2012 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_DETAIL_LOCKABLE_WRAPPER_HPP
#define BOOST_THREAD_DETAIL_LOCKABLE_WRAPPER_HPP

#include <boost/thread/detail/config.hpp>

#if ! defined BOOST_THREAD_NO_CXX11_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif
#include <boost/config/abi_prefix.hpp>

namespace boost
{

#if ! defined BOOST_THREAD_NO_CXX11_HDR_INITIALIZER_LIST
  namespace thread_detail
  {
    template <typename Mutex>
    struct lockable_wrapper
    {
      Mutex* m;
      explicit lockable_wrapper(Mutex& m_) :
        m(&m_)
      {}
    };
    template <typename Mutex>
    struct lockable_adopt_wrapper
    {
      Mutex* m;
      explicit lockable_adopt_wrapper(Mutex& m_) :
        m(&m_)
      {}
    };
  }
#endif

}

#include <boost/config/abi_suffix.hpp>

#endif // header
