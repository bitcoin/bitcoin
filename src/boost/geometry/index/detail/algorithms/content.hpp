// Boost.Geometry Index
//
// n-dimensional content (hypervolume) - 2d area, 3d volume, ...
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

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_CONTENT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_CONTENT_HPP

#include <boost/geometry/core/static_assert.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

template <typename Indexable>
struct default_content_result
{
    typedef typename select_most_precise<
        typename coordinate_type<Indexable>::type,
        long double
    >::type type;
};

namespace dispatch {

template <typename Box,
          std::size_t CurrentDimension = dimension<Box>::value>
struct content_box
{
    BOOST_STATIC_ASSERT(0 < CurrentDimension);

    static inline typename detail::default_content_result<Box>::type apply(Box const& b)
    {
        return content_box<Box, CurrentDimension - 1>::apply(b) *
            ( get<max_corner, CurrentDimension - 1>(b) - get<min_corner, CurrentDimension - 1>(b) );
    }
};

template <typename Box>
struct content_box<Box, 1>
{
    static inline typename detail::default_content_result<Box>::type apply(Box const& b)
    {
        return get<max_corner, 0>(b) - get<min_corner, 0>(b);
    }
};

template <typename Indexable, typename Tag>
struct content
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Indexable and Tag.",
        Indexable, Tag);
};

template <typename Indexable>
struct content<Indexable, point_tag>
{
    static typename detail::default_content_result<Indexable>::type apply(Indexable const&)
    {
        return 0;
    }
};

template <typename Indexable>
struct content<Indexable, box_tag>
{
    static typename default_content_result<Indexable>::type apply(Indexable const& b)
    {
        return dispatch::content_box<Indexable>::apply(b);
    }
};

} // namespace dispatch

template <typename Indexable>
typename default_content_result<Indexable>::type content(Indexable const& b)
{
    return dispatch::content
            <
                Indexable,
                typename tag<Indexable>::type
            >::apply(b);
}

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_CONTENT_HPP
