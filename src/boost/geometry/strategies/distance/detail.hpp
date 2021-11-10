// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DISTANCE_DETAIL_HPP
#define BOOST_GEOMETRY_STRATEGIES_DISTANCE_DETAIL_HPP


#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace distance
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Geometry1, typename Geometry2>
using enable_if_pp_t = std::enable_if_t
    <
        util::is_pointlike<Geometry1>::value && util::is_pointlike<Geometry2>::value
    >;

template <typename Geometry1, typename Geometry2>
using enable_if_ps_t = std::enable_if_t
    <
        (util::is_pointlike<Geometry1>::value && util::is_segmental<Geometry2>::value)
     || (util::is_segmental<Geometry1>::value && util::is_pointlike<Geometry2>::value)
     || (util::is_segmental<Geometry1>::value && util::is_segmental<Geometry2>::value)
    >;

template <typename Geometry1, typename Geometry2>
using enable_if_pb_t = std::enable_if_t
    <
        util::is_pointlike<Geometry1>::value && util::is_box<Geometry2>::value
    >;

template <typename Geometry1, typename Geometry2>
using enable_if_sb_t = std::enable_if_t
    <
        util::is_segmental<Geometry1>::value && util::is_box<Geometry2>::value
    >;

template <typename Geometry1, typename Geometry2>
using enable_if_bb_t = std::enable_if_t
    <
        util::is_box<Geometry1>::value && util::is_box<Geometry2>::value
    >;

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace strategies::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DISTANCE_DETAIL_HPP
