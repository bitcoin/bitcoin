// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_SEGMENT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_SEGMENT_HPP

#include <cstddef>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/dispatch/envelope.hpp>

// TEMP
#include <boost/geometry/strategies/detail.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{

// TEMP
template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct envelope_segment_call_strategy
{
    template <typename Point, typename Segment, typename Box>
    static inline void apply(Point const& p1, Point const& p2,
                             Segment const& segment, Box& mbr,
                             Strategy const& strategy)
    {
        strategy.envelope(segment, mbr).apply(p1, p2, mbr);
    }
};

template <typename Strategy>
struct envelope_segment_call_strategy<Strategy, false>
{
    template <typename Point, typename Segment, typename Box>
    static inline void apply(Point const& p1, Point const& p2,
                             Segment const&, Box& mbr,
                             Strategy const& strategy)
    {
        strategy.apply(p1, p2, mbr);
    }
};

struct envelope_segment
{
    template <typename Segment, typename Box, typename Strategy>
    static inline void apply(Segment const& segment, Box& mbr,
                             Strategy const& strategy)
    {
        typename point_type<Segment>::type p[2];
        detail::assign_point_from_index<0>(segment, p[0]);
        detail::assign_point_from_index<1>(segment, p[1]);

        // TEMP - expand calls this and Umbrella strategies are not yet supported there
        envelope_segment_call_strategy<Strategy>::apply(p[0], p[1], segment, mbr, strategy);
    }
};

}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Segment>
struct envelope<Segment, segment_tag>
{
    template <typename Box, typename Strategy>
    static inline void apply(Segment const& segment,
                             Box& mbr,
                             Strategy const& strategy)
    {
        detail::envelope::envelope_segment
            /*<
               dimension<Segment>::value
            >*/::apply(segment, mbr, strategy);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_SEGMENT_HPP
