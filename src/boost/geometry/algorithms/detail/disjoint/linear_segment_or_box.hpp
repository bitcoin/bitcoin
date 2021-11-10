// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_SEGMENT_OR_BOX_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_SEGMENT_OR_BOX_HPP


#include <boost/geometry/algorithms/detail/disjoint/multirange_geometry.hpp>
#include <boost/geometry/algorithms/dispatch/disjoint.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/util/range.hpp>
#include <boost/geometry/views/closeable_view.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{


template
<
    typename SegmentOrBox,
    typename Tag = typename tag<SegmentOrBox>::type
>
struct disjoint_point_segment_or_box
    : not_implemented<Tag>
{};

template <typename Segment>
struct disjoint_point_segment_or_box<Segment, segment_tag>
{
    template <typename Point, typename Strategy>
    static inline bool apply(Point const& point, Segment const& segment, Strategy const& strategy)
    {
        return dispatch::disjoint
            <
                Point, Segment
            >::apply(point, segment, strategy);
    }
};

template <typename Box>
struct disjoint_point_segment_or_box<Box, box_tag>
{
    template <typename Point, typename Strategy>
    static inline bool apply(Point const& point, Box const& box, Strategy const& strategy)
    {
        return dispatch::disjoint
            <
                Point, Box
            >::apply(point, box, strategy);
    }
};


template <typename Range, typename SegmentOrBox>
struct disjoint_range_segment_or_box
{
    template <typename Strategy>
    static inline bool apply(Range const& range,
                             SegmentOrBox const& segment_or_box,
                             Strategy const& strategy)
    {
        using point_type = typename point_type<Range>::type;
        using range_segment = typename geometry::model::referring_segment<point_type const>;

        detail::closed_view<Range const> const view(range);

        auto const count = ::boost::size(view);

        if ( count == 0 )
        {
            return false;
        }
        else if ( count == 1 )
        {
            return disjoint_point_segment_or_box
                <
                    SegmentOrBox
                >::apply(range::front(view), segment_or_box, strategy);
        }
        else
        {
            auto it0 = ::boost::begin(view);
            auto it1 = ::boost::begin(view) + 1;
            auto const last = ::boost::end(view);

            for ( ; it1 != last ; ++it0, ++it1 )
            {
                point_type const& p0 = *it0;
                point_type const& p1 = *it1;
                range_segment rng_segment(p0, p1);
                if ( !dispatch::disjoint
                         <
                             range_segment, SegmentOrBox
                         >::apply(rng_segment, segment_or_box, strategy) )
                {
                    return false;
                }
            }
            return true;
        }
    }
};




template
<
    typename Linear,
    typename SegmentOrBox,
    typename Tag = typename tag<Linear>::type
>
struct disjoint_linear_segment_or_box
    : not_implemented<Linear, SegmentOrBox>
{};


template <typename Linestring, typename SegmentOrBox>
struct disjoint_linear_segment_or_box<Linestring, SegmentOrBox, linestring_tag>
    : disjoint_range_segment_or_box<Linestring, SegmentOrBox>
{};


template <typename MultiLinestring, typename SegmentOrBox>
struct disjoint_linear_segment_or_box
    <
        MultiLinestring, SegmentOrBox, multi_linestring_tag
    > : multirange_constant_size_geometry<MultiLinestring, SegmentOrBox>
{};


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Linear, typename Segment>
struct disjoint<Linear, Segment, 2, linear_tag, segment_tag, false>
    : detail::disjoint::disjoint_linear_segment_or_box<Linear, Segment>
{};


template <typename Linear, typename Box, std::size_t DimensionCount>
struct disjoint<Linear, Box, DimensionCount, linear_tag, box_tag, false>
    : detail::disjoint::disjoint_linear_segment_or_box<Linear, Box>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_SEGMENT_OR_BOX_HPP
