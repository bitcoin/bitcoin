// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_LENGTH_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_LENGTH_CARTESIAN_HPP


#include <boost/geometry/strategies/cartesian/distance_pythagoras.hpp>

#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/length/services.hpp>

#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace length
{

template <typename CalculationType = void>
struct cartesian
    : public strategies::detail::cartesian_base
{
    template <typename Geometry1, typename Geometry2>
    static auto distance(Geometry1 const&, Geometry2 const&,
                         distance::detail::enable_if_pp_t<Geometry1, Geometry2> * = nullptr)
    {
        return strategy::distance::pythagoras<CalculationType>();
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, cartesian_tag>
{
    using type = strategies::length::cartesian<>;
};


template <typename CT>
struct strategy_converter<strategy::distance::pythagoras<CT> >
{
    static auto get(strategy::distance::pythagoras<CT> const&)
    {
        return strategies::length::cartesian<CT>();
    }
};


} // namespace services

}} // namespace strategies::length

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_LENGTH_CARTESIAN_HPP
