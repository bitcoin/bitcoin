// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_IS_CONVEX_SERVICES_HPP
#define BOOST_GEOMETRY_STRATEGIES_IS_CONVEX_SERVICES_HPP


#include <boost/geometry/core/cs.hpp>

#include <boost/mpl/assert.hpp>


namespace boost { namespace geometry
{


namespace strategies { namespace is_convex {

namespace services
{

template
<
    typename Geometry,
    typename CSTag = typename geometry::cs_tag<Geometry>::type
>
struct default_strategy
{
    BOOST_MPL_ASSERT_MSG
    (
        false, NOT_IMPLEMENTED_FOR_THIS_COORDINATE_SYSTEM
        , (types<Geometry, CSTag>)
    );
};

template <typename Strategy>
struct strategy_converter
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Strategy.",
        Strategy);
};

} // namespace services

}} // namespace strategies::is_convex


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_IS_CONVEX_SERVICES_HPP
