// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_CLOSEST_FEATURE_POINT_TO_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_CLOSEST_FEATURE_POINT_TO_RANGE_HPP

#include <utility>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace closest_feature
{


// returns the segment (pair of iterators) that realizes the closest
// distance of the point to the range
template
<
    typename Point,
    typename Range,
    closure_selector Closure
>
class point_to_point_range
{
protected:
    typedef typename boost::range_iterator<Range const>::type iterator_type;

    template <typename Strategy, typename Distance>
    static inline void apply(Point const& point,
                             iterator_type first,
                             iterator_type last,
                             Strategy const& strategy,
                             iterator_type& it_min1,
                             iterator_type& it_min2,
                             Distance& dist_min)
    {
        BOOST_GEOMETRY_ASSERT( first != last );

        Distance const zero = Distance(0);

        iterator_type it = first;
        iterator_type prev = it++;
        if (it == last)
        {
            it_min1 = it_min2 = first;
            dist_min = strategy.apply(point, *first, *first);
            return;
        }

        // start with first segment distance
        dist_min = strategy.apply(point, *prev, *it);
        iterator_type prev_min_dist = prev;

        // check if other segments are closer
        for (++prev, ++it; it != last; ++prev, ++it)
        {
            Distance const dist = strategy.apply(point, *prev, *it);

            // Stop only if we find exactly zero distance
            // otherwise it may stop at some very small value and miss the min
            if (dist == zero)
            {
                dist_min = zero;
                it_min1 = prev;
                it_min2 = it;
                return;
            }
            else if (dist < dist_min)
            {
                dist_min = dist;
                prev_min_dist = prev;
            }
        }

        it_min1 = it_min2 = prev_min_dist;
        ++it_min2;
    }

public:
    typedef typename std::pair<iterator_type, iterator_type> return_type;

    template <typename Strategy, typename Distance>
    static inline return_type apply(Point const& point,
                                    iterator_type first,
                                    iterator_type last,
                                    Strategy const& strategy,
                                    Distance& dist_min)
    {
        iterator_type it_min1, it_min2;
        apply(point, first, last, strategy, it_min1, it_min2, dist_min);

        return std::make_pair(it_min1, it_min2);
    }

    template <typename Strategy>
    static inline return_type apply(Point const& point,
                                    iterator_type first,
                                    iterator_type last,
                                    Strategy const& strategy)
    {
        typename strategy::distance::services::return_type
            <
                Strategy,
                Point,
                typename boost::range_value<Range>::type
            >::type dist_min;

        return apply(point, first, last, strategy, dist_min);
    }

    template <typename Strategy, typename Distance>
    static inline return_type apply(Point const& point,
                                    Range const& range,
                                    Strategy const& strategy,
                                    Distance& dist_min)
    {
        return apply(point,
                     boost::begin(range),
                     boost::end(range),
                     strategy,
                     dist_min);
    }

    template <typename Strategy>
    static inline return_type apply(Point const& point,
                                    Range const& range,
                                    Strategy const& strategy)
    {
        return apply(point, boost::begin(range), boost::end(range), strategy);
    }
};



// specialization for open ranges
template <typename Point, typename Range>
class point_to_point_range<Point, Range, open>
    : point_to_point_range<Point, Range, closed>
{
private:
    typedef point_to_point_range<Point, Range, closed> base_type;
    typedef typename base_type::iterator_type iterator_type;

    template <typename Strategy, typename Distance>
    static inline void apply(Point const& point,
                             iterator_type first,
                             iterator_type last,
                             Strategy const& strategy,
                             iterator_type& it_min1,
                             iterator_type& it_min2,
                             Distance& dist_min)
    {
        BOOST_GEOMETRY_ASSERT( first != last );

        base_type::apply(point, first, last, strategy,
                         it_min1, it_min2, dist_min);

        iterator_type it_back = --last;
        Distance const zero = Distance(0);
        Distance dist = strategy.apply(point, *it_back, *first);

        if (geometry::math::equals(dist, zero))
        {
            dist_min = zero;
            it_min1 = it_back;
            it_min2 = first;
        }
        else if (dist < dist_min)
        {
            dist_min = dist;
            it_min1 = it_back;
            it_min2 = first;
        }
    }    

public:
    typedef typename std::pair<iterator_type, iterator_type> return_type;

    template <typename Strategy, typename Distance>
    static inline return_type apply(Point const& point,
                                    iterator_type first,
                                    iterator_type last,
                                    Strategy const& strategy,
                                    Distance& dist_min)
    {
        iterator_type it_min1, it_min2;

        apply(point, first, last, strategy, it_min1, it_min2, dist_min);

        return std::make_pair(it_min1, it_min2);
    }

    template <typename Strategy>
    static inline return_type apply(Point const& point,
                                    iterator_type first,
                                    iterator_type last,
                                    Strategy const& strategy)
    {
        typedef typename strategy::distance::services::return_type
            <
                Strategy,
                Point,
                typename boost::range_value<Range>::type
            >::type distance_return_type;

        distance_return_type dist_min;

        return apply(point, first, last, strategy, dist_min);
    }

    template <typename Strategy, typename Distance>
    static inline return_type apply(Point const& point,
                                    Range const& range,
                                    Strategy const& strategy,
                                    Distance& dist_min)
    {
        return apply(point,
                     boost::begin(range),
                     boost::end(range),
                     strategy,
                     dist_min);
    }

    template <typename Strategy>
    static inline return_type apply(Point const& point,
                                    Range const& range,
                                    Strategy const& strategy)
    {
        return apply(point, boost::begin(range), boost::end(range), strategy);
    }
};


}} // namespace detail::closest_feature
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_CLOSEST_FEATURE_POINT_TO_RANGE_HPP
