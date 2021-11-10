// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_AZIMUTH_SERVICES_HPP
#define BOOST_GEOMETRY_STRATEGIES_AZIMUTH_SERVICES_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/static_assert.hpp>


namespace boost { namespace geometry
{


namespace strategies { namespace azimuth {

namespace services
{

template
<
    typename Point1, typename Point2,
    typename CSTag1 = typename geometry::cs_tag<Point1>::type,
    typename CSTag2 = typename geometry::cs_tag<Point2>::type
>
struct default_strategy
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for these Points' coordinate systems.",
        Point1, Point2, CSTag1, CSTag2);
};

template <typename Strategy>
struct strategy_converter
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Strategy.",
        Strategy);
};


} // namespace services

}} // namespace strategies::azimuth


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AZIMUTH_SERVICES_HPP
