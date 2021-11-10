//
// detail/functional.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_FUNCTIONAL_HPP
#define BOOST_ASIO_DETAIL_FUNCTIONAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <functional>

#if !defined(BOOST_ASIO_HAS_STD_FUNCTION)
# include <boost/function.hpp>
#endif // !defined(BOOST_ASIO_HAS_STD_FUNCTION)

namespace boost {
namespace asio {
namespace detail {

#if defined(BOOST_ASIO_HAS_STD_FUNCTION)
using std::function;
#else // defined(BOOST_ASIO_HAS_STD_FUNCTION)
using boost::function;
#endif // defined(BOOST_ASIO_HAS_STD_FUNCTION)

} // namespace detail

#if defined(BOOST_ASIO_HAS_STD_REFERENCE_WRAPPER)
using std::ref;
using std::reference_wrapper;
#endif // defined(BOOST_ASIO_HAS_STD_REFERENCE_WRAPPER)

} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_DETAIL_FUNCTIONAL_HPP
