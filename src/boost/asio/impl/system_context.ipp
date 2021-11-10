//
// impl/system_context.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_SYSTEM_CONTEXT_IPP
#define BOOST_ASIO_IMPL_SYSTEM_CONTEXT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/system_context.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

struct system_context::thread_function
{
  detail::scheduler* scheduler_;

  void operator()()
  {
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    try
    {
#endif// !defined(BOOST_ASIO_NO_EXCEPTIONS)
      boost::system::error_code ec;
      scheduler_->run(ec);
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
      std::terminate();
    }
#endif// !defined(BOOST_ASIO_NO_EXCEPTIONS)
  }
};

system_context::system_context()
  : scheduler_(add_scheduler(new detail::scheduler(*this, 0, false)))
{
  scheduler_.work_started();

  thread_function f = { &scheduler_ };
  num_threads_ = detail::thread::hardware_concurrency() * 2;
  num_threads_ = num_threads_ ? num_threads_ : 2;
  threads_.create_threads(f, num_threads_);
}

system_context::~system_context()
{
  scheduler_.work_finished();
  scheduler_.stop();
  threads_.join();
}

void system_context::stop()
{
  scheduler_.stop();
}

bool system_context::stopped() const BOOST_ASIO_NOEXCEPT
{
  return scheduler_.stopped();
}

void system_context::join()
{
  scheduler_.work_finished();
  threads_.join();
}

detail::scheduler& system_context::add_scheduler(detail::scheduler* s)
{
  detail::scoped_ptr<detail::scheduler> scoped_impl(s);
  boost::asio::add_service<detail::scheduler>(*this, scoped_impl.get());
  return *scoped_impl.release();
}

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_SYSTEM_CONTEXT_IPP
