// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_LENGTH_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_LENGTH_SPHERICAL_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/length/services.hpp>

#include <boost/geometry/strategies/spherical/distance_haversine.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace length
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

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  distance::detail::enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::haversine
                <
                    typename base_t::radius_type, CalculationType
                >(base_t::radius());
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, spherical_equatorial_tag>
{
    using type = strategies::length::spherical<>;
};


template <typename R, typename CT>
struct strategy_converter<strategy::distance::haversine<R, CT> >
{
    static auto get(strategy::distance::haversine<R, CT> const& s)
    {
        return strategies::length::spherical<R, CT>(s.radius());
    }
};


} // namespace services

}} // namespace strategies::length

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_LENGTH_SPHERICAL_HPP
