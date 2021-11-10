// Boost.Geometry

// Copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_RELATE_HPP
#define BOOST_GEOMETRY_STRATEGY_RELATE_HPP


#include <type_traits>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/topological_dimension.hpp>

#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/intersection.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry
{

namespace strategy
{

namespace point_in_geometry
{

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template
<
    typename Point,
    typename Geometry,
    typename Tag1 = typename tag<Point>::type,
    typename Tag2 = typename tag<Geometry>::type
>
struct default_strategy
    : strategy::within::services::default_strategy
        <
            Point,
            Geometry
        >
{
    typedef typename default_strategy::type within_strategy_type;

    typedef typename strategy::covered_by::services::default_strategy
        <
            Point,
            Geometry
        >::type covered_by_strategy_type;

    static const bool same_strategies = std::is_same<within_strategy_type, covered_by_strategy_type>::value;
    BOOST_GEOMETRY_STATIC_ASSERT(same_strategies,
        "Default within and covered_by strategies not compatible.",
        within_strategy_type, covered_by_strategy_type);
};

template<typename Point, typename Geometry>
struct default_strategy<Point, Geometry, point_tag, point_tag>
    : strategy::within::services::default_strategy<Point, Geometry>
{};

template<typename Point, typename Geometry>
struct default_strategy<Point, Geometry, point_tag, multi_point_tag>
    : strategy::within::services::default_strategy<Point, Geometry>
{};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


} // namespace point_in_geometry
    
namespace relate
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Geometry>
struct default_intersection_strategy
    : strategy::intersection::services::default_strategy
        <
            typename cs_tag<Geometry>::type
        >
{};

template <typename PointLike, typename Geometry>
struct default_point_in_geometry_strategy
    : point_in_geometry::services::default_strategy
        <
            typename point_type<PointLike>::type,
            Geometry
        >
{};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template
<
    typename Geometry1,
    typename Geometry2,
    int TopDim1 = geometry::topological_dimension<Geometry1>::value,
    int TopDim2 = geometry::topological_dimension<Geometry2>::value
>
struct default_strategy
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for these types.",
        Geometry1, Geometry2);
};

template <typename PointLike1, typename PointLike2>
struct default_strategy<PointLike1, PointLike2, 0, 0>
    : detail::default_point_in_geometry_strategy<PointLike1, PointLike2>
{};

template <typename PointLike, typename Geometry, int TopDim2>
struct default_strategy<PointLike, Geometry, 0, TopDim2>
    : detail::default_point_in_geometry_strategy<PointLike, Geometry>
{};

template <typename Geometry, typename PointLike, int TopDim1>
struct default_strategy<Geometry, PointLike, TopDim1, 0>
    : detail::default_point_in_geometry_strategy<PointLike, Geometry>
{};

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, 1, 1>
    : detail::default_intersection_strategy<Geometry1>
{};

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, 1, 2>
    : detail::default_intersection_strategy<Geometry1>
{};

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, 2, 1>
    : detail::default_intersection_strategy<Geometry1>
{};

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, 2, 2>
    : detail::default_intersection_strategy<Geometry1>
{};

} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

} // namespace relate

} // namespace strategy


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_RELATE_HPP
