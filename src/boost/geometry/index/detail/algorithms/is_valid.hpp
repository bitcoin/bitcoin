// Boost.Geometry Index
//
// n-dimensional Indexable validity check
//
// Copyright (c) 2011-2014 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_IS_VALID_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_IS_VALID_HPP

#include <cstddef>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/static_assert.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

namespace dispatch {

template <typename Box,
          std::size_t Dimension = geometry::dimension<Box>::value>
struct is_valid_box
{
    static inline bool apply(Box const& b)
    {
        return is_valid_box<Box, Dimension - 1>::apply(b) &&
            ( get<min_corner, Dimension - 1>(b) <= get<max_corner, Dimension - 1>(b) );
    }
};

template <typename Box>
struct is_valid_box<Box, 1>
{
    static inline bool apply(Box const& b)
    {
        return get<min_corner, 0>(b) <= get<max_corner, 0>(b);
    }
};

template <typename Indexable,
          typename Tag = typename geometry::tag<Indexable>::type>
struct is_valid
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Indexable type.",
        Indexable, Tag);
};

template <typename Indexable>
struct is_valid<Indexable, point_tag>
{
    static inline bool apply(Indexable const&)
    {
        return true;
    }
};

template <typename Indexable>
struct is_valid<Indexable, box_tag>
{
    static inline bool apply(Indexable const& b)
    {
        return dispatch::is_valid_box<Indexable>::apply(b);
    }
};

template <typename Indexable>
struct is_valid<Indexable, segment_tag>
{
    static inline bool apply(Indexable const&)
    {
        return true;
    }
};

} // namespace dispatch

template <typename Indexable>
inline bool is_valid(Indexable const& b)
{
    // CONSIDER: detection of NaNs
    // e.g. by comparison of b with copy of b

    return dispatch::is_valid<Indexable>::apply(b);
}

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_DETAIL_INDEX_ALGORITHMS_IS_VALID_HPP
