// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_AZIMUTH_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_AZIMUTH_GEOGRAPHIC_HPP


// TODO: move this file to boost/geometry/strategy
#include <boost/geometry/strategies/geographic/azimuth.hpp>

#include <boost/geometry/strategies/azimuth/services.hpp>
#include <boost/geometry/strategies/detail.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace azimuth
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic : strategies::detail::geographic_base<Spheroid>
{
    using base_t = strategies::detail::geographic_base<Spheroid>;

public:
    geographic()
        : base_t()
    {}

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}

    auto azimuth() const
    {
        return strategy::azimuth::geographic
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }
};


namespace services
{

template <typename Point1, typename Point2>
struct default_strategy<Point1, Point2, geographic_tag, geographic_tag>
{
    using type = strategies::azimuth::geographic<>;
};


template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::azimuth::geographic<FP, S, CT> >
{
    static auto get(strategy::azimuth::geographic<FP, S, CT> const& strategy)
    {
        return strategies::azimuth::geographic<FP, S, CT>(strategy.model());
    }
};

} // namespace services

}} // namespace strategies::azimuth

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AZIMUTH_GEOGRAPHIC_HPP
