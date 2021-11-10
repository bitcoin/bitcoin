// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_DYNAMIC_GEOMETRY_CONCEPT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_DYNAMIC_GEOMETRY_CONCEPT_HPP


#include <utility>

#include <boost/concept_check.hpp>

#include <boost/geometry/core/geometry_types.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>

#include <boost/geometry/geometries/concepts/box_concept.hpp>
#include <boost/geometry/geometries/concepts/concept_type.hpp>
#include <boost/geometry/geometries/concepts/geometry_collection_concept.hpp>
#include <boost/geometry/geometries/concepts/linestring_concept.hpp>
#include <boost/geometry/geometries/concepts/multi_point_concept.hpp>
#include <boost/geometry/geometries/concepts/multi_linestring_concept.hpp>
#include <boost/geometry/geometries/concepts/multi_polygon_concept.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/concepts/polygon_concept.hpp>
#include <boost/geometry/geometries/concepts/ring_concept.hpp>
#include <boost/geometry/geometries/concepts/segment_concept.hpp>


namespace boost { namespace geometry { namespace concepts
{

namespace detail
{

template <typename Geometry, typename SubGeometry>
struct GeometryType<Geometry, SubGeometry, dynamic_geometry_tag, false>
    : concepts::concept_type<SubGeometry>::type
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    BOOST_CONCEPT_USAGE(GeometryType)
    {
        Geometry* dg = nullptr;
        SubGeometry* sg = nullptr;
        *dg = std::move(*sg);
    }
#endif // DOXYGEN_NO_CONCEPT_MEMBERS
};

template <typename Geometry, typename SubGeometry>
struct GeometryType<Geometry const, SubGeometry, dynamic_geometry_tag, false>
    : concepts::concept_type<SubGeometry const>::type
{};


} // namespace detail


template <typename Geometry>
struct DynamicGeometry
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    using sequence_t = typename traits::geometry_types<Geometry>::type;
    BOOST_CONCEPT_ASSERT((detail::GeometryTypes<Geometry, sequence_t>));

    BOOST_CONCEPT_USAGE(DynamicGeometry)
    {
        Geometry* dg = nullptr;
        traits::visit<Geometry>::apply([](auto &&) {}, *dg);
    }
#endif // DOXYGEN_NO_CONCEPT_MEMBERS
};


template <typename Geometry>
struct ConstDynamicGeometry
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    using sequence_t = typename traits::geometry_types<Geometry>::type;
    BOOST_CONCEPT_ASSERT((detail::GeometryTypes<Geometry const, sequence_t>));

    BOOST_CONCEPT_USAGE(ConstDynamicGeometry)
    {
        Geometry const* dg = nullptr;
        traits::visit<Geometry>::apply([](auto &&) {}, *dg);
    }
#endif // DOXYGEN_NO_CONCEPT_MEMBERS
};


template <typename Geometry>
struct concept_type<Geometry, dynamic_geometry_tag>
{
    using type = DynamicGeometry<Geometry>;
};

template <typename Geometry>
struct concept_type<Geometry const, dynamic_geometry_tag>
{
    using type = ConstDynamicGeometry<Geometry>;
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_DYNAMIC_GEOMETRY_CONCEPT_HPP
