// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_AS_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_AS_RANGE_HPP


#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct as_range : not_implemented<Geometry, Tag>
{};

template <typename Geometry>
struct as_range<Geometry, linestring_tag>
{
    static inline typename ring_return_type<Geometry>::type get(Geometry& geometry)
    {
        return geometry;
    }
};

template <typename Geometry>
struct as_range<Geometry, ring_tag>
    : as_range<Geometry, linestring_tag>
{};

template <typename Geometry>
struct as_range<Geometry, polygon_tag>
{
    static inline typename ring_return_type<Geometry>::type get(Geometry& geometry)
    {
        return exterior_ring(geometry);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

// Will probably be replaced by the more generic "view_as", therefore in detail
namespace detail
{

/*!
\brief Function getting either the range (ring, linestring) itself
or the outer ring (polygon)
\details Utility to handle polygon's outer ring as a range
\ingroup utility
*/
template <typename Geometry>
inline typename ring_return_type<Geometry>::type as_range(Geometry& geometry)
{
    return dispatch::as_range<Geometry>::get(geometry);
}

}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_AS_RANGE_HPP
