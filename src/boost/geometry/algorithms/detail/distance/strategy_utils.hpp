// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHS_DETAIL_DISTANCE_STRATEGY_UTILS_HPP
#define BOOST_GEOMETRY_ALGORITHS_DETAIL_DISTANCE_STRATEGY_UTILS_HPP


#include <utility>

#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/strategies/distance.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template <typename Geometry1, typename Geometry2, typename Strategies>
using strategy_t = decltype(
    std::declval<Strategies>().distance(std::declval<Geometry1>(), std::declval<Geometry2>()));


template <typename Geometry1, typename Geometry2, typename Strategies>
using return_t = typename strategy::distance::services::return_type
    <
        strategy_t<Geometry1, Geometry2, Strategies>,
        typename point_type<Geometry1>::type,
        typename point_type<Geometry2>::type
    >::type;


template <typename Geometry1, typename Geometry2, typename Strategies>
using cstrategy_t = typename strategy::distance::services::comparable_type
    <
        strategy_t<Geometry1, Geometry2, Strategies>
    >::type;


template <typename Geometry1, typename Geometry2, typename Strategies>
using creturn_t = typename strategy::distance::services::return_type
    <
        cstrategy_t<Geometry1, Geometry2, Strategies>,
        typename point_type<Geometry1>::type,
        typename point_type<Geometry2>::type
    >::type;


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHS_DETAIL_DISTANCE_STRATEGY_UTILS_HPP
