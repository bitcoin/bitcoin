//
// detail/source_location.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_SOURCE_LOCATION_HPP
#define BOOST_ASIO_DETAIL_SOURCE_LOCATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_SOURCE_LOCATION)

#if defined(BOOST_ASIO_HAS_STD_SOURCE_LOCATION)
# include <source_location>
#elif defined(BOOST_ASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)
# include <experimental/source_location>
#else // defined(BOOST_ASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)
# error BOOST_ASIO_HAS_SOURCE_LOCATION is set \
  but no source_location is available
#endif // defined(BOOST_ASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)

namespace boost {
namespace asio {
namespace detail {

#if defined(BOOST_ASIO_HAS_STD_SOURCE_LOCATION)
using std::source_location;
#elif defined(BOOST_ASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)
using std::experimental::source_location;
#endif // defined(BOOST_ASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)

} // namespace detail
} // namespace asio
} // namespace boost

#endif // defined(BOOST_ASIO_HAS_SOURCE_LOCATION)

#endif // BOOST_ASIO_DETAIL_SOURCE_LOCATION_HPP
