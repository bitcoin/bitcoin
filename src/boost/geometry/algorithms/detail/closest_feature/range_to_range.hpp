// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, 2019, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_CLOSEST_FEATURE_RANGE_TO_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_CLOSEST_FEATURE_RANGE_TO_RANGE_HPP

#include <cstddef>

#include <iterator>
#include <utility>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>
#include <boost/geometry/index/rtree.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace closest_feature
{


// returns a pair of a objects where the first is an object of the
// r-tree range and the second an object of the query range that
// realizes the closest feature of the two ranges
class range_to_range_rtree
{
private:
    template
    <
        typename RTreeRangeIterator,
        typename QueryRangeIterator,
        typename Strategies,
        typename RTreeValueType,
        typename Distance
    >
    static inline void apply(RTreeRangeIterator rtree_first,
                             RTreeRangeIterator rtree_last,
                             QueryRangeIterator queries_first,
                             QueryRangeIterator queries_last,
                             Strategies const& strategies,
                             RTreeValueType& rtree_min,
                             QueryRangeIterator& qit_min,
                             Distance& dist_min)
    {
        typedef index::parameters
            <
                index::linear<8>, Strategies
            > index_parameters_type;
        typedef index::rtree<RTreeValueType, index_parameters_type> rtree_type;

        BOOST_GEOMETRY_ASSERT( rtree_first != rtree_last );
        BOOST_GEOMETRY_ASSERT( queries_first != queries_last );

        Distance const zero = Distance(0);
        dist_min = zero;

        // create -- packing algorithm
        rtree_type rt(rtree_first, rtree_last,
                      index_parameters_type(index::linear<8>(), strategies));

        RTreeValueType t_v;
        bool first = true;

        for (QueryRangeIterator qit = queries_first;
             qit != queries_last; ++qit, first = false)
        {
            std::size_t n = rt.query(index::nearest(*qit, 1), &t_v);

            BOOST_GEOMETRY_ASSERT( n > 0 );
            // n above is unused outside BOOST_GEOMETRY_ASSERT,
            // hence the call to boost::ignore_unused below
            //
            // however, t_v (initialized by the call to rt.query(...))
            // is used below, which is why we cannot put the call to
            // rt.query(...) inside BOOST_GEOMETRY_ASSERT
            boost::ignore_unused(n);

            Distance dist = dispatch::distance
                <
                    RTreeValueType,
                    typename std::iterator_traits
                        <
                            QueryRangeIterator
                        >::value_type,
                    Strategies
                >::apply(t_v, *qit, strategies);

            if (first || dist < dist_min)
            {
                dist_min = dist;
                rtree_min = t_v;
                qit_min = qit;
                if ( math::equals(dist_min, zero) )
                {
                    return;
                }
            }
        }
    }

public:
    template <typename RTreeRangeIterator, typename QueryRangeIterator>
    struct return_type
    {
        typedef std::pair
            <
                typename std::iterator_traits<RTreeRangeIterator>::value_type,
                QueryRangeIterator
            > type;
    };


    template
    <
        typename RTreeRangeIterator,
        typename QueryRangeIterator,
        typename Strategy,
        typename Distance
    >
    static inline typename return_type
        <
            RTreeRangeIterator, QueryRangeIterator
        >::type apply(RTreeRangeIterator rtree_first,
                      RTreeRangeIterator rtree_last,
                      QueryRangeIterator queries_first,
                      QueryRangeIterator queries_last,
                      Strategy const& strategy,
                      Distance& dist_min)
    {
        typedef typename std::iterator_traits
            <
                RTreeRangeIterator
            >::value_type rtree_value_type;

        rtree_value_type rtree_min;
        QueryRangeIterator qit_min;

        apply(rtree_first, rtree_last, queries_first, queries_last,
              strategy, rtree_min, qit_min, dist_min);

        return std::make_pair(rtree_min, qit_min);        
    }


    template
    <
        typename RTreeRangeIterator,
        typename QueryRangeIterator,
        typename Strategy
    >
    static inline typename return_type
        <
            RTreeRangeIterator, QueryRangeIterator
        >::type apply(RTreeRangeIterator rtree_first,
                      RTreeRangeIterator rtree_last,
                      QueryRangeIterator queries_first,
                      QueryRangeIterator queries_last,
                      Strategy const& strategy)
    {
        typedef typename std::iterator_traits
            <
                RTreeRangeIterator
            >::value_type rtree_value_type;

        typename strategy::distance::services::return_type
            <
                Strategy,
                typename point_type<rtree_value_type>::type,
                typename point_type
                    <
                        typename std::iterator_traits
                            <
                                QueryRangeIterator
                            >::value_type
                    >::type
            >::type dist_min;

        return apply(rtree_first, rtree_last, queries_first, queries_last,
                     strategy, dist_min);
    }
};


}} // namespace detail::closest_feature
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_CLOSEST_FEATURE_RANGE_TO_RANGE_HPP
