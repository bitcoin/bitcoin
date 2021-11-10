// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_CORE_TOPOLOGICAL_DIMENSION_HPP
#define BOOST_GEOMETRY_CORE_TOPOLOGICAL_DIMENSION_HPP


#include <type_traits>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{


template <typename GeometryTag>
struct top_dim {};


template <>
struct top_dim<point_tag>      : std::integral_constant<int, 0> {};


template <>
struct top_dim<linestring_tag> : std::integral_constant<int, 1> {};


template <>
struct top_dim<segment_tag>    : std::integral_constant<int, 1> {};


// ring: topological dimension of two, but some people say: 1 !!
// NOTE: This is not OGC LinearRing!
template <>
struct top_dim<ring_tag>       : std::integral_constant<int, 2> {};


// TODO: This is wrong! Boxes may have various topological dimensions
template <>
struct top_dim<box_tag>        : std::integral_constant<int, 2> {};


template <>
struct top_dim<polygon_tag>    : std::integral_constant<int, 2> {};


template <>
struct top_dim<multi_point_tag> : std::integral_constant<int, 0> {};


template <>
struct top_dim<multi_linestring_tag> : std::integral_constant<int, 1> {};


template <>
struct top_dim<multi_polygon_tag> : std::integral_constant<int, 2> {};


} // namespace core_dispatch
#endif





/*!
    \brief Meta-function returning the topological dimension of a geometry
    \details The topological dimension defines a point as 0-dimensional,
        a linestring as 1-dimensional,
        and a ring or polygon as 2-dimensional.
    \see http://www.math.okstate.edu/mathdept/dynamics/lecnotes/node36.html
    \ingroup core
*/
template <typename Geometry>
struct topological_dimension
    : core_dispatch::top_dim<typename tag<Geometry>::type> {};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_TOPOLOGICAL_DIMENSION_HPP
