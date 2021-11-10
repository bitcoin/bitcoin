// Boost.Geometry Index
//
// Abs of difference
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_DIFF_ABS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_DIFF_ABS_HPP

#include <type_traits>

namespace boost { namespace geometry { namespace index { namespace detail
{

template
<
    typename T,
    std::enable_if_t<std::is_integral<T>::value, int> = 0
>
inline T diff_abs(T const& v1, T const& v2)
{
    return v1 < v2 ? v2 - v1 : v1 - v2;
}

template
<
    typename T,
    std::enable_if_t<! std::is_integral<T>::value, int> = 0
>
inline T diff_abs(T const& v1, T const& v2)
{
    return ::fabs(v1 - v2);
}

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_DIFF_ABS_HPP
