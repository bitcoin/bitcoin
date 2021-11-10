// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2020 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_PIECE_BORDER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_PIECE_BORDER_HPP


#include <boost/array.hpp>
#include <boost/core/addressof.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/config.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/detail/buffer/buffer_box.hpp>
#include <boost/geometry/algorithms/detail/buffer/buffer_policies.hpp>
#include <boost/geometry/algorithms/detail/expand_by_epsilon.hpp>
#include <boost/geometry/strategies/cartesian/turn_in_ring_winding.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/segment.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{
template <typename It, typename T, typename Compare>
inline bool get_range_around(It begin, It end, T const& value, Compare const& compare, It& lower, It& upper)
{
    lower = end;
    upper = end;

    // Get first element not smaller than value
    if (begin == end)
    {
        return false;
    }
    if (compare(value, *begin))
    {
        // The value is smaller than the first item, therefore not in range
        return false;
    }
    // *(begin + std::distance(begin, end) - 1))
    if (compare(*(end - 1), value))
    {
        // The last item is larger than the value, therefore not in range
        return false;
    }

    // Assign the iterators.
    // lower >= begin and lower < end
    // upper > lower and upper <= end
    // lower_bound points to first element NOT LESS than value - but because
    // we want the first value LESS than value, we decrease it
    lower = std::lower_bound(begin, end, value, compare);
    // upper_bound points to first element of which value is LESS
    upper = std::upper_bound(begin, end, value, compare);

    if (lower != begin)
    {
        --lower;
    }
    if (upper != end)
    {
        ++upper;
    }
    return true;
}

}


namespace detail { namespace buffer
{

//! Contains the border of the piece, consisting of 4 parts:
//! 1: the part of the offsetted ring (referenced, not copied)
//! 2: the part of the original (one or two points)
//! 3: the left part (from original to offsetted)
//! 4: the right part (from offsetted to original)
//! Besides that, it contains some properties of the piece(border);
//!   - convexity
//!   - envelope
//!   - monotonicity of the offsetted ring
//!   - min/max radius of a point buffer
//!   - if it is a "reversed" piece (linear features with partly negative buffers)
template <typename Ring, typename Point>
struct piece_border
{
    typedef typename geometry::coordinate_type<Point>::type coordinate_type;
    typedef typename default_comparable_distance_result<Point>::type radius_type;
    typedef typename geometry::strategy::buffer::turn_in_ring_winding<coordinate_type>::state_type state_type;

    bool m_reversed;

    // Points from the offsetted ring. They are not copied, this structure
    // refers to those points
    Ring const* m_ring;
    std::size_t m_begin;
    std::size_t m_end;

    // Points from the original (one or two, depending on piece shape)
    // Note, if there are 2 points, they are REVERSED w.r.t. the original
    // Therefore here we can walk in its order.
    boost::array<Point, 2> m_originals;
    std::size_t m_original_size;

    geometry::model::box<Point> m_envelope;
    bool m_has_envelope;

    // True if piece is determined as "convex"
    bool m_is_convex;

    // True if offsetted part is monotonically changing in x-direction
    bool m_is_monotonic_increasing;
    bool m_is_monotonic_decreasing;

    radius_type m_min_comparable_radius;
    radius_type m_max_comparable_radius;

    piece_border()
        : m_reversed(false)
        , m_ring(NULL)
        , m_begin(0)
        , m_end(0)
        , m_original_size(0)
        , m_has_envelope(false)
        , m_is_convex(false)
        , m_is_monotonic_increasing(false)
        , m_is_monotonic_decreasing(false)
        , m_min_comparable_radius(0)
        , m_max_comparable_radius(0)
    {
    }

    // Only used for debugging (SVG)
    Ring get_full_ring() const
    {
        Ring result;
        if (ring_or_original_empty())
        {
            return result;
        }
        std::copy(m_ring->begin() + m_begin,
                  m_ring->begin() + m_end,
                  std::back_inserter(result));
        std::copy(m_originals.begin(),
                  m_originals.begin() + m_original_size,
                  std::back_inserter(result));
        // Add the closing point
        result.push_back(*(m_ring->begin() + m_begin));

        return result;
    }

    template <typename Strategy>
    void get_properties_of_border(bool is_point_buffer, Point const& center,
                                  Strategy const& strategy)
    {
        m_has_envelope = calculate_envelope(m_envelope, strategy);
        if (m_has_envelope)
        {
            // Take roundings into account, enlarge box
            geometry::detail::expand_by_epsilon(m_envelope);
        }
        if (! ring_or_original_empty() && is_point_buffer)
        {
            // Determine min/max radius
            calculate_radii(center, m_ring->begin() + m_begin, m_ring->begin() + m_end);
        }
    }

    template <typename Strategy>
    void get_properties_of_offsetted_ring_part(Strategy const& strategy)
    {
        if (! ring_or_original_empty())
        {
            m_is_convex = is_convex(strategy);
            check_monotonicity(m_ring->begin() + m_begin, m_ring->begin() + m_end);
        }
    }

    void set_offsetted(Ring const& ring, std::size_t begin, std::size_t end)
    {
        BOOST_GEOMETRY_ASSERT(begin <= end);
        BOOST_GEOMETRY_ASSERT(begin < boost::size(ring));
        BOOST_GEOMETRY_ASSERT(end <= boost::size(ring));

        m_ring = boost::addressof(ring);
        m_begin = begin;
        m_end = end;
    }

    void add_original_point(Point const& point)
    {
        BOOST_GEOMETRY_ASSERT(m_original_size < 2);
        m_originals[m_original_size++] = point;
    }

    template <typename Box, typename Strategy>
    bool calculate_envelope(Box& envelope, Strategy const& strategy) const
    {
        geometry::assign_inverse(envelope);
        if (ring_or_original_empty())
        {
            return false;
        }
        expand_envelope(envelope, m_ring->begin() + m_begin, m_ring->begin() + m_end, strategy);
        expand_envelope(envelope, m_originals.begin(), m_originals.begin() + m_original_size, strategy);
        return true;
    }


    // Whatever the return value, the state should be checked.
    template <typename TurnPoint, typename State>
    bool point_on_piece(TurnPoint const& point,
                        bool one_sided, bool is_linear_end_point,
                        State& state) const
    {
        if (ring_or_original_empty())
        {
            return false;
        }

        // Walk over the different parts of the ring, in clockwise order
        // For performance reasons: start with the helper part (one segment)
        // then the original part (one segment, if any), then the other helper
        // part (one segment), and only then the offsetted part
        // (probably more segments, check monotonicity)
        geometry::strategy::buffer::turn_in_ring_winding<coordinate_type> tir;

        Point const offsetted_front = *(m_ring->begin() + m_begin);
        Point const offsetted_back = *(m_ring->begin() + m_end - 1);

        // For onesided buffers, or turns colocated with linear end points,
        // the place on the ring is changed to offsetted (because of colocation)
        geometry::strategy::buffer::place_on_ring_type const por_original
            = adapted_place_on_ring(geometry::strategy::buffer::place_on_ring_original,
                                    one_sided, is_linear_end_point);
        geometry::strategy::buffer::place_on_ring_type const por_from_offsetted
            = adapted_place_on_ring(geometry::strategy::buffer::place_on_ring_from_offsetted,
                                    one_sided, is_linear_end_point);
        geometry::strategy::buffer::place_on_ring_type const por_to_offsetted
            = adapted_place_on_ring(geometry::strategy::buffer::place_on_ring_to_offsetted,
                                    one_sided, is_linear_end_point);

        bool continue_processing = true;
        if (m_original_size == 1)
        {
            // One point. Walk from last offsetted to point, and from point to first offsetted
            continue_processing = step(point, offsetted_back, m_originals[0], tir, por_from_offsetted, state)
                && step(point, m_originals[0], offsetted_front, tir, por_to_offsetted, state);
        }
        else if (m_original_size == 2)
        {
            // Two original points. Walk from last offsetted point to first original point,
            // then along original, then from second oginal to first offsetted point
            continue_processing = step(point, offsetted_back, m_originals[0], tir, por_from_offsetted, state)
                    && step(point, m_originals[0], m_originals[1], tir, por_original, state)
                    && step(point, m_originals[1], offsetted_front, tir, por_to_offsetted, state);
        }

        if (continue_processing)
        {
            // Check the offsetted ring (in rounded joins, these might be
            // several segments)
            walk_offsetted(point, m_ring->begin() + m_begin, m_ring->begin() + m_end,
                           tir, state);
        }

        return true;
    }

    //! Returns true if empty (no ring, or no points, or no original)
    bool ring_or_original_empty() const
    {
        return m_ring == NULL || m_begin >= m_end || m_original_size == 0;
    }

private :

    static geometry::strategy::buffer::place_on_ring_type
        adapted_place_on_ring(geometry::strategy::buffer::place_on_ring_type target,
                              bool one_sided, bool is_linear_end_point)
    {
        return one_sided || is_linear_end_point
               ? geometry::strategy::buffer::place_on_ring_offsetted
               : target;
    }

    template <typename TurnPoint, typename Iterator, typename Strategy, typename State>
    bool walk_offsetted(TurnPoint const& point, Iterator begin, Iterator end, Strategy const & strategy, State& state) const
    {
        Iterator it = begin;
        Iterator beyond = end;

        // Move iterators if the offsetted ring is monotonic increasing or decreasing
        if (m_is_monotonic_increasing)
        {
            if (! get_range_around(begin, end, point, geometry::less<Point, 0>(), it, beyond))
            {
                return true;
            }
        }
        else if (m_is_monotonic_decreasing)
        {
            if (! get_range_around(begin, end, point, geometry::greater<Point, 0>(), it, beyond))
            {
                return true;
            }
        }

        for (Iterator previous = it++ ; it != beyond ; ++previous, ++it )
        {
            if (! step(point, *previous, *it, strategy,
                 geometry::strategy::buffer::place_on_ring_offsetted, state))
            {
                return false;
            }
        }
        return true;
    }

    template <typename TurnPoint, typename TiRStrategy, typename State>
    bool step(TurnPoint const& point, Point const& p1, Point const& p2, TiRStrategy const & strategy,
              geometry::strategy::buffer::place_on_ring_type place_on_ring, State& state) const
    {
        // A step between original/offsetted ring is always convex
        // (unless the join strategy generates points left of it -
        //  future: convexity might be added to the buffer-join-strategy)
        // Therefore, if the state count > 0, it means the point is left of it,
        // and because it is convex, we can stop

        typedef typename geometry::coordinate_type<Point>::type coordinate_type;
        typedef geometry::detail::distance_measure<coordinate_type> dm_type;
        dm_type const dm = geometry::detail::get_distance_measure(point, p1, p2);
        if (m_is_convex && dm.measure > 0)
        {
            // The point is left of this segment of a convex piece
            state.m_count = 0;
            return false;
        }
        // Call strategy, and if it is on the border, return false
        // to stop further processing.
        return strategy.apply(point, p1, p2, dm, place_on_ring, state);
    }

    template <typename It, typename Box, typename Strategy>
    void expand_envelope(Box& envelope, It begin, It end, Strategy const& strategy) const
    {
        for (It it = begin; it != end; ++it)
        {
            geometry::expand(envelope, *it, strategy);
        }
    }

    template <typename Strategy>
    bool is_convex(Strategy const& strategy) const
    {
        if (ring_or_original_empty())
        {
            // Convexity is undetermined, and for this case it does not matter,
            // because it is only used for optimization in point_on_piece,
            // but that is not called if the piece border is not valid
            return false;
        }

        if (m_end - m_begin <= 2)
        {
            // The offsetted ring part of this piece has only two points.
            // If this is true, and the original ring part has only one point,
            // a triangle and it is convex. If the original ring part has two
            // points, it is a rectangle and theoretically could be concave,
            // but because of the way the buffer is generated, that is never
            // the case.
            return true;
        }

        // The offsetted ring part of thie piece has at least three points
        // (this is often the case in a piece marked as "join")

        // We can assume all points of the offset ring are different, and also
        // that all points on the original are different, and that the offsetted
        // ring is different from the original(s)
        Point const offsetted_front = *(m_ring->begin() + m_begin);
        Point const offsetted_second = *(m_ring->begin() + m_begin + 1);

        // These two points will be reassigned in every is_convex call
        Point previous = offsetted_front;
        Point current = offsetted_second;

        // Verify the offsetted range (from the second point on), the original,
        // and loop through the first two points of the offsetted range
        bool const result = is_convex(previous, current, m_ring->begin() + m_begin + 2, m_ring->begin() + m_end, strategy)
            && is_convex(previous, current, m_originals.begin(), m_originals.begin() + m_original_size, strategy)
            && is_convex(previous, current, offsetted_front, strategy)
            && is_convex(previous, current, offsetted_second, strategy);

        return result;
    }

    template <typename It, typename Strategy>
    bool is_convex(Point& previous, Point& current, It begin, It end, Strategy const& strategy) const
    {
        for (It it = begin; it != end; ++it)
        {
            if (! is_convex(previous, current, *it, strategy))
            {
                return false;
            }
        }
        return true;
    }

    template <typename Strategy>
    bool is_convex(Point& previous, Point& current, Point const& next, Strategy const& strategy) const
    {
        int const side = strategy.side().apply(previous, current, next);
        if (side == 1)
        {
            // Next is on the left side of clockwise ring: piece is not convex
            return false;
        }
        if (! equals::equals_point_point(current, next, strategy))
        {
            previous = current;
            current = next;
        }
        return true;
    }

    template <int Direction>
    inline void step_for_monotonicity(Point const& current, Point const& next)
    {
        if (geometry::get<Direction>(current) >= geometry::get<Direction>(next))
        {
            m_is_monotonic_increasing = false;
        }
        if (geometry::get<Direction>(current) <= geometry::get<Direction>(next))
        {
            m_is_monotonic_decreasing = false;
        }
    }

    template <typename It>
    void check_monotonicity(It begin, It end)
    {
        m_is_monotonic_increasing = true;
        m_is_monotonic_decreasing = true;

        if (begin == end || begin + 1 == end)
        {
            return;
        }

        It it = begin;
        for (It previous = it++; it != end; ++previous, ++it)
        {
            step_for_monotonicity<0>(*previous, *it);
        }
    }

    template <typename It>
    inline void calculate_radii(Point const& center, It begin, It end)
    {
        typedef geometry::model::referring_segment<Point const> segment_type;

        bool first = true;

        // An offsetted point-buffer ring around a point is supposed to be closed,
        // therefore walking from start to end is fine.
        It it = begin;
        for (It previous = it++; it != end; ++previous, ++it)
        {
            Point const& p0 = *previous;
            Point const& p1 = *it;
            segment_type const s(p0, p1);
            radius_type const d = geometry::comparable_distance(center, s);

            if (first || d < m_min_comparable_radius)
            {
                m_min_comparable_radius = d;
            }
            if (first || d > m_max_comparable_radius)
            {
                m_max_comparable_radius = d;
            }
            first = false;
        }
    }

};

}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_PIECE_BORDER_HPP
