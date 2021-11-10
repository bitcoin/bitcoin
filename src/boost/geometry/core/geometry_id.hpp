// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_CORE_GEOMETRY_ID_HPP
#define BOOST_GEOMETRY_CORE_GEOMETRY_ID_HPP


#include <type_traits>

#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename GeometryTag>
struct geometry_id
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        GeometryTag);
};


template <>
struct geometry_id<point_tag>            : std::integral_constant<int, 1> {};


template <>
struct geometry_id<linestring_tag>       : std::integral_constant<int, 2> {};


template <>
struct geometry_id<polygon_tag>          : std::integral_constant<int, 3> {};


template <>
struct geometry_id<multi_point_tag>      : std::integral_constant<int, 4> {};


template <>
struct geometry_id<multi_linestring_tag> : std::integral_constant<int, 5> {};


template <>
struct geometry_id<multi_polygon_tag>    : std::integral_constant<int, 6> {};


template <>
struct geometry_id<segment_tag>          : std::integral_constant<int, 92> {};


template <>
struct geometry_id<ring_tag>             : std::integral_constant<int, 93> {};


template <>
struct geometry_id<box_tag>              : std::integral_constant<int, 94> {};


} // namespace core_dispatch
#endif



/*!
\brief Meta-function returning the id of a geometry type
\details The meta-function geometry_id defines a numerical ID (based on
    std::integral_constant<int, ...> ) for each geometry concept. A numerical ID is
    sometimes useful, and within Boost.Geometry it is used for the
    reverse_dispatch metafuntion.
\note Used for e.g. reverse meta-function
\ingroup core
*/
template <typename Geometry>
struct geometry_id : core_dispatch::geometry_id<typename tag<Geometry>::type>
{};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_GEOMETRY_ID_HPP
