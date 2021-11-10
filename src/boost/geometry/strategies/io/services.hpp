// Boost.Geometry

// Copyright (c) 2019-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_IO_SERVICES_HPP
#define BOOST_GEOMETRY_STRATEGIES_IO_SERVICES_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/static_assert.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace io
{

namespace services
{

template
<
    typename Geometry,
    typename CSTag = typename geometry::cs_tag<Geometry>::type
>
struct default_strategy
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this coordinate system.",
        Geometry, CSTag);
};

} // namespace services

}} // namespace strategies::io

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_IO_SERVICES_HPP
