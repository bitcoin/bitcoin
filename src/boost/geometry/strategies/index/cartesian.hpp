// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_INDEX_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_INDEX_CARTESIAN_HPP


#include <boost/geometry/strategies/distance/cartesian.hpp>
#include <boost/geometry/strategies/index/services.hpp>



namespace boost { namespace geometry { namespace strategies { namespace index
{

template <typename CalculationType = void>
class cartesian
    : public distance::cartesian<CalculationType>
{};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, cartesian_tag>
{
    using type = strategies::index::cartesian<>;
};


} // namespace services


}}}} // namespace boost::geometry::strategy::index

#endif // BOOST_GEOMETRY_STRATEGIES_INDEX_CARTESIAN_HPP
