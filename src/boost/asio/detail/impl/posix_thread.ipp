//
// detail/impl/posix_thread.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_POSIX_THREAD_IPP
#define BOOST_ASIO_DETAIL_IMPL_POSIX_THREAD_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_PTHREADS)

#include <boost/asio/detail/posix_thread.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

posix_thread::~posix_thread()
{
  if (!joined_)
    ::pthread_detach(thread_);
}

void posix_thread::join()
{
  if (!joined_)
  {
    ::pthread_join(thread_, 0);
    joined_ = true;
  }
}

std::size_t posix_thread::hardware_concurrency()
{
#if defined(_SC_NPROCESSORS_ONLN)
  long result = sysconf(_SC_NPROCESSORS_ONLN);
  if (result > 0)
    return result;
#endif // defined(_SC_NPROCESSORS_ONLN)
  return 0;
}

void posix_thread::start_thread(func_base* arg)
{
  int error = ::pthread_create(&thread_, 0,
        boost_asio_detail_posix_thread_function, arg);
  if (error != 0)
  {
    delete arg;
    boost::system::error_code ec(error,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(ec, "thread");
  }
}

void* boost_asio_detail_posix_thread_function(void* arg)
{
  posix_thread::auto_func_base_ptr func = {
      static_cast<posix_thread::func_base*>(arg) };
  func.ptr->run();
  return 0;
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_PTHREADS)

#endif // BOOST_ASIO_DETAIL_IMPL_POSIX_THREAD_IPP
