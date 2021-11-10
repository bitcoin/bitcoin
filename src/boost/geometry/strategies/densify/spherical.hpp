// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DENSIFY_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_DENSIFY_SPHERICAL_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/densify/services.hpp>

#include <boost/geometry/strategies/spherical/densify.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace densify
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

    template <typename Geometry>
    auto densify(Geometry const&) const
    {
        return strategy::densify::spherical
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
    using type = strategies::densify::spherical<>;
};


template <typename R, typename CT>
struct strategy_converter<strategy::densify::spherical<R, CT> >
{
    static auto get(strategy::densify::spherical<R, CT> const& s)
    {
        return strategies::densify::spherical<R, CT>(s.radius());
    }
};


} // namespace services

}} // namespace strategies::densify

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DENSIFY_SPHERICAL_HPP
