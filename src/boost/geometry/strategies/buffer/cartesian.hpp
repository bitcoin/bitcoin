// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_BUFFER_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_BUFFER_CARTESIAN_HPP


#include <boost/geometry/strategies/buffer/services.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/relate/cartesian.hpp>

#include <boost/geometry/strategies/cartesian/distance_projected_point.hpp>
#include <boost/geometry/strategies/cartesian/distance_pythagoras.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace buffer
{

template <typename CalculationType = void>
struct cartesian
    : public strategies::relate::cartesian<CalculationType>
{
    // Additional strategies for simplify()

    template <typename Geometry1, typename Geometry2>
    static auto distance(Geometry1 const&, Geometry2 const&,
                         distance::detail::enable_if_ps_t<Geometry1, Geometry2> * = nullptr)
    {
        return strategy::distance::projected_point
            <
                CalculationType,
                strategy::distance::pythagoras<CalculationType>
            >();
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, cartesian_tag>
{
    using type = strategies::buffer::cartesian<>;
};


} // namespace services

}} // namespace strategies::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_BUFFER_CARTESIAN_HPP
