// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2021 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_APPROXIMATELY_EQUALS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_APPROXIMATELY_EQUALS_HPP

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template <typename Point1, typename Point2, typename E>
inline bool approximately_equals(Point1 const& a, Point2 const& b,
                                 E const& multiplier)
{
    using coor_t = typename select_coordinate_type<Point1, Point2>::type;
    using calc_t = typename geometry::select_most_precise<coor_t, E>::type;

    calc_t const& a0 = geometry::get<0>(a);
    calc_t const& b0 = geometry::get<0>(b);
    calc_t const& a1 = geometry::get<1>(a);
    calc_t const& b1 = geometry::get<1>(b);

    math::detail::equals_factor_policy<calc_t> policy(a0, b0, a1, b1);
    policy.factor *= multiplier;

    return math::detail::equals_by_policy(a0, b0, policy)
        && math::detail::equals_by_policy(a1, b1, policy);
}

}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_APPROXIMATELY_EQUALS_HPP
