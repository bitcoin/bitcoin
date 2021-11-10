// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2020 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_TURN_IN_RING_WINDING_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_TURN_IN_RING_WINDING_HPP

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/config.hpp>
#include <boost/geometry/algorithms/detail/make/make.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace buffer
{

#ifndef DOXYGEN_NO_DETAIL

enum place_on_ring_type
{
    // +----offsetted----> (offsetted is considered as outside)
    // |                 |
    // |                 |
    // left              right (first point outside, rest inside)
    // |                 |
    // |                 |
    // <-----original----+ (original is considered as inside)
    place_on_ring_offsetted,
    place_on_ring_original,
    place_on_ring_to_offsetted,
    place_on_ring_from_offsetted,
};

template <typename CalculationType>
class turn_in_ring_winding
{

    // Implements the winding rule.
    // Basic calculations (on a clockwise ring of 5 segments)
    // (as everywhere in BG, -1 = right, 0 = on segment, +1 = left)
    // +--------2--------+  // P : For 1/3, nothing happens, it returns
    // |                 |  //     For 2, side is right (-1), multiplier=2, -2
    // |        P        |  //     For 4, side is right (-1), multiplier=1, -1
    // 1                 3  //     For 5, side is right (-1), multiplier=1, -1, total -4
    // |             Q   |  // Q : For 2: -2, for 4: -2, total -4
    // |                 |  // R : For 2: -2, for 5: +2, total 0
    // +----5---*----4---+  // S : For 2: -1, 3: nothing, 4: +1, total 0
    //
    //     R             S
    //


public:

    struct counter
    {
        inline counter()
            : m_count(0)
            , m_min_distance(0)
            , m_close_to_offset(false)
        {}

        //! Returns -1 for outside, 1 for inside
        inline int code() const
        {
            return m_count == 0 ? -1 : 1;
        }

        //! Counter, is increased if point is left of a segment (outside),
        //! and decreased if point is right of a segment (inside)
        int m_count;

        //! Indicate an indication of distance. It is always set, unless
        //! the point is located on the border-part of the original.
        //! It is not guaranteed to be the minimum distance, because it is only
        //! calculated for a selection of the offsetted ring.
        CalculationType m_min_distance;
        bool m_close_to_offset;
    };

    typedef counter state_type;

    template <typename Point, typename PointOfSegment>
    static inline bool in_vertical_range(Point const& point,
                             PointOfSegment const& s1,
                             PointOfSegment const& s2)
    {
        CalculationType const py = get<1>(point);
        CalculationType const s1y = get<1>(s1);
        CalculationType const s2y = get<1>(s2);

        return (s1y >= py && s2y <= py)
            || (s2y >= py && s1y <= py);
    }

    template <typename Dm, typename Point, typename PointOfSegment>
    static inline void apply_on_boundary(Point const& point,
                             PointOfSegment const& s1,
                             PointOfSegment const& s2,
                             place_on_ring_type place_on_ring,
                             counter& the_state)
    {
        if (place_on_ring == place_on_ring_offsetted)
        {
            // Consider the point as "outside"
            the_state.m_count = 0;
            the_state.m_close_to_offset = true;
            the_state.m_min_distance = 0;
        }
        else if (place_on_ring == place_on_ring_to_offsetted
            || place_on_ring == place_on_ring_from_offsetted)
        {
            // Check distance from "point" to either s1 or s2
            // on a line perpendicular to s1-s2
            typedef model::infinite_line<CalculationType> line_type;

            line_type const line
                    = detail::make::make_perpendicular_line<CalculationType>(s1, s2,
                    place_on_ring == place_on_ring_to_offsetted ? s2 : s1);

            Dm perp;
            perp.measure = arithmetic::side_value(line, point);

            // If it is to the utmost point s1 or s2, it is "outside"
            the_state.m_count = perp.is_zero() ? 0 : 1;
            the_state.m_close_to_offset = true;
            the_state.m_min_distance = geometry::math::abs(perp.measure);
        }
        else
        {
            // It is on the border, the part of the original
            // Consider it as "inside".
            the_state.m_count = 1;
        }
    }

    template <typename Dm, typename Point, typename PointOfSegment>
    static inline bool apply(Point const& point,
                             PointOfSegment const& s1,
                             PointOfSegment const& s2,
                             Dm const& dm,
                             place_on_ring_type place_on_ring,
                             counter& the_state)
    {
        CalculationType const px = get<0>(point);
        CalculationType const s1x = get<0>(s1);
        CalculationType const s2x = get<0>(s2);

        bool const in_horizontal_range
                 = (s1x >= px && s2x <= px)
                || (s2x >= px && s1x <= px);

        bool const vertical = s1x == s2x;
        bool const measured_on_boundary = dm.is_zero();

        if (measured_on_boundary
            && (in_horizontal_range
                || (vertical && in_vertical_range(point, s1, s2))))
        {
            apply_on_boundary<Dm>(point, s1, s2, place_on_ring, the_state);
            // Indicate that no further processing is necessary.
            return false;
        }

        bool const is_on_right_side = dm.is_negative();

        if (place_on_ring == place_on_ring_offsetted
            && is_on_right_side
            && (! the_state.m_close_to_offset
                || -dm.measure < the_state.m_min_distance))
        {
            // This part of the ring was the offsetted part,
            // keep track of the distance WITHIN the ring
            // with respect to the offsetted part
            // NOTE: this is also done if it is NOT in the horizontal range.
            the_state.m_min_distance = -dm.measure;
            the_state.m_close_to_offset = true;
        }

        if (in_horizontal_range)
        {
            // Use only absolute comparisons, because the ring is continuous -
            // what was missed is there earlier or later, and turns should
            // not be counted twice (which can happen if an epsilon is used).
            bool const eq1 = s1x == px;
            bool const eq2 = s2x == px;

            // Account for  1 or  2 for left side (outside)
            //     and for -1 or -2 for right side (inside)
            int const side = is_on_right_side ? -1 : 1;
            int const multiplier = eq1 || eq2 ? 1 : 2;

            the_state.m_count += side * multiplier;
        }

        return true;
    }
};

#endif // DOXYGEN_NO_DETAIL

}} // namespace strategy::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_TURN_IN_RING_WINDING_HPP

