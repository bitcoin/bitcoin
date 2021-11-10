// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DENSIFY_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_DENSIFY_CARTESIAN_HPP


#include <boost/geometry/strategies/cartesian/densify.hpp>

#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/densify/services.hpp>

#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace densify
{

template <typename CalculationType = void>
struct cartesian
    : public strategies::detail::cartesian_base
{
    template <typename Geometry>
    static auto densify(Geometry const&)
    {
        return strategy::densify::cartesian<CalculationType>();
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, cartesian_tag>
{
    using type = strategies::densify::cartesian<>;
};


template <typename CT>
struct strategy_converter<strategy::densify::cartesian<CT> >
{
    static auto get(strategy::densify::cartesian<CT> const&)
    {
        return strategies::densify::cartesian<CT>();
    }
};


} // namespace services

}} // namespace strategies::densify

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DENSIFY_CARTESIAN_HPP
