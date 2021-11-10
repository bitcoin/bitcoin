//  (C) Copyright 2008-10 Anthony Williams
//  (C) Copyright 2011-2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_FUTURES_FUTURE_ERROR_HPP
#define BOOST_THREAD_FUTURES_FUTURE_ERROR_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/futures/future_error_code.hpp>
#include <boost/system/error_code.hpp>

#include <stdexcept>

namespace boost
{
  class BOOST_SYMBOL_VISIBLE future_error
      : public std::logic_error
  {
      system::error_code ec_;
  public:
      future_error(system::error_code ec)
      : logic_error(ec.message()),
        ec_(ec)
      {
      }

      const system::error_code& code() const BOOST_NOEXCEPT
      {
        return ec_;
      }
  };

    class BOOST_SYMBOL_VISIBLE future_uninitialized:
        public future_error
    {
    public:
        future_uninitialized() :
          future_error(system::make_error_code(future_errc::no_state))
        {}
    };
    class BOOST_SYMBOL_VISIBLE broken_promise:
        public future_error
    {
    public:
        broken_promise():
          future_error(system::make_error_code(future_errc::broken_promise))
        {}
    };
    class BOOST_SYMBOL_VISIBLE future_already_retrieved:
        public future_error
    {
    public:
        future_already_retrieved():
          future_error(system::make_error_code(future_errc::future_already_retrieved))
        {}
    };
    class BOOST_SYMBOL_VISIBLE promise_already_satisfied:
        public future_error
    {
    public:
        promise_already_satisfied():
          future_error(system::make_error_code(future_errc::promise_already_satisfied))
        {}
    };

    class BOOST_SYMBOL_VISIBLE task_already_started:
        public future_error
    {
    public:
        task_already_started():
        future_error(system::make_error_code(future_errc::promise_already_satisfied))
        {}
    };

    class BOOST_SYMBOL_VISIBLE task_moved:
        public future_error
    {
    public:
        task_moved():
          future_error(system::make_error_code(future_errc::no_state))
        {}
    };

    class promise_moved:
        public future_error
    {
    public:
          promise_moved():
          future_error(system::make_error_code(future_errc::no_state))
        {}
    };
}

#endif // header
