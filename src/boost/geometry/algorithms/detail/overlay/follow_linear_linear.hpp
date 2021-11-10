// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html


#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_LINEAR_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_LINEAR_LINEAR_HPP

#include <cstddef>
#include <algorithm>
#include <iterator>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>
#include <boost/throw_exception.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/algorithms/detail/overlay/follow.hpp>
#include <boost/geometry/algorithms/detail/overlay/inconsistent_turns_exception.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>

#include <boost/geometry/algorithms/detail/turns/debug_turn.hpp>

#include <boost/geometry/algorithms/detail/tupled_output.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/util/condition.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

namespace following { namespace linear
{




// follower for linear/linear geometries set operations

template <typename Turn, typename Operation>
static inline bool is_entering(Turn const& turn,
                               Operation const& operation)
{
    if ( turn.method != method_touch && turn.method != method_touch_interior )
    {
        return false;
    }
    return operation.operation == operation_intersection;
}



template <typename Turn, typename Operation>
static inline bool is_staying_inside(Turn const& turn,
                                     Operation const& operation, 
                                     bool entered)
{
    if ( !entered )
    {
        return false;
    }

    if ( turn.method != method_equal && turn.method != method_collinear )
    {
        return false;
    }
    return operation.operation == operation_continue;
}



template <typename Turn, typename Operation>
static inline bool is_leaving(Turn const& turn,
                              Operation const& operation,
                              bool entered)
{
    if ( !entered )
    {
        return false;
    }

    if ( turn.method != method_touch
         && turn.method != method_touch_interior
         && turn.method != method_equal
         && turn.method != method_collinear )
    {
        return false;
    }

    if ( operation.operation == operation_blocked )
    {
        return true;
    }

    if ( operation.operation != operation_union )
    {
        return false;
    }

    return operation.is_collinear;
}



template <typename Turn, typename Operation>
static inline bool is_isolated_point(Turn const& turn,
                                     Operation const& operation,
                                     bool entered)
{
    if ( entered )
    {
        return false;
    }

    if ( turn.method == method_none )
    {
        BOOST_GEOMETRY_ASSERT( operation.operation == operation_continue );
        return true;
    }

    if ( turn.method == method_crosses )
    {
        return true;
    }

    if ( turn.method != method_touch && turn.method != method_touch_interior )
    {
        return false;
    }

    if ( operation.operation == operation_blocked )
    {
        return true;
    }

    if ( operation.operation != operation_union )
    {
        return false;
    }

    return !operation.is_collinear;
}





// GeometryOut - linestring or tuple of at least point and linestring
template
<
    typename GeometryOut,
    typename Linestring,
    typename Linear,
    overlay_type OverlayType,
    bool FollowIsolatedPoints,
    bool FollowContinueTurns
>
class follow_linestring_linear
{
protected:
    // allow spikes (false indicates: do not remove spikes)
    typedef following::action_selector<OverlayType, false> action;

    typedef geometry::detail::output_geometry_access
        <
            GeometryOut, linestring_tag, linestring_tag
        > linear;
    typedef geometry::detail::output_geometry_access
        <
            GeometryOut, point_tag, linestring_tag
        > pointlike;

    template
    <
        typename TurnIterator,
        typename TurnOperationIterator,
        typename LinestringOut,
        typename SegmentIdentifier,
        typename OutputIterator,
        typename SideStrategy
    >
    static inline OutputIterator
    process_turn(TurnIterator it,
                 TurnOperationIterator op_it,
                 bool& entered,
                 std::size_t& enter_count,
                 Linestring const& linestring,
                 LinestringOut& current_piece,
                 SegmentIdentifier& current_segment_id,
                 OutputIterator oit,
                 SideStrategy const& strategy)
    {
        // We don't rescale linear/linear
        detail::no_rescale_policy robust_policy;

        if ( is_entering(*it, *op_it) )
        {
            detail::turns::debug_turn(*it, *op_it, "-> Entering");

            entered = true;
            if ( enter_count == 0 )
            {
                action::enter(current_piece,
                              linestring,
                              current_segment_id,
                              op_it->seg_id.segment_index,
                              it->point, *op_it, strategy, robust_policy,
                              linear::get(oit));
            }
            ++enter_count;
        }
        else if ( is_leaving(*it, *op_it, entered) )
        {
            detail::turns::debug_turn(*it, *op_it, "-> Leaving");

            --enter_count;
            if ( enter_count == 0 )
            {
                entered = false;
                action::leave(current_piece,
                              linestring,
                              current_segment_id,
                              op_it->seg_id.segment_index,
                              it->point, *op_it, strategy, robust_policy,
                              linear::get(oit));
            }
        }
        else if ( BOOST_GEOMETRY_CONDITION(FollowIsolatedPoints)
                  && is_isolated_point(*it, *op_it, entered) )
        {
            detail::turns::debug_turn(*it, *op_it, "-> Isolated point");

            action::template isolated_point
                <
                    typename pointlike::type
                >(it->point, pointlike::get(oit));
        }
        else if ( BOOST_GEOMETRY_CONDITION(FollowContinueTurns)
                  && is_staying_inside(*it, *op_it, entered) )
        {
            detail::turns::debug_turn(*it, *op_it, "-> Staying inside");

            entered = true;
        }
        return oit;
    }

    template
    <
        typename SegmentIdentifier,
        typename LinestringOut,
        typename OutputIterator,
        typename SideStrategy
    >
    static inline OutputIterator
    process_end(bool entered,
                Linestring const& linestring,
                SegmentIdentifier const& current_segment_id,
                LinestringOut& current_piece,
                OutputIterator oit,
                SideStrategy const& strategy)
    {
        if ( action::is_entered(entered) )
        {
            // We don't rescale linear/linear
            detail::no_rescale_policy robust_policy;

            detail::copy_segments::copy_segments_linestring
                <
                    false, false // do not reverse; do not remove spikes
                >::apply(linestring,
                         current_segment_id,
                         static_cast<signed_size_type>(boost::size(linestring) - 1),
                         strategy,
                         robust_policy,
                         current_piece);
        }

        // Output the last one, if applicable
        if (::boost::size(current_piece) > 1)
        {
            *linear::get(oit)++ = current_piece;
        }

        return oit;
    }

public:
    template <typename TurnIterator, typename OutputIterator, typename SideStrategy>
    static inline OutputIterator
    apply(Linestring const& linestring, Linear const&,
          TurnIterator first, TurnIterator beyond,
          OutputIterator oit,
          SideStrategy const& strategy)
    {
        // Iterate through all intersection points (they are
        // ordered along the each line)

        typename linear::type current_piece;
        geometry::segment_identifier current_segment_id(0, -1, -1, -1);

        bool entered = false;
        std::size_t enter_count = 0;

        for (TurnIterator it = first; it != beyond; ++it)
        {
            oit = process_turn(it, boost::begin(it->operations),
                               entered, enter_count, 
                               linestring,
                               current_piece, current_segment_id,
                               oit,
                               strategy);
        }

#if ! defined(BOOST_GEOMETRY_OVERLAY_NO_THROW)
        if (enter_count != 0)
        {
            BOOST_THROW_EXCEPTION(inconsistent_turns_exception());
        }
#else
        BOOST_GEOMETRY_ASSERT(enter_count == 0);
#endif

        return process_end(entered, linestring,
                           current_segment_id, current_piece,
                           oit,
                           strategy);
    }
};




template
<
    typename LinestringOut,
    typename MultiLinestring,
    typename Linear,
    overlay_type OverlayType,
    bool FollowIsolatedPoints,
    bool FollowContinueTurns
>
class follow_multilinestring_linear
    : follow_linestring_linear
        <
            LinestringOut,
            typename boost::range_value<MultiLinestring>::type,
            Linear,
            OverlayType,
            FollowIsolatedPoints,
            FollowContinueTurns
        >
{
protected:
    typedef typename boost::range_value<MultiLinestring>::type Linestring;

    typedef follow_linestring_linear
        <
            LinestringOut, Linestring, Linear,
            OverlayType, FollowIsolatedPoints, FollowContinueTurns
        > Base;

    typedef following::action_selector<OverlayType> action;

    typedef typename boost::range_iterator
        <
            MultiLinestring const
        >::type linestring_iterator;


    template <typename OutputIt, overlay_type OT>
    struct copy_linestrings_in_range
    {
        static inline OutputIt
        apply(linestring_iterator, linestring_iterator, OutputIt oit)
        {
            return oit;
        }
    };

    template <typename OutputIt>
    struct copy_linestrings_in_range<OutputIt, overlay_difference>
    {
        static inline OutputIt
        apply(linestring_iterator first, linestring_iterator beyond,
              OutputIt oit)
        {
            for (linestring_iterator ls_it = first; ls_it != beyond; ++ls_it)
            {
                LinestringOut line_out;
                geometry::convert(*ls_it, line_out);
                *oit++ = line_out;
            }
            return oit;
        }
    };

    template <typename TurnIterator>
    static inline signed_size_type get_multi_index(TurnIterator it)
    {
        return boost::begin(it->operations)->seg_id.multi_index;
    }

    class has_other_multi_id
    {
    private:
        signed_size_type m_multi_id;

    public:
        has_other_multi_id(signed_size_type multi_id)
            : m_multi_id(multi_id) {}

        template <typename Turn>
        bool operator()(Turn const& turn) const
        {
            return boost::begin(turn.operations)->seg_id.multi_index
                != m_multi_id;
        }
    };

public:
    template <typename TurnIterator, typename OutputIterator, typename SideStrategy>
    static inline OutputIterator
    apply(MultiLinestring const& multilinestring, Linear const& linear,
          TurnIterator first, TurnIterator beyond,
          OutputIterator oit,
          SideStrategy const& strategy)
    {
        BOOST_GEOMETRY_ASSERT( first != beyond );

        typedef copy_linestrings_in_range
            <
                OutputIterator, OverlayType
            > copy_linestrings;

        linestring_iterator ls_first = boost::begin(multilinestring);
        linestring_iterator ls_beyond = boost::end(multilinestring);

        // Iterate through all intersection points (they are
        // ordered along the each linestring)

        signed_size_type current_multi_id = get_multi_index(first);

        oit = copy_linestrings::apply(ls_first,
                                      ls_first + current_multi_id,
                                      oit);

        TurnIterator per_ls_next = first;
        do {
            TurnIterator per_ls_current = per_ls_next;

            // find turn with different multi-index
            per_ls_next = std::find_if(per_ls_current, beyond,
                                       has_other_multi_id(current_multi_id));

            oit = Base::apply(*(ls_first + current_multi_id),
                              linear, per_ls_current, per_ls_next, oit, strategy);

            signed_size_type next_multi_id = -1;
            linestring_iterator ls_next = ls_beyond;
            if ( per_ls_next != beyond )
            {
                next_multi_id = get_multi_index(per_ls_next);
                ls_next = ls_first + next_multi_id;
            }
            oit = copy_linestrings::apply(ls_first + current_multi_id + 1,
                                          ls_next,
                                          oit);

            current_multi_id = next_multi_id;
        }
        while ( per_ls_next != beyond );

        return oit;
    }
};






template
<
    typename LinestringOut,
    typename Geometry1,
    typename Geometry2,
    overlay_type OverlayType,
    bool FollowIsolatedPoints,
    bool FollowContinueTurns,
    typename TagIn1 = typename tag<Geometry1>::type
>
struct follow
    : not_implemented<Geometry1>
{};



template
<
    typename LinestringOut,
    typename Linestring,
    typename Linear,
    overlay_type OverlayType,
    bool FollowIsolatedPoints,
    bool FollowContinueTurns
>
struct follow
    <
        LinestringOut, Linestring, Linear,
        OverlayType, FollowIsolatedPoints, FollowContinueTurns,
        linestring_tag
    > : follow_linestring_linear
        <
            LinestringOut, Linestring, Linear,
            OverlayType, FollowIsolatedPoints, FollowContinueTurns
        >
{};


template
<
    typename LinestringOut,
    typename MultiLinestring,
    typename Linear,
    overlay_type OverlayType,
    bool FollowIsolatedPoints,
    bool FollowContinueTurns
>
struct follow
    <
        LinestringOut, MultiLinestring, Linear,
        OverlayType, FollowIsolatedPoints, FollowContinueTurns,
        multi_linestring_tag
    > : follow_multilinestring_linear
        <
            LinestringOut, MultiLinestring, Linear,
            OverlayType, FollowIsolatedPoints, FollowContinueTurns
        >
{};



}} // namespace following::linear

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_LINEAR_LINEAR_HPP
