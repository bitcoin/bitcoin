// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2020 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2016, 2018.
// Modifications copyright (c) 2016-2018 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_PIECE_VISITOR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_PIECE_VISITOR_HPP

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/config.hpp>

#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_box.hpp>
#include <boost/geometry/algorithms/detail/disjoint/box_box.hpp>
#include <boost/geometry/algorithms/detail/buffer/buffer_policies.hpp>
#include <boost/geometry/geometries/box.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL

namespace detail { namespace buffer
{

template
<
    typename CsTag,
    typename Turns,
    typename Pieces,
    typename DistanceStrategy

>
class turn_in_piece_visitor
{
    Turns& m_turns; // because partition is currently operating on const input only
    Pieces const& m_pieces; // to check for piece-type
    DistanceStrategy const& m_distance_strategy; // to check if point is on original or one_sided

    template <typename Operation, typename Piece>
    inline bool skip(Operation const& op, Piece const& piece) const
    {
        if (op.piece_index == piece.index)
        {
            return true;
        }
        Piece const& pc = m_pieces[op.piece_index];
        if (pc.left_index == piece.index || pc.right_index == piece.index)
        {
            if (pc.type == strategy::buffer::buffered_flat_end)
            {
                // If it is a flat end, don't compare against its neighbor:
                // it will always be located on one of the helper segments
                return true;
            }
            if (pc.type == strategy::buffer::buffered_concave)
            {
                // If it is concave, the same applies: the IP will be
                // located on one of the helper segments
                return true;
            }
        }

        return false;
    }

    template <typename NumericType>
    inline bool is_one_sided(NumericType const& left, NumericType const& right) const
    {
        static NumericType const zero = 0;
        return geometry::math::equals(left, zero)
            || geometry::math::equals(right, zero);
    }

    template <typename Point>
    inline bool has_zero_distance_at(Point const& point) const
    {
        return is_one_sided(m_distance_strategy.apply(point, point,
                strategy::buffer::buffer_side_left),
            m_distance_strategy.apply(point, point,
                strategy::buffer::buffer_side_right));
    }

public:

    inline turn_in_piece_visitor(Turns& turns, Pieces const& pieces,
                                 DistanceStrategy const& distance_strategy)
        : m_turns(turns)
        , m_pieces(pieces)
        , m_distance_strategy(distance_strategy)
    {}

    template <typename Turn, typename Piece>
    inline bool apply(Turn const& turn, Piece const& piece)
    {
        if (! turn.is_turn_traversable)
        {
            // Already handled
            return true;
        }

        if (piece.type == strategy::buffer::buffered_flat_end
            || piece.type == strategy::buffer::buffered_concave)
        {
            // Turns cannot be located within flat-end or concave pieces
            return true;
        }

        if (skip(turn.operations[0], piece) || skip(turn.operations[1], piece))
        {
            return true;
        }

        return apply(turn, piece, piece.m_piece_border);
    }

    template <typename Turn, typename Piece, typename Border>
    inline bool apply(Turn const& turn, Piece const& piece, Border const& border)
    {
        if (! geometry::covered_by(turn.point, border.m_envelope))
        {
            // Easy check: if turn is not in the (expanded) envelope
            return true;
        }

        if (piece.type == geometry::strategy::buffer::buffered_point)
        {
            // Optimization for a buffer around points: if distance from center
            // is not between min/max radius, it is either inside or outside,
            // and more expensive checks are not necessary.
            typedef typename Border::radius_type radius_type;

            radius_type const d = geometry::comparable_distance(piece.m_center, turn.point);

            if (d < border.m_min_comparable_radius)
            {
                Turn& mutable_turn = m_turns[turn.turn_index];
                mutable_turn.is_turn_traversable = false;
                return true;
            }
            if (d > border.m_max_comparable_radius)
            {
                return true;
            }
        }

        // Check if buffer is one-sided (at this point), because then a point
        // on the original is not considered as within.
        bool const one_sided = has_zero_distance_at(turn.point);

        typename Border::state_type state;
        if (! border.point_on_piece(turn.point, one_sided, turn.is_linear_end_point, state))
        {
            return true;
        }

        if (state.code() == 1)
        {
            // It is WITHIN a piece, or on the piece border, but not
            // on the offsetted part of it.

            // TODO - at further removing rescaling, this threshold can be
            // adapted, or ideally, go.
            // This threshold is minimized to the point where fragile
            // unit tests of hard cases start to fail (5 in multi_polygon)
            // But it is acknowlegded that such a threshold depends on the
            // scale of the input.
            if (state.m_min_distance > 1.0e-5 || ! state.m_close_to_offset)
            {
                Turn& mutable_turn = m_turns[turn.turn_index];
                mutable_turn.is_turn_traversable = false;

                // Keep track of the information if this turn was close
                // to an offset (without threshold). Because if it was,
                // it might have been classified incorrectly and in the
                // pretraversal step, it can in hindsight be classified
                // as "outside".
                mutable_turn.close_to_offset = state.m_close_to_offset;
            }
        }

        return true;
    }
};


}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_PIECE_VISITOR_HPP
