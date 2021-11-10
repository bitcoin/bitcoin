// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_AZIMUTH_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_AZIMUTH_CARTESIAN_HPP


// TODO: move this file to boost/geometry/strategy
#include <boost/geometry/strategies/cartesian/azimuth.hpp>

#include <boost/geometry/strategies/azimuth/services.hpp>
#include <boost/geometry/strategies/detail.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace azimuth
{

template <typename CalculationType = void>
struct cartesian : strategies::detail::cartesian_base
{
    static auto azimuth()
    {
        return strategy::azimuth::cartesian<CalculationType>();
    }
};


namespace services
{

template <typename Point1, typename Point2>
struct default_strategy<Point1, Point2, cartesian_tag, cartesian_tag>
{
    using type = strategies::azimuth::cartesian<>;
};


template <typename CT>
struct strategy_converter<strategy::azimuth::cartesian<CT> >
{
    static auto get(strategy::azimuth::cartesian<CT> const&)
    {
        return strategies::azimuth::cartesian<CT>();
    }
};


} // namespace services

}} // namespace strategies::azimuth

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AZIMUTH_CARTESIAN_HPP
