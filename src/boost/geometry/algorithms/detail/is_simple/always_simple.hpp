// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2017, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_ALWAYS_SIMPLE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_ALWAYS_SIMPLE_HPP

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/dispatch/is_simple.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_simple
{


template <typename Geometry>
struct always_simple
{
    template <typename Strategy>
    static inline bool apply(Geometry const&, Strategy const&)
    {
        return true;
    }
};


}} // namespace detail::is_simple
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// A point is always simple
template <typename Point>
struct is_simple<Point, point_tag>
    : detail::is_simple::always_simple<Point>
{};


// A valid segment is always simple.
// A segment is a curve.
// A curve is simple if it does not pass through the same point twice,
// with the possible exception of its two endpoints
//
// Reference: OGC 06-103r4 (6.1.6.1)
template <typename Segment>
struct is_simple<Segment, segment_tag>
    : detail::is_simple::always_simple<Segment>
{};


// A valid box is always simple
// A box is a Polygon, and it satisfies the conditions for Polygon validity.
//
// Reference (for polygon validity): OGC 06-103r4 (6.1.11.1)
template <typename Box>
struct is_simple<Box, box_tag>
    : detail::is_simple::always_simple<Box>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_ALWAYS_SIMPLE_HPP
