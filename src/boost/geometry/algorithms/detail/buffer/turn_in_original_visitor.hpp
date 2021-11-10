// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2016-2020.
// Modifications copyright (c) 2016-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_ORIGINAL_VISITOR
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_ORIGINAL_VISITOR


#include <boost/core/ignore_unused.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/algorithms/detail/buffer/buffer_policies.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/strategies/agnostic/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/buffer.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{


template <typename Strategy>    
struct original_get_box
{
    explicit original_get_box(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Box, typename Original>
    inline void apply(Box& total, Original const& original) const
    {
        assert_coordinate_type_equal(total, original.m_box);
        geometry::expand(total, original.m_box, m_strategy);
    }

    Strategy const& m_strategy;
};

template <typename Strategy>
struct original_overlaps_box
{
    explicit original_overlaps_box(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Box, typename Original>
    inline bool apply(Box const& box, Original const& original) const
    {
        assert_coordinate_type_equal(box, original.m_box);
        return ! detail::disjoint::disjoint_box_box(box, original.m_box,
                                                    m_strategy);
    }

    Strategy const& m_strategy;
};

struct include_turn_policy
{
    template <typename Turn>
    static inline bool apply(Turn const& turn)
    {
        return turn.is_turn_traversable;
    }
};

template <typename Strategy>
struct turn_in_original_overlaps_box
{
    explicit turn_in_original_overlaps_box(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Box, typename Turn>
    inline bool apply(Box const& box, Turn const& turn) const
    {
        if (! turn.is_turn_traversable || turn.within_original)
        {
            // Skip all points already processed
            return false;
        }

        return ! geometry::detail::disjoint::disjoint_point_box(
                    turn.point, box, m_strategy);
    }

    Strategy const& m_strategy;
};

//! Check if specified is in range of specified iterators
//! Return value of strategy (true if we can bail out)
template
<
    typename Strategy,
    typename State,
    typename Point,
    typename Iterator
>
inline bool point_in_range(Strategy& strategy, State& state,
        Point const& point, Iterator begin, Iterator end)
{
    boost::ignore_unused(strategy);

    Iterator it = begin;
    for (Iterator previous = it++; it != end; ++previous, ++it)
    {
        if (! strategy.apply(point, *previous, *it, state))
        {
            // We're probably on the boundary
            return false;
        }
    }
    return true;
}

template
<
    typename Strategy,
    typename State,
    typename Point,
    typename CoordinateType,
    typename Iterator
>
inline bool point_in_section(Strategy& strategy, State& state,
        Point const& point, CoordinateType const& point_x,
        Iterator begin, Iterator end,
        int direction)
{
    if (direction == 0)
    {
        // Not a monotonic section, or no change in X-direction
        return point_in_range(strategy, state, point, begin, end);
    }

    // We're in a monotonic section in x-direction
    Iterator it = begin;

    for (Iterator previous = it++; it != end; ++previous, ++it)
    {
        // Depending on sections.direction we can quit for this section
        CoordinateType const previous_x = geometry::get<0>(*previous);

        if (direction == 1 && point_x < previous_x)
        {
            // Section goes upwards, x increases, point is is below section
            return true;
        }
        else if (direction == -1 && point_x > previous_x)
        {
            // Section goes downwards, x decreases, point is above section
            return true;
        }

        if (! strategy.apply(point, *previous, *it, state))
        {
            // We're probably on the boundary
            return false;
        }
    }
    return true;
}


template <typename Point, typename Original, typename PointInGeometryStrategy>
inline int point_in_original(Point const& point, Original const& original,
                             PointInGeometryStrategy const& strategy)
{
    typename PointInGeometryStrategy::state_type state;

    if (boost::size(original.m_sections) == 0
        || boost::size(original.m_ring) - boost::size(original.m_sections) < 16)
    {
        // There are no sections, or it does not profit to walk over sections
        // instead of over points. Boundary of 16 is arbitrary but can influence
        // performance
        point_in_range(strategy, state, point,
                original.m_ring.begin(), original.m_ring.end());
        return strategy.result(state);
    }

    typedef typename Original::sections_type sections_type;
    typedef typename boost::range_iterator<sections_type const>::type iterator_type;
    typedef typename boost::range_value<sections_type const>::type section_type;
    typedef typename geometry::coordinate_type<Point>::type coordinate_type;

    coordinate_type const point_x = geometry::get<0>(point);

    // Walk through all monotonic sections of this original
    for (iterator_type it = boost::begin(original.m_sections);
        it != boost::end(original.m_sections);
        ++it)
    {
        section_type const& section = *it;

        if (! section.duplicate
            && section.begin_index < section.end_index
            && point_x >= geometry::get<min_corner, 0>(section.bounding_box)
            && point_x <= geometry::get<max_corner, 0>(section.bounding_box))
        {
            // x-coordinate of point overlaps with section
            if (! point_in_section(strategy, state, point, point_x,
                    boost::begin(original.m_ring) + section.begin_index,
                    boost::begin(original.m_ring) + section.end_index + 1,
                    section.directions[0]))
            {
                // We're probably on the boundary
                break;
            }
        }
    }

    return strategy.result(state);
}


template <typename Turns, typename Strategy>
class turn_in_original_visitor
{
public:
    turn_in_original_visitor(Turns& turns, Strategy const& strategy)
        : m_mutable_turns(turns)
        , m_strategy(strategy)
    {}

    template <typename Turn, typename Original>
    inline bool apply(Turn const& turn, Original const& original)
    {
        if (boost::empty(original.m_ring))
        {
            // Skip empty rings
            return true;
        }

        if (! turn.is_turn_traversable || turn.within_original)
        {
            // Skip all points already processed
            return true;
        }

        if (geometry::disjoint(turn.point, original.m_box, m_strategy))
        {
            // Skip all disjoint
            return true;
        }

        int const code = point_in_original(turn.point, original,
                                           m_strategy.relate(turn.point, original.m_ring));

        if (code == -1)
        {
            return true;
        }

        Turn& mutable_turn = m_mutable_turns[turn.turn_index];

        if (code == 0)
        {
            // On border of original: always discard
            mutable_turn.is_turn_traversable = false;
        }

        // Point is inside an original ring
        if (original.m_is_interior)
        {
            mutable_turn.count_in_original--;
        }
        else if (original.m_has_interiors)
        {
            mutable_turn.count_in_original++;
        }
        else
        {
            // It is an exterior ring and there are no interior rings.
            // Then we are completely ready with this turn
            mutable_turn.within_original = true;
            mutable_turn.count_in_original = 1;
        }

        return true;
    }

private :
    Turns& m_mutable_turns;
    Strategy const& m_strategy;
};


}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_ORIGINAL_VISITOR
