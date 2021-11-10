// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ASSIGN_PARENTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ASSIGN_PARENTS_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_ring.hpp>
#include <boost/geometry/algorithms/detail/overlay/range_in_geometry.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>

#include <boost/geometry/geometries/box.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{



template
<
    typename Item,
    typename InnerGeometry,
    typename Geometry1, typename Geometry2,
    typename RingCollection,
    typename Strategy
>
static inline bool within_selected_input(Item const& item2,
        InnerGeometry const& inner_geometry,
        ring_identifier const& outer_id,
        Geometry1 const& geometry1, Geometry2 const& geometry2,
        RingCollection const& collection,
        Strategy const& strategy)
{
    typedef typename geometry::tag<Geometry1>::type tag1;
    typedef typename geometry::tag<Geometry2>::type tag2;

    // NOTE: range_in_geometry first checks the item2.point and then
    // if this point is on boundary it checks points of inner_geometry
    // ring until a point inside/outside other geometry ring is found
    switch (outer_id.source_index)
    {
        // covered_by
        case 0 :
            return range_in_geometry(item2.point, inner_geometry,
                get_ring<tag1>::apply(outer_id, geometry1), strategy) >= 0;
        case 1 :
            return range_in_geometry(item2.point, inner_geometry,
                get_ring<tag2>::apply(outer_id, geometry2), strategy) >= 0;
        case 2 :
            return range_in_geometry(item2.point, inner_geometry,
                get_ring<void>::apply(outer_id, collection), strategy) >= 0;
    }
    return false;
}

template
<
    typename Item,
    typename Geometry1, typename Geometry2,
    typename RingCollection,
    typename Strategy
>
static inline bool within_selected_input(Item const& item2,
        ring_identifier const& inner_id, ring_identifier const& outer_id,
        Geometry1 const& geometry1, Geometry2 const& geometry2,
        RingCollection const& collection,
        Strategy const& strategy)
{
    typedef typename geometry::tag<Geometry1>::type tag1;
    typedef typename geometry::tag<Geometry2>::type tag2;

    switch (inner_id.source_index)
    {
        case 0 :
            return within_selected_input(item2,
               get_ring<tag1>::apply(inner_id, geometry1),
               outer_id, geometry1, geometry2, collection, strategy);
        case 1 :
            return within_selected_input(item2,
               get_ring<tag2>::apply(inner_id, geometry2),
               outer_id, geometry1, geometry2, collection, strategy);
        case 2 :
            return within_selected_input(item2,
                get_ring<void>::apply(inner_id, collection),
                outer_id, geometry1, geometry2, collection, strategy);
    }
    return false;
}


template <typename Point, typename AreaType>
struct ring_info_helper
{
    ring_identifier id;
    AreaType real_area;
    AreaType abs_area;
    model::box<Point> envelope;

    inline ring_info_helper()
        : real_area(0), abs_area(0)
    {}

    inline ring_info_helper(ring_identifier i, AreaType const& a)
        : id(i), real_area(a), abs_area(geometry::math::abs(a))
    {}
};


template <typename Strategy>
struct ring_info_helper_get_box
{
    ring_info_helper_get_box(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Box, typename InputItem>
    inline void apply(Box& total, InputItem const& item) const
    {
        assert_coordinate_type_equal(total, item.envelope);
        geometry::expand(total, item.envelope, m_strategy);
    }

    Strategy const& m_strategy;
};

template <typename Strategy>
struct ring_info_helper_overlaps_box
{
    ring_info_helper_overlaps_box(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Box, typename InputItem>
    inline bool apply(Box const& box, InputItem const& item) const
    {
        assert_coordinate_type_equal(box, item.envelope);
        return ! geometry::detail::disjoint::disjoint_box_box(
                    box, item.envelope, m_strategy);
    }

    Strategy const& m_strategy;
};

// Segments intersection Strategy
template
<
    typename Geometry1,
    typename Geometry2,
    typename Collection,
    typename RingMap,
    typename Strategy
>
struct assign_visitor
{
    typedef typename RingMap::mapped_type ring_info_type;

    Geometry1 const& m_geometry1;
    Geometry2 const& m_geometry2;
    Collection const& m_collection;
    RingMap& m_ring_map;
    Strategy const& m_strategy;
    bool m_check_for_orientation;

    inline assign_visitor(Geometry1 const& g1, Geometry2 const& g2, Collection const& c,
                          RingMap& map, Strategy const& strategy, bool check)
        : m_geometry1(g1)
        , m_geometry2(g2)
        , m_collection(c)
        , m_ring_map(map)
        , m_strategy(strategy)
        , m_check_for_orientation(check)
    {}

    template <typename Item>
    inline bool apply(Item const& outer, Item const& inner, bool first = true)
    {
        if (first && outer.abs_area < inner.abs_area)
        {
            // Apply with reversed arguments
            apply(inner, outer, false);
            return true;
        }

        if (m_check_for_orientation
         || (math::larger(outer.real_area, 0)
          && math::smaller(inner.real_area, 0)))
        {
            ring_info_type& inner_in_map = m_ring_map[inner.id];

            if (geometry::covered_by(inner_in_map.point, outer.envelope, m_strategy)
               && within_selected_input(inner_in_map, inner.id, outer.id,
                                        m_geometry1, m_geometry2, m_collection,
                                        m_strategy)
               )
            {
                // Assign a parent if there was no earlier parent, or the newly
                // found parent is smaller than the previous one
                if (inner_in_map.parent.source_index == -1
                    || outer.abs_area < inner_in_map.parent_area)
                {
                    inner_in_map.parent = outer.id;
                    inner_in_map.parent_area = outer.abs_area;
                }
            }
        }

        return true;
    }
};


template
<
    overlay_type OverlayType,
    typename Geometry1, typename Geometry2,
    typename RingCollection,
    typename RingMap,
    typename Strategy
>
inline void assign_parents(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            RingCollection const& collection,
            RingMap& ring_map,
            Strategy const& strategy)
{
    static bool const is_difference = OverlayType == overlay_difference;
    static bool const is_buffer = OverlayType == overlay_buffer;
    static bool const is_dissolve = OverlayType == overlay_dissolve;
    static bool const check_for_orientation = is_buffer || is_dissolve;

    typedef typename geometry::tag<Geometry1>::type tag1;
    typedef typename geometry::tag<Geometry2>::type tag2;

    typedef typename RingMap::mapped_type ring_info_type;
    typedef typename ring_info_type::point_type point_type;
    typedef model::box<point_type> box_type;
    typedef typename geometry::area_result
        <
            point_type, Strategy // TODO: point_type is technically incorrect
        >::type area_result_type;

    typedef typename RingMap::iterator map_iterator_type;

    {
        typedef ring_info_helper<point_type, area_result_type> helper;
        typedef std::vector<helper> vector_type;
        typedef typename boost::range_iterator<vector_type const>::type vector_iterator_type;

        std::size_t count_total = ring_map.size();
        std::size_t count_positive = 0;
        std::size_t index_positive = 0; // only used if count_positive>0
        std::size_t index = 0;

        // Copy to vector (with new approach this might be obsolete as well, using the map directly)
        vector_type vector(count_total);

        for (map_iterator_type it = boost::begin(ring_map);
            it != boost::end(ring_map); ++it, ++index)
        {
            vector[index] = helper(it->first, it->second.get_area());
            helper& item = vector[index];
            switch(it->first.source_index)
            {
                case 0 :
                    geometry::envelope(get_ring<tag1>::apply(it->first, geometry1),
                                       item.envelope, strategy);
                    break;
                case 1 :
                    geometry::envelope(get_ring<tag2>::apply(it->first, geometry2),
                                       item.envelope, strategy);
                    break;
                case 2 :
                    geometry::envelope(get_ring<void>::apply(it->first, collection),
                                       item.envelope, strategy);
                    break;
            }

            // Expand envelope slightly
            expand_by_epsilon(item.envelope);

            if (item.real_area > 0)
            {
                count_positive++;
                index_positive = index;
            }
        }

        if (! check_for_orientation)
        {
            if (count_positive == count_total)
            {
                // Optimization for only positive rings
                // -> no assignment of parents or reversal necessary, ready here.
                return;
            }

            if (count_positive == 1 && ! is_difference && ! is_dissolve)
            {
                // Optimization for one outer ring
                // -> assign this as parent to all others (all interior rings)
                // In unions, this is probably the most occuring case and gives
                //    a dramatic improvement (factor 5 for star_comb testcase)
                // In difference or other cases where interior rings might be
                // located outside the outer ring, this cannot be done
                ring_identifier id_of_positive = vector[index_positive].id;
                ring_info_type& outer = ring_map[id_of_positive];
                index = 0;
                for (vector_iterator_type it = boost::begin(vector);
                    it != boost::end(vector); ++it, ++index)
                {
                    if (index != index_positive)
                    {
                        ring_info_type& inner = ring_map[it->id];
                        inner.parent = id_of_positive;
                        outer.children.push_back(it->id);
                    }
                }
                return;
            }
        }

        assign_visitor
            <
                Geometry1, Geometry2,
                RingCollection, RingMap,
                Strategy
            > visitor(geometry1, geometry2, collection, ring_map, strategy, check_for_orientation);

        geometry::partition
            <
                box_type
            >::apply(vector, visitor,
                     ring_info_helper_get_box<Strategy>(strategy),
                     ring_info_helper_overlaps_box<Strategy>(strategy));
    }

    if (check_for_orientation)
    {
        for (map_iterator_type it = boost::begin(ring_map);
            it != boost::end(ring_map); ++it)
        {
            ring_info_type& info = it->second;
            if (geometry::math::equals(info.get_area(), 0))
            {
                info.discarded = true;
            }
            else if (info.parent.source_index >= 0)
            {
                const ring_info_type& parent = ring_map[info.parent];
                bool const pos = math::larger(info.get_area(), 0);
                bool const parent_pos = math::larger(parent.area, 0);

                bool const double_neg = is_dissolve && ! pos && ! parent_pos;

                if ((pos && parent_pos) || double_neg)
                {
                    // Discard positive inner ring with positive parent
                    // Also, for some cases (dissolve), negative inner ring
                    // with negative parent should be discarded
                    info.discarded = true;
                }

                if (pos || info.discarded)
                {
                    // Remove parent ID from any positive or discarded inner rings
                    info.parent.source_index = -1;
                }
            }
            else if (info.parent.source_index < 0
                    && math::smaller(info.get_area(), 0))
            {
                // Reverse negative ring without parent
                info.reversed = true;
            }
        }
    }

    // Assign childlist
    for (map_iterator_type it = boost::begin(ring_map);
        it != boost::end(ring_map); ++it)
    {
        if (it->second.parent.source_index >= 0)
        {
            ring_map[it->second.parent].children.push_back(it->first);
        }
    }
}


// Version for one geometry (called by buffer/dissolve)
template
<
    overlay_type OverlayType,
    typename Geometry,
    typename RingCollection,
    typename RingMap,
    typename Strategy
>
inline void assign_parents(Geometry const& geometry,
            RingCollection const& collection,
            RingMap& ring_map,
            Strategy const& strategy)
{
    // Call it with an empty geometry as second geometry (source_id == 1)
    // (ring_map should be empty for source_id==1)
    Geometry empty;
    assign_parents<OverlayType>(geometry, empty,
            collection, ring_map, strategy);
}


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ASSIGN_PARENTS_HPP
