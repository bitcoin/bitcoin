// Boost.Geometry Index
//
// n-dimensional box's margin value (hypersurface), 2d perimeter, 3d surface, etc...
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

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_MARGIN_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_MARGIN_HPP

#include <boost/geometry/core/static_assert.hpp>

// WARNING! comparable_margin() will work only if the same Geometries are compared
// so it shouldn't be used in the case of Variants!

namespace boost { namespace geometry { namespace index { namespace detail {

template <typename Box>
struct default_margin_result
{
    typedef typename select_most_precise<
        typename coordinate_type<Box>::type,
        long double
    >::type type;
};

//template <typename Box,
//          std::size_t CurrentDimension,
//          std::size_t EdgeDimension = dimension<Box>::value>
//struct margin_for_each_edge
//{
//    BOOST_STATIC_ASSERT(0 < CurrentDimension);
//    BOOST_STATIC_ASSERT(0 < EdgeDimension);
//
//    static inline typename default_margin_result<Box>::type apply(Box const& b)
//    {
//        return margin_for_each_edge<Box, CurrentDimension, EdgeDimension - 1>::apply(b) *
//            ( geometry::get<max_corner, EdgeDimension - 1>(b) - geometry::get<min_corner, EdgeDimension - 1>(b) );
//    }
//};
//
//template <typename Box, std::size_t CurrentDimension>
//struct margin_for_each_edge<Box, CurrentDimension, CurrentDimension>
//{
//    BOOST_STATIC_ASSERT(0 < CurrentDimension);
//
//    static inline typename default_margin_result<Box>::type apply(Box const& b)
//    {
//        return margin_for_each_edge<Box, CurrentDimension, CurrentDimension - 1>::apply(b);
//    }
//};
//
//template <typename Box, std::size_t CurrentDimension>
//struct margin_for_each_edge<Box, CurrentDimension, 1>
//{
//    BOOST_STATIC_ASSERT(0 < CurrentDimension);
//
//    static inline typename default_margin_result<Box>::type apply(Box const& b)
//    {
//        return geometry::get<max_corner, 0>(b) - geometry::get<min_corner, 0>(b);
//    }
//};
//
//template <typename Box>
//struct margin_for_each_edge<Box, 1, 1>
//{
//    static inline typename default_margin_result<Box>::type apply(Box const& /*b*/)
//    {
//        return 1;
//    }
//};
//
//template <typename Box,
//          std::size_t CurrentDimension = dimension<Box>::value>
//struct margin_for_each_dimension
//{
//    BOOST_STATIC_ASSERT(0 < CurrentDimension);
//
//    static inline typename default_margin_result<Box>::type apply(Box const& b)
//    {
//        return margin_for_each_dimension<Box, CurrentDimension - 1>::apply(b) +
//            margin_for_each_edge<Box, CurrentDimension>::apply(b);
//    }
//};
//
//template <typename Box>
//struct margin_for_each_dimension<Box, 1>
//{
//    static inline typename default_margin_result<Box>::type apply(Box const& b)
//    {
//        return margin_for_each_edge<Box, 1>::apply(b);
//    }
//};

// TODO - test if this definition of margin is ok for Dimension > 2
// Now it's sum of edges lengths
// maybe margin_for_each_dimension should be used to get more or less hypersurface?

template <typename Box,
          std::size_t CurrentDimension = dimension<Box>::value>
struct simple_margin_for_each_dimension
{
    BOOST_STATIC_ASSERT(0 < CurrentDimension);

    static inline typename default_margin_result<Box>::type apply(Box const& b)
    {
        return simple_margin_for_each_dimension<Box, CurrentDimension - 1>::apply(b) +
            geometry::get<max_corner, CurrentDimension - 1>(b) - geometry::get<min_corner, CurrentDimension - 1>(b);
    }
};

template <typename Box>
struct simple_margin_for_each_dimension<Box, 1>
{
    static inline typename default_margin_result<Box>::type apply(Box const& b)
    {
        return geometry::get<max_corner, 0>(b) - geometry::get<min_corner, 0>(b);
    }
};

namespace dispatch {

template <typename Geometry, typename Tag>
struct comparable_margin
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry, Tag);
};

template <typename Geometry>
struct comparable_margin<Geometry, point_tag>
{
    typedef typename default_margin_result<Geometry>::type result_type;

    static inline result_type apply(Geometry const& ) { return 0; }
};

template <typename Box>
struct comparable_margin<Box, box_tag>
{
    typedef typename default_margin_result<Box>::type result_type;

    static inline result_type apply(Box const& g)
    {
        //return detail::margin_for_each_dimension<Box>::apply(g);
        return detail::simple_margin_for_each_dimension<Box>::apply(g);
    }
};

} // namespace dispatch

template <typename Geometry>
typename default_margin_result<Geometry>::type comparable_margin(Geometry const& g)
{
    return dispatch::comparable_margin<
        Geometry,
        typename tag<Geometry>::type
    >::apply(g);
}

//template <typename Box>
//typename default_margin_result<Box>::type margin(Box const& b)
//{
//    return 2 * detail::margin_for_each_dimension<Box>::apply(b);
//}

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_MARGIN_HPP
