// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/convex_hull/services.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/geographic/side.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace convex_hull
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic : public strategies::detail::geographic_base<Spheroid>
{
    using base_t = strategies::detail::geographic_base<Spheroid>;

public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}

    auto side() const
    {
        return strategy::side::geographic
            <
                FormulaPolicy,
                Spheroid,
                CalculationType
            >(base_t::m_spheroid);
    }
};

namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, geographic_tag>
{
    using type = strategies::convex_hull::geographic<>;
};

} // namespace services

}} // namespace strategies::convex_hull

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_GEOGRAPHIC_HPP
