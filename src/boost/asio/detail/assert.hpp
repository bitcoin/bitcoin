//
// detail/assert.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_ASSERT_HPP
#define BOOST_ASIO_DETAIL_ASSERT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_BOOST_ASSERT)
# include <boost/assert.hpp>
#else // defined(BOOST_ASIO_HAS_BOOST_ASSERT)
# include <cassert>
#endif // defined(BOOST_ASIO_HAS_BOOST_ASSERT)

#if defined(BOOST_ASIO_HAS_BOOST_ASSERT)
# define BOOST_ASIO_ASSERT(expr) BOOST_ASSERT(expr)
#else // defined(BOOST_ASIO_HAS_BOOST_ASSERT)
# define BOOST_ASIO_ASSERT(expr) assert(expr)
#endif // defined(BOOST_ASIO_HAS_BOOST_ASSERT)

#endif // BOOST_ASIO_DETAIL_ASSERT_HPP
