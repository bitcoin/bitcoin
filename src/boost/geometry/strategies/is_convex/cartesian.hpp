// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_IS_CONVEX_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_IS_CONVEX_CARTESIAN_HPP


#include <boost/geometry/strategies/cartesian/point_in_point.hpp>
#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>
#include <boost/geometry/strategies/convex_hull/cartesian.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/is_convex/services.hpp>
#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace is_convex
{

template <typename CalculationType = void>
class cartesian : public strategies::convex_hull::cartesian<CalculationType>
{
public:
    template <typename Geometry1, typename Geometry2>
    static auto relate(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                             && util::is_pointlike<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::cartesian_point_point();
    }
};

namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, cartesian_tag>
{
    using type = strategies::is_convex::cartesian<>;
};

template <typename CT>
struct strategy_converter<strategy::side::side_robust<CT>>
{
    static auto get(strategy::side::side_robust<CT> const& )
    {
        return strategies::is_convex::cartesian<CT>();
    }
};

template <typename CT>
struct strategy_converter<strategy::side::side_by_triangle<CT>>
{
    static auto get(strategy::side::side_by_triangle<CT> const& )
    {
        return strategies::is_convex::cartesian<CT>();
    }
};

} // namespace services

}} // namespace strategies::is_convex

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_IS_CONVEX_CARTESIAN_HPP
