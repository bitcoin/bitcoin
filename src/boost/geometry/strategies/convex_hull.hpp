// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2018.
// Modifications copyright (c) 2018, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_HPP
#define BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_HPP

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/strategies/tags.hpp>


namespace boost { namespace geometry
{




/*!
    \brief Traits class binding a convex hull calculation strategy to a coordinate system
    \ingroup convex_hull
    \tparam Tag tag of coordinate system
    \tparam Geometry the geometry type (hull operates internally per hull over geometry)
    \tparam Point point-type of output points
*/
template
<
    typename Geometry1,
    typename Point,
    typename CsTag = typename cs_tag<Point>::type
>
struct strategy_convex_hull
{
    typedef strategy::not_implemented type;
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_HPP
