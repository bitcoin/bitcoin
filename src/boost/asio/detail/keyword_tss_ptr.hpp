//
// detail/keyword_tss_ptr.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_KEYWORD_TSS_PTR_HPP
#define BOOST_ASIO_DETAIL_KEYWORD_TSS_PTR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_THREAD_KEYWORD_EXTENSION)

#include <boost/asio/detail/noncopyable.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename T>
class keyword_tss_ptr
  : private noncopyable
{
public:
  // Constructor.
  keyword_tss_ptr()
  {
  }

  // Destructor.
  ~keyword_tss_ptr()
  {
  }

  // Get the value.
  operator T*() const
  {
    return value_;
  }

  // Set the value.
  void operator=(T* value)
  {
    value_ = value;
  }

private:
  static BOOST_ASIO_THREAD_KEYWORD T* value_;
};

template <typename T>
BOOST_ASIO_THREAD_KEYWORD T* keyword_tss_ptr<T>::value_;

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_THREAD_KEYWORD_EXTENSION)

#endif // BOOST_ASIO_DETAIL_KEYWORD_TSS_PTR_HPP
