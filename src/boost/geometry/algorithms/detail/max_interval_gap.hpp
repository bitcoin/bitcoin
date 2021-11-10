// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_MAX_INTERVAL_GAP_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_MAX_INTERVAL_GAP_HPP

#include <cstddef>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/algorithms/detail/sweep.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace max_interval_gap
{

// the class Interval must provide the following:
// * must define the type value_type
// * must define the type difference_type
// * must have the methods:
//   value_type get<Index>() const
//   difference_type length() const
// where an Index value of 0 (resp., 1) refers to the left (resp.,
// right) endpoint of the interval

template <typename Interval>
class sweep_event
{
public:
    typedef Interval interval_type;
    typedef typename Interval::value_type time_type;

    sweep_event(Interval const& interval, bool start_event = true)
        : m_interval(std::cref(interval))
        , m_start_event(start_event)
    {}

    inline bool is_start_event() const
    {
        return m_start_event;
    }

    inline interval_type const& interval() const
    {
        return m_interval;
    }

    inline time_type time() const
    {
        return (m_start_event)
            ? interval().template get<0>()
            : interval().template get<1>();
    }

    inline bool operator<(sweep_event const& other) const
    {
        if (! math::equals(time(), other.time()))
        {
            return time() < other.time();
        }
        // a start-event is before an end-event with the same event time
        return is_start_event() && ! other.is_start_event();
    }

private:
    std::reference_wrapper<Interval const> m_interval;
    bool m_start_event;
};

template <typename Event>
struct event_greater
{
    inline bool operator()(Event const& event1, Event const& event2) const
    {
        return event2 < event1;
    }
};


struct initialization_visitor
{
    template <typename Range, typename PriorityQueue, typename EventVisitor>
    static inline void apply(Range const& range,
                             PriorityQueue& queue,
                             EventVisitor&)
    {
        BOOST_GEOMETRY_ASSERT(queue.empty());

        // it is faster to build the queue directly from the entire
        // range, rather than insert elements one after the other
        PriorityQueue pq(boost::begin(range), boost::end(range));
        std::swap(pq, queue);
    }
};


template <typename Event>
class event_visitor
{
    typedef typename Event::time_type event_time_type;
    typedef typename Event::interval_type::difference_type difference_type;

public:
    event_visitor()
        : m_overlap_count(0)
        , m_max_gap_left(0)
        , m_max_gap_right(0)
    {}

    template <typename PriorityQueue>
    inline void apply(Event const& event, PriorityQueue& queue)
    {
        if (event.is_start_event())
        {
            ++m_overlap_count;
            queue.push(Event(event.interval(), false));
        }
        else
        {
            --m_overlap_count;
            if (m_overlap_count == 0 && ! queue.empty())
            {
                // we may have a gap
                BOOST_GEOMETRY_ASSERT(queue.top().is_start_event());

                event_time_type next_event_time
                    = queue.top().interval().template get<0>();
                difference_type gap = next_event_time - event.time();
                if (gap > max_gap())
                {
                    m_max_gap_left = event.time();
                    m_max_gap_right = next_event_time;
                }
            }
        }
    }

    event_time_type const& max_gap_left() const
    {
        return m_max_gap_left;
    }

    event_time_type const& max_gap_right() const
    {
        return m_max_gap_right;
    }

    difference_type max_gap() const
    {
        return m_max_gap_right - m_max_gap_left;
    }

private:
    std::size_t m_overlap_count;
    event_time_type m_max_gap_left, m_max_gap_right;
};

}} // namespace detail::max_interval_gap
#endif // DOXYGEN_NO_DETAIL


// Given a range of intervals I1, I2, ..., In, maximum_gap() returns
// the maximum length of an interval M that satisfies the following
// properties:
//
// 1. M.left >= min(I1, I2, ..., In)
// 2. M.right <= max(I1, I2, ..., In)
// 3. intersection(interior(M), Ik) is the empty set for all k=1, ..., n
// 4. length(M) is maximal
//
// where M.left and M.right denote the left and right extreme values
// for the interval M, and length(M) is equal to M.right - M.left.
//
// If M does not exist (or, alternatively, M is identified as the
// empty set), 0 is returned.
//
// The algorithm proceeds for performing a sweep: the left endpoints
// are inserted into a min-priority queue with the priority being the
// value of the endpoint. The sweep algorithm maintains an "overlap
// counter" that counts the number of overlaping intervals at any
// specific sweep-time value.
// There are two types of events encountered during the sweep:
// (a) a start event: the left endpoint of an interval is found.
//     In this case the overlap count is increased by one and the
//     right endpoint of the interval in inserted into the event queue
// (b) an end event: the right endpoint of an interval is found.
//     In this case the overlap count is decreased by one. If the
//     updated overlap count is 0, then we could expect to have a gap
//     in-between intervals. This gap is measured as the (absolute)
//     distance of the current interval right endpoint (being
//     processed) to the upcoming left endpoint of the next interval
//     to be processed (if such an interval exists). If the measured
//     gap is greater than the current maximum gap, it is recorded.
// The initial maximum gap is initialized to 0. This value is returned
// if no gap is found during the sweeping procedure.

template <typename RangeOfIntervals, typename T>
inline typename boost::range_value<RangeOfIntervals>::type::difference_type
maximum_gap(RangeOfIntervals const& range_of_intervals,
            T& max_gap_left, T& max_gap_right)
{
    typedef typename boost::range_value<RangeOfIntervals>::type interval_type;
    typedef detail::max_interval_gap::sweep_event<interval_type> event_type;

    // create a min-priority queue for the events
    std::priority_queue
        <
            event_type,
            std::vector<event_type>, 
            detail::max_interval_gap::event_greater<event_type>
        > queue;

    // define initialization and event-process visitors
    detail::max_interval_gap::initialization_visitor init_visitor;
    detail::max_interval_gap::event_visitor<event_type> sweep_visitor;

    // perform the sweep
    geometry::sweep(range_of_intervals,
                    queue,
                    init_visitor,
                    sweep_visitor);

    max_gap_left = sweep_visitor.max_gap_left();
    max_gap_right = sweep_visitor.max_gap_right();
    return sweep_visitor.max_gap();
}

template <typename RangeOfIntervals>
inline typename boost::range_value<RangeOfIntervals>::type::difference_type
maximum_gap(RangeOfIntervals const& range_of_intervals)
{
    typedef typename boost::range_value<RangeOfIntervals>::type interval_type;

    typename interval_type::value_type left, right;

    return maximum_gap(range_of_intervals, left, right);
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_MAX_INTERVAL_GAP_HPP
