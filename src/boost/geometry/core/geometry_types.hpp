// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_CORE_GEOMETRY_TYPES_HPP
#define BOOST_GEOMETRY_CORE_GEOMETRY_TYPES_HPP

#include <boost/range/value_type.hpp>

#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

namespace boost { namespace geometry
{

namespace traits
{

// DynamicGeometry or GeometryCollection
template <typename Geometry>
struct geometry_types;

} // namespace traits

#ifndef DOXYGEN_NO_DISPATCH
namespace traits_dispatch
{

template <typename Geometry, typename Tag = typename geometry::tag<Geometry>::type>
struct geometry_types
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};

// By default treat GeometryCollection as a range of DynamicGeometries
template <typename Geometry>
struct geometry_types<Geometry, geometry_collection_tag>
    : traits::geometry_types<typename boost::range_value<Geometry>::type>
{};

} // namespace traits_dispatch
#endif // DOXYGEN_NO_DISPATCH

namespace traits
{

// DynamicGeometry or GeometryCollection
template <typename Geometry>
struct geometry_types
    : traits_dispatch::geometry_types<Geometry>
{};

} // namespace traits

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_GEOMETRY_TYPES_HPP
