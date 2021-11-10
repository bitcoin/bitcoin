// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_AREA_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_AREA_CARTESIAN_HPP


#include <boost/geometry/strategy/cartesian/area.hpp>
#include <boost/geometry/strategy/cartesian/area_box.hpp>

#include <boost/geometry/strategies/area/services.hpp>
#include <boost/geometry/strategies/detail.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace area
{

template <typename CalculationType = void>
struct cartesian : strategies::detail::cartesian_base
{
    template <typename Geometry>
    static auto area(Geometry const&,
                     std::enable_if_t<! util::is_box<Geometry>::value> * = nullptr)
    {
        return strategy::area::cartesian<CalculationType>();
    }

    template <typename Geometry>
    static auto area(Geometry const&,
                     std::enable_if_t<util::is_box<Geometry>::value> * = nullptr)
    {
        return strategy::area::cartesian_box<CalculationType>();
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, cartesian_tag>
{
    using type = strategies::area::cartesian<>;
};


template <typename CT>
struct strategy_converter<strategy::area::cartesian<CT> >
{
    static auto get(strategy::area::cartesian<CT> const&)
    {
        return strategies::area::cartesian<CT>();
    }
};


} // namespace services

}} // namespace strategies::area

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AREA_CARTESIAN_HPP
