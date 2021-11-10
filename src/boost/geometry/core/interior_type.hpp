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


#ifndef BOOST_GEOMETRY_CORE_INTERIOR_TYPE_HPP
#define BOOST_GEOMETRY_CORE_INTERIOR_TYPE_HPP

#include <type_traits>

#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

namespace boost { namespace geometry
{

namespace traits
{

/*!
\brief Traits class indicating interior container type of a polygon
\details defines inner container type, so the container containing
        the interior rings
\ingroup traits
\par Geometries:
    - polygon
\par Specializations should provide:
    - typedef X type ( e.g. std::vector&lt;myring&lt;P&gt;&gt; )
\tparam Geometry geometry
*/
template <typename Geometry>
struct interior_const_type
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};

template <typename Geometry>
struct interior_mutable_type
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};


} // namespace traits




#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{


template <typename GeometryTag, typename Geometry>
struct interior_return_type
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};


template <typename Polygon>
struct interior_return_type<polygon_tag, Polygon>
{
    typedef typename std::remove_const<Polygon>::type nc_polygon_type;

    typedef std::conditional_t
        <
            std::is_const<Polygon>::value,
            typename traits::interior_const_type<nc_polygon_type>::type,
            typename traits::interior_mutable_type<nc_polygon_type>::type
        > type;
};




template <typename GeometryTag, typename Geometry>
struct interior_type
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};


template <typename Polygon>
struct interior_type<polygon_tag, Polygon>
{
    typedef typename std::remove_reference
        <
            typename interior_return_type<polygon_tag, Polygon>::type
        >::type type;
};


} // namespace core_dispatch
#endif


/*!
\brief \brief_meta{type, interior_type (container type
    of inner rings), \meta_geometry_type}
\details Interior rings should be organized as a container
    (std::vector, std::deque, boost::array) with
    Boost.Range support. This metafunction defines the type
    of the container.
\tparam Geometry A type fullfilling the Polygon or MultiPolygon concept.
\ingroup core

\qbk{[include reference/core/interior_type.qbk]}
*/
template <typename Geometry>
struct interior_type
{
    typedef typename core_dispatch::interior_type
        <
            typename tag<Geometry>::type,
            Geometry
        >::type type;
};

template <typename Geometry>
struct interior_return_type
{
    typedef typename core_dispatch::interior_return_type
        <
            typename tag<Geometry>::type,
            Geometry
        >::type type;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_INTERIOR_TYPE_HPP
