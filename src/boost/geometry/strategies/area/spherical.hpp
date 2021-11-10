// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_AREA_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_AREA_SPHERICAL_HPP


#include <boost/geometry/strategy/spherical/area.hpp>
#include <boost/geometry/strategy/spherical/area_box.hpp>

#include <boost/geometry/strategies/area/services.hpp>
#include <boost/geometry/strategies/detail.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace area
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
    spherical()
        : base_t()
    {}

    template <typename RadiusOrSphere>
    explicit spherical(RadiusOrSphere const& radius_or_sphere)
        : base_t(radius_or_sphere)
    {}

    template <typename Geometry>
    auto area(Geometry const&,
              std::enable_if_t<! util::is_box<Geometry>::value> * = nullptr) const
    {
        return strategy::area::spherical
            <
                typename base_t::radius_type, CalculationType
            >(base_t::m_radius);
    }

    template <typename Geometry>
    auto area(Geometry const&,
              std::enable_if_t<util::is_box<Geometry>::value> * = nullptr) const
    {
        return strategy::area::spherical_box
            <
                typename base_t::radius_type, CalculationType
            >(base_t::m_radius);
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, spherical_tag>
{
    using type = strategies::area::spherical<>;
};

template <typename Geometry>
struct default_strategy<Geometry, spherical_equatorial_tag>
{
    using type = strategies::area::spherical<>;
};

template <typename Geometry>
struct default_strategy<Geometry, spherical_polar_tag>
{
    using type = strategies::area::spherical<>;
};


template <typename R, typename CT>
struct strategy_converter<strategy::area::spherical<R, CT> >
{
    static auto get(strategy::area::spherical<R, CT> const& strategy)
    {
        return strategies::area::spherical<R, CT>(strategy.model());
    }
};


} // namespace services

}} // namespace strategies::area

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AREA_SPHERICAL_HPP
