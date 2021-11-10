// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_COORDINATE_TYPE_HPP
#define BOOST_GEOMETRY_CORE_COORDINATE_TYPE_HPP


#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/type_traits_std.hpp>


namespace boost { namespace geometry
{

namespace traits
{

/*!
\brief Traits class which indicate the coordinate type (double,float,...) of a point
\ingroup traits
\par Geometries:
    - point
\par Specializations should provide:
    - typedef T type; (double,float,int,etc)
*/
template <typename Point, typename Enable = void>
struct coordinate_type
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Point type.",
        Point);
};

} // namespace traits

#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename GeometryTag, typename Geometry>
struct coordinate_type
{
    typedef typename point_type<GeometryTag, Geometry>::type point_type;

    // Call its own specialization on point-tag
    typedef typename coordinate_type<point_tag, point_type>::type type;
};

template <typename Point>
struct coordinate_type<point_tag, Point>
{
    typedef typename traits::coordinate_type
        <
            typename util::remove_cptrref<Point>::type
        >::type type;
};


} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_meta{type, coordinate type (int\, float\, double\, etc), \meta_point_type}
\tparam Geometry \tparam_geometry
\ingroup core

\qbk{[include reference/core/coordinate_type.qbk]}
*/
template <typename Geometry>
struct coordinate_type
{
    typedef typename core_dispatch::coordinate_type
        <
            typename tag<Geometry>::type,
            typename util::remove_cptrref<Geometry>::type
        >::type type;
};

template <typename Geometry>
struct fp_coordinate_type
{
    typedef typename promote_floating_point
        <
            typename coordinate_type<Geometry>::type
        >::type type;
};

/*!
\brief assert_coordinate_type_equal, a compile-time check for equality of two coordinate types
\ingroup utility
*/
template <typename Geometry1, typename Geometry2>
constexpr inline void assert_coordinate_type_equal(Geometry1 const& , Geometry2 const& )
{
  static_assert(std::is_same
      <
          typename coordinate_type<Geometry1>::type,
          typename coordinate_type<Geometry2>::type
      >::value, "Coordinate types in geometries should be the same");
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_COORDINATE_TYPE_HPP
