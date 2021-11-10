// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_HANDLE_COLOCATIONS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_HANDLE_COLOCATIONS_HPP

#include <cstddef>
#include <algorithm>
#include <map>
#include <vector>

#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/algorithms/detail/overlay/cluster_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/do_reverse.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_clusters.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_ring.hpp>
#include <boost/geometry/algorithms/detail/overlay/is_self_turn.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/sort_by_side.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/util/condition.hpp>

#if defined(BOOST_GEOMETRY_DEBUG_HANDLE_COLOCATIONS)
#  include <iostream>
#  include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#  include <boost/geometry/io/wkt/wkt.hpp>
#  define BOOST_GEOMETRY_DEBUG_IDENTIFIER
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template <typename Turns, typename Clusters>
inline void remove_clusters(Turns& turns, Clusters& clusters)
{
    typename Clusters::iterator it = clusters.begin();
    while (it != clusters.end())
    {
        // Hold iterator and increase. We can erase cit, this keeps the
        // iterator valid (cf The standard associative-container erase idiom)
        typename Clusters::iterator current_it = it;
        ++it;

        std::set<signed_size_type> const& turn_indices
                = current_it->second.turn_indices;
        if (turn_indices.size() == 1)
        {
            signed_size_type const turn_index = *turn_indices.begin();
            turns[turn_index].cluster_id = -1;
            clusters.erase(current_it);
        }
    }
}

template <typename Turns, typename Clusters>
inline void cleanup_clusters(Turns& turns, Clusters& clusters)
{
    // Removes discarded turns from clusters
    for (typename Clusters::iterator mit = clusters.begin();
         mit != clusters.end(); ++mit)
    {
        cluster_info& cinfo = mit->second;
        std::set<signed_size_type>& ids = cinfo.turn_indices;
        for (std::set<signed_size_type>::iterator sit = ids.begin();
             sit != ids.end(); /* no increment */)
        {
            std::set<signed_size_type>::iterator current_it = sit;
            ++sit;

            signed_size_type const turn_index = *current_it;
            if (turns[turn_index].discarded)
            {
                ids.erase(current_it);
            }
        }
    }

    remove_clusters(turns, clusters);
}

template <typename Turn, typename IdSet>
inline void discard_colocated_turn(Turn& turn, IdSet& ids, signed_size_type id)
{
    turn.discarded = true;
    // Set cluster id to -1, but don't clear colocated flags
    turn.cluster_id = -1;
    // To remove it later from clusters
    ids.insert(id);
}

template <bool Reverse>
inline bool is_interior(segment_identifier const& seg_id)
{
    return Reverse ? seg_id.ring_index == -1 : seg_id.ring_index >= 0;
}

template <bool Reverse0, bool Reverse1>
inline bool is_ie_turn(segment_identifier const& ext_seg_0,
                       segment_identifier const& ext_seg_1,
                       segment_identifier const& int_seg_0,
                       segment_identifier const& other_seg_1)
{
    if (ext_seg_0.source_index == ext_seg_1.source_index)
    {
        // External turn is a self-turn, dont discard internal turn for this
        return false;
    }


    // Compares two segment identifiers from two turns (external / one internal)

    // From first turn [0], both are from same polygon (multi_index),
    // one is exterior (-1), the other is interior (>= 0),
    // and the second turn [1] handles the same ring

    // For difference, where the rings are processed in reversal, all interior
    // rings become exterior and vice versa. But also the multi property changes:
    // rings originally from the same multi should now be considered as from
    // different multi polygons.
    // But this is not always the case, and at this point hard to figure out
    // (not yet implemented, TODO)

    bool const same_multi0 = ! Reverse0
                             && ext_seg_0.multi_index == int_seg_0.multi_index;

    bool const same_multi1 = ! Reverse1
                             && ext_seg_1.multi_index == other_seg_1.multi_index;

    boost::ignore_unused(same_multi1);

    return same_multi0
            && same_multi1
            && ! is_interior<Reverse0>(ext_seg_0)
            && is_interior<Reverse0>(int_seg_0)
            && ext_seg_1.ring_index == other_seg_1.ring_index;

    // The other way round is tested in another call
}

template
<
    bool Reverse0, bool Reverse1, // Reverse interpretation interior/exterior
    typename Turns,
    typename Clusters
>
inline void discard_interior_exterior_turns(Turns& turns, Clusters& clusters)
{
    typedef std::set<signed_size_type>::const_iterator set_iterator;
    typedef typename boost::range_value<Turns>::type turn_type;

    std::set<signed_size_type> ids_to_remove;

    for (typename Clusters::iterator cit = clusters.begin();
         cit != clusters.end(); ++cit)
    {
        cluster_info& cinfo = cit->second;
        std::set<signed_size_type>& ids = cinfo.turn_indices;

        ids_to_remove.clear();

        for (set_iterator it = ids.begin(); it != ids.end(); ++it)
        {
            turn_type& turn = turns[*it];
            segment_identifier const& seg_0 = turn.operations[0].seg_id;
            segment_identifier const& seg_1 = turn.operations[1].seg_id;

            if (! (turn.both(operation_union)
                   || turn.combination(operation_union, operation_blocked)))
            {
                // Not a uu/ux, so cannot be colocated with a iu turn
                continue;
            }

            for (set_iterator int_it = ids.begin(); int_it != ids.end(); ++int_it)
            {
                if (*it == *int_it)
                {
                    continue;
                }

                // Turn with, possibly, an interior ring involved
                turn_type& int_turn = turns[*int_it];
                segment_identifier const& int_seg_0 = int_turn.operations[0].seg_id;
                segment_identifier const& int_seg_1 = int_turn.operations[1].seg_id;

                if (is_ie_turn<Reverse0, Reverse1>(seg_0, seg_1, int_seg_0, int_seg_1))
                {
                    discard_colocated_turn(int_turn, ids_to_remove, *int_it);
                }
                if (is_ie_turn<Reverse1, Reverse0>(seg_1, seg_0, int_seg_1, int_seg_0))
                {
                    discard_colocated_turn(int_turn, ids_to_remove, *int_it);
                }
            }
        }

        // Erase from the ids (which cannot be done above)
        for (set_iterator sit = ids_to_remove.begin();
             sit != ids_to_remove.end(); ++sit)
        {
            ids.erase(*sit);
        }
    }
}

template
<
    overlay_type OverlayType,
    typename Turns,
    typename Clusters
>
inline void set_colocation(Turns& turns, Clusters const& clusters)
{
    typedef std::set<signed_size_type>::const_iterator set_iterator;
    typedef typename boost::range_value<Turns>::type turn_type;

    for (typename Clusters::const_iterator cit = clusters.begin();
         cit != clusters.end(); ++cit)
    {
        cluster_info const& cinfo = cit->second;
        std::set<signed_size_type> const& ids = cinfo.turn_indices;

        bool both_target = false;
        for (set_iterator it = ids.begin(); it != ids.end(); ++it)
        {
            turn_type const& turn = turns[*it];
            if (turn.both(operation_from_overlay<OverlayType>::value))
            {
                both_target = true;
                break;
            }
        }

        if (both_target)
        {
            for (set_iterator it = ids.begin(); it != ids.end(); ++it)
            {
                turn_type& turn = turns[*it];
                turn.has_colocated_both = true;
            }
        }
    }
}

template
<
    typename Turns,
    typename Clusters
>
inline void check_colocation(bool& has_blocked,
        signed_size_type cluster_id, Turns const& turns, Clusters const& clusters)
{
    typedef typename boost::range_value<Turns>::type turn_type;

    has_blocked = false;

    typename Clusters::const_iterator mit = clusters.find(cluster_id);
    if (mit == clusters.end())
    {
        return;
    }

    cluster_info const& cinfo = mit->second;

    for (std::set<signed_size_type>::const_iterator it
         = cinfo.turn_indices.begin();
         it != cinfo.turn_indices.end(); ++it)
    {
        turn_type const& turn = turns[*it];
        if (turn.any_blocked())
        {
            has_blocked = true;
        }
    }
}

template
<
    typename Turns,
    typename Clusters
>
inline void assign_cluster_ids(Turns& turns, Clusters const& clusters)
{
    for (auto& turn : turns)
    {
        turn.cluster_id = -1;
    }
    for (auto const& kv : clusters)
    {
        for (const auto& index : kv.second.turn_indices)
        {
            turns[index].cluster_id = kv.first;
        }
    }
}

// Checks colocated turns and flags combinations of uu/other, possibly a
// combination of a ring touching another geometry's interior ring which is
// tangential to the exterior ring

// This function can be extended to replace handle_tangencies: at each
// colocation incoming and outgoing vectors should be inspected

template
<
    bool Reverse1, bool Reverse2,
    overlay_type OverlayType,
    typename Geometry0,
    typename Geometry1,
    typename Turns,
    typename Clusters,
    typename RobustPolicy
>
inline bool handle_colocations(Turns& turns, Clusters& clusters,
                               RobustPolicy const& robust_policy)
{
    static const detail::overlay::operation_type target_operation
            = detail::overlay::operation_from_overlay<OverlayType>::value;

    get_clusters(turns, clusters, robust_policy);

    if (clusters.empty())
    {
        return false;
    }

    assign_cluster_ids(turns, clusters);

    // Get colocated information here, and not later, to keep information
    // on turns which are discarded afterwards
    set_colocation<OverlayType>(turns, clusters);

    if (BOOST_GEOMETRY_CONDITION(target_operation == operation_intersection))
    {
        discard_interior_exterior_turns
            <
                do_reverse<geometry::point_order<Geometry0>::value>::value != Reverse1,
                do_reverse<geometry::point_order<Geometry1>::value>::value != Reverse2
            >(turns, clusters);
    }

    // There might be clusters having only one turn, if the rest is discarded
    // This is cleaned up later, after gathering the properties.

#if defined(BOOST_GEOMETRY_DEBUG_HANDLE_COLOCATIONS)
    std::cout << "*** Colocations " << map.size() << std::endl;
    for (auto const& kv : map)
    {
        std::cout << kv.first << std::endl;
        for (auto const& toi : kv.second)
        {
            detail::debug::debug_print_turn(turns[toi.turn_index]);
            std::cout << std::endl;
        }
    }
#endif

    return true;
}


struct is_turn_index
{
    is_turn_index(signed_size_type index)
        : m_index(index)
    {}

    template <typename Indexed>
    inline bool operator()(Indexed const& indexed) const
    {
        // Indexed is a indexed_turn_operation<Operation>
        return indexed.turn_index == m_index;
    }

    signed_size_type m_index;
};

template
<
    typename Sbs,
    typename Point,
    typename Turns,
    typename Geometry1,
    typename Geometry2
>
inline bool fill_sbs(Sbs& sbs, Point& turn_point,
                     cluster_info const& cinfo,
                     Turns const& turns,
                     Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    typedef typename boost::range_value<Turns>::type turn_type;

    std::set<signed_size_type> const& ids = cinfo.turn_indices;

    if (ids.empty())
    {
        return false;
    }

    bool first = true;
    for (std::set<signed_size_type>::const_iterator sit = ids.begin();
         sit != ids.end(); ++sit)
    {
        signed_size_type turn_index = *sit;
        turn_type const& turn = turns[turn_index];
        if (first)
        {
            turn_point = turn.point;
        }
        for (int i = 0; i < 2; i++)
        {
            sbs.add(turn, turn.operations[i], turn_index, i, geometry1, geometry2, first);
            first = false;
        }
    }
    return true;
}

template
<
    bool Reverse1, bool Reverse2,
    overlay_type OverlayType,
    typename Turns,
    typename Clusters,
    typename Geometry1,
    typename Geometry2,
    typename SideStrategy
>
inline void gather_cluster_properties(Clusters& clusters, Turns& turns,
        operation_type for_operation,
        Geometry1 const& geometry1, Geometry2 const& geometry2,
        SideStrategy const& strategy)
{
    typedef typename boost::range_value<Turns>::type turn_type;
    typedef typename turn_type::point_type point_type;
    typedef typename turn_type::turn_operation_type turn_operation_type;

    // Define sorter, sorting counter-clockwise such that polygons are on the
    // right side
    typedef sort_by_side::side_sorter
        <
            Reverse1, Reverse2, OverlayType, point_type, SideStrategy, std::less<int>
        > sbs_type;

    for (typename Clusters::iterator mit = clusters.begin();
         mit != clusters.end(); ++mit)
    {
        cluster_info& cinfo = mit->second;

        sbs_type sbs(strategy);
        point_type turn_point; // should be all the same for all turns in cluster
        if (! fill_sbs(sbs, turn_point, cinfo, turns, geometry1, geometry2))
        {
            continue;
        }

        sbs.apply(turn_point);

        sbs.find_open();
        sbs.assign_zones(for_operation);

        cinfo.open_count = sbs.open_count(for_operation);

        bool const set_startable = OverlayType != overlay_dissolve;

        // Unset the startable flag for all 'closed' zones. This does not
        // apply for self-turns, because those counts are not from both
        // polygons
        for (std::size_t i = 0; i < sbs.m_ranked_points.size(); i++)
        {
            typename sbs_type::rp const& ranked = sbs.m_ranked_points[i];
            turn_type& turn = turns[ranked.turn_index];
            turn_operation_type& op = turn.operations[ranked.operation_index];

            if (set_startable
                    && for_operation == operation_union && cinfo.open_count == 0)
            {
                op.enriched.startable = false;
            }

            if (ranked.direction != sort_by_side::dir_to)
            {
                continue;
            }

            op.enriched.count_left = ranked.count_left;
            op.enriched.count_right = ranked.count_right;
            op.enriched.rank = ranked.rank;
            op.enriched.zone = ranked.zone;

            if (! set_startable)
            {
                continue;
            }

            if (BOOST_GEOMETRY_CONDITION(OverlayType != overlay_difference)
                    && is_self_turn<OverlayType>(turn))
            {
                // Difference needs the self-turns, TODO: investigate
                continue;
            }

            if ((for_operation == operation_union
                    && ranked.count_left != 0)
             || (for_operation == operation_intersection
                    && ranked.count_right != 2))
            {
                op.enriched.startable = false;
            }
        }

    }
}


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_HANDLE_COLOCATIONS_HPP
