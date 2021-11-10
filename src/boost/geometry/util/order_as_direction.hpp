// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2021.
// Modifications copyright (c) 2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_ORDER_AS_DIRECTION_HPP
#define BOOST_GEOMETRY_UTIL_ORDER_AS_DIRECTION_HPP

#include <boost/geometry/core/point_order.hpp>

namespace boost { namespace geometry
{


/*!
\brief Flag for iterating a reversible_view in forward or reverse direction
\ingroup views
*/
enum iterate_direction { iterate_forward, iterate_reverse };


template<order_selector Order>
struct order_as_direction
{};


template<>
struct order_as_direction<clockwise>
{
    static const iterate_direction value = iterate_forward;
};


template<>
struct order_as_direction<counterclockwise>
{
    static const iterate_direction value = iterate_reverse;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_UTIL_ORDER_AS_DIRECTION_HPP
