//  (C) Copyright 2008-10 Anthony Williams
//  (C) Copyright 2011-2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_FUTURES_FUTURE_STATUS_HPP
#define BOOST_THREAD_FUTURES_FUTURE_STATUS_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/core/scoped_enum.hpp>

namespace boost
{
  //enum class future_status
  BOOST_SCOPED_ENUM_DECLARE_BEGIN(future_status)
  {
      ready,
      timeout,
      deferred
  }
  BOOST_SCOPED_ENUM_DECLARE_END(future_status)
  namespace future_state
  {
      enum state { uninitialized, waiting, ready, moved, deferred };
  }
}

#endif // header
