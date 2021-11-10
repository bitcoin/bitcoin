// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2014-2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURNS_HPP


#include <cstddef>
#include <map>

#include <boost/array.hpp>
#include <boost/concept_check.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/disjoint/box_box.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_point.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info_ll.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info_la.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>
#include <boost/geometry/algorithms/detail/recalculate.hpp>
#include <boost/geometry/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/algorithms/detail/sections/section_box_policies.hpp>
#include <boost/geometry/algorithms/detail/sections/section_functions.hpp>
#include <boost/geometry/algorithms/detail/sections/sectionalize.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/iterators/ever_circling_iterator.hpp>

#include <boost/geometry/strategies/intersection_strategies.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/type_traits.hpp>

#include <boost/geometry/views/detail/closed_clockwise_view.hpp>


#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
#  include <sstream>
#  include <boost/geometry/io/dsv/write.hpp>
#endif


namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace get_turns
{


struct no_interrupt_policy
{
    static bool const enabled = false;

    // variable required by self_get_turn_points::get_turns
    static bool const has_intersections = false;

    template <typename Range>
    static inline bool apply(Range const&)
    {
        return false;
    }
};

template
<
    bool IsAreal,
    typename Section,
    typename Point,
    typename CircularIterator,
    typename Strategy,
    typename RobustPolicy
>
struct unique_sub_range_from_section
{
    typedef Point point_type;

    unique_sub_range_from_section(Section const& section, signed_size_type index,
                          CircularIterator circular_iterator,
                          Point const& previous, Point const& current,
                          Strategy const& strategy,
                          RobustPolicy const& robust_policy)
        : m_section(section)
        , m_index(index)
        , m_previous_point(previous)
        , m_current_point(current)
        , m_circular_iterator(circular_iterator)
        , m_point_retrieved(false)
        , m_strategy(strategy)
        , m_robust_policy(robust_policy)
    {}

    inline bool is_first_segment() const
    {
        return !IsAreal && m_section.is_non_duplicate_first && m_index == m_section.begin_index;
    }
    inline bool is_last_segment() const
    {
        return size() == 2u;
    }

    inline std::size_t size() const
    {
        return IsAreal ? 3
            : m_section.is_non_duplicate_last && m_index + 1 >= m_section.end_index ? 2 : 3;
    }

    inline Point const& at(std::size_t index) const
    {
        BOOST_GEOMETRY_ASSERT(index < size());
        switch (index)
        {
            case 0 : return m_previous_point;
            case 1 : return m_current_point;
            case 2 : return get_next_point();
            default : return m_previous_point;
        }
    }

private :
    inline Point const& get_next_point() const
    {
        if (! m_point_retrieved)
        {
            advance_to_non_duplicate_next(m_current_point, m_circular_iterator);
            m_point = *m_circular_iterator;
            m_point_retrieved = true;
        }
        return m_point;
    }

    inline void advance_to_non_duplicate_next(Point const& current, CircularIterator& circular_iterator) const
    {
        typedef typename robust_point_type<Point, RobustPolicy>::type robust_point_type;
        robust_point_type current_robust_point;
        robust_point_type next_robust_point;
        geometry::recalculate(current_robust_point, current, m_robust_policy);
        geometry::recalculate(next_robust_point, *circular_iterator, m_robust_policy);

        // To see where the next segments bend to, in case of touch/intersections
        // on end points, we need (in case of degenerate/duplicate points) an extra
        // iterator which moves to the REAL next point, so non duplicate.
        // This needs an extra comparison (disjoint).
        // (Note that within sections, non duplicate points are already asserted,
        //   by the sectionalize process).

        // So advance to the "non duplicate next"
        // (the check is defensive, to avoid endless loops)
        std::size_t check = 0;
        while (! detail::disjoint::disjoint_point_point(
                    current_robust_point, next_robust_point, m_strategy)
               && check++ < m_section.range_count)
        {
            circular_iterator++;
            geometry::recalculate(next_robust_point, *circular_iterator, m_robust_policy);
        }
    }

    Section const& m_section;
    signed_size_type m_index;
    Point const& m_previous_point;
    Point const& m_current_point;
    mutable CircularIterator m_circular_iterator;
    mutable Point m_point;
    mutable bool m_point_retrieved;
    Strategy m_strategy;
    RobustPolicy m_robust_policy;
};

template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename Section1, typename Section2,
    typename TurnPolicy
>
class get_turns_in_sections
{
    using range1_view = detail::closed_clockwise_view
        <
            typename ring_type<Geometry1>::type const,
            geometry::closure<Geometry1>::value,
            Reverse1 ? counterclockwise : clockwise
        >;
    using range2_view = detail::closed_clockwise_view
        <
            typename ring_type<Geometry2>::type const,
            geometry::closure<Geometry2>::value,
            Reverse2 ? counterclockwise : clockwise
        >;

    using range1_iterator = typename boost::range_iterator<range1_view const>::type;
    using range2_iterator = typename boost::range_iterator<range2_view const>::type;

    using circular1_iterator = ever_circling_iterator<range1_iterator>;
    using circular2_iterator = ever_circling_iterator<range2_iterator>;

    template <typename Geometry, typename Section>
    static inline bool adjacent(Section const& section,
            signed_size_type index1, signed_size_type index2)
    {
        // About n-2:
        //   (square: range_count=5, indices 0,1,2,3
        //    -> 0-3 are adjacent, don't check on intersections)
        // Also tested for open polygons, and/or duplicates
        // About first condition: will be optimized by compiler (static)
        // It checks if it is areal (box, ring, (multi)polygon)
        signed_size_type const n = static_cast<signed_size_type>(section.range_count);

        boost::ignore_unused(n, index1, index2);

        return util::is_areal<Geometry>::value
               && index1 == 0
               && index2 >= n - 2
                ;
    }


public :
    // Returns true if terminated, false if interrupted
    template <typename Strategy, typename RobustPolicy, typename Turns, typename InterruptPolicy>
    static inline bool apply(
            int source_id1, Geometry1 const& geometry1, Section1 const& sec1,
            int source_id2, Geometry2 const& geometry2, Section2 const& sec2,
            bool skip_larger, bool skip_adjacent,
            Strategy const& strategy,
            RobustPolicy const& robust_policy,
            Turns& turns,
            InterruptPolicy& interrupt_policy)
    {
        boost::ignore_unused(interrupt_policy);

        static bool const areal1 = util::is_areal<Geometry1>::value;
        static bool const areal2 = util::is_areal<Geometry2>::value;

        if ((sec1.duplicate && (sec1.count + 1) < sec1.range_count)
           || (sec2.duplicate && (sec2.count + 1) < sec2.range_count))
        {
            // Skip sections containig only duplicates.
            // They are still important (can indicate non-disjointness)
            // but they will be found processing adjacent sections.
            // Do NOT skip if they are the ONLY section
            return true;
        }

        range1_view const view1(range_by_section(geometry1, sec1));
        range2_view const view2(range_by_section(geometry2, sec2));

        range1_iterator begin_range_1 = boost::begin(view1);
        range1_iterator end_range_1 = boost::end(view1);

        range2_iterator begin_range_2 = boost::begin(view2);
        range2_iterator end_range_2 = boost::end(view2);

        int const dir1 = sec1.directions[0];
        int const dir2 = sec2.directions[0];
        signed_size_type index1 = sec1.begin_index;
        signed_size_type ndi1 = sec1.non_duplicate_index;

        range1_iterator prev1, it1, end1;

        get_start_point_iterator(sec1, view1, prev1, it1, end1,
                    index1, ndi1, dir1, sec2.bounding_box, robust_policy);

        // We need a circular iterator because it might run through the closing point.
        // One circle is actually enough but this one is just convenient.
        circular1_iterator next1(begin_range_1, end_range_1, it1, true);
        next1++;

        // Walk through section and stop if we exceed the other box
        // section 2:    [--------------]
        // section 1: |----|---|---|---|---|
        for (prev1 = it1++, next1++;
            it1 != end1 && ! detail::section::exceeding<0>(dir1, *prev1, sec1.bounding_box, sec2.bounding_box, robust_policy);
            ++prev1, ++it1, ++index1, ++next1, ++ndi1)
        {
            unique_sub_range_from_section
                <
                    areal1, Section1, point1_type, circular1_iterator,
                    Strategy, RobustPolicy
                > unique_sub_range1(sec1, index1,
                                    circular1_iterator(begin_range_1, end_range_1, next1, true),
                                    *prev1, *it1,
                                    strategy, robust_policy);

            signed_size_type index2 = sec2.begin_index;
            signed_size_type ndi2 = sec2.non_duplicate_index;

            range2_iterator prev2, it2, end2;

            get_start_point_iterator(sec2, view2, prev2, it2, end2,
                        index2, ndi2, dir2, sec1.bounding_box, robust_policy);
            circular2_iterator next2(begin_range_2, end_range_2, it2, true);
            next2++;

            for (prev2 = it2++, next2++;
                it2 != end2 && ! detail::section::exceeding<0>(dir2, *prev2, sec2.bounding_box, sec1.bounding_box, robust_policy);
                ++prev2, ++it2, ++index2, ++next2, ++ndi2)
            {
                bool skip = false;

                if (source_id1 == source_id2
                        && sec1.ring_id.multi_index == sec2.ring_id.multi_index
                        && sec1.ring_id.ring_index == sec2.ring_id.ring_index)
                {
                    // Sources and rings are the same

                    if (skip_larger && index1 >= index2)
                    {
                        // Skip to avoid getting all intersections twice
                        skip = true;
                    }
                    else if (skip_adjacent)
                    {
                        // In some cases (dissolve, has_self_intersections)
                        // neighbouring segments should be checked
                        // (for example to detect spikes properly)

                        // skip if it is a neighbouring segment.
                        // (including, for areas, first-last segment
                        //  and two segments with one or more degenerate/duplicate
                        //  (zero-length) segments in between)
                        skip = ndi2 == ndi1 + 1
                            || adjacent<Geometry1>(sec1, index1, index2);
                    }
                }

                if (! skip)
                {
                    unique_sub_range_from_section
                        <
                            areal2, Section2, point2_type, circular2_iterator,
                            Strategy, RobustPolicy
                        > unique_sub_range2(sec2, index2,
                                            circular2_iterator(begin_range_2, end_range_2, next2),
                                            *prev2, *it2,
                                            strategy, robust_policy);

                    typedef typename boost::range_value<Turns>::type turn_info;

                    turn_info ti;
                    ti.operations[0].seg_id
                        = segment_identifier(source_id1, sec1.ring_id.multi_index,
                                             sec1.ring_id.ring_index, index1);
                    ti.operations[1].seg_id
                        = segment_identifier(source_id2, sec2.ring_id.multi_index,
                                             sec2.ring_id.ring_index, index2);

                    std::size_t const size_before = boost::size(turns);

                    TurnPolicy::apply(unique_sub_range1, unique_sub_range2,
                                      ti, strategy, robust_policy,
                                      std::back_inserter(turns));

                    if (InterruptPolicy::enabled)
                    {
                        if (interrupt_policy.apply(
                                std::make_pair(range::pos(turns, size_before),
                                               boost::end(turns))))
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }


private :
    typedef typename geometry::point_type<Geometry1>::type point1_type;
    typedef typename geometry::point_type<Geometry2>::type point2_type;

    // It is NOT possible to have section-iterators here
    // because of the logistics of "index" (the section-iterator automatically
    // skips to the begin-point, we loose the index or have to recalculate it)
    // So we mimic it here
    template <typename Range, typename Section, typename Box, typename RobustPolicy>
    static inline void get_start_point_iterator(Section const& section,
            Range const& range,
            typename boost::range_iterator<Range const>::type& it,
            typename boost::range_iterator<Range const>::type& prev,
            typename boost::range_iterator<Range const>::type& end,
            signed_size_type& index, signed_size_type& ndi,
            int dir, Box const& other_bounding_box, RobustPolicy const& robust_policy)
    {
        it = boost::begin(range) + section.begin_index;
        end = boost::begin(range) + section.end_index + 1;

        // Mimic section-iterator:
        // Skip to point such that section interects other box
        prev = it++;
        for(; it != end && detail::section::preceding<0>(dir, *it, section.bounding_box, other_bounding_box, robust_policy);
            prev = it++, index++, ndi++)
        {}
        // Go back one step because we want to start completely preceding
        it = prev;
    }
};

template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename TurnPolicy,
    typename Strategy,
    typename RobustPolicy,
    typename Turns,
    typename InterruptPolicy
>
struct section_visitor
{
    int m_source_id1;
    Geometry1 const& m_geometry1;
    int m_source_id2;
    Geometry2 const& m_geometry2;
    Strategy const& m_strategy;
    RobustPolicy const& m_rescale_policy;
    Turns& m_turns;
    InterruptPolicy& m_interrupt_policy;

    section_visitor(int id1, Geometry1 const& g1,
                    int id2, Geometry2 const& g2,
                    Strategy const& strategy,
                    RobustPolicy const& robust_policy,
                    Turns& turns,
                    InterruptPolicy& ip)
        : m_source_id1(id1), m_geometry1(g1)
        , m_source_id2(id2), m_geometry2(g2)
        , m_strategy(strategy)
        , m_rescale_policy(robust_policy)
        , m_turns(turns)
        , m_interrupt_policy(ip)
    {}

    template <typename Section>
    inline bool apply(Section const& sec1, Section const& sec2)
    {
        if (! detail::disjoint::disjoint_box_box(sec1.bounding_box,
                                                 sec2.bounding_box,
                                                 m_strategy) )
        {
            // false if interrupted
            return get_turns_in_sections
                    <
                        Geometry1,
                        Geometry2,
                        Reverse1, Reverse2,
                        Section, Section,
                        TurnPolicy
                    >::apply(m_source_id1, m_geometry1, sec1,
                             m_source_id2, m_geometry2, sec2,
                             false, false,
                             m_strategy,
                             m_rescale_policy,
                             m_turns, m_interrupt_policy);
        }
        return true;
    }

};

template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename TurnPolicy
>
class get_turns_generic
{

public:
    template <typename Strategy, typename RobustPolicy, typename Turns, typename InterruptPolicy>
    static inline void apply(
            int source_id1, Geometry1 const& geometry1,
            int source_id2, Geometry2 const& geometry2,
            Strategy const& strategy,
            RobustPolicy const& robust_policy,
            Turns& turns,
            InterruptPolicy& interrupt_policy)
    {
        // First create monotonic sections...
        typedef typename boost::range_value<Turns>::type ip_type;
        typedef typename ip_type::point_type point_type;

        typedef model::box
            <
                typename geometry::robust_point_type
                <
                    point_type, RobustPolicy
                >::type
            > box_type;
        typedef geometry::sections<box_type, 2> sections_type;

        sections_type sec1, sec2;
        typedef std::integer_sequence<std::size_t, 0, 1> dimensions;

        geometry::sectionalize<Reverse1, dimensions>(geometry1, robust_policy,
                                                     sec1, strategy, 0);
        geometry::sectionalize<Reverse2, dimensions>(geometry2, robust_policy,
                                                     sec2, strategy, 1);

        // ... and then partition them, intersecting overlapping sections in visitor method
        section_visitor
            <
                Geometry1, Geometry2,
                Reverse1, Reverse2,
                TurnPolicy,
                Strategy, RobustPolicy,
                Turns, InterruptPolicy
            > visitor(source_id1, geometry1, source_id2, geometry2,
                      strategy, robust_policy, turns, interrupt_policy);

        geometry::partition
            <
                box_type
            >::apply(sec1, sec2, visitor,
                     detail::section::get_section_box<Strategy>(strategy),
                     detail::section::overlaps_section_box<Strategy>(strategy));
    }
};


// Get turns for a range with a box, following Cohen-Sutherland (cs) approach
template
<
    typename Range, typename Box,
    bool ReverseRange, bool ReverseBox,
    typename TurnPolicy
>
struct get_turns_cs
{
    typedef typename geometry::point_type<Range>::type range_point_type;
    typedef typename geometry::point_type<Box>::type box_point_type;
    typedef boost::array<box_point_type, 4> box_array;

    using view_type = detail::closed_clockwise_view
        <
            Range const,
            geometry::closure<Range>::value,
            ReverseRange ? counterclockwise : clockwise
        >;

    using iterator_type = typename boost::range_iterator<view_type const>::type;

    struct unique_sub_range_from_box_policy
    {
        typedef box_point_type point_type;

        unique_sub_range_from_box_policy(box_array const& box)
          : m_box(box)
          , m_index(0)
        {}

        static inline bool is_first_segment() { return false; }
        static inline bool is_last_segment() { return false; }
        static inline std::size_t size() { return 4; }

        inline box_point_type const& at(std::size_t index) const
        {
            BOOST_GEOMETRY_ASSERT(index < size());
            return m_box[(m_index + index) % 4];
        }

        inline void next()
        {
            m_index++;
        }

    private :
        box_array const& m_box;
        std::size_t m_index;
    };

    struct unique_sub_range_from_view_policy
    {
        typedef range_point_type point_type;

        unique_sub_range_from_view_policy(view_type const& view, point_type const& pi, point_type const& pj, iterator_type it)
          : m_view(view)
          , m_pi(pi)
          , m_pj(pj)
          , m_circular_iterator(boost::begin(view), boost::end(view), it, true)
        {
            ++m_circular_iterator;
        }

        static inline bool is_first_segment() { return false; }
        static inline bool is_last_segment() { return false; }
        static inline std::size_t size() { return 3; }

        inline point_type const& at(std::size_t index) const
        {
            BOOST_GEOMETRY_ASSERT(index < size());
            switch (index)
            {
                case 0 : return m_pi;
                case 1 : return m_pj;
                case 2 : return *m_circular_iterator;
                default : return m_pi;
            }
        }

    private :
        view_type const& m_view;
        point_type const& m_pi;
        point_type const& m_pj;
        ever_circling_iterator<iterator_type> m_circular_iterator;
    };

    template <typename IntersectionStrategy, typename RobustPolicy, typename Turns, typename InterruptPolicy>
    static inline void apply(
                int source_id1, Range const& range,
                int source_id2, Box const& box,
                IntersectionStrategy const& intersection_strategy,
                RobustPolicy const& robust_policy,
                Turns& turns,
                InterruptPolicy& interrupt_policy,
                signed_size_type multi_index = -1,
                signed_size_type ring_index = -1)
    {
        if ( boost::size(range) <= 1)
        {
            return;
        }

        box_array box_points;
        assign_box_corners_oriented<ReverseBox>(box, box_points);

        view_type const view(range);

        // TODO: in this code, possible duplicate points are not yet taken
        // into account (not in the iterator, nor in the retrieve policy)
        iterator_type it = boost::begin(view);

        //bool first = true;

        //char previous_side[2] = {0, 0};

        signed_size_type index = 0;

        for (iterator_type prev = it++;
            it != boost::end(view);
            prev = it++, index++)
        {
            segment_identifier seg_id(source_id1,
                        multi_index, ring_index, index);

            unique_sub_range_from_view_policy view_unique_sub_range(view, *prev, *it, it);

            /*if (first)
            {
                previous_side[0] = get_side<0>(box, *prev);
                previous_side[1] = get_side<1>(box, *prev);
            }

            char current_side[2];
            current_side[0] = get_side<0>(box, *it);
            current_side[1] = get_side<1>(box, *it);

            // There can NOT be intersections if
            // 1) EITHER the two points are lying on one side of the box (! 0 && the same)
            // 2) OR same in Y-direction
            // 3) OR all points are inside the box (0)
            if (! (
                (current_side[0] != 0 && current_side[0] == previous_side[0])
                || (current_side[1] != 0 && current_side[1] == previous_side[1])
                || (current_side[0] == 0
                        && current_side[1] == 0
                        && previous_side[0] == 0
                        && previous_side[1] == 0)
                  )
                )*/
            if (true)
            {
                get_turns_with_box(seg_id, source_id2,
                        view_unique_sub_range,
                        box_points,
                        intersection_strategy,
                        robust_policy,
                        turns,
                        interrupt_policy);
                // Future performance enhancement:
                // return if told by the interrupt policy
            }
        }
    }

private:
    template<std::size_t Index, typename Point>
    static inline int get_side(Box const& box, Point const& point)
    {
        // Inside -> 0
        // Outside -> -1 (left/below) or 1 (right/above)
        // On border -> -2 (left/lower) or 2 (right/upper)
        // The only purpose of the value is to not be the same,
        // and to denote if it is inside (0)

        typename coordinate_type<Point>::type const& c = get<Index>(point);
        typename coordinate_type<Box>::type const& left = get<min_corner, Index>(box);
        typename coordinate_type<Box>::type const& right = get<max_corner, Index>(box);

        if (geometry::math::equals(c, left)) return -2;
        else if (geometry::math::equals(c, right)) return 2;
        else if (c < left) return -1;
        else if (c > right) return 1;
        else return 0;
    }

    template
    <
        typename IntersectionStrategy,
        typename Turns,
        typename InterruptPolicy,
        typename RobustPolicy
    >
    static inline void get_turns_with_box(segment_identifier const& seg_id, int source_id2,
            unique_sub_range_from_view_policy const& range_unique_sub_range,
            box_array const& box,
            IntersectionStrategy const& intersection_strategy,
            RobustPolicy const& robust_policy,
            // Output
            Turns& turns,
            InterruptPolicy& interrupt_policy)
    {
        boost::ignore_unused(interrupt_policy);

        // Depending on code some relations can be left out

        typedef typename boost::range_value<Turns>::type turn_info;

        turn_info ti;
        ti.operations[0].seg_id = seg_id;

        unique_sub_range_from_box_policy box_unique_sub_range(box);
        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 0);
        TurnPolicy::apply(range_unique_sub_range, box_unique_sub_range,
                          ti, intersection_strategy, robust_policy,
                          std::back_inserter(turns));

        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 1);
        box_unique_sub_range.next();
        TurnPolicy::apply(range_unique_sub_range, box_unique_sub_range,
                          ti, intersection_strategy, robust_policy,
                          std::back_inserter(turns));

        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 2);
        box_unique_sub_range.next();
        TurnPolicy::apply(range_unique_sub_range, box_unique_sub_range,
                          ti, intersection_strategy, robust_policy,
                          std::back_inserter(turns));

        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 3);
        box_unique_sub_range.next();
        TurnPolicy::apply(range_unique_sub_range, box_unique_sub_range,
                          ti, intersection_strategy, robust_policy,
                          std::back_inserter(turns));

        if (InterruptPolicy::enabled)
        {
            interrupt_policy.apply(turns);
        }

    }

};


template
<
    typename Polygon, typename Box,
    bool Reverse, bool ReverseBox,
    typename TurnPolicy
>
struct get_turns_polygon_cs
{
    template <typename IntersectionStrategy, typename RobustPolicy, typename Turns, typename InterruptPolicy>
    static inline void apply(
            int source_id1, Polygon const& polygon,
            int source_id2, Box const& box,
            IntersectionStrategy const& intersection_strategy,
            RobustPolicy const& robust_policy,
            Turns& turns,
            InterruptPolicy& interrupt_policy,
            signed_size_type multi_index = -1)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        typedef detail::get_turns::get_turns_cs
            <
                ring_type, Box,
                Reverse, ReverseBox,
                TurnPolicy
            > intersector_type;

        intersector_type::apply(
                source_id1, geometry::exterior_ring(polygon),
                source_id2, box,
                intersection_strategy,
                robust_policy,
                turns,
                interrupt_policy,
                multi_index, -1);

        signed_size_type i = 0;

        typename interior_return_type<Polygon const>::type
            rings = interior_rings(polygon);
        for (typename detail::interior_iterator<Polygon const>::type
                it = boost::begin(rings); it != boost::end(rings); ++it, ++i)
        {
            intersector_type::apply(
                    source_id1, *it,
                    source_id2, box,
                    intersection_strategy,
                    robust_policy,
                    turns, interrupt_policy,
                    multi_index, i);
        }

    }
};


template
<
    typename Multi, typename Box,
    bool Reverse, bool ReverseBox,
    typename TurnPolicy
>
struct get_turns_multi_polygon_cs
{
    template <typename IntersectionStrategy, typename RobustPolicy, typename Turns, typename InterruptPolicy>
    static inline void apply(
            int source_id1, Multi const& multi,
            int source_id2, Box const& box,
            IntersectionStrategy const& intersection_strategy,
            RobustPolicy const& robust_policy,
            Turns& turns,
            InterruptPolicy& interrupt_policy)
    {
        typedef typename boost::range_iterator
            <
                Multi const
            >::type iterator_type;

        signed_size_type i = 0;
        for (iterator_type it = boost::begin(multi);
             it != boost::end(multi);
             ++it, ++i)
        {
            // Call its single version
            get_turns_polygon_cs
                <
                    typename boost::range_value<Multi>::type, Box,
                    Reverse, ReverseBox,
                    TurnPolicy
                >::apply(source_id1, *it, source_id2, box,
                         intersection_strategy, robust_policy,
                         turns, interrupt_policy, i);
        }
    }
};


// GET_TURN_INFO_TYPE

template <typename Geometry>
struct topological_tag_base
{
    typedef typename tag_cast<typename tag<Geometry>::type, pointlike_tag, linear_tag, areal_tag>::type type;
};

template <typename Geometry1, typename Geometry2, typename AssignPolicy,
          typename Tag1 = typename tag<Geometry1>::type, typename Tag2 = typename tag<Geometry2>::type,
          typename TagBase1 = typename topological_tag_base<Geometry1>::type, typename TagBase2 = typename topological_tag_base<Geometry2>::type>
struct get_turn_info_type
    : overlay::get_turn_info<AssignPolicy>
{};

template <typename Geometry1, typename Geometry2, typename AssignPolicy, typename Tag1, typename Tag2>
struct get_turn_info_type<Geometry1, Geometry2, AssignPolicy, Tag1, Tag2, linear_tag, linear_tag>
    : overlay::get_turn_info_linear_linear<AssignPolicy>
{};

template <typename Geometry1, typename Geometry2, typename AssignPolicy, typename Tag1, typename Tag2>
struct get_turn_info_type<Geometry1, Geometry2, AssignPolicy, Tag1, Tag2, linear_tag, areal_tag>
    : overlay::get_turn_info_linear_areal<AssignPolicy>
{};

template <typename Geometry1, typename Geometry2, typename SegmentRatio,
          typename Tag1 = typename tag<Geometry1>::type, typename Tag2 = typename tag<Geometry2>::type,
          typename TagBase1 = typename topological_tag_base<Geometry1>::type, typename TagBase2 = typename topological_tag_base<Geometry2>::type>
struct turn_operation_type
{
    typedef overlay::turn_operation<typename point_type<Geometry1>::type, SegmentRatio> type;
};

template <typename Geometry1, typename Geometry2, typename SegmentRatio, typename Tag1, typename Tag2>
struct turn_operation_type<Geometry1, Geometry2, SegmentRatio, Tag1, Tag2, linear_tag, linear_tag>
{
    typedef overlay::turn_operation_linear<typename point_type<Geometry1>::type, SegmentRatio> type;
};

template <typename Geometry1, typename Geometry2, typename SegmentRatio, typename Tag1, typename Tag2>
struct turn_operation_type<Geometry1, Geometry2, SegmentRatio, Tag1, Tag2, linear_tag, areal_tag>
{
    typedef overlay::turn_operation_linear<typename point_type<Geometry1>::type, SegmentRatio> type;
};

}} // namespace detail::get_turns
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// Because this is "detail" method, and most implementations will use "generic",
// we take the freedom to derive it from "generic".
template
<
    typename GeometryTag1, typename GeometryTag2,
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename TurnPolicy
>
struct get_turns
    : detail::get_turns::get_turns_generic
        <
            Geometry1, Geometry2,
            Reverse1, Reverse2,
            TurnPolicy
        >
{};


template
<
    typename Polygon, typename Box,
    bool ReversePolygon, bool ReverseBox,
    typename TurnPolicy
>
struct get_turns
    <
        polygon_tag, box_tag,
        Polygon, Box,
        ReversePolygon, ReverseBox,
        TurnPolicy
    > : detail::get_turns::get_turns_polygon_cs
            <
                Polygon, Box,
                ReversePolygon, ReverseBox,
                TurnPolicy
            >
{};


template
<
    typename Ring, typename Box,
    bool ReverseRing, bool ReverseBox,
    typename TurnPolicy
>
struct get_turns
    <
        ring_tag, box_tag,
        Ring, Box,
        ReverseRing, ReverseBox,
        TurnPolicy
    > : detail::get_turns::get_turns_cs
            <
                Ring, Box, ReverseRing, ReverseBox,
                TurnPolicy
            >

{};


template
<
    typename MultiPolygon,
    typename Box,
    bool ReverseMultiPolygon, bool ReverseBox,
    typename TurnPolicy
>
struct get_turns
    <
        multi_polygon_tag, box_tag,
        MultiPolygon, Box,
        ReverseMultiPolygon, ReverseBox,
        TurnPolicy
    >
    : detail::get_turns::get_turns_multi_polygon_cs
        <
            MultiPolygon, Box,
            ReverseMultiPolygon, ReverseBox,
            TurnPolicy
        >
{};


template
<
    typename GeometryTag1, typename GeometryTag2,
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename TurnPolicy
>
struct get_turns_reversed
{
    template <typename Strategy, typename RobustPolicy, typename Turns, typename InterruptPolicy>
    static inline void apply(int source_id1, Geometry1 const& g1,
                             int source_id2, Geometry2 const& g2,
                             Strategy const& strategy,
                             RobustPolicy const& robust_policy,
                             Turns& turns,
                             InterruptPolicy& interrupt_policy)
    {
        get_turns
            <
                GeometryTag2, GeometryTag1,
                Geometry2, Geometry1,
                Reverse2, Reverse1,
                TurnPolicy
            >::apply(source_id2, g2, source_id1, g1,
                     strategy, robust_policy,
                     turns, interrupt_policy);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



/*!
\brief \brief_calc2{turn points}
\ingroup overlay
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Turns type of turn-container (e.g. vector of "intersection/turn point"'s)
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param intersection_strategy segments intersection strategy
\param robust_policy policy to handle robustness issues
\param turns container which will contain turn points
\param interrupt_policy policy determining if process is stopped
    when intersection is found
 */
template
<
    bool Reverse1, bool Reverse2,
    typename AssignPolicy,
    typename Geometry1,
    typename Geometry2,
    typename Strategy,
    typename RobustPolicy,
    typename Turns,
    typename InterruptPolicy
>
inline void get_turns(Geometry1 const& geometry1,
                      Geometry2 const& geometry2,
                      Strategy const& strategy,
                      RobustPolicy const& robust_policy,
                      Turns& turns,
                      InterruptPolicy& interrupt_policy)
{
    concepts::check_concepts_and_equal_dimensions<Geometry1 const, Geometry2 const>();

    typedef detail::overlay::get_turn_info<AssignPolicy> TurnPolicy;
    //typedef detail::get_turns::get_turn_info_type<Geometry1, Geometry2, AssignPolicy> TurnPolicy;

    std::conditional_t
        <
            reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::get_turns_reversed
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                Reverse1, Reverse2,
                TurnPolicy
            >,
            dispatch::get_turns
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                Reverse1, Reverse2,
                TurnPolicy
            >
        >::apply(0, geometry1,
                 1, geometry2,
                 strategy,
                 robust_policy,
                 turns, interrupt_policy);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURNS_HPP
