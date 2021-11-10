// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2020 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2021.
// Modifications copyright (c) 2017-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_BUFFER_INSERTER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_BUFFER_INSERTER_HPP

#include <cstddef>
#include <iterator>

#include <boost/core/ignore_unused.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/rbegin.hpp>
#include <boost/range/rend.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/direction_code.hpp>
#include <boost/geometry/algorithms/detail/buffer/buffered_piece_collection.hpp>
#include <boost/geometry/algorithms/detail/buffer/line_line_intersection.hpp>

#include <boost/geometry/algorithms/num_interior_rings.hpp>
#include <boost/geometry/algorithms/simplify.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>

#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/strategies/side.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/type_traits.hpp>

#include <boost/geometry/views/detail/closed_clockwise_view.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{

template <typename Range, typename DistanceStrategy, typename Strategies>
inline void simplify_input(Range const& range,
        DistanceStrategy const& distance,
        Range& simplified,
        Strategies const& strategies)
{
    // We have to simplify the ring before to avoid very small-scaled
    // features in the original (convex/concave/convex) being enlarged
    // in a very large scale and causing issues (IP's within pieces).
    // This might be reconsidered later. Simplifying with a very small
    // distance (1%% of the buffer) will never be visible in the result,
    // if it is using round joins. For miter joins they are even more
    // sensitive to small scale input features, however the result will
    // look better.
    // It also gets rid of duplicate points

    geometry::detail::simplify::simplify_range<2>::apply(range,
        simplified, distance.simplify_distance(),
        detail::simplify::douglas_peucker(),
        strategies);

}


template <typename RingOutput>
struct buffer_range
{
    typedef typename point_type<RingOutput>::type output_point_type;
    typedef typename coordinate_type<RingOutput>::type coordinate_type;

    template
    <
        typename Collection,
        typename Point,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline
    void add_join(Collection& collection,
            Point const& penultimate_input,
            Point const& previous_input,
            output_point_type const& prev_perp1,
            output_point_type const& prev_perp2,
            Point const& input,
            output_point_type const& perp1,
            output_point_type const& perp2,
            geometry::strategy::buffer::buffer_side_selector side,
            DistanceStrategy const& distance,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            RobustPolicy const& ,
            Strategies const& strategies)
    {
        geometry::strategy::buffer::join_selector const join
                = get_join_type(penultimate_input, previous_input, input,
                                strategies);

        switch(join)
        {
            case geometry::strategy::buffer::join_continue :
                // No join, we get two consecutive sides
                break;
            case geometry::strategy::buffer::join_concave :
                {
                    std::vector<output_point_type> range_out;
                    range_out.push_back(prev_perp2);
                    range_out.push_back(previous_input);
                    collection.add_piece(geometry::strategy::buffer::buffered_concave, previous_input, range_out);

                    range_out.clear();
                    range_out.push_back(previous_input);
                    range_out.push_back(perp1);
                    collection.add_piece(geometry::strategy::buffer::buffered_concave, previous_input, range_out);
                }
                break;
            case geometry::strategy::buffer::join_spike :
                {
                    // For linestrings, only add spike at one side to avoid
                    // duplicates
                    std::vector<output_point_type> range_out;
                    end_strategy.apply(penultimate_input, prev_perp2, previous_input, perp1, side, distance, range_out);
                    collection.add_endcap(end_strategy, range_out, previous_input);
                    collection.set_current_ring_concave();
                }
                break;
            case geometry::strategy::buffer::join_convex :
                {
                    // The corner is convex, we create a join
                    // TODO (future) - avoid a separate vector, add the piece directly
                    output_point_type intersection_point;
                    if (line_line_intersection::apply(prev_perp1, prev_perp2,
                                                      perp1, perp2, previous_input,
                                                      segment_strategy.equidistant(),
                                                      intersection_point))
                    {
                        std::vector<output_point_type> range_out;
                        if (join_strategy.apply(intersection_point,
                                    previous_input, prev_perp2, perp1,
                                    distance.apply(previous_input, input, side),
                                    range_out))
                        {
                            collection.add_piece(geometry::strategy::buffer::buffered_join,
                                    previous_input, range_out);
                        }
                    }
                }
                break;
        }
    }

    // Returns true if collinear point p2 continues after p0 and p1.
    // If it turns back (spike), it returns false.
    static inline bool same_direction(output_point_type const& p0,
            output_point_type const& p1,
            output_point_type const& p2)
    {
        typedef typename cs_tag<output_point_type>::type cs_tag;
        return direction_code<cs_tag>(p0, p1, p2) == 1;
    }

    template <typename Strategies>
    static inline geometry::strategy::buffer::join_selector get_join_type(
            output_point_type const& p0,
            output_point_type const& p1,
            output_point_type const& p2,
            Strategies const& strategies)
    {
        int const side = strategies.side().apply(p0, p1, p2);
        return side == -1 ? geometry::strategy::buffer::join_convex
            :  side == 1  ? geometry::strategy::buffer::join_concave
            :  same_direction(p0, p1, p2) ? geometry::strategy::buffer::join_continue
            : geometry::strategy::buffer::join_spike;
    }

    template
    <
        typename Collection,
        typename Iterator,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline geometry::strategy::buffer::result_code iterate(Collection& collection,
                Iterator begin, Iterator end,
                geometry::strategy::buffer::buffer_side_selector side,
                DistanceStrategy const& distance_strategy,
                SegmentStrategy const& segment_strategy,
                JoinStrategy const& join_strategy,
                EndStrategy const& end_strategy,
                RobustPolicy const& robust_policy,
                Strategies const& strategies,
                bool linear,
                output_point_type& first_p1,
                output_point_type& first_p2,
                output_point_type& last_p1,
                output_point_type& last_p2)
    {
        boost::ignore_unused(segment_strategy);

        typedef typename std::iterator_traits
        <
            Iterator
        >::value_type point_type;

        point_type second_point, penultimate_point, ultimate_point; // last two points from begin/end

        /*
         * last.p1    last.p2  these are the "previous (last) perpendicular points"
         * --------------
         * |            |
         * *------------*____  <- *prev
         * pup          |    | p1           "current perpendicular point 1"
         *              |    |
         *              |    |       this forms a "side", a side is a piece
         *              |    |
         *              *____| p2
         *
         *              ^
         *             *it
         *
         * pup: penultimate_point
         */

        bool const mark_flat
            = linear
                && end_strategy.get_piece_type() == geometry::strategy::buffer::buffered_flat_end;

        geometry::strategy::buffer::result_code result = geometry::strategy::buffer::result_no_output;
        bool first = true;

        Iterator it = begin;

        std::vector<output_point_type> generated_side;
        generated_side.reserve(2);

        for (Iterator prev = it++; it != end; ++it)
        {
            generated_side.clear();
            geometry::strategy::buffer::result_code error_code
                = segment_strategy.apply(*prev, *it, side,
                                distance_strategy, generated_side);

            if (error_code == geometry::strategy::buffer::result_no_output)
            {
                // Because input is simplified, this is improbable,
                // but it can happen for degenerate geometries
                // Further handling of this side is skipped
                continue;
            }
            else if (error_code == geometry::strategy::buffer::result_error_numerical)
            {
                return error_code;
            }

            BOOST_GEOMETRY_ASSERT(! generated_side.empty());

            result = geometry::strategy::buffer::result_normal;

            if (! first)
            {
                 add_join(collection,
                        penultimate_point,
                        *prev, last_p1, last_p2,
                        *it, generated_side.front(), generated_side.back(),
                        side,
                        distance_strategy, segment_strategy, join_strategy, end_strategy,
                        robust_policy, strategies);
            }

            collection.add_side_piece(*prev, *it, generated_side, first);

            if (first && mark_flat)
            {
                collection.mark_flat_start(*prev);
            }

            penultimate_point = *prev;
            ultimate_point = *it;
            last_p1 = generated_side.front();
            last_p2 = generated_side.back();
            prev = it;
            if (first)
            {
                first = false;
                second_point = *it;
                first_p1 = generated_side.front();
                first_p2 = generated_side.back();
            }
        }

        if (mark_flat)
        {
            collection.mark_flat_end(ultimate_point);
        }

        return result;
    }
};

template
<
    typename Multi,
    typename PolygonOutput,
    typename Policy
>
struct buffer_multi
{
    template
    <
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline void apply(Multi const& multi,
            Collection& collection,
            DistanceStrategy const& distance_strategy,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            PointStrategy const& point_strategy,
            RobustPolicy const& robust_policy,
            Strategies const& strategies)
    {
        for (typename boost::range_iterator<Multi const>::type
                it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            Policy::apply(*it, collection,
                distance_strategy, segment_strategy,
                join_strategy, end_strategy, point_strategy,
                robust_policy, strategies);
        }
    }
};

struct visit_pieces_default_policy
{
    template <typename Collection>
    static inline void apply(Collection const&, int)
    {}
};

template
<
    typename OutputPointType,
    typename Point,
    typename Collection,
    typename DistanceStrategy,
    typename PointStrategy
>
inline void buffer_point(Point const& point, Collection& collection,
        DistanceStrategy const& distance_strategy,
        PointStrategy const& point_strategy)
{
    collection.start_new_ring(false);
    std::vector<OutputPointType> range_out;
    point_strategy.apply(point, distance_strategy, range_out);
    collection.add_piece(geometry::strategy::buffer::buffered_point, range_out, false);
    collection.set_piece_center(point);
    collection.finish_ring(geometry::strategy::buffer::result_normal);
}


}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename RingInput,
    typename RingOutput
>
struct buffer_inserter
{};



template
<
    typename Point,
    typename RingOutput
>
struct buffer_inserter<point_tag, Point, RingOutput>
{
    template
    <
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline void apply(Point const& point, Collection& collection,
            DistanceStrategy const& distance_strategy,
            SegmentStrategy const& ,
            JoinStrategy const& ,
            EndStrategy const& ,
            PointStrategy const& point_strategy,
            RobustPolicy const& ,
            Strategies const& )
    {
        detail::buffer::buffer_point
        <
            typename point_type<RingOutput>::type
        >(point, collection, distance_strategy, point_strategy);
    }
};

// Not a specialization, but called from specializations of ring and of polygon.
// Calling code starts/finishes ring before/after apply
template
<
    typename RingInput,
    typename RingOutput
>
struct buffer_inserter_ring
{
    typedef typename point_type<RingOutput>::type output_point_type;

    template
    <
        typename Collection,
        typename Iterator,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline geometry::strategy::buffer::result_code iterate(Collection& collection,
                Iterator begin, Iterator end,
                geometry::strategy::buffer::buffer_side_selector side,
                DistanceStrategy const& distance_strategy,
                SegmentStrategy const& segment_strategy,
                JoinStrategy const& join_strategy,
                EndStrategy const& end_strategy,
                RobustPolicy const& robust_policy,
                Strategies const& strategies)
    {
        output_point_type first_p1, first_p2, last_p1, last_p2;

        typedef detail::buffer::buffer_range<RingOutput> buffer_range;

        geometry::strategy::buffer::result_code result
            = buffer_range::iterate(collection, begin, end,
                side,
                distance_strategy, segment_strategy, join_strategy, end_strategy,
                robust_policy, strategies,
                false, first_p1, first_p2, last_p1, last_p2);

        // Generate closing join
        if (result == geometry::strategy::buffer::result_normal)
        {
            buffer_range::add_join(collection,
                *(end - 2),
                *(end - 1), last_p1, last_p2,
                *(begin + 1), first_p1, first_p2,
                side,
                distance_strategy, segment_strategy, join_strategy, end_strategy,
                robust_policy, strategies);
        }

        // Buffer is closed automatically by last closing corner
        return result;
    }

    template
    <
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline geometry::strategy::buffer::result_code apply(RingInput const& ring,
            Collection& collection,
            DistanceStrategy const& distance,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            PointStrategy const& point_strategy,
            RobustPolicy const& robust_policy,
            Strategies const& strategies)
    {
        RingInput simplified;
        detail::buffer::simplify_input(ring, distance, simplified, strategies);

        geometry::strategy::buffer::result_code code = geometry::strategy::buffer::result_no_output;

        std::size_t n = boost::size(simplified);
        std::size_t const min_points = core_detail::closure::minimum_ring_size
            <
                geometry::closure<RingInput>::value
            >::value;

        if (n >= min_points)
        {
            detail::closed_clockwise_view<RingInput const> view(simplified);
            if (distance.negative())
            {
                // Walk backwards (rings will be reversed afterwards)
                code = iterate(collection, boost::rbegin(view), boost::rend(view),
                        geometry::strategy::buffer::buffer_side_right,
                        distance, segment_strategy, join_strategy, end_strategy,
                        robust_policy, strategies);
            }
            else
            {
                code = iterate(collection, boost::begin(view), boost::end(view),
                        geometry::strategy::buffer::buffer_side_left,
                        distance, segment_strategy, join_strategy, end_strategy,
                        robust_policy, strategies);
            }
        }

        if (code == geometry::strategy::buffer::result_no_output && n >= 1)
        {
            // Use point_strategy to buffer degenerated ring
            detail::buffer::buffer_point<output_point_type>
                (
                    geometry::range::front(simplified),
                    collection, distance, point_strategy
                );
        }
        return code;
    }
};


template
<
    typename RingInput,
    typename RingOutput
>
struct buffer_inserter<ring_tag, RingInput, RingOutput>
{
    template
    <
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline geometry::strategy::buffer::result_code apply(RingInput const& ring,
            Collection& collection,
            DistanceStrategy const& distance,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            PointStrategy const& point_strategy,
            RobustPolicy const& robust_policy,
            Strategies const& strategies)
    {
        collection.start_new_ring(distance.negative());
        geometry::strategy::buffer::result_code const code
            = buffer_inserter_ring<RingInput, RingOutput>::apply(ring,
                collection, distance,
                segment_strategy, join_strategy, end_strategy, point_strategy,
                robust_policy, strategies);
        collection.finish_ring(code, ring, false, false);
        return code;
    }
};

template
<
    typename Linestring,
    typename Polygon
>
struct buffer_inserter<linestring_tag, Linestring, Polygon>
{
    typedef typename ring_type<Polygon>::type output_ring_type;
    typedef typename point_type<output_ring_type>::type output_point_type;
    typedef typename point_type<Linestring>::type input_point_type;

    template
    <
        typename Collection,
        typename Iterator,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline geometry::strategy::buffer::result_code iterate(Collection& collection,
                Iterator begin, Iterator end,
                geometry::strategy::buffer::buffer_side_selector side,
                DistanceStrategy const& distance_strategy,
                SegmentStrategy const& segment_strategy,
                JoinStrategy const& join_strategy,
                EndStrategy const& end_strategy,
                RobustPolicy const& robust_policy,
                Strategies const& strategies,
                output_point_type& first_p1)
    {
        input_point_type const& ultimate_point = *(end - 1);
        input_point_type const& penultimate_point = *(end - 2);

        // For the end-cap, we need to have the last perpendicular point on the
        // other side of the linestring. If it is the second pass (right),
        // we have it already from the first phase (left).
        // But for the first pass, we have to generate it
        output_point_type reverse_p1;
        if (side == geometry::strategy::buffer::buffer_side_right)
        {
            reverse_p1 = first_p1;
        }
        else
        {
            std::vector<output_point_type> generated_side;
            geometry::strategy::buffer::result_code code
                = segment_strategy.apply(ultimate_point, penultimate_point,
                    geometry::strategy::buffer::buffer_side_right,
                    distance_strategy, generated_side);
            if (code != geometry::strategy::buffer::result_normal)
            {
                // No output or numerical error
                return code;
            }
            reverse_p1 = generated_side.front();
        }

        output_point_type first_p2, last_p1, last_p2;

        geometry::strategy::buffer::result_code result
            = detail::buffer::buffer_range<output_ring_type>::iterate(collection,
                begin, end, side,
                distance_strategy, segment_strategy, join_strategy, end_strategy,
                robust_policy, strategies,
                true, first_p1, first_p2, last_p1, last_p2);

        if (result == geometry::strategy::buffer::result_normal)
        {
            std::vector<output_point_type> range_out;
            end_strategy.apply(penultimate_point, last_p2, ultimate_point, reverse_p1,
                               side, distance_strategy, range_out);
            collection.add_endcap(end_strategy, range_out, ultimate_point);
        }
        return result;
    }

    template
    <
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline geometry::strategy::buffer::result_code apply(Linestring const& linestring,
            Collection& collection,
            DistanceStrategy const& distance,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            PointStrategy const& point_strategy,
            RobustPolicy const& robust_policy,
            Strategies const& strategies)
    {
        Linestring simplified;
        detail::buffer::simplify_input(linestring, distance, simplified, strategies);

        geometry::strategy::buffer::result_code code = geometry::strategy::buffer::result_no_output;
        std::size_t n = boost::size(simplified);
        if (n > 1)
        {
            collection.start_new_ring(false);
            output_point_type first_p1;
            code = iterate(collection,
                    boost::begin(simplified), boost::end(simplified),
                    geometry::strategy::buffer::buffer_side_left,
                    distance, segment_strategy, join_strategy, end_strategy,
                    robust_policy, strategies,
                    first_p1);

            if (code == geometry::strategy::buffer::result_normal)
            {
                code = iterate(collection,
                        boost::rbegin(simplified), boost::rend(simplified),
                        geometry::strategy::buffer::buffer_side_right,
                        distance, segment_strategy, join_strategy, end_strategy,
                        robust_policy, strategies,
                        first_p1);
            }
            collection.finish_ring(code);
        }
        if (code == geometry::strategy::buffer::result_no_output && n >= 1)
        {
            // Use point_strategy to buffer degenerated linestring
            detail::buffer::buffer_point<output_point_type>
                (
                    geometry::range::front(simplified),
                    collection, distance, point_strategy
                );
        }
        return code;
    }
};


template
<
    typename PolygonInput,
    typename PolygonOutput
>
struct buffer_inserter<polygon_tag, PolygonInput, PolygonOutput>
{
private:
    typedef typename ring_type<PolygonInput>::type input_ring_type;
    typedef typename ring_type<PolygonOutput>::type output_ring_type;

    typedef buffer_inserter_ring<input_ring_type, output_ring_type> policy;


    template
    <
        typename Iterator,
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline
    void iterate(Iterator begin, Iterator end,
            Collection& collection,
            DistanceStrategy const& distance,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            PointStrategy const& point_strategy,
            RobustPolicy const& robust_policy,
            Strategies const& strategies,
            bool is_interior)
    {
        for (Iterator it = begin; it != end; ++it)
        {
            // For exterior rings, it deflates if distance is negative.
            // For interior rings, it is vice versa
            bool const deflate = is_interior
                    ? ! distance.negative()
                    : distance.negative();

            collection.start_new_ring(deflate);
            geometry::strategy::buffer::result_code const code
                    = policy::apply(*it, collection, distance, segment_strategy,
                    join_strategy, end_strategy, point_strategy,
                    robust_policy, strategies);

            collection.finish_ring(code, *it, is_interior, false);
        }
    }

    template
    <
        typename InteriorRings,
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline
    void apply_interior_rings(InteriorRings const& interior_rings,
            Collection& collection,
            DistanceStrategy const& distance,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            PointStrategy const& point_strategy,
            RobustPolicy const& robust_policy,
            Strategies const& strategies)
    {
        iterate(boost::begin(interior_rings), boost::end(interior_rings),
            collection, distance, segment_strategy,
            join_strategy, end_strategy, point_strategy,
            robust_policy, strategies, true);
    }

public:
    template
    <
        typename Collection,
        typename DistanceStrategy,
        typename SegmentStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename PointStrategy,
        typename RobustPolicy,
        typename Strategies
    >
    static inline void apply(PolygonInput const& polygon,
            Collection& collection,
            DistanceStrategy const& distance,
            SegmentStrategy const& segment_strategy,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            PointStrategy const& point_strategy,
            RobustPolicy const& robust_policy,
            Strategies const& strategies)
    {
        {
            collection.start_new_ring(distance.negative());

            geometry::strategy::buffer::result_code const code
                = policy::apply(exterior_ring(polygon), collection,
                    distance, segment_strategy,
                    join_strategy, end_strategy, point_strategy,
                    robust_policy, strategies);

            collection.finish_ring(code, exterior_ring(polygon), false,
                    geometry::num_interior_rings(polygon) > 0u);
        }

        apply_interior_rings(interior_rings(polygon),
                collection, distance, segment_strategy,
                join_strategy, end_strategy, point_strategy,
                robust_policy, strategies);
    }
};


template
<
    typename Multi,
    typename PolygonOutput
>
struct buffer_inserter<multi_tag, Multi, PolygonOutput>
    : public detail::buffer::buffer_multi
             <
                Multi,
                PolygonOutput,
                dispatch::buffer_inserter
                <
                    typename single_tag_of
                                <
                                    typename tag<Multi>::type
                                >::type,
                    typename boost::range_value<Multi const>::type,
                    typename geometry::ring_type<PolygonOutput>::type
                >
            >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{

template
<
    typename GeometryOutput,
    typename GeometryInput,
    typename OutputIterator,
    typename DistanceStrategy,
    typename SegmentStrategy,
    typename JoinStrategy,
    typename EndStrategy,
    typename PointStrategy,
    typename Strategies,
    typename RobustPolicy,
    typename VisitPiecesPolicy
>
inline void buffer_inserter(GeometryInput const& geometry_input, OutputIterator out,
        DistanceStrategy const& distance_strategy,
        SegmentStrategy const& segment_strategy,
        JoinStrategy const& join_strategy,
        EndStrategy const& end_strategy,
        PointStrategy const& point_strategy,
        Strategies const& strategies,
        RobustPolicy const& robust_policy,
        VisitPiecesPolicy& visit_pieces_policy
    )
{
    boost::ignore_unused(visit_pieces_policy);

    typedef detail::buffer::buffered_piece_collection
    <
        typename geometry::ring_type<GeometryOutput>::type,
        Strategies,
        DistanceStrategy,
        RobustPolicy
    > collection_type;
    collection_type collection(strategies, distance_strategy, robust_policy);
    collection_type const& const_collection = collection;

    bool const areal = util::is_areal<GeometryInput>::value;

    dispatch::buffer_inserter
        <
            typename tag_cast
                <
                    typename tag<GeometryInput>::type,
                    multi_tag
                >::type,
            GeometryInput,
            GeometryOutput
        >::apply(geometry_input, collection,
                 distance_strategy, segment_strategy, join_strategy,
                 end_strategy, point_strategy,
                 robust_policy, strategies);

    collection.get_turns();
    if (BOOST_GEOMETRY_CONDITION(areal))
    {
        collection.check_turn_in_original();
    }

    collection.verify_turns();

    // Visit the piece collection. This does nothing (by default), but
    // optionally a debugging tool can be attached (e.g. console or svg),
    // or the piece collection can be unit-tested
    // phase 0: turns (before discarded)
    visit_pieces_policy.apply(const_collection, 0);

    collection.discard_rings();
    collection.block_turns();
    collection.enrich();

    // phase 1: turns (after enrichment/clustering)
    visit_pieces_policy.apply(const_collection, 1);

    if (BOOST_GEOMETRY_CONDITION(areal))
    {
        collection.deflate_check_turns();
    }

    collection.traverse();

    // Reverse all offsetted rings / traversed rings if:
    // - they were generated on the negative side (deflate) of polygons
    // - the output is counter clockwise
    // and avoid reversing twice
    bool reverse = distance_strategy.negative() && areal;
    if (BOOST_GEOMETRY_CONDITION(
            geometry::point_order<GeometryOutput>::value == counterclockwise))
    {
        reverse = ! reverse;
    }
    if (reverse)
    {
        collection.reverse();
    }

    if (BOOST_GEOMETRY_CONDITION(distance_strategy.negative() && areal))
    {
        collection.discard_nonintersecting_deflated_rings();
    }

    collection.template assign<GeometryOutput>(out);

    // Visit collection again
    // phase 2: rings (after traversing)
    visit_pieces_policy.apply(const_collection, 2);
}

template
<
    typename GeometryOutput,
    typename GeometryInput,
    typename OutputIterator,
    typename DistanceStrategy,
    typename SegmentStrategy,
    typename JoinStrategy,
    typename EndStrategy,
    typename PointStrategy,
    typename Strategies,
    typename RobustPolicy
>
inline void buffer_inserter(GeometryInput const& geometry_input, OutputIterator out,
        DistanceStrategy const& distance_strategy,
        SegmentStrategy const& segment_strategy,
        JoinStrategy const& join_strategy,
        EndStrategy const& end_strategy,
        PointStrategy const& point_strategy,
        Strategies const& strategies,
        RobustPolicy const& robust_policy)
{
    detail::buffer::visit_pieces_default_policy visitor;
    buffer_inserter<GeometryOutput>(geometry_input, out,
        distance_strategy, segment_strategy, join_strategy,
        end_strategy, point_strategy,
        strategies, robust_policy, visitor);
}
#endif // DOXYGEN_NO_DETAIL

}} // namespace detail::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_BUFFER_INSERTER_HPP
