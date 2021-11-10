// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DUMMY_GEOMETRIES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DUMMY_GEOMETRIES_HPP


#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct dummy_point {};
struct dummy_segment {};
struct dummy_box {};
struct dummy_linestring {};
struct dummy_ring {};
struct dummy_polygon {};
struct dummy_multi_point {};
struct dummy_multi_linestring {};
struct dummy_multi_polygon {};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

namespace traits
{

template <> struct tag<geometry::detail::dummy_point> { typedef point_tag type; };
template <> struct tag<geometry::detail::dummy_segment> { typedef segment_tag type; };
template <> struct tag<geometry::detail::dummy_box> { typedef box_tag type; };
template <> struct tag<geometry::detail::dummy_linestring> { typedef linestring_tag type; };
template <> struct tag<geometry::detail::dummy_ring> { typedef ring_tag type; };
template <> struct tag<geometry::detail::dummy_polygon> { typedef polygon_tag type; };
template <> struct tag<geometry::detail::dummy_multi_point> { typedef multi_point_tag type; };
template <> struct tag<geometry::detail::dummy_multi_linestring> { typedef multi_linestring_tag type; };
template <> struct tag<geometry::detail::dummy_multi_polygon> { typedef multi_polygon_tag type; };

} // namespace traits

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DUMMY_GEOMETRIES_HPP
