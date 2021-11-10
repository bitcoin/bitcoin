// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2018-2021.
// Modifications copyright (c) 2018-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_ASSIGN_VALUES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_ASSIGN_VALUES_HPP


#include <cstddef>
#include <type_traits>

#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/algorithm.hpp>
#include <boost/geometry/util/is_inverse_spheroidal_coordinates.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace assign
{


struct assign_zero_point
{
    template <typename Point>
    static inline void apply(Point& point)
    {
        typedef typename coordinate_type<Point>::type coordinate_type;

        coordinate_type const zero = 0;
        detail::for_each_dimension<Point>([&](auto dimension)
        {
            set<dimension>(point, zero);
        });
    }
};


struct assign_inverse_box_or_segment
{

    template <typename BoxOrSegment>
    static inline void apply(BoxOrSegment& geometry)
    {
        typedef typename coordinate_type<BoxOrSegment>::type coordinate_type;

        coordinate_type const highest = geometry::bounds<coordinate_type>::highest();
        coordinate_type const lowest = geometry::bounds<coordinate_type>::lowest();
        detail::for_each_dimension<BoxOrSegment>([&](auto dimension)
        {
            set<0, dimension>(geometry, highest);
            set<1, dimension>(geometry, lowest);
        });
    }

};


struct assign_zero_box_or_segment
{
    template <typename BoxOrSegment>
    static inline void apply(BoxOrSegment& geometry)
    {
        typedef typename coordinate_type<BoxOrSegment>::type coordinate_type;

        coordinate_type const zero = 0;
        detail::for_each_dimension<BoxOrSegment>([&](auto dimension)
        {
            set<0, dimension>(geometry, zero);
            set<1, dimension>(geometry, zero);
        });
    }
};


template
<
    std::size_t Corner1, std::size_t Corner2,
    typename Box, typename Point
>
inline void assign_box_2d_corner(Box const& box, Point& point)
{
    // Be sure both are 2-Dimensional
    assert_dimension<Box, 2>();
    assert_dimension<Point, 2>();

    // Copy coordinates
    typedef typename coordinate_type<Point>::type coordinate_type;

    geometry::set<0>(point, boost::numeric_cast<coordinate_type>(get<Corner1, 0>(box)));
    geometry::set<1>(point, boost::numeric_cast<coordinate_type>(get<Corner2, 1>(box)));
}



template <typename Geometry>
struct assign_2d_box_or_segment
{
    typedef typename coordinate_type<Geometry>::type coordinate_type;

    // Here we assign 4 coordinates to a box of segment
    // -> Most logical is: x1,y1,x2,y2
    // In case the user reverses x1/x2 or y1/y2, for a box, we could reverse them (THAT IS NOT IMPLEMENTED)

    template <typename Type>
    static inline void apply(Geometry& geometry,
                Type const& x1, Type const& y1, Type const& x2, Type const& y2)
    {
        geometry::set<0, 0>(geometry, boost::numeric_cast<coordinate_type>(x1));
        geometry::set<0, 1>(geometry, boost::numeric_cast<coordinate_type>(y1));
        geometry::set<1, 0>(geometry, boost::numeric_cast<coordinate_type>(x2));
        geometry::set<1, 1>(geometry, boost::numeric_cast<coordinate_type>(y2));
    }
};


}} // namespace detail::assign
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename GeometryTag, typename Geometry, std::size_t DimensionCount>
struct assign
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not or not yet implemented for this Geometry type.",
        GeometryTag, Geometry, std::integral_constant<std::size_t, DimensionCount>);
};

template <typename Point>
struct assign<point_tag, Point, 2>
{
    typedef typename coordinate_type<Point>::type coordinate_type;

    template <typename T>
    static inline void apply(Point& point, T const& c1, T const& c2)
    {
        set<0>(point, boost::numeric_cast<coordinate_type>(c1));
        set<1>(point, boost::numeric_cast<coordinate_type>(c2));
    }
};

template <typename Point>
struct assign<point_tag, Point, 3>
{
    typedef typename coordinate_type<Point>::type coordinate_type;

    template <typename T>
    static inline void apply(Point& point, T const& c1, T const& c2, T const& c3)
    {
        set<0>(point, boost::numeric_cast<coordinate_type>(c1));
        set<1>(point, boost::numeric_cast<coordinate_type>(c2));
        set<2>(point, boost::numeric_cast<coordinate_type>(c3));
    }
};

template <typename Box>
struct assign<box_tag, Box, 2>
    : detail::assign::assign_2d_box_or_segment<Box>
{};

template <typename Segment>
struct assign<segment_tag, Segment, 2>
    : detail::assign::assign_2d_box_or_segment<Segment>
{};



template <typename GeometryTag, typename Geometry>
struct assign_zero {};


template <typename Point>
struct assign_zero<point_tag, Point>
    : detail::assign::assign_zero_point
{};

template <typename Box>
struct assign_zero<box_tag, Box>
    : detail::assign::assign_zero_box_or_segment
{};

template <typename Segment>
struct assign_zero<segment_tag, Segment>
    : detail::assign::assign_zero_box_or_segment
{};


template <typename GeometryTag, typename Geometry>
struct assign_inverse {};

template <typename Box>
struct assign_inverse<box_tag, Box>
    : detail::assign::assign_inverse_box_or_segment
{};

template <typename Segment>
struct assign_inverse<segment_tag, Segment>
    : detail::assign::assign_inverse_box_or_segment
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_ASSIGN_VALUES_HPP
