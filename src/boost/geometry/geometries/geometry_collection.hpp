// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_GEOMETRY_COLLECTION_HPP
#define BOOST_GEOMETRY_GEOMETRIES_GEOMETRY_COLLECTION_HPP

#include <vector>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

namespace boost { namespace geometry
{

namespace model
{

/*!
\brief Basic geometry_collection class representing a container of DynamicGeometries.
\ingroup geometries
\tparam DynamicGeometry Type adapted to DynamicGeometry Concept.
\tparam Container \tparam_container
\tparam Allocator \tparam_allocator
*/
template
<
    typename DynamicGeometry,
    template <typename, typename> class Container = std::vector,
    template <typename> class Allocator = std::allocator
>
class geometry_collection
    : public Container<DynamicGeometry, Allocator<DynamicGeometry>>
{
    typedef Container<DynamicGeometry, Allocator<DynamicGeometry>> base_type;

public:
    geometry_collection() = default;

    geometry_collection(std::initializer_list<DynamicGeometry> l)
        : base_type(l.begin(), l.end())
    {}
};

} // namespace model


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template
<
    typename DynamicGeometry,
    template <typename, typename> class Container,
    template <typename> class Allocator
>
struct tag<model::geometry_collection<DynamicGeometry, Container, Allocator>>
{
    using type = geometry_collection_tag;
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_GEOMETRY_COLLECTION_HPP
