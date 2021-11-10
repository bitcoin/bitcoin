// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_POLYGON_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_POLYGON_HPP

// Adapts Geometries from Boost.Polygon for usage in Boost.Geometry
// boost::polygon::polygon_with_holes_data -> boost::geometry::polygon

#include <boost/polygon/polygon.hpp>

#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>

#include <boost/geometry/geometries/adapted/boost_polygon/ring_proxy.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/hole_iterator.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/holes_proxy.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename CoordinateType>
struct tag<boost::polygon::polygon_with_holes_data<CoordinateType> >
{
    typedef polygon_tag type;
};

template <typename CoordinateType>
struct ring_const_type<boost::polygon::polygon_with_holes_data<CoordinateType> >
{
    typedef adapt::bp::ring_proxy<boost::polygon::polygon_with_holes_data<CoordinateType> const> type;
};

template <typename CoordinateType>
struct ring_mutable_type<boost::polygon::polygon_with_holes_data<CoordinateType> >
{
    typedef adapt::bp::ring_proxy<boost::polygon::polygon_with_holes_data<CoordinateType> > type;
};

template <typename CoordinateType>
struct interior_const_type<boost::polygon::polygon_with_holes_data<CoordinateType> >
{
    typedef adapt::bp::holes_proxy<boost::polygon::polygon_with_holes_data<CoordinateType> const> type;
};

template <typename CoordinateType>
struct interior_mutable_type<boost::polygon::polygon_with_holes_data<CoordinateType> >
{
    typedef adapt::bp::holes_proxy<boost::polygon::polygon_with_holes_data<CoordinateType> > type;
};


template <typename CoordinateType>
struct exterior_ring<boost::polygon::polygon_with_holes_data<CoordinateType> >
{
    typedef boost::polygon::polygon_with_holes_data<CoordinateType> polygon_type;
    typedef adapt::bp::ring_proxy<polygon_type> proxy;
    typedef adapt::bp::ring_proxy<polygon_type const> const_proxy;

    static inline proxy get(polygon_type& p)
    {
        return proxy(p);
    }

    static inline const_proxy get(polygon_type const& p)
    {
        return const_proxy(p);
    }
};

template <typename CoordinateType>
struct interior_rings<boost::polygon::polygon_with_holes_data<CoordinateType> >
{
    typedef boost::polygon::polygon_with_holes_data<CoordinateType> polygon_type;
    typedef adapt::bp::holes_proxy<polygon_type> proxy;
    typedef adapt::bp::holes_proxy<polygon_type const> const_proxy;

    static inline proxy get(polygon_type& p)
    {
        return proxy(p);
    }

    static inline const_proxy get(polygon_type const& p)
    {
        return const_proxy(p);
    }
};



} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_POLYGON_HPP

