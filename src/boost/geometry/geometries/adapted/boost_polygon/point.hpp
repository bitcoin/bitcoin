// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_POINT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_POINT_HPP

// Adapts Geometries from Boost.Polygon for usage in Boost.Geometry
// boost::polygon::point_data -> boost::geometry::point


#include <type_traits>

#include <boost/polygon/polygon.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{


template <typename CoordinateType>
struct tag<boost::polygon::point_data<CoordinateType> >
{
    typedef point_tag type;
};


template <typename CoordinateType>
struct coordinate_type<boost::polygon::point_data<CoordinateType> >
{
    typedef CoordinateType type;
};


template <typename CoordinateType>
struct coordinate_system<boost::polygon::point_data<CoordinateType> >
{
    typedef cs::cartesian type;
};


template <typename CoordinateType>
struct dimension<boost::polygon::point_data<CoordinateType> >
    : std::integral_constant<std::size_t, 2>
{};


template <typename CoordinateType>
struct access<boost::polygon::point_data<CoordinateType>, 0>
{
    typedef boost::polygon::point_data<CoordinateType> point_type;

    static inline CoordinateType get(point_type const& p)
    {
        return p.x();
    }

    static inline void set(point_type& p, CoordinateType const& value)
    {
        p.x(value);
    }
};


template <typename CoordinateType>
struct access<boost::polygon::point_data<CoordinateType>, 1>
{
    typedef boost::polygon::point_data<CoordinateType> point_type;

    static inline CoordinateType get(point_type const& p)
    {
        return p.y();
    }

    static inline void set(point_type& p, CoordinateType const& value)
    {
        p.y(value);
    }
};


} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_POINT_HPP
