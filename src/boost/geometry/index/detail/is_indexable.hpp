// Boost.Geometry Index
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_IS_INDEXABLE_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_IS_INDEXABLE_HPP

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

template
<
    typename Geometry,
    typename Tag = typename geometry::tag<Geometry>::type
>
struct is_indexable
{
    static const bool value = false;
};

template <typename Point>
struct is_indexable<Point, geometry::point_tag>
{
    static const bool value = true;
};

template <typename Box>
struct is_indexable<Box, geometry::box_tag>
{
    static const bool value = true;
};

template <typename Segment>
struct is_indexable<Segment, geometry::segment_tag>
{
    static const bool value = true;
};

}}}} // namespave boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_IS_INDEXABLE_HPP
