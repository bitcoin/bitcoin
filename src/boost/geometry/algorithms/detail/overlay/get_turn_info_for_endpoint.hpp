// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013-2020.
// Modifications copyright (c) 2013-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_FOR_ENDPOINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_FOR_ENDPOINT_HPP

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay {

// SEGMENT_INTERSECTION RESULT

//                   C  H0  H1  A0  A1   O              IP1 IP2

// D0 and D1 == 0

// |-------->        2   0   0   0   0   F              i/i x/x
// |-------->
//
// |-------->        2   0   0   0   0   T              i/x x/i
// <--------|
//
// |----->           1   0   0   0   0   T              x/x
//       <-----|
//

// |--------->       2   0   0   0   1   T              i/x x/i
//      <----|
//
// |--------->       2   0   0   0   0   F              i/i x/x
//      |---->
//
// |--------->       2   0   0  -1   1   F              i/i u/x
// |---->
//
// |--------->       2   0   0  -1   0   T              i/x u/i
// <----|

// |------->         2   0   0   1  -1   F   and        i/i x/u
//     |------->     2   0   0  -1   1   F   symmetric  i/i u/x
// |------->
//
//     |------->     2   0   0  -1  -1   T              i/u u/i
// <-------|
//
// |------->         2   0   0   1   1   T              i/x x/i
//     <-------|
//
// |-------->        2   0   0  -1   1   F              i/i u/x
//   |---->
//
// |-------->        2   0   0  -1   1   T              i/x u/i
//   <----|

//       |----->     1  -1  -1  -1  -1   T              u/u
// <-----|
//
//       |----->     1  -1   0  -1   0   F   and        u/x
// |----->           1   0  -1   0  -1   F   symmetric  x/u
//       |----->

// D0 or D1 != 0

//          ^
//          |
//          +        1  -1   1  -1   1   F   and        u/x  (P is vertical)
// |-------->        1   1  -1   1  -1   F   symmetric  x/u  (P is horizontal)
// ^
// |
// +
//
//          +
//          |
//          v
// |-------->        1   1   1   1   1   F              x/x  (P is vertical)
//
// ^
// |
// +
// |-------->        1  -1  -1  -1  -1   F              u/u  (P is vertical)
//
//      ^
//      |
//      +
// |-------->        1   0  -1   0  -1   F              u/u  (P is vertical)
//
//      +
//      |
//      v
// |-------->        1   0   1   0   1   F              u/x  (P is vertical)
//

class linear_intersections
{
public:
    template
    <
        typename Point1, typename Point2, typename IntersectionResult,
        typename Strategy
    >
    linear_intersections(Point1 const& pi,
                         Point2 const& qi,
                         IntersectionResult const& result,
                         bool is_p_last, bool is_q_last,
                         Strategy const& strategy)
    {
        int arrival_a = result.direction.arrival[0];
        int arrival_b = result.direction.arrival[1];
        bool same_dirs = result.direction.dir_a == 0
                      && result.direction.dir_b == 0;

        if ( same_dirs )
        {
            if ( result.intersection_points.count == 2 )
            {
                if ( ! result.direction.opposite )
                {
                    ips[0].p_operation = operation_intersection;
                    ips[0].q_operation = operation_intersection;
                    ips[1].p_operation = union_or_blocked_same_dirs(arrival_a, is_p_last);
                    ips[1].q_operation = union_or_blocked_same_dirs(arrival_b, is_q_last);

                    ips[0].is_pi
                        = equals::equals_point_point(
                            pi, result.intersection_points.intersections[0], strategy);
                    ips[0].is_qi
                        = equals::equals_point_point(
                            qi, result.intersection_points.intersections[0], strategy);
                    ips[1].is_pj = arrival_a != -1;
                    ips[1].is_qj = arrival_b != -1;
                }
                else
                {
                    ips[0].p_operation = operation_intersection;
                    ips[0].q_operation = union_or_blocked_same_dirs(arrival_b, is_q_last);
                    ips[1].p_operation = union_or_blocked_same_dirs(arrival_a, is_p_last);
                    ips[1].q_operation = operation_intersection;

                    ips[0].is_pi = arrival_b != 1;
                    ips[0].is_qj = arrival_b != -1;
                    ips[1].is_pj = arrival_a != -1;
                    ips[1].is_qi = arrival_a != 1;
                }
            }
            else
            {
                BOOST_GEOMETRY_ASSERT(result.intersection_points.count == 1);
                ips[0].p_operation = union_or_blocked_same_dirs(arrival_a, is_p_last);
                ips[0].q_operation = union_or_blocked_same_dirs(arrival_b, is_q_last);

                ips[0].is_pi = arrival_a == -1;
                ips[0].is_qi = arrival_b == -1;
                ips[0].is_pj = arrival_a == 0;
                ips[0].is_qj = arrival_b == 0;
            }
        }
        else
        {
            ips[0].p_operation = union_or_blocked_different_dirs(arrival_a, is_p_last);
            ips[0].q_operation = union_or_blocked_different_dirs(arrival_b, is_q_last);

            ips[0].is_pi = arrival_a == -1;
            ips[0].is_qi = arrival_b == -1;
            ips[0].is_pj = arrival_a == 1;
            ips[0].is_qj = arrival_b == 1;
        }
    }

    struct ip_info
    {
        inline ip_info()
            : p_operation(operation_none), q_operation(operation_none)
            , is_pi(false), is_pj(false), is_qi(false), is_qj(false)
        {}

        operation_type p_operation, q_operation;
        bool is_pi, is_pj, is_qi, is_qj;
    };

    template <std::size_t I>
    ip_info const& get() const
    {
        BOOST_STATIC_ASSERT(I < 2);
        return ips[I];
    }
    
private:

    // only if collinear (same_dirs)
    static inline operation_type union_or_blocked_same_dirs(int arrival, bool is_last)
    {
        if ( arrival == 1 )
            return operation_blocked;
        else if ( arrival == -1 )
            return operation_union;
        else
            return is_last ? operation_blocked : operation_union;
            //return operation_blocked;
    }

    // only if not collinear (!same_dirs)
    static inline operation_type union_or_blocked_different_dirs(int arrival, bool is_last)
    {
        if ( arrival == 1 )
            //return operation_blocked;
            return is_last ? operation_blocked : operation_union;
        else
            return operation_union;
    }

    ip_info ips[2];
};

template <bool EnableFirst, bool EnableLast>
struct get_turn_info_for_endpoint
{
    typedef std::pair<operation_type, operation_type> operations_pair;

    BOOST_STATIC_ASSERT(EnableFirst || EnableLast);

    template<typename UniqueSubRange1,
             typename UniqueSubRange2,
             typename TurnInfo,
             typename IntersectionInfo,
             typename OutputIterator,
             typename Strategy
    >
    static inline bool apply(UniqueSubRange1 const& range_p,
                             UniqueSubRange2 const& range_q,
                             TurnInfo const& tp_model,
                             IntersectionInfo const& inters,
                             method_type /*method*/,
                             OutputIterator out,
                             Strategy const& strategy)
    {
        std::size_t ip_count = inters.i_info().count;
        // no intersection points
        if (ip_count == 0)
        {
            return false;
        }

        if (! range_p.is_first_segment()
            && ! range_q.is_first_segment()
            && ! range_p.is_last_segment()
            && ! range_q.is_last_segment())
        {
            // Not an end-point from segment p or q
            return false;
        }

        linear_intersections intersections(range_p.at(0),
                                           range_q.at(0),
                                           inters.result(),
                                           range_p.is_last_segment(),
                                           range_q.is_last_segment(),
                                           strategy);

        bool append0_last
            = analyse_segment_and_assign_ip(range_p, range_q,
                                            intersections.template get<0>(),
                                            tp_model, inters, 0, out);

        // NOTE: opposite && ip_count == 1 may be true!
        bool opposite = inters.d_info().opposite;

        // don't ignore only for collinear opposite
        bool result_ignore_ip0 = append0_last && ( ip_count == 1 || !opposite );

        if ( intersections.template get<1>().p_operation == operation_none )
            return result_ignore_ip0;
        
        bool append1_last
            = analyse_segment_and_assign_ip(range_p, range_q,
                                            intersections.template get<1>(),
                                            tp_model, inters, 1, out);

        // don't ignore only for collinear opposite
        bool result_ignore_ip1 = append1_last && !opposite /*&& ip_count == 2*/;

        return result_ignore_ip0 || result_ignore_ip1;
    }

    template <typename UniqueSubRange1,
              typename UniqueSubRange2,
              typename TurnInfo,
              typename IntersectionInfo,
              typename OutputIterator>
    static inline
    bool analyse_segment_and_assign_ip(UniqueSubRange1 const& range_p,
                                       UniqueSubRange2 const& range_q,
                                       linear_intersections::ip_info const& ip_info,
                                       TurnInfo const& tp_model,
                                       IntersectionInfo const& inters,
                                       unsigned int ip_index,
                                       OutputIterator out)
    {
        // TODO - calculate first/last only if needed
        bool is_p_first_ip = range_p.is_first_segment() && ip_info.is_pi;
        bool is_p_last_ip = range_p.is_last_segment() && ip_info.is_pj;
        bool is_q_first_ip = range_q.is_first_segment() && ip_info.is_qi;
        bool is_q_last_ip = range_q.is_last_segment() && ip_info.is_qj;
        bool append_first = EnableFirst && (is_p_first_ip || is_q_first_ip);
        bool append_last = EnableLast && (is_p_last_ip || is_q_last_ip);

        operation_type p_operation = ip_info.p_operation;
        operation_type q_operation = ip_info.q_operation;

        if ( append_first || append_last )
        {
            bool handled = handle_internal<0>(range_p, range_q,
                                              is_p_first_ip, is_p_last_ip,
                                              is_q_first_ip, is_q_last_ip,
                                              ip_info.is_qi, ip_info.is_qj,
                                              tp_model, inters, ip_index,
                                              p_operation, q_operation);
            if ( !handled )
            {
                // Reverse p/q
                handle_internal<1>(range_q, range_p,
                                   is_q_first_ip, is_q_last_ip,
                                   is_p_first_ip, is_p_last_ip,
                                   ip_info.is_pi, ip_info.is_pj,
                                   tp_model, inters, ip_index,
                                   q_operation, p_operation);
            }

            if ( p_operation != operation_none )
            {
                method_type method = endpoint_ip_method(ip_info.is_pi, ip_info.is_pj,
                                                        ip_info.is_qi, ip_info.is_qj);
                turn_position p_pos = ip_position(is_p_first_ip, is_p_last_ip);
                turn_position q_pos = ip_position(is_q_first_ip, is_q_last_ip);

                // handle spikes

                // P is spike and should be handled
                if (ip_info.is_pj // this check is redundant (also in is_spike_p) but faster
                  && inters.i_info().count == 2
                  && inters.is_spike_p() )
                {
                    assign(inters.result(), ip_index, method, operation_blocked, q_operation,
                           p_pos, q_pos, is_p_first_ip, is_q_first_ip, true, false, tp_model, out);
                    assign(inters.result(), ip_index, method, operation_intersection, q_operation,
                           p_pos, q_pos, is_p_first_ip, is_q_first_ip, true, false, tp_model, out);
                }
                // Q is spike and should be handled
                else if (ip_info.is_qj // this check is redundant (also in is_spike_q) but faster
                       && inters.i_info().count == 2
                       && inters.is_spike_q() )
                {
                    assign(inters.result(), ip_index, method, p_operation, operation_blocked,
                           p_pos, q_pos, is_p_first_ip, is_q_first_ip, false, true, tp_model, out);
                    assign(inters.result(), ip_index, method, p_operation, operation_intersection,
                           p_pos, q_pos, is_p_first_ip, is_q_first_ip, false, true, tp_model, out);
                }
                // no spikes
                else
                {
                    assign(inters.result(), ip_index, method, p_operation, q_operation,
                           p_pos, q_pos, is_p_first_ip, is_q_first_ip, false, false, tp_model, out);
                }
            }
        }

        return append_last;
    }

    // TODO: IT'S ALSO PROBABLE THAT ALL THIS FUNCTION COULD BE INTEGRATED WITH handle_segment
    //       however now it's lazily calculated and then it would be always calculated

    template<std::size_t G1Index,
             typename UniqueRange1,
             typename UniqueRange2,
             typename TurnInfo,
             typename IntersectionInfo
    >
    static inline bool handle_internal(UniqueRange1 const& range1,
                                       UniqueRange2 const& range2,
                                       bool first1, bool last1, bool first2, bool last2,
                                       bool ip_i2, bool ip_j2, TurnInfo const& tp_model,
                                       IntersectionInfo const& inters, unsigned int ip_index,
                                       operation_type & op1, operation_type & op2)
    {
        boost::ignore_unused(ip_index, tp_model);

        auto const strategy = inters.strategy();

        if ( !first2 && !last2 )
        {
            if ( first1 )
            {
#ifdef BOOST_GEOMETRY_DEBUG_GET_TURNS_LINEAR_LINEAR
                // may this give false positives for INTs?
                typename IntersectionResult::point_type const&
                    inters_pt = inters.i_info().intersections[ip_index];
                BOOST_GEOMETRY_ASSERT(ip_i2 == equals::equals_point_point(i2, inters_pt, strategy));
                BOOST_GEOMETRY_ASSERT(ip_j2 == equals::equals_point_point(j2, inters_pt, strategy));
#endif
                if ( ip_i2 )
                {
                    // don't output this IP - for the first point of other geometry segment
                    op1 = operation_none;
                    op2 = operation_none;
                    return true;
                }
                else if ( ip_j2 )
                {
                    auto const sides = strategy.side();
                    int const side_pj_q2 = sides.apply(range2.at(1), range2.at(2), range1.at(1));
                    int const side_pj_q1 = sides.apply(range2.at(0), range2.at(1), range1.at(1));
                    int const side_qk_q1 = sides.apply(range2.at(0), range2.at(1), range2.at(2));

                    operations_pair operations = operations_of_equal(side_pj_q2, side_pj_q1, side_qk_q1);

// TODO: must the above be calculated?
// wouldn't it be enough to check if segments are collinear?

                    if ( operations_both(operations, operation_continue) )
                    {
                        if ( op1 != operation_union 
                          || op2 != operation_union
                          || ! ( G1Index == 0 ? inters.is_spike_q() : inters.is_spike_p() ) )
                        {
                            // THIS IS WRT THE ORIGINAL SEGMENTS! NOT THE ONES ABOVE!
                            bool opposite = inters.d_info().opposite;

                            op1 = operation_intersection;
                            op2 = opposite ? operation_union : operation_intersection;
                        }
                    }
                    else
                    {
                        BOOST_GEOMETRY_ASSERT(operations_combination(operations, operation_intersection, operation_union));
                        //op1 = operation_union;
                        //op2 = operation_union;
                    }

                    return true;
                }
                // else do nothing - shouldn't be handled this way
            }
            else if ( last1 )
            {
#ifdef BOOST_GEOMETRY_DEBUG_GET_TURNS_LINEAR_LINEAR
                // may this give false positives for INTs?
                typename IntersectionResult::point_type const&
                    inters_pt = inters.i_info().intersections[ip_index];
                BOOST_GEOMETRY_ASSERT(ip_i2 == equals::equals_point_point(i2, inters_pt, strategy));
                BOOST_GEOMETRY_ASSERT(ip_j2 == equals::equals_point_point(j2, inters_pt, strategy));
#endif
                if ( ip_i2 )
                {
                    // don't output this IP - for the first point of other geometry segment
                    op1 = operation_none;
                    op2 = operation_none;
                    return true;
                }
                else if ( ip_j2 )
                {
                    auto const sides = strategy.side();
                    int const side_pi_q2 = sides.apply(range2.at(1), range2.at(2), range1.at(0));
                    int const side_pi_q1 = sides.apply(range2.at(0), range2.at(1), range1.at(0));
                    int const side_qk_q1 = sides.apply(range2.at(0), range2.at(1), range2.at(2));

                    operations_pair operations = operations_of_equal(side_pi_q2, side_pi_q1, side_qk_q1);

// TODO: must the above be calculated?
// wouldn't it be enough to check if segments are collinear?

                    if ( operations_both(operations, operation_continue) )
                    {
                        if ( op1 != operation_blocked
                          || op2 != operation_union
                          || ! ( G1Index == 0 ? inters.is_spike_q() : inters.is_spike_p() ) )
                        {
                            // THIS IS WRT THE ORIGINAL SEGMENTS! NOT THE ONES ABOVE!
                            bool second_going_out = inters.i_info().count > 1;

                            op1 = operation_blocked;
                            op2 = second_going_out ? operation_union : operation_intersection;
                        }
                    }
                    else
                    {
                        BOOST_GEOMETRY_ASSERT(operations_combination(operations, operation_intersection, operation_union));
                        //op1 = operation_blocked;
                        //op2 = operation_union;
                    }

                    return true;
                }
                // else do nothing - shouldn't be handled this way
            }
            // else do nothing - shouldn't be handled this way
        }

        return false;
    }

    static inline method_type endpoint_ip_method(bool ip_pi, bool ip_pj, bool ip_qi, bool ip_qj)
    {
        if ( (ip_pi || ip_pj) && (ip_qi || ip_qj) )
            return method_touch;
        else
            return method_touch_interior;
    }

    static inline turn_position ip_position(bool is_ip_first_i, bool is_ip_last_j)
    {
        return is_ip_first_i ? position_front :
               ( is_ip_last_j ? position_back : position_middle );
    }

    template <typename IntersectionResult,
              typename TurnInfo,
              typename OutputIterator>
    static inline void assign(IntersectionResult const& result,
                              unsigned int ip_index,
                              method_type method,
                              operation_type op0, operation_type op1,
                              turn_position pos0, turn_position pos1,
                              bool is_p_first_ip, bool is_q_first_ip,
                              bool is_p_spike, bool is_q_spike,
                              TurnInfo const& tp_model,
                              OutputIterator out)
    {
        TurnInfo tp = tp_model;
        
        //geometry::convert(ip, tp.point);
        //tp.method = method;
        base_turn_handler::assign_point(tp, method, result.intersection_points, ip_index);

        tp.operations[0].operation = op0;
        tp.operations[1].operation = op1;
        tp.operations[0].position = pos0;
        tp.operations[1].position = pos1;

        if ( result.intersection_points.count > 1 )
        {
            // NOTE: is_collinear is NOT set for the first endpoint
            // for which there is no preceding segment

            //BOOST_GEOMETRY_ASSERT( result.direction.dir_a == 0 && result.direction.dir_b == 0 );
            if ( ! is_p_first_ip )
            {
                tp.operations[0].is_collinear = op0 != operation_intersection
                                             || is_p_spike;
            }

            if ( ! is_q_first_ip )
            {
                tp.operations[1].is_collinear = op1 != operation_intersection
                                             || is_q_spike;
            }
        }
        else //if ( result.intersection_points.count == 1 )
        {
            if ( op0 == operation_blocked && op1 == operation_intersection )
            {
                tp.operations[0].is_collinear = true;
            }
            else if ( op0 == operation_intersection && op1 == operation_blocked )
            {
                tp.operations[1].is_collinear = true;
            }
        }

        *out++ = tp;
    }

    static inline operations_pair operations_of_equal(int side_px_q2,
                                                      int side_px_q1,
                                                      int side_qk_q1)
    {
        // If px (pi or pj) is collinear with qj-qk (q2), they continue collinearly.
        // This can be on either side of q1, or collinear
        // The second condition checks if they do not continue
        // oppositely
        if (side_px_q2 == 0 && side_px_q1 == side_qk_q1)
        {
            return std::make_pair(operation_continue, operation_continue);
        }

        // If they turn to same side (not opposite sides)
        if ( ! base_turn_handler::opposite(side_px_q1, side_qk_q1) )
        {
            // If px is left of q2 or collinear: p: union, q: intersection
            if (side_px_q2 != -1 )
            {
                return std::make_pair(operation_union, operation_intersection);
            }
            else
            {
               return std::make_pair(operation_intersection, operation_union);
            }
        }
        else
        {
            // They turn opposite sides. If p turns left (or collinear),
           // p: union, q: intersection
            if (side_px_q1 != -1 )
            {
                return std::make_pair(operation_union, operation_intersection);
            }
           else
            {
                return std::make_pair(operation_intersection, operation_union);
            }
        }
   }

    static inline bool operations_both(operations_pair const& operations,
                                       operation_type const op)
    {
        return operations.first == op && operations.second == op;
    }

    static inline bool operations_combination(operations_pair const& operations,
                                              operation_type const op1,
                                              operation_type const op2)
    {
        return ( operations.first == op1 && operations.second == op2 )
            || ( operations.first == op2 && operations.second == op1 );
    }
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_FOR_ENDPOINT_HPP
