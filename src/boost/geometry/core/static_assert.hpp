// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_CORE_STATIC_ASSERT_HPP
#define BOOST_GEOMETRY_CORE_STATIC_ASSERT_HPP

#include <type_traits>

#include <boost/static_assert.hpp>

namespace boost { namespace geometry { namespace detail
{

template <bool Check, typename ...Ts>
struct static_assert_check : std::integral_constant<bool, Check> {};

#define BOOST_GEOMETRY_STATIC_ASSERT(CHECK, MESSAGE, ...) \
static_assert(boost::geometry::detail::static_assert_check<(CHECK), __VA_ARGS__>::value, MESSAGE)


#define BOOST_GEOMETRY_STATIC_ASSERT_FALSE(MESSAGE, ...) \
static_assert(boost::geometry::detail::static_assert_check<false, __VA_ARGS__>::value, MESSAGE)


}}} // namespace boost::geometry::detail

#endif // BOOST_GEOMETRY_CORE_STATIC_ASSERT_HPP
