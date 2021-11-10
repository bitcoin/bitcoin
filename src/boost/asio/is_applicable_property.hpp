//
// is_applicable_property.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IS_APPLICABLE_PROPERTY_HPP
#define BOOST_ASIO_IS_APPLICABLE_PROPERTY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename T, typename Property, typename = void>
struct is_applicable_property_trait : false_type
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename Property>
struct is_applicable_property_trait<T, Property,
  typename void_type<
    typename enable_if<
      !!Property::template is_applicable_property_v<T>
    >::type
  >::type> : true_type
{
};

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace detail

template <typename T, typename Property, typename = void>
struct is_applicable_property :
  detail::is_applicable_property_trait<T, Property>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename Property>
BOOST_ASIO_CONSTEXPR const bool is_applicable_property_v
  = is_applicable_property<T, Property>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_IS_APPLICABLE_PROPERTY_HPP
