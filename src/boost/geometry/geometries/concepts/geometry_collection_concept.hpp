// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_GEOMETRY_COLLECTION_CONCEPT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_GEOMETRY_COLLECTION_CONCEPT_HPP


#include <utility>

#include <boost/concept_check.hpp>
#include <boost/range/concepts.hpp>

#include <boost/geometry/core/geometry_types.hpp>
#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>

#include <boost/geometry/geometries/concepts/box_concept.hpp>
#include <boost/geometry/geometries/concepts/concept_type.hpp>
#include <boost/geometry/geometries/concepts/linestring_concept.hpp>
#include <boost/geometry/geometries/concepts/multi_point_concept.hpp>
#include <boost/geometry/geometries/concepts/multi_linestring_concept.hpp>
#include <boost/geometry/geometries/concepts/multi_polygon_concept.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/concepts/polygon_concept.hpp>
#include <boost/geometry/geometries/concepts/ring_concept.hpp>
#include <boost/geometry/geometries/concepts/segment_concept.hpp>

#include <boost/geometry/util/sequence.hpp>
#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry { namespace concepts
{

namespace detail
{

template
<
    typename Geometry,
    typename SubGeometry,
    typename Tag = typename tag<Geometry>::type,
    bool IsSubDynamicOrCollection = util::is_dynamic_geometry<SubGeometry>::value
                                 || util::is_geometry_collection<SubGeometry>::value
>
struct GeometryType;

// Prevent recursive concept checking
template <typename Geometry, typename SubGeometry, typename Tag>
struct GeometryType<Geometry, SubGeometry, Tag, true> {};

template <typename Geometry, typename SubGeometry, typename Tag>
struct GeometryType<Geometry const, SubGeometry, Tag, true> {};


template <typename Geometry, typename SubGeometry>
struct GeometryType<Geometry, SubGeometry, geometry_collection_tag, false>
    : concepts::concept_type<SubGeometry>::type
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    BOOST_CONCEPT_USAGE(GeometryType)
    {
        Geometry* gc = nullptr;
        SubGeometry* sg = nullptr;
        traits::emplace_back<Geometry>::apply(*gc, std::move(*sg));
    }
#endif // DOXYGEN_NO_CONCEPT_MEMBERS
};

template <typename Geometry, typename SubGeometry>
struct GeometryType<Geometry const, SubGeometry, geometry_collection_tag, false>
    : concepts::concept_type<SubGeometry const>::type
{};


template <typename Geometry, typename ...SubGeometries>
struct GeometryTypesPack {};

template <typename Geometry, typename SubGeometry, typename ...SubGeometries>
struct GeometryTypesPack<Geometry, SubGeometry, SubGeometries...>
    : GeometryTypesPack<Geometry, SubGeometries...>
    , GeometryType<Geometry, SubGeometry>
{};


template <typename Geometry, typename SubGeometriesSequence>
struct GeometryTypes;

template <typename Geometry, typename ...SubGeometries>
struct GeometryTypes<Geometry, util::type_sequence<SubGeometries...>>
    : GeometryTypesPack<Geometry, SubGeometries...>
{};


} // namespace detail


template <typename Geometry>
struct GeometryCollection
    : boost::ForwardRangeConcept<Geometry>
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    using sequence_t = typename traits::geometry_types<Geometry>::type;
    BOOST_CONCEPT_ASSERT( (detail::GeometryTypes<Geometry, sequence_t>) );

    BOOST_CONCEPT_USAGE(GeometryCollection)
    {
        Geometry* gc = nullptr;        
        traits::clear<Geometry>::apply(*gc);        
        traits::iter_visit<Geometry>::apply([](auto &&) {}, boost::begin(*gc));
    }
#endif // DOXYGEN_NO_CONCEPT_MEMBERS
};


template <typename Geometry>
struct ConstGeometryCollection
    : boost::ForwardRangeConcept<Geometry>
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    using sequence_t = typename traits::geometry_types<Geometry>::type;
    BOOST_CONCEPT_ASSERT( (detail::GeometryTypes<Geometry const, sequence_t>) );

    BOOST_CONCEPT_USAGE(ConstGeometryCollection)
    {
        Geometry const* gc = nullptr;
        traits::iter_visit<Geometry>::apply([](auto &&) {}, boost::begin(*gc));
    }
#endif // DOXYGEN_NO_CONCEPT_MEMBERS
};


template <typename Geometry>
struct concept_type<Geometry, geometry_collection_tag>
{
    using type = GeometryCollection<Geometry>;
};

template <typename Geometry>
struct concept_type<Geometry const, geometry_collection_tag>
{
    using type = ConstGeometryCollection<Geometry>;
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_GEOMETRY_COLLECTION_CONCEPT_HPP
