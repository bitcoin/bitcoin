// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_LINEAR_HPP

#include <algorithm>
#include <deque>

#include <boost/range/begin.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/range.hpp>

#include <boost/geometry/policies/predicate_based_interrupt_policy.hpp>
#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>
#include <boost/geometry/policies/robustness/segment_ratio.hpp>

#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>

#include <boost/geometry/algorithms/detail/disjoint/linear_linear.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_duplicates.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_spikes.hpp>

#include <boost/geometry/algorithms/detail/is_simple/debug_print_boundary_points.hpp>
#include <boost/geometry/algorithms/detail/is_simple/failure_policy.hpp>
#include <boost/geometry/algorithms/detail/is_valid/debug_print_turns.hpp>

#include <boost/geometry/algorithms/dispatch/is_simple.hpp>

#include <boost/geometry/strategies/intersection.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_simple
{


template <typename Turn>
inline bool check_segment_indices(Turn const& turn,
                                  signed_size_type last_index)
{
    return
        (turn.operations[0].seg_id.segment_index == 0
         && turn.operations[1].seg_id.segment_index == last_index)
        ||
        (turn.operations[0].seg_id.segment_index == 0
         && turn.operations[1].seg_id.segment_index == last_index);
}


template
<
    typename Geometry,
    typename Strategy,
    typename Tag = typename tag<Geometry>::type
>
class is_acceptable_turn
    : not_implemented<Geometry>
{};

template <typename Linestring, typename Strategy>
class is_acceptable_turn<Linestring, Strategy, linestring_tag>
{
public:
    is_acceptable_turn(Linestring const& linestring, Strategy const& strategy)
        : m_linestring(linestring)
        , m_is_closed(geometry::detail::equals::equals_point_point(
                          range::front(linestring), range::back(linestring), strategy))
    {}

    template <typename Turn>
    inline bool apply(Turn const& turn) const
    {
        BOOST_GEOMETRY_ASSERT(boost::size(m_linestring) > 1);
        return m_is_closed
            && turn.method == overlay::method_none
            && check_segment_indices(turn, boost::size(m_linestring) - 2)
            && turn.operations[0].fraction.is_zero();
    }

private:
    Linestring const& m_linestring;
    bool const m_is_closed;
};

template <typename MultiLinestring, typename Strategy>
class is_acceptable_turn<MultiLinestring, Strategy, multi_linestring_tag>
{
private:
    template <typename Point, typename Linestring>
    inline bool is_boundary_point_of(Point const& point, Linestring const& linestring) const
    {
        BOOST_GEOMETRY_ASSERT(boost::size(linestring) > 1);
        using geometry::detail::equals::equals_point_point;
        return ! equals_point_point(range::front(linestring), range::back(linestring), m_strategy)
            && (equals_point_point(point, range::front(linestring), m_strategy)
                || equals_point_point(point, range::back(linestring), m_strategy));
    }

    template <typename Turn, typename Linestring>
    inline bool is_closing_point_of(Turn const& turn, Linestring const& linestring) const
    {
        BOOST_GEOMETRY_ASSERT(boost::size(linestring) > 1);
        using geometry::detail::equals::equals_point_point;
        return turn.method == overlay::method_none
            && check_segment_indices(turn, boost::size(linestring) - 2)
            && equals_point_point(range::front(linestring), range::back(linestring), m_strategy)
            && turn.operations[0].fraction.is_zero();
    }

    template <typename Linestring1, typename Linestring2>
    inline bool have_same_boundary_points(Linestring1 const& ls1, Linestring2 const& ls2) const
    {
        using geometry::detail::equals::equals_point_point;
        return equals_point_point(range::front(ls1), range::front(ls2), m_strategy)
             ? equals_point_point(range::back(ls1), range::back(ls2), m_strategy)
             : (equals_point_point(range::front(ls1), range::back(ls2), m_strategy)
                && equals_point_point(range::back(ls1), range::front(ls2), m_strategy));
    }

public:
    is_acceptable_turn(MultiLinestring const& multilinestring, Strategy const& strategy)
        : m_multilinestring(multilinestring)
        , m_strategy(strategy)
    {}

    template <typename Turn>
    inline bool apply(Turn const& turn) const
    {
        typedef typename boost::range_value<MultiLinestring>::type linestring_type;
        
        linestring_type const& ls1 =
            range::at(m_multilinestring, turn.operations[0].seg_id.multi_index);

        linestring_type const& ls2 =
            range::at(m_multilinestring, turn.operations[1].seg_id.multi_index);

        if (turn.operations[0].seg_id.multi_index
            == turn.operations[1].seg_id.multi_index)
        {
            return is_closing_point_of(turn, ls1);
        }

        return
            is_boundary_point_of(turn.point, ls1)
            && is_boundary_point_of(turn.point, ls2)
            &&
            ( boost::size(ls1) != 2
              || boost::size(ls2) != 2
              || ! have_same_boundary_points(ls1, ls2) );
    }

private:
    MultiLinestring const& m_multilinestring;
    Strategy const& m_strategy;
};


template <typename Linear, typename Strategy>
inline bool has_self_intersections(Linear const& linear, Strategy const& strategy)
{
    typedef typename point_type<Linear>::type point_type;

    // compute self turns
    typedef detail::overlay::turn_info<point_type> turn_info;

    std::deque<turn_info> turns;

    typedef detail::overlay::get_turn_info
        <
            detail::disjoint::assign_disjoint_policy
        > turn_policy;

    typedef is_acceptable_turn
        <
            Linear, Strategy
        > is_acceptable_turn_type;

    is_acceptable_turn_type predicate(linear, strategy);
    detail::overlay::predicate_based_interrupt_policy
        <
            is_acceptable_turn_type
        > interrupt_policy(predicate);

    // TODO: skip_adjacent should be set to false
    detail::self_get_turn_points::get_turns
        <
            false, turn_policy
        >::apply(linear,
                 strategy,
                 detail::no_rescale_policy(),
                 turns,
                 interrupt_policy, 0, true);

    detail::is_valid::debug_print_turns(turns.begin(), turns.end());
    debug_print_boundary_points(linear);

    return interrupt_policy.has_intersections;
}


template <typename Linestring, bool CheckSelfIntersections = true>
struct is_simple_linestring
{
    template <typename Strategy>
    static inline bool apply(Linestring const& linestring,
                             Strategy const& strategy)
    {
        simplicity_failure_policy policy;
        return ! boost::empty(linestring)
            && ! detail::is_valid::has_duplicates<Linestring>::apply(linestring, policy, strategy)
            && ! detail::is_valid::has_spikes<Linestring>::apply(linestring, policy, strategy);
    }
};

template <typename Linestring>
struct is_simple_linestring<Linestring, true>
{
    template <typename Strategy>
    static inline bool apply(Linestring const& linestring,
                             Strategy const& strategy)
    {
        return is_simple_linestring<Linestring, false>::apply(linestring, strategy)
            && ! has_self_intersections(linestring, strategy);
    }
};


template <typename MultiLinestring>
struct is_simple_multilinestring
{
private:
    template <typename Strategy>
    struct per_linestring
    {
        per_linestring(Strategy const& strategy)
            : m_strategy(strategy)
        {}

        template <typename Linestring>
        inline bool apply(Linestring const& linestring) const
        {
            return detail::is_simple::is_simple_linestring
                <
                    Linestring,
                    false // do not compute self-intersections
                >::apply(linestring, m_strategy);
        }

        Strategy const& m_strategy;
    };

public:
    template <typename Strategy>
    static inline bool apply(MultiLinestring const& multilinestring,
                             Strategy const& strategy)
    {
        typedef per_linestring<Strategy> per_ls;

        // check each of the linestrings for simplicity
        // but do not compute self-intersections yet; these will be
        // computed for the entire multilinestring
        if ( ! detail::check_iterator_range
                 <
                     per_ls, // do not compute self-intersections
                     true // allow empty multilinestring
                 >::apply(boost::begin(multilinestring),
                          boost::end(multilinestring),
                          per_ls(strategy))
             )
        {
            return false;
        }

        return ! has_self_intersections(multilinestring, strategy);
    }
};



}} // namespace detail::is_simple
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// A linestring is a curve.
// A curve is simple if it does not pass through the same point twice,
// with the possible exception of its two endpoints
//
// Reference: OGC 06-103r4 (6.1.6.1)
template <typename Linestring>
struct is_simple<Linestring, linestring_tag>
    : detail::is_simple::is_simple_linestring<Linestring>
{};


// A MultiLinestring is a MultiCurve
// A MultiCurve is simple if all of its elements are simple and the
// only intersections between any two elements occur at Points that
// are on the boundaries of both elements.
//
// Reference: OGC 06-103r4 (6.1.8.1; Fig. 9)
template <typename MultiLinestring>
struct is_simple<MultiLinestring, multi_linestring_tag>
    : detail::is_simple::is_simple_multilinestring<MultiLinestring>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_LINEAR_HPP
