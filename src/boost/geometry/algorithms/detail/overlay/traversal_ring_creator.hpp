// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSAL_RING_CREATOR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSAL_RING_CREATOR_HPP

#include <cstddef>

#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/overlay/backtrack_check_si.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/closure.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template
<
    bool Reverse1,
    bool Reverse2,
    overlay_type OverlayType,
    typename Geometry1,
    typename Geometry2,
    typename Turns,
    typename TurnInfoMap,
    typename Clusters,
    typename Strategy,
    typename RobustPolicy,
    typename Visitor,
    typename Backtrack
>
struct traversal_ring_creator
{
    typedef traversal
            <
                Reverse1, Reverse2, OverlayType,
                Geometry1, Geometry2, Turns, Clusters,
                RobustPolicy,
                decltype(std::declval<Strategy>().side()),
                Visitor
            > traversal_type;

    typedef typename boost::range_value<Turns>::type turn_type;
    typedef typename turn_type::turn_operation_type turn_operation_type;

    static const operation_type target_operation
        = operation_from_overlay<OverlayType>::value;

    inline traversal_ring_creator(Geometry1 const& geometry1, Geometry2 const& geometry2,
            Turns& turns, TurnInfoMap& turn_info_map,
            Clusters const& clusters,
            Strategy const& strategy,
            RobustPolicy const& robust_policy, Visitor& visitor)
        : m_trav(geometry1, geometry2, turns, clusters,
                 robust_policy, strategy.side(), visitor)
        , m_geometry1(geometry1)
        , m_geometry2(geometry2)
        , m_turns(turns)
        , m_turn_info_map(turn_info_map)
        , m_clusters(clusters)
        , m_strategy(strategy)
        , m_robust_policy(robust_policy)
        , m_visitor(visitor)
    {
    }

    template <typename Ring>
    inline traverse_error_type travel_to_next_turn(signed_size_type start_turn_index,
                int start_op_index,
                signed_size_type& turn_index,
                int& op_index,
                Ring& current_ring,
                bool is_start)
    {
        int const previous_op_index = op_index;
        signed_size_type const previous_turn_index = turn_index;
        turn_type& previous_turn = m_turns[turn_index];
        turn_operation_type& previous_op = previous_turn.operations[op_index];
        segment_identifier previous_seg_id;

        signed_size_type to_vertex_index = -1;
        if (! m_trav.select_turn_from_enriched(turn_index, previous_seg_id,
                          to_vertex_index, start_turn_index, start_op_index,
                          previous_turn, previous_op, is_start))
        {
            return is_start
                    ? traverse_error_no_next_ip_at_start
                    : traverse_error_no_next_ip;
        }
        if (to_vertex_index >= 0)
        {
            if (previous_op.seg_id.source_index == 0)
            {
                geometry::copy_segments<Reverse1>(m_geometry1,
                        previous_op.seg_id, to_vertex_index,
                        m_strategy, m_robust_policy, current_ring);
            }
            else
            {
                geometry::copy_segments<Reverse2>(m_geometry2,
                        previous_op.seg_id, to_vertex_index,
                        m_strategy, m_robust_policy, current_ring);
            }
        }

        if (m_turns[turn_index].discarded)
        {
            return is_start
                ? traverse_error_dead_end_at_start
                : traverse_error_dead_end;
        }

        if (is_start)
        {
            // Register the start
            previous_op.visited.set_started();
            m_visitor.visit_traverse(m_turns, previous_turn, previous_op, "Start");
        }

        if (! m_trav.select_turn(start_turn_index, start_op_index,
                turn_index, op_index,
                previous_op_index, previous_turn_index, previous_seg_id,
                is_start, current_ring.size() > 1))
        {
            return is_start
                ? traverse_error_no_next_ip_at_start
                : traverse_error_no_next_ip;
        }

        {
            // Check operation (TODO: this might be redundant or should be catched before)
            const turn_type& current_turn = m_turns[turn_index];
            const turn_operation_type& op = current_turn.operations[op_index];
            if (op.visited.finalized()
                || m_trav.is_visited(current_turn, op, turn_index, op_index))
            {
                return traverse_error_visit_again;
            }
        }

        // Update registration and append point
        turn_type& current_turn = m_turns[turn_index];
        turn_operation_type& op = current_turn.operations[op_index];
        detail::overlay::append_no_collinear(current_ring, current_turn.point,
                                             m_strategy, m_robust_policy);

        // Register the visit
        m_trav.set_visited(current_turn, op);
        m_visitor.visit_traverse(m_turns, current_turn, op, "Visit");

        return traverse_error_none;
    }

    template <typename Ring>
    inline traverse_error_type traverse(Ring& ring,
            signed_size_type start_turn_index, int start_op_index)
    {
        turn_type const& start_turn = m_turns[start_turn_index];
        turn_operation_type& start_op = m_turns[start_turn_index].operations[start_op_index];

        detail::overlay::append_no_collinear(ring, start_turn.point,
                                             m_strategy, m_robust_policy);

        signed_size_type current_turn_index = start_turn_index;
        int current_op_index = start_op_index;

        traverse_error_type error = travel_to_next_turn(start_turn_index,
                    start_op_index,
                    current_turn_index, current_op_index,
                    ring, true);

        if (error != traverse_error_none)
        {
            // This is not necessarily a problem, it happens for clustered turns
            // which are "build in" or otherwise point inwards
            return error;
        }

        if (current_turn_index == start_turn_index)
        {
            start_op.visited.set_finished();
            m_visitor.visit_traverse(m_turns, m_turns[current_turn_index], start_op, "Early finish");
            return traverse_error_none;
        }

        if (start_turn.is_clustered())
        {
            turn_type& turn = m_turns[current_turn_index];
            turn_operation_type& op = turn.operations[current_op_index];
            if (turn.cluster_id == start_turn.cluster_id
                && op.enriched.get_next_turn_index() == start_turn_index)
            {
                op.visited.set_finished();
                m_visitor.visit_traverse(m_turns, m_turns[current_turn_index], start_op, "Early finish (cluster)");
                return traverse_error_none;
            }
        }

        std::size_t const max_iterations = 2 + 2 * m_turns.size();
        for (std::size_t i = 0; i <= max_iterations; i++)
        {
            // We assume clockwise polygons only, non self-intersecting, closed.
            // However, the input might be different, and checking validity
            // is up to the library user.

            // Therefore we make here some sanity checks. If the input
            // violates the assumptions, the output polygon will not be correct
            // but the routine will stop and output the current polygon, and
            // will continue with the next one.

            // Below three reasons to stop.
            error = travel_to_next_turn(start_turn_index, start_op_index,
                    current_turn_index, current_op_index,
                    ring, false);

            if (error != traverse_error_none)
            {
                return error;
            }

            if (current_turn_index == start_turn_index
                    && current_op_index == start_op_index)
            {
                start_op.visited.set_finished();
                m_visitor.visit_traverse(m_turns, start_turn, start_op, "Finish");
                return traverse_error_none;
            }
        }

        return traverse_error_endless_loop;
    }

    template <typename Rings>
    void traverse_with_operation(turn_type const& start_turn,
            std::size_t turn_index, int op_index,
            Rings& rings, std::size_t& finalized_ring_size,
            typename Backtrack::state_type& state)
    {
        typedef typename boost::range_value<Rings>::type ring_type;

        turn_operation_type const& start_op = start_turn.operations[op_index];

        if (! start_op.visited.none()
            || ! start_op.enriched.startable
            || start_op.visited.rejected()
            || ! (start_op.operation == target_operation
                || start_op.operation == detail::overlay::operation_continue))
        {
            return;
        }

        ring_type ring;
        traverse_error_type traverse_error = traverse(ring, turn_index, op_index);

        if (traverse_error == traverse_error_none)
        {
            std::size_t const min_num_points
                    = core_detail::closure::minimum_ring_size
                            <
                                geometry::closure<ring_type>::value
                            >::value;

            if (geometry::num_points(ring) >= min_num_points)
            {
                clean_closing_dups_and_spikes(ring, m_strategy, m_robust_policy);
                rings.push_back(ring);

                m_trav.finalize_visit_info(m_turn_info_map);
                finalized_ring_size++;
            }
        }
        else
        {
            Backtrack::apply(finalized_ring_size,
                             rings, ring, m_turns, start_turn,
                             m_turns[turn_index].operations[op_index],
                             traverse_error,
                             m_geometry1, m_geometry2,
                             m_strategy, m_robust_policy,
                             state, m_visitor);
        }
    }

    int get_operation_index(turn_type const& turn) const
    {
        // When starting with a continue operation, the one
        // with the smallest (for intersection) or largest (for union)
        // remaining distance (#8310b)
        // Also to avoid skipping a turn in between, which can happen
        // in rare cases (e.g. #130)
        static const bool is_union
            = operation_from_overlay<OverlayType>::value == operation_union;

        turn_operation_type const& op0 = turn.operations[0];
        turn_operation_type const& op1 = turn.operations[1];
        return op0.remaining_distance <= op1.remaining_distance
                ? (is_union ? 1 : 0)
                : (is_union ? 0 : 1);
    }

    template <typename Rings>
    void iterate(Rings& rings, std::size_t& finalized_ring_size,
                 typename Backtrack::state_type& state)
    {
        for (std::size_t turn_index = 0; turn_index < m_turns.size(); ++turn_index)
        {
            turn_type const& turn = m_turns[turn_index];

            if (turn.discarded || turn.blocked())
            {
                // Skip discarded and blocked turns
                continue;
            }

            if (turn.both(operation_continue))
            {
                traverse_with_operation(turn, turn_index,
                        get_operation_index(turn),
                        rings, finalized_ring_size, state);
            }
            else
            {
                for (int op_index = 0; op_index < 2; op_index++)
                {
                    traverse_with_operation(turn, turn_index, op_index,
                            rings, finalized_ring_size, state);
                }
            }
        }
    }

    template <typename Rings>
    void iterate_with_preference(std::size_t phase,
                 Rings& rings, std::size_t& finalized_ring_size,
                 typename Backtrack::state_type& state)
    {
        for (std::size_t turn_index = 0; turn_index < m_turns.size(); ++turn_index)
        {
            turn_type const& turn = m_turns[turn_index];

            if (turn.discarded || turn.blocked())
            {
                // Skip discarded and blocked turns
                continue;
            }

            turn_operation_type const& op0 = turn.operations[0];
            turn_operation_type const& op1 = turn.operations[1];

            if (phase == 0)
            {
                if (! op0.enriched.prefer_start && ! op1.enriched.prefer_start)
                {
                    // Not preferred, take next one
                    continue;
                }
            }

            if (turn.both(operation_continue))
            {
                traverse_with_operation(turn, turn_index,
                        get_operation_index(turn),
                        rings, finalized_ring_size, state);
            }
            else
            {
                bool const forward = op0.enriched.prefer_start;

                int op_index = forward ? 0 : 1;
                int const increment = forward ? 1 : -1;

                for (int i = 0; i < 2; i++, op_index += increment)
                {
                    traverse_with_operation(turn, turn_index, op_index,
                            rings, finalized_ring_size, state);
                }
            }
        }
    }

private:
    traversal_type m_trav;

    Geometry1 const& m_geometry1;
    Geometry2 const& m_geometry2;
    Turns& m_turns;
    TurnInfoMap& m_turn_info_map; // contains turn-info information per ring
    Clusters const& m_clusters;
    Strategy const& m_strategy;
    RobustPolicy const& m_robust_policy;
    Visitor& m_visitor;
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSAL_RING_CREATOR_HPP
