//  (C) Copyright 2013,2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_EXECUTORS_WORK_HPP
#define BOOST_THREAD_EXECUTORS_WORK_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE

#include <boost/thread/detail/nullary_function.hpp>
#include <boost/thread/csbl/functional.hpp>

namespace boost
{
  namespace executors
  {
    typedef detail::nullary_function<void()> work;

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    typedef detail::nullary_function<void()> work_pq;
    //typedef csbl::function<void()> work_pq;
#else
    typedef csbl::function<void()> work_pq;
#endif
  }
} // namespace boost

#endif
#endif //  BOOST_THREAD_EXECUTORS_WORK_HPP
