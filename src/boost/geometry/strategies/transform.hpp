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

#ifndef BOOST_GEOMETRY_STRATEGIES_TRANSFORM_HPP
#define BOOST_GEOMETRY_STRATEGIES_TRANSFORM_HPP

#include <cstddef>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/strategies/tags.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace transform { namespace services
{

/*!
    \brief Traits class binding a transformation strategy to a coordinate system
    \ingroup transform
    \details Can be specialized
    - per coordinate system family (tag)
    - per coordinate system (or groups of them)
    - per dimension
    - per point type
    \tparam CoordinateSystemTag 1,2 coordinate system tags
    \tparam CoordinateSystem 1,2 coordinate system
    \tparam D 1, 2 dimension
    \tparam Point 1, 2 point type
 */
template
<
    typename CoordinateSystemTag1, typename CoordinateSystemTag2,
    typename CoordinateSystem1, typename CoordinateSystem2,
    std::size_t Dimension1, std::size_t Dimension2,
    typename Point1, typename Point2
>
struct default_strategy
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for these Point types.",
        Point1, Point2);
};

}}} // namespace strategy::transform::services


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_TRANSFORM_HPP
