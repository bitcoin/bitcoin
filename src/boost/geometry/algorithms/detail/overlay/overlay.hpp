// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013-2017 Adam Wulkiewicz, Lodz, Poland

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP


#include <deque>
#include <map>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/overlay/cluster_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/is_self_turn.hpp>
#include <boost/geometry/algorithms/detail/overlay/needs_self_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/traverse.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>

#include <boost/geometry/algorithms/detail/recalculate.hpp>

#include <boost/geometry/algorithms/is_empty.hpp>
#include <boost/geometry/algorithms/reverse.hpp>

#include <boost/geometry/algorithms/detail/overlay/add_rings.hpp>
#include <boost/geometry/algorithms/detail/overlay/assign_parents.hpp>
#include <boost/geometry/algorithms/detail/overlay/ring_properties.hpp>
#include <boost/geometry/algorithms/detail/overlay/select_rings.hpp>
#include <boost/geometry/algorithms/detail/overlay/do_reverse.hpp>

#include <boost/geometry/policies/robustness/segment_ratio_type.hpp>

#include <boost/geometry/util/condition.hpp>

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
#  include <boost/geometry/io/dsv/write.hpp>
#endif


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


//! Default visitor for overlay, doing nothing
struct overlay_null_visitor
{
    void print(char const* ) {}

    template <typename Turns>
    void print(char const* , Turns const& , int) {}

    template <typename Turns>
    void print(char const* , Turns const& , int , int ) {}

    template <typename Turns>
    void visit_turns(int , Turns const& ) {}

    template <typename Clusters, typename Turns>
    void visit_clusters(Clusters const& , Turns const& ) {}

    template <typename Turns, typename Turn, typename Operation>
    void visit_traverse(Turns const& , Turn const& , Operation const& , char const*)
    {}

    template <typename Turns, typename Turn, typename Operation>
    void visit_traverse_reject(Turns const& , Turn const& , Operation const& , traverse_error_type )
    {}

    template <typename Rings>
    void visit_generated_rings(Rings const& )
    {}
};

template
<
    overlay_type OverlayType,
    typename TurnInfoMap,
    typename Turns,
    typename Clusters
>
inline void get_ring_turn_info(TurnInfoMap& turn_info_map, Turns const& turns, Clusters const& clusters)
{
    typedef typename boost::range_value<Turns>::type turn_type;
    typedef typename turn_type::turn_operation_type turn_operation_type;
    typedef typename turn_type::container_type container_type;

    static const operation_type target_operation
            = operation_from_overlay<OverlayType>::value;
    static const operation_type opposite_operation
            = target_operation == operation_union
            ? operation_intersection
            : operation_union;

    for (typename boost::range_iterator<Turns const>::type
            it = boost::begin(turns);
         it != boost::end(turns);
         ++it)
    {
        turn_type const& turn = *it;

        bool cluster_checked = false;
        bool has_blocked = false;

        if (is_self_turn<OverlayType>(turn) && turn.discarded)
        {
            // Discarded self-turns don't count as traversed
            continue;
        }

        for (typename boost::range_iterator<container_type const>::type
                op_it = boost::begin(turn.operations);
            op_it != boost::end(turn.operations);
            ++op_it)
        {
            turn_operation_type const& op = *op_it;
            ring_identifier const ring_id = ring_id_by_seg_id(op.seg_id);

            if (! is_self_turn<OverlayType>(turn)
                && (
                    (BOOST_GEOMETRY_CONDITION(target_operation == operation_union)
                      && op.enriched.count_left > 0)
                  || (BOOST_GEOMETRY_CONDITION(target_operation == operation_intersection)
                      && op.enriched.count_right <= 2)))
            {
                // Avoid including untraversed rings which have polygons on
                // their left side (union) or not two on their right side (int)
                // This can only be done for non-self-turns because of count
                // information
                turn_info_map[ring_id].has_blocked_turn = true;
                continue;
            }

            if (turn.any_blocked())
            {
                turn_info_map[ring_id].has_blocked_turn = true;
            }
            if (turn_info_map[ring_id].has_traversed_turn
                    || turn_info_map[ring_id].has_blocked_turn)
            {
                continue;
            }

            // Check information in colocated turns
            if (! cluster_checked && turn.is_clustered())
            {
                check_colocation(has_blocked, turn.cluster_id, turns, clusters);
                cluster_checked = true;
            }

            // Block rings where any other turn is blocked,
            // and (with exceptions): i for union and u for intersection
            // Exceptions: don't block self-uu for intersection
            //             don't block self-ii for union
            //             don't block (for union) i/u if there is an self-ii too
            if (has_blocked
                || (op.operation == opposite_operation
                    && ! turn.has_colocated_both
                    && ! (turn.both(opposite_operation)
                          && is_self_turn<OverlayType>(turn))))
            {
                turn_info_map[ring_id].has_blocked_turn = true;
            }
        }
    }
}

template
<
    typename GeometryOut, overlay_type OverlayType, bool ReverseOut,
    typename Geometry1, typename Geometry2,
    typename OutputIterator, typename Strategy
>
inline OutputIterator return_if_one_input_is_empty(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out, Strategy const& strategy)
{
    typedef typename geometry::ring_type<GeometryOut>::type ring_type;
    typedef std::deque<ring_type> ring_container_type;

    typedef ring_properties
        <
            typename geometry::point_type<ring_type>::type,
            typename geometry::area_result<ring_type, Strategy>::type
        > properties;

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

    // Union: return either of them
    // Intersection: return nothing
    // Difference: return first of them
    if (OverlayType == overlay_intersection
        || (OverlayType == overlay_difference && geometry::is_empty(geometry1)))
    {
        return out;
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif


    std::map<ring_identifier, ring_turn_info> empty;
    std::map<ring_identifier, properties> all_of_one_of_them;

    select_rings<OverlayType>(geometry1, geometry2, empty, all_of_one_of_them, strategy);
    ring_container_type rings;
    assign_parents<OverlayType>(geometry1, geometry2, rings, all_of_one_of_them, strategy);
    return add_rings<GeometryOut>(all_of_one_of_them, geometry1, geometry2, rings, out, strategy);
}


template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2, bool ReverseOut,
    typename GeometryOut,
    overlay_type OverlayType
>
struct overlay
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy, typename Visitor>
    static inline OutputIterator apply(
                Geometry1 const& geometry1, Geometry2 const& geometry2,
                RobustPolicy const& robust_policy,
                OutputIterator out,
                Strategy const& strategy,
                Visitor& visitor)
    {
        bool const is_empty1 = geometry::is_empty(geometry1);
        bool const is_empty2 = geometry::is_empty(geometry2);

        if (is_empty1 && is_empty2)
        {
            return out;
        }

        if (is_empty1 || is_empty2)
        {
            return return_if_one_input_is_empty
                <
                    GeometryOut, OverlayType, ReverseOut
                >(geometry1, geometry2, out, strategy);
        }

        typedef typename geometry::point_type<GeometryOut>::type point_type;
        typedef detail::overlay::traversal_turn_info
        <
            point_type,
            typename segment_ratio_type<point_type, RobustPolicy>::type
        > turn_info;
        typedef std::deque<turn_info> turn_container_type;

        typedef typename geometry::ring_type<GeometryOut>::type ring_type;
        typedef std::deque<ring_type> ring_container_type;

        // Define the clusters, mapping cluster_id -> turns
        typedef std::map
            <
                signed_size_type,
                cluster_info
            > cluster_type;

        turn_container_type turns;

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "get turns" << std::endl;
#endif
        detail::get_turns::no_interrupt_policy policy;
        geometry::get_turns
            <
                Reverse1, Reverse2,
                assign_policy_only_start_turns
            >(geometry1, geometry2, strategy, robust_policy, turns, policy);

        visitor.visit_turns(1, turns);

#if ! defined(BOOST_GEOMETRY_NO_SELF_TURNS)
        if (! turns.empty() || OverlayType == overlay_dissolve)
        {
            // Calculate self turns if the output contains turns already,
            // and if necessary (e.g.: multi-geometry, polygon with interior rings)
            if (needs_self_turns<Geometry1>::apply(geometry1))
            {
                self_get_turn_points::self_turns<Reverse1, assign_policy_only_start_turns>(geometry1,
                    strategy, robust_policy, turns, policy, 0);
            }
            if (needs_self_turns<Geometry2>::apply(geometry2))
            {
                self_get_turn_points::self_turns<Reverse2, assign_policy_only_start_turns>(geometry2,
                    strategy, robust_policy, turns, policy, 1);
            }
        }
#endif


#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "enrich" << std::endl;
#endif

        cluster_type clusters;
        std::map<ring_identifier, ring_turn_info> turn_info_per_ring;

        geometry::enrich_intersection_points<Reverse1, Reverse2, OverlayType>(
            turns, clusters, geometry1, geometry2, robust_policy, strategy);

        visitor.visit_turns(2, turns);

        visitor.visit_clusters(clusters, turns);

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "traverse" << std::endl;
#endif
        // Traverse through intersection/turn points and create rings of them.
        // These rings are always in clockwise order.
        // In CCW polygons they are marked as "to be reversed" below.
        ring_container_type rings;
        traverse<Reverse1, Reverse2, Geometry1, Geometry2, OverlayType>::apply
                (
                    geometry1, geometry2,
                    strategy,
                    robust_policy,
                    turns, rings,
                    turn_info_per_ring,
                    clusters,
                    visitor
                );
        visitor.visit_turns(3, turns);

        get_ring_turn_info<OverlayType>(turn_info_per_ring, turns, clusters);

        typedef ring_properties
            <
                point_type,
                typename geometry::area_result<ring_type, Strategy>::type
            > properties;

        // Select all rings which are NOT touched by any intersection point
        std::map<ring_identifier, properties> selected_ring_properties;
        select_rings<OverlayType>(geometry1, geometry2, turn_info_per_ring,
                selected_ring_properties, strategy);

        // Add rings created during traversal
        {
            ring_identifier id(2, 0, -1);
            for (typename boost::range_iterator<ring_container_type>::type
                    it = boost::begin(rings);
                 it != boost::end(rings);
                 ++it)
            {
                selected_ring_properties[id] = properties(*it, strategy);
                selected_ring_properties[id].reversed = ReverseOut;
                id.multi_index++;
            }
        }

        assign_parents<OverlayType>(geometry1, geometry2,
            rings, selected_ring_properties, strategy);

        // NOTE: There is no need to check result area for union because
        // as long as the polygons in the input are valid the resulting
        // polygons should be valid as well.
        // By default the area is checked (this is old behavior) however this
        // can be changed with #define. This may be important in non-cartesian CSes.
        // The result may be too big, so the area is negative. In this case either
        // it can be returned or an exception can be thrown.
        return add_rings<GeometryOut>(selected_ring_properties, geometry1, geometry2, rings, out,
                                      strategy,
                                      OverlayType == overlay_union ? 
#if defined(BOOST_GEOMETRY_UNION_THROW_INVALID_OUTPUT_EXCEPTION)
                                      add_rings_throw_if_reversed
#elif defined(BOOST_GEOMETRY_UNION_RETURN_INVALID)
                                      add_rings_add_unordered
#else
                                      add_rings_ignore_unordered
#endif
                                      : add_rings_ignore_unordered);
    }

    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(
                Geometry1 const& geometry1, Geometry2 const& geometry2,
                RobustPolicy const& robust_policy,
                OutputIterator out,
                Strategy const& strategy)
    {
        overlay_null_visitor visitor;
        return apply(geometry1, geometry2, robust_policy, out, strategy, visitor);
    }
};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP
