// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_SIMPLIFY_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_SIMPLIFY_SPHERICAL_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/simplify/services.hpp>

#include <boost/geometry/strategies/agnostic/simplify_douglas_peucker.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/strategies/spherical/distance_cross_track.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>

#include <boost/geometry/strategy/spherical/area.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace simplify
{

template
<
    typename RadiusTypeOrSphere = double,
    typename CalculationType = void
>
class spherical
    : public strategies::detail::spherical_base<RadiusTypeOrSphere>
{
    using base_t = strategies::detail::spherical_base<RadiusTypeOrSphere>;

public:
    spherical() = default;

    template <typename RadiusOrSphere>
    explicit spherical(RadiusOrSphere const& radius_or_sphere)
        : base_t(radius_or_sphere)
    {}

    // TODO: Replace this if calculate_point_order() is used in simplify
    template <typename Geometry>
    auto area(Geometry const&) const
    {
        return strategy::area::spherical
            <
                typename base_t::radius_type, CalculationType
            >(base_t::radius());
    }

    // For perimeter()
    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  distance::detail::enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::haversine
                <
                    typename base_t::radius_type, CalculationType
                >(base_t::radius());
    }

    // For douglas_peucker
    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  distance::detail::enable_if_ps_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cross_track
            <
                CalculationType,
                strategy::distance::haversine<typename base_t::radius_type, CalculationType>
            >(base_t::radius());
    }

    // For equals()
    template <typename Geometry1, typename Geometry2>
    static auto relate(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                             && util::is_pointlike<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::spherical_point_point();
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, spherical_equatorial_tag>
{
    using type = strategies::simplify::spherical<>;
};

template <typename P, typename CT, typename S>
struct strategy_converter
    <
        strategy::simplify::douglas_peucker
            <
                P,
                strategy::distance::cross_track<CT, S>
            >
    >
{
    template <typename Strategy>
    static auto get(Strategy const& )
    {
        return strategies::simplify::spherical<typename S::radius_type, CT>();
    }
};

template <typename P, typename CT, typename S>
struct strategy_converter
    <
        strategy::simplify::douglas_peucker
            <
                P,
                strategy::distance::comparable::cross_track<CT, S>
            >
    >
{
    template <typename Strategy>
    static auto get(Strategy const& )
    {
        return strategies::distance::detail::comparable
            <
                strategies::simplify::spherical<typename S::radius_type, CT>
            >();
    }
};


} // namespace services

}} // namespace strategies::simplify

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SIMPLIFY_SPHERICAL_HPP
