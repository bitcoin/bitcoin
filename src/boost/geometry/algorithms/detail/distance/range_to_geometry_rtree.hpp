// Boost.Geometry

// Copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_RANGE_TO_GEOMETRY_RTREE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_RANGE_TO_GEOMETRY_RTREE_HPP

#include <iterator>
#include <utility>

#include <boost/geometry/algorithms/detail/closest_feature/range_to_range.hpp>
#include <boost/geometry/algorithms/detail/distance/is_comparable.hpp>
#include <boost/geometry/algorithms/detail/distance/iterator_selector.hpp>
#include <boost/geometry/algorithms/detail/distance/strategy_utils.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/iterators/detail/has_one_element.hpp>

#include <boost/geometry/strategies/distance.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template
<
    typename PointOrSegmentIterator,
    typename Geometry,
    typename Strategies
>
class point_or_segment_range_to_geometry_rtree
{
private:
    typedef typename std::iterator_traits
        <
            PointOrSegmentIterator
        >::value_type point_or_segment_type;

    typedef iterator_selector<Geometry const> selector_type;

    typedef detail::closest_feature::range_to_range_rtree range_to_range;

    typedef distance::strategy_t<point_or_segment_type, Geometry, Strategies> strategy_type;

public:
    typedef distance::return_t<point_or_segment_type, Geometry, Strategies> return_type;

    static inline return_type apply(PointOrSegmentIterator first,
                                    PointOrSegmentIterator last,
                                    Geometry const& geometry,
                                    Strategies const& strategies)
    {
        BOOST_GEOMETRY_ASSERT( first != last );

        if ( detail::has_one_element(first, last) )
        {
            return dispatch::distance
                <
                    point_or_segment_type, Geometry, Strategies
                >::apply(*first, geometry, strategies);
        }

        distance::creturn_t<point_or_segment_type, Geometry, Strategies> cd_min;

        std::pair
            <
                point_or_segment_type,
                typename selector_type::iterator_type
            > closest_features
            = range_to_range::apply(first,
                                    last,
                                    selector_type::begin(geometry),
                                    selector_type::end(geometry),
                                    strategies,
                                    cd_min);

        return
            is_comparable<strategy_type>::value
            ?
            cd_min
            :
            dispatch::distance
                <
                    point_or_segment_type,                    
                    typename std::iterator_traits
                        <
                            typename selector_type::iterator_type
                        >::value_type,
                    Strategies
                >::apply(closest_features.first,
                         *closest_features.second,
                         strategies);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_RANGE_TO_GEOMETRY_RTREE_HPP
