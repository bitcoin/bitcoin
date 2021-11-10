// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2020 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CLUSTER_EXITS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CLUSTER_EXITS_HPP

#include <cstddef>
#include <set>
#include <vector>

#include <boost/range/value_type.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>
#include <boost/geometry/util/condition.hpp>

#if defined(BOOST_GEOMETRY_DEBUG_INTERSECTION) \
    || defined(BOOST_GEOMETRY_OVERLAY_REPORT_WKT) \
    || defined(BOOST_GEOMETRY_DEBUG_TRAVERSE)
#  include <string>
#  include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#  include <boost/geometry/io/wkt/wkt.hpp>
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

// Structure to check relatively simple cluster cases
template <overlay_type OverlayType,
          typename Turns,
          typename Sbs>
struct cluster_exits
{
private :
    static const operation_type target_operation = operation_from_overlay<OverlayType>::value;
    typedef typename boost::range_value<Turns>::type turn_type;
    typedef typename turn_type::turn_operation_type turn_operation_type;

    struct linked_turn_op_info
    {
        explicit linked_turn_op_info(signed_size_type ti = -1, int oi = -1,
                    signed_size_type nti = -1)
            : turn_index(ti)
            , op_index(oi)
            , next_turn_index(nti)
            , rank_index(-1)
        {}

        signed_size_type turn_index;
        int op_index;
        signed_size_type next_turn_index;
        signed_size_type rank_index;
    };

    typedef typename std::vector<linked_turn_op_info>::const_iterator const_it_type;
    typedef typename std::vector<linked_turn_op_info>::iterator it_type;
    typedef typename std::set<signed_size_type>::const_iterator sit_type;

    inline signed_size_type get_rank(Sbs const& sbs,
            linked_turn_op_info const& info) const
    {
        for (std::size_t i = 0; i < sbs.m_ranked_points.size(); i++)
        {
            typename Sbs::rp const& rp = sbs.m_ranked_points[i];
            if (rp.turn_index == info.turn_index
                    && rp.operation_index == info.op_index
                    && rp.direction == sort_by_side::dir_to)
            {
                return rp.rank;
            }
        }
        return -1;
    }

    std::set<signed_size_type> const& m_ids;
    std::vector<linked_turn_op_info> possibilities;
    std::vector<linked_turn_op_info> blocked;

    bool m_valid;

    bool collect(Turns const& turns)
    {
        for (sit_type it = m_ids.begin(); it != m_ids.end(); ++it)
        {
            signed_size_type cluster_turn_index = *it;
            turn_type const& cluster_turn = turns[cluster_turn_index];
            if (cluster_turn.discarded)
            {
                continue;
            }
            if (cluster_turn.both(target_operation))
            {
                // Not (yet) supported, can be cluster of u/u turns
                return false;
            }
            for (int i = 0; i < 2; i++)
            {
                turn_operation_type const& op = cluster_turn.operations[i];
                turn_operation_type const& other_op = cluster_turn.operations[1 - i];
                signed_size_type const ni = op.enriched.get_next_turn_index();

                if (op.operation == target_operation
                    || op.operation == operation_continue)
                {
                    if (ni == cluster_turn_index)
                    {
                        // Not (yet) supported, traveling to itself, can be
                        // hole
                        return false;
                    }
                    possibilities.push_back(
                        linked_turn_op_info(cluster_turn_index, i, ni));
                }
                else if (op.operation == operation_blocked
                         && ! (ni == other_op.enriched.get_next_turn_index())
                         && m_ids.count(ni) == 0)
                {
                    // Points to turn, not part of this cluster,
                    // and that way is blocked. But if the other operation
                    // points at the same turn, it is still fine.
                    blocked.push_back(
                        linked_turn_op_info(cluster_turn_index, i, ni));
                }
            }
        }
        return true;
    }

    bool check_blocked(Sbs const& sbs)
    {
        if (blocked.empty())
        {
            return true;
        }

        for (it_type it = possibilities.begin(); it != possibilities.end(); ++it)
        {
            linked_turn_op_info& info = *it;
            info.rank_index = get_rank(sbs, info);
        }
        for (it_type it = blocked.begin(); it != blocked.end(); ++it)
        {
            linked_turn_op_info& info = *it;
            info.rank_index = get_rank(sbs, info);
        }

        for (const_it_type it = possibilities.begin(); it != possibilities.end(); ++it)
        {
            linked_turn_op_info const& lti = *it;
            for (const_it_type bit = blocked.begin(); bit != blocked.end(); ++bit)
            {
                linked_turn_op_info const& blti = *bit;
                if (blti.next_turn_index == lti.next_turn_index
                        && blti.rank_index == lti.rank_index)
                {
                    return false;
                }
            }
        }
        return true;
    }

public :
    cluster_exits(Turns const& turns,
                  std::set<signed_size_type> const& ids,
                  Sbs const& sbs)
        : m_ids(ids)
        , m_valid(collect(turns) && check_blocked(sbs))
    {
    }

    inline bool apply(signed_size_type& turn_index,
            int& op_index,
            bool first_run = true) const
    {
        if (! m_valid)
        {
            return false;
        }

        // Traversal can either enter the cluster in the first turn,
        // or it can start halfway.
        // If there is one (and only one) possibility pointing outside
        // the cluster, take that one.
        linked_turn_op_info target;
        for (const_it_type it = possibilities.begin();
             it != possibilities.end(); ++it)
        {
            linked_turn_op_info const& lti = *it;
            if (m_ids.count(lti.next_turn_index) == 0)
            {
                if (target.turn_index >= 0
                    && target.next_turn_index != lti.next_turn_index)
                {
                    // Points to different target
                    return false;
                }
                if (first_run
                    && BOOST_GEOMETRY_CONDITION(OverlayType == overlay_buffer)
                    && target.turn_index >= 0)
                {
                    // Target already assigned, so there are more targets
                    // or more ways to the same target
                    return false;
                }

                target = lti;
            }
        }
        if (target.turn_index < 0)
        {
            return false;
        }

        turn_index = target.turn_index;
        op_index = target.op_index;

        return true;
    }
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CLUSTER_EXITS_HPP
