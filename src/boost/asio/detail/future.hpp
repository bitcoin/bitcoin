//
// detail/future.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_FUTURE_HPP
#define BOOST_ASIO_DETAIL_FUTURE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#if defined(BOOST_ASIO_HAS_STD_FUTURE)
# include <future>
// Even though the future header is available, libstdc++ may not implement the
// std::future class itself. However, we need to have already included the
// future header to reliably test for _GLIBCXX_HAS_GTHREADS.
# if defined(__GNUC__) && !defined(BOOST_ASIO_HAS_CLANG_LIBCXX)
#  if defined(_GLIBCXX_HAS_GTHREADS)
#   define BOOST_ASIO_HAS_STD_FUTURE_CLASS 1
#  endif // defined(_GLIBCXX_HAS_GTHREADS)
# else // defined(__GNUC__) && !defined(BOOST_ASIO_HAS_CLANG_LIBCXX)
#  define BOOST_ASIO_HAS_STD_FUTURE_CLASS 1
# endif // defined(__GNUC__) && !defined(BOOST_ASIO_HAS_CLANG_LIBCXX)
#endif // defined(BOOST_ASIO_HAS_STD_FUTURE)

#endif // BOOST_ASIO_DETAIL_FUTURE_HPP
