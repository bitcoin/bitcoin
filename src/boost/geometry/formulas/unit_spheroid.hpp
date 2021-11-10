// Boost.Geometry

// Copyright (c) 2017 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_UNIT_SPHEROID_HPP
#define BOOST_GEOMETRY_FORMULAS_UNIT_SPHEROID_HPP

#include <boost/geometry/core/radius.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace formula
{

template <typename ResultType, typename Spheroid>
inline ResultType unit_spheroid_b(Spheroid const& spheroid)
{
    return ResultType(get_radius<2>(spheroid))
         / ResultType(get_radius<0>(spheroid));
}

template <typename ResultSpheroid, typename Spheroid>
inline ResultSpheroid unit_spheroid(Spheroid const& spheroid)
{
    typedef typename radius_type<ResultSpheroid>::type radius_t;
    return ResultSpheroid(radius_t(1),
                          unit_spheroid_b<radius_t>(spheroid));
}

} // namespace formula
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_FORMULAS_UNIT_SPHEROID_HPP
