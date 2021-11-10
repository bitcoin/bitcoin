// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017-2017 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_NEEDS_SELF_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_NEEDS_SELF_TURNS_HPP

#include <boost/range/begin.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/algorithms/num_interior_rings.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct needs_self_turns
{
};

template <typename Geometry>
struct needs_self_turns<Geometry, box_tag>
{
    static inline bool apply(Geometry const&)
    {
        return false;
    }
};

template <typename Geometry>
struct needs_self_turns<Geometry, ring_tag>
{
    static inline bool apply(Geometry const&)
    {
        return false;
    }
};

template <typename Geometry>
struct needs_self_turns<Geometry, polygon_tag>
{
    static inline bool apply(Geometry const& polygon)
    {
        return geometry::num_interior_rings(polygon) > 0;
    }
};

template <typename Geometry>
struct needs_self_turns<Geometry, multi_polygon_tag>
{
    static inline bool apply(Geometry const& multi)
    {
        typedef typename boost::range_value<Geometry>::type polygon_type;
        std::size_t const n = boost::size(multi);
        return n > 1 || (n == 1
             && needs_self_turns<polygon_type>
                         ::apply(*boost::begin(multi)));
    }
};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_NEEDS_SELF_TURNS_HPP
