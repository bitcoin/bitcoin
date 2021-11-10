// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_AZIMUTH_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_AZIMUTH_SPHERICAL_HPP


// TODO: move this file to boost/geometry/strategy
#include <boost/geometry/strategies/spherical/azimuth.hpp>

#include <boost/geometry/strategies/azimuth/services.hpp>
#include <boost/geometry/strategies/detail.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace azimuth
{

template <typename CalculationType = void>
class spherical : strategies::detail::spherical_base<void>
{
    using base_t = strategies::detail::spherical_base<void>;

public:
    
    static auto azimuth()
    {
        return strategy::azimuth::spherical<CalculationType>();
    }
};


namespace services
{


template <typename Point1, typename Point2>
struct default_strategy<Point1, Point2, spherical_equatorial_tag, spherical_equatorial_tag>
{
    using type = strategies::azimuth::spherical<>;
};

template <typename CT>
struct strategy_converter<strategy::azimuth::spherical<CT> >
{
    static auto get(strategy::azimuth::spherical<CT> const&)
    {
        return strategies::azimuth::spherical<CT>();
    }
};


} // namespace services

}} // namespace strategies::azimuth

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AZIMUTH_SPHERICAL_HPP
