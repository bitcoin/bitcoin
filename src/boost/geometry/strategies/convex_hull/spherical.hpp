// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_SPHERICAL_HPP


#include <boost/geometry/strategies/convex_hull/services.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/spherical/ssf.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace convex_hull
{

template <typename CalculationType = void>
class spherical : public strategies::detail::spherical_base<void>
{
public:
    static auto side()
    {
        return strategy::side::spherical_side_formula<CalculationType>();
    }
};

namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, spherical_equatorial_tag>
{
    using type = strategies::convex_hull::spherical<>;
};

} // namespace services

}} // namespace strategies::convex_hull

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CONVEX_HULL_SPHERICAL_HPP
