// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_LINE_INTERPOLATE_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_LINE_INTERPOLATE_SPHERICAL_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/line_interpolate/services.hpp>

#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/strategies/spherical/line_interpolate.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace line_interpolate
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

    template <typename Geometry>
    auto line_interpolate(Geometry const&) const
    {
        // NOTE: radius is ignored, but pass it just in case
        return strategy::line_interpolate::spherical<CalculationType>(base_t::radius());
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, spherical_equatorial_tag>
{
    using type = strategies::line_interpolate::spherical<>;
};


template <typename CT, typename DS>
struct strategy_converter<strategy::line_interpolate::spherical<CT, DS> >
{
    static auto get(strategy::line_interpolate::spherical<CT, DS> const& s)
    {
        typedef typename strategy::line_interpolate::spherical<CT, DS>::radius_type radius_type;
        return strategies::line_interpolate::spherical<radius_type, CT>(s.radius());
    }
};

} // namespace services

}} // namespace strategies::line_interpolate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_LINE_INTERPOLATE_SPHERICAL_HPP
