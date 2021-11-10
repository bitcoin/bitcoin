// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2018-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_LINEAR_TO_BOX_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_LINEAR_TO_BOX_HPP

#include <iterator>

#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/detail/distance/strategy_utils.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/iterators/segment_iterator.hpp>

#include <boost/geometry/core/point_type.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{

template <typename Linear, typename Box, typename Strategies>
struct linear_to_box
{
    typedef distance::return_t<Linear, Box, Strategies> return_type;

    template <typename Iterator>
    static inline return_type apply(Box const& box,
                                    Iterator begin,
                                    Iterator end,
                                    Strategies const& strategies)
    {
        bool first = true;
        return_type d_min(0);
        for (Iterator it = begin; it != end; ++it, first = false)
        {
            typedef typename std::iterator_traits<Iterator>::value_type
                    Segment;

            return_type d = dispatch::distance<Segment, Box, Strategies>
                                    ::apply(*it, box, strategies);

            if ( first || d < d_min )
            {
                d_min = d;
            }
        }
        return d_min;
    }

    static inline return_type apply(Linear const& linear,
                                    Box const& box,
                                    Strategies const& strategies)
    {
        if ( geometry::intersects(linear, box) )
        {
            return return_type(0);
        }

        return apply(box,
                     geometry::segments_begin(linear),
                     geometry::segments_end(linear),
                     strategies);
    }


    static inline return_type apply(Box const& box,
                                    Linear const& linear,
                                    Strategies const& strategies)
    {
        return apply(linear, box, strategies);
    }
};

}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Linear, typename Box, typename Strategy>
struct distance
    <
        Linear, Box, Strategy,
        linear_tag, box_tag,
        strategy_tag_distance_segment_box, false
    >
    : detail::distance::linear_to_box
        <
            Linear, Box, Strategy
        >
{};


template <typename Areal, typename Box, typename Strategy>
struct distance
    <
        Areal, Box, Strategy,
        areal_tag, box_tag,
        strategy_tag_distance_segment_box, false
    >
    : detail::distance::linear_to_box
        <
            Areal, Box, Strategy
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_LINEAR_TO_BOX_HPP
