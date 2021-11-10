// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2020.
// Modifications copyright (c) 2013-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_LA_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_LA_HPP

#include <boost/throw_exception.hpp>

#include <boost/geometry/core/assert.hpp>

#include <boost/geometry/util/condition.hpp>

#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info_for_endpoint.hpp>

// TEMP, for spikes detector
//#include <boost/geometry/algorithms/detail/overlay/get_turn_info_ll.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay {

template<typename AssignPolicy>
struct get_turn_info_linear_areal
{
    // Currently only Linear spikes are handled
    // Areal spikes are ignored
    static const bool handle_spikes = true;

    template
    <
        typename UniqueSubRange1,
        typename UniqueSubRange2,
        typename TurnInfo,
        typename UmbrellaStrategy,
        typename RobustPolicy,
        typename OutputIterator
    >
    static inline OutputIterator apply(
                UniqueSubRange1 const& range_p,
                UniqueSubRange2 const& range_q,
                TurnInfo const& tp_model,
                UmbrellaStrategy const& umbrella_strategy,
                RobustPolicy const& robust_policy,
                OutputIterator out)
    {
        typedef intersection_info
            <
                UniqueSubRange1, UniqueSubRange2,
                typename TurnInfo::point_type,
                UmbrellaStrategy,
                RobustPolicy
            > inters_info;

        inters_info inters(range_p, range_q, umbrella_strategy, robust_policy);

        char const method = inters.d_info().how;

        // Copy, to copy possibly extended fields
        TurnInfo tp = tp_model;

        // Select method and apply
        switch(method)
        {
            case 'a' : // collinear, "at"
            case 'f' : // collinear, "from"
            case 's' : // starts from the middle
                get_turn_info_for_endpoint<true, true>(range_p, range_q,
                    tp_model, inters, method_none, out,
                    umbrella_strategy);
                break;

            case 'd' : // disjoint: never do anything
                break;

            case 'm' :
            {
                if ( get_turn_info_for_endpoint<false, true>(range_p, range_q,
                        tp_model, inters, method_touch_interior, out,
                        umbrella_strategy) )
                {
                    // do nothing
                }
                else
                {
                    typedef touch_interior<TurnInfo> handler;

                    // If Q (1) arrives (1)
                    if ( inters.d_info().arrival[1] == 1 )
                    {
                        handler::template apply<0>(range_p, range_q, tp,
                                    inters.i_info(), inters.d_info(),
                                    inters.sides(), umbrella_strategy);
                    }
                    else
                    {
                        // Swap p/q
                        handler::template apply<1>(range_q, range_p,
                                    tp, inters.i_info(), inters.d_info(),
                                    inters.swapped_sides(), umbrella_strategy);
                    }

                    if ( tp.operations[1].operation == operation_blocked )
                    {
                        tp.operations[0].is_collinear = true;
                    }

                    replace_method_and_operations_tm(tp.method,
                                                     tp.operations[0].operation,
                                                     tp.operations[1].operation);
                    
                    // this function assumes that 'u' must be set for a spike
                    calculate_spike_operation(tp.operations[0].operation,
                                              inters,
                                              umbrella_strategy);
                    
                    *out++ = tp;
                }
            }
            break;
            case 'i' :
            {
                crosses<TurnInfo>::apply(tp, inters.i_info(), inters.d_info());

                replace_operations_i(tp.operations[0].operation, tp.operations[1].operation);

                *out++ = tp;
            }
            break;
            case 't' :
            {
                // Both touch (both arrive there)
                if ( get_turn_info_for_endpoint<false, true>(range_p, range_q,
                        tp_model, inters, method_touch, out,
                        umbrella_strategy) )
                {
                    // do nothing
                }
                else 
                {
                    touch<TurnInfo>::apply(range_p, range_q, tp,
                        inters.i_info(), inters.d_info(), inters.sides(),
                        umbrella_strategy);

                    if ( tp.operations[1].operation == operation_blocked )
                    {
                        tp.operations[0].is_collinear = true;
                    }

                    // workarounds for touch<> not taking spikes into account starts here
                    // those was discovered empirically
                    // touch<> is not symmetrical!
                    // P spikes and Q spikes may produce various operations!
                    // Only P spikes are valid for L/A
                    // TODO: this is not optimal solution - think about rewriting touch<>

                    if ( tp.operations[0].operation == operation_blocked )
                    {
                        // a spike on P on the same line with Q1
                        if ( inters.is_spike_p() )
                        {
                            if ( inters.sides().qk_wrt_p1() == 0 )
                            {
                                tp.operations[0].is_collinear = true;
                            }
                            else
                            {
                                tp.operations[0].operation = operation_union;                                
                            }
                        }
                    }
                    else if ( tp.operations[0].operation == operation_continue
                           && tp.operations[1].operation == operation_continue )
                    {
                        // P spike on the same line with Q2 (opposite)
                        if ( inters.sides().pk_wrt_q1() == -inters.sides().qk_wrt_q1()
                          && inters.is_spike_p() )
                        {
                            tp.operations[0].operation = operation_union;
                            tp.operations[1].operation = operation_union;
                        }
                    }
                    else if ( tp.operations[0].operation == operation_none
                           && tp.operations[1].operation == operation_none )
                    {
                        // spike not handled by touch<>
                        if ( inters.is_spike_p() )
                        {
                            tp.operations[0].operation = operation_intersection;
                            tp.operations[1].operation = operation_union;

                            if ( inters.sides().pk_wrt_q2() == 0 )
                            {
                                tp.operations[0].operation = operation_continue; // will be converted to i
                                tp.operations[0].is_collinear = true;
                            }
                        }
                    }

                    // workarounds for touch<> not taking spikes into account ends here

                    replace_method_and_operations_tm(tp.method,
                                                     tp.operations[0].operation,
                                                     tp.operations[1].operation);

                    bool ignore_spike
                        = calculate_spike_operation(tp.operations[0].operation,
                                                    inters,
                                                    umbrella_strategy);

                    if ( ! BOOST_GEOMETRY_CONDITION(handle_spikes)
                      || ignore_spike
                      || ! append_opposite_spikes<append_touches>( // for 'i' or 'c' i???
                                tp, inters, out) )
                    {
                        *out++ = tp;
                    }
                }
            }
            break;
            case 'e':
            {
                if ( get_turn_info_for_endpoint<true, true>(range_p, range_q,
                        tp_model, inters, method_equal, out,
                        umbrella_strategy) )
                {
                    // do nothing
                }
                else
                {
                    tp.operations[0].is_collinear = true;

                    if ( ! inters.d_info().opposite )
                    {
                        // Both equal
                        // or collinear-and-ending at intersection point
                        equal<TurnInfo>::apply(range_p, range_q, tp,
                            inters.i_info(), inters.d_info(), inters.sides(),
                            umbrella_strategy);

                        turn_transformer_ec<false> transformer(method_touch);
                        transformer(tp);

                        // conditionally handle spikes
                        if ( ! BOOST_GEOMETRY_CONDITION(handle_spikes)
                          || ! append_collinear_spikes(tp, inters,
                                                       method_touch, append_equal, out) )
                        {
                            *out++ = tp; // no spikes
                        }
                    }
                    else
                    {
                        equal_opposite
                            <
                                TurnInfo,
                                AssignPolicy
                            >::apply(range_p, range_q,
                                     tp, out, inters);
                    }
                }
            }
            break;
            case 'c' :
            {
                // Collinear
                if ( get_turn_info_for_endpoint<true, true>(
                        range_p, range_q,
                        tp_model, inters, method_collinear, out,
                        umbrella_strategy) )
                {
                    // do nothing
                }
                else
                {
                    tp.operations[0].is_collinear = true;

                    if ( ! inters.d_info().opposite )
                    {
                        method_type method_replace = method_touch_interior;
                        append_version_c version = append_collinear;

                        if ( inters.d_info().arrival[0] == 0 )
                        {
                            // Collinear, but similar thus handled as equal
                            equal<TurnInfo>::apply(range_p, range_q, tp,
                                inters.i_info(), inters.d_info(), inters.sides(),
                                umbrella_strategy);

                            method_replace = method_touch;
                            version = append_equal;
                        }
                        else
                        {
                            collinear<TurnInfo>::apply(range_p, range_q, tp,
                                inters.i_info(), inters.d_info(), inters.sides());

                            //method_replace = method_touch_interior;
                            //version = append_collinear;
                        }

                        turn_transformer_ec<false> transformer(method_replace);
                        transformer(tp);

                        // conditionally handle spikes
                        if ( ! BOOST_GEOMETRY_CONDITION(handle_spikes)
                          || ! append_collinear_spikes(tp, inters,
                                                       method_replace, version, out) )
                        {
                            // no spikes
                            *out++ = tp;
                        }
                    }
                    else
                    {
                        // Is this always 'm' ?
                        turn_transformer_ec<false> transformer(method_touch_interior);

                        // conditionally handle spikes
                        if ( BOOST_GEOMETRY_CONDITION(handle_spikes) )
                        {
                            append_opposite_spikes<append_collinear_opposite>(
                                    tp, inters, out);
                        }

                        // TODO: ignore for spikes?
                        //       E.g. pass is_p_valid = !is_p_last && !is_pj_spike,
                        //       the same with is_q_valid

                        collinear_opposite
                            <
                                TurnInfo,
                                AssignPolicy
                            >::apply(range_p, range_q,
                                tp, out, inters,
                                inters.sides(), transformer);
                    }
                }
            }
            break;
            case '0' :
            {
                // degenerate points
                if ( BOOST_GEOMETRY_CONDITION(AssignPolicy::include_degenerate) )
                {
                    only_convert::apply(tp, inters.i_info());

                    if ( range_p.is_first_segment()
                      && equals::equals_point_point(range_p.at(0), tp.point,
                                                    umbrella_strategy) )
                    {
                        tp.operations[0].position = position_front;
                    }
                    else if ( range_p.is_last_segment()
                           && equals::equals_point_point(range_p.at(1), tp.point,
                                                         umbrella_strategy) )
                    {
                        tp.operations[0].position = position_back;
                    }
                    // tp.operations[1].position = position_middle;

                    *out++ = tp;
                }
            }
            break;
            default :
            {
#if defined(BOOST_GEOMETRY_DEBUG_ROBUSTNESS)
                std::cout << "TURN: Unknown method: " << method << std::endl;
#endif
#if ! defined(BOOST_GEOMETRY_OVERLAY_NO_THROW)
                BOOST_THROW_EXCEPTION(turn_info_exception(method));
#endif
            }
            break;
        }

        return out;
    }

    template <typename Operation,
              typename IntersectionInfo,
              typename Strategy>
    static inline bool calculate_spike_operation(Operation & op,
                                                 IntersectionInfo const& inters,
                                                 Strategy const& strategy)
    {
        bool is_p_spike = ( op == operation_union || op == operation_intersection )
                       && inters.is_spike_p();

        if ( is_p_spike )
        {
            int const pk_q1 = inters.sides().pk_wrt_q1();
            
            bool going_in = pk_q1 < 0; // Pk on the right
            bool going_out = pk_q1 > 0; // Pk on the left

            int const qk_q1 = inters.sides().qk_wrt_q1();

            // special cases
            if ( qk_q1 < 0 ) // Q turning R
            { 
                // spike on the edge point
                // if it's already known that the spike is going out this musn't be checked
                if ( ! going_out
                  && equals::equals_point_point(inters.rpj(), inters.rqj(), strategy) )
                {
                    int const pk_q2 = inters.sides().pk_wrt_q2();
                    going_in = pk_q1 < 0 && pk_q2 < 0; // Pk on the right of both
                    going_out = pk_q1 > 0 || pk_q2 > 0; // Pk on the left of one of them
                }
            }
            else if ( qk_q1 > 0 ) // Q turning L
            {
                // spike on the edge point
                // if it's already known that the spike is going in this musn't be checked
                if ( ! going_in
                  && equals::equals_point_point(inters.rpj(), inters.rqj(), strategy) )
                {
                    int const pk_q2 = inters.sides().pk_wrt_q2();
                    going_in = pk_q1 < 0 || pk_q2 < 0; // Pk on the right of one of them
                    going_out = pk_q1 > 0 && pk_q2 > 0; // Pk on the left of both
                }
            }

            if ( going_in )
            {
                op = operation_intersection;
                return true;
            }
            else if ( going_out )
            {
                op = operation_union;
                return true;
            }
        }

        return false;
    }

    enum append_version_c { append_equal, append_collinear };

    template <typename TurnInfo,
              typename IntersectionInfo,
              typename OutIt>
    static inline bool append_collinear_spikes(TurnInfo & tp,
                                               IntersectionInfo const& inters,
                                               method_type method, append_version_c version,
                                               OutIt out)
    {
        // method == touch || touch_interior
        // both position == middle

        bool is_p_spike = ( version == append_equal ?
                            ( tp.operations[0].operation == operation_union
                           || tp.operations[0].operation == operation_intersection ) :
                            tp.operations[0].operation == operation_continue )
                       && inters.is_spike_p();

        // TODO: throw an exception for spike in Areal?
        /*bool is_q_spike = tp.operations[1].operation == operation_continue
                       && inters.is_spike_q();

        // both are collinear spikes on the same IP, we can just follow both
        if ( is_p_spike && is_q_spike )
        {
            return false;
        }
        // spike on Linear - it's turning back on the boundary of Areal
        else*/
        if ( is_p_spike )
        {
            tp.method = method;
            tp.operations[0].operation = operation_blocked;
            tp.operations[1].operation = operation_union;
            *out++ = tp;
            tp.operations[0].operation = operation_continue; // boundary
            //tp.operations[1].operation = operation_union;
            *out++ = tp;

            return true;
        }
        // spike on Areal - Linear is going outside
        /*else if ( is_q_spike )
        {
            tp.method = method;
            tp.operations[0].operation = operation_union;
            tp.operations[1].operation = operation_continue;
            *out++ = tp;
            *out++ = tp;

            return true;
        }*/

        return false;
    }

    enum append_version_o { append_touches, append_collinear_opposite };

    template <append_version_o Version,
              typename TurnInfo,
              typename IntersectionInfo,
              typename OutIt>
    static inline bool append_opposite_spikes(TurnInfo & tp,
                                              IntersectionInfo const& inters,
                                              OutIt out)
    {
        static const bool is_version_touches = (Version == append_touches);

        bool is_p_spike = ( is_version_touches ?
                            ( tp.operations[0].operation == operation_continue
                           || tp.operations[0].operation == operation_intersection ) : // i ???
                            true )
                       && inters.is_spike_p();
        
        // TODO: throw an exception for spike in Areal?
        /*bool is_q_spike = ( ( Version == append_touches
                           && tp.operations[1].operation == operation_continue )
                         || ( Version == append_collinear_opposite
                           && tp.operations[1].operation == operation_none ) )
                       && inters.is_spike_q();

        if ( is_p_spike && is_q_spike )
        {
            // u/u or nothing?
            return false;
        }
        else*/
        if ( is_p_spike )
        {
            if ( BOOST_GEOMETRY_CONDITION(is_version_touches)
              || inters.d_info().arrival[0] == 1 )
            {
                if ( BOOST_GEOMETRY_CONDITION(is_version_touches) )
                {
                    tp.operations[0].is_collinear = true;
                    //tp.operations[1].is_collinear = false;
                    tp.method = method_touch;
                }
                else
                {
                    tp.operations[0].is_collinear = true;
                    //tp.operations[1].is_collinear = false;

                    BOOST_GEOMETRY_ASSERT(inters.i_info().count > 1);
                    base_turn_handler::assign_point(tp, method_touch_interior, inters.i_info(), 1);
                }

                tp.operations[0].operation = operation_blocked;
                tp.operations[1].operation = operation_continue; // boundary
                *out++ = tp;
                tp.operations[0].operation = operation_continue; // boundary
                //tp.operations[1].operation = operation_continue; // boundary
                *out++ = tp;

                return true;
            }
        }
        /*else if ( is_q_spike )
        {
            tp.operations[0].is_collinear = true;
            tp.method = is_version_touches ? method_touch : method_touch_interior;
            tp.operations[0].operation = operation_continue;
            tp.operations[1].operation = operation_continue; // boundary
            *out++ = tp;
            *out++ = tp;

            return true;
        }*/

        return false;
    }

    static inline void replace_method_and_operations_tm(method_type & method,
                                                        operation_type & op0,
                                                        operation_type & op1)
    {
        if ( op0 == operation_blocked && op1 == operation_blocked )
        {
            // NOTE: probably only if methods are WRT IPs, not segments!
            method = (method == method_touch ? method_equal : method_collinear);
        }

        // Assuming G1 is always Linear
        if ( op0 == operation_blocked )
        {
            op0 = operation_continue;
        }

        if ( op1 == operation_blocked )
        {
            op1 = operation_continue;
        }
        else if ( op1 == operation_intersection )
        {
            op1 = operation_union;
        }

        // spikes in 'm'
        if ( method == method_error )
        {
            method = method_touch_interior;
            op0 = operation_union;
            op1 = operation_union;
        }
    }

    template <bool IsFront>
    class turn_transformer_ec
    {
    public:
        explicit turn_transformer_ec(method_type method_t_or_m)
            : m_method(method_t_or_m)
        {}

        template <typename Turn>
        void operator()(Turn & turn) const
        {
            operation_type & op0 = turn.operations[0].operation;
            operation_type & op1 = turn.operations[1].operation;

            // NOTE: probably only if methods are WRT IPs, not segments!
            if ( BOOST_GEOMETRY_CONDITION(IsFront)
              || op0 == operation_intersection || op0 == operation_union
              || op1 == operation_intersection || op1 == operation_union )
            {
                turn.method = m_method;
            }

            turn.operations[0].is_collinear = op0 != operation_blocked;

            // Assuming G1 is always Linear
            if ( op0 == operation_blocked )
            {
                op0 = operation_continue;
            }

            if ( op1 == operation_blocked )
            {
                op1 = operation_continue;
            }
            else if ( op1 == operation_intersection )
            {
                op1 = operation_union;
            }
        }

    private:
        method_type m_method;
    };

    static inline void replace_operations_i(operation_type & /*op0*/, operation_type & op1)
    {
        // assuming Linear is always the first one
        op1 = operation_union;
    }

    // NOTE: Spikes may NOT be handled for Linear endpoints because it's not
    //       possible to define a spike on an endpoint. Areal geometries must
    //       NOT have spikes at all. One thing that could be done is to throw
    //       an exception when spike is detected in Areal geometry.
    
    template <bool EnableFirst,
              bool EnableLast,
              typename UniqueSubRange1,
              typename UniqueSubRange2,
              typename TurnInfo,
              typename IntersectionInfo,
              typename OutputIterator,
              typename Strategy>
    static inline bool get_turn_info_for_endpoint(
                            UniqueSubRange1 const& range_p,
                            UniqueSubRange2 const& range_q,
                            TurnInfo const& tp_model,
                            IntersectionInfo const& inters,
                            method_type /*method*/,
                            OutputIterator out,
                            Strategy const& strategy)
    {
        namespace ov = overlay;
        typedef ov::get_turn_info_for_endpoint<EnableFirst, EnableLast> get_info_e;

        const std::size_t ip_count = inters.i_info().count;
        // no intersection points
        if (ip_count == 0)
        {
            return false;
        }

        if (! range_p.is_first_segment() && ! range_p.is_last_segment())
        {
            // P sub-range has no end-points
            return false;
        }

        linear_intersections intersections(range_p.at(0),
                                           range_q.at(0),
                                           inters.result(),
                                           range_p.is_last_segment(),
                                           range_q.is_last_segment(),
                                           strategy);
        linear_intersections::ip_info const& ip0 = intersections.template get<0>();
        linear_intersections::ip_info const& ip1 = intersections.template get<1>();

        const bool opposite = inters.d_info().opposite;

        // ANALYSE AND ASSIGN FIRST

        // IP on the first point of Linear Geometry
        bool was_first_point_handled = false;
        if ( BOOST_GEOMETRY_CONDITION(EnableFirst)
          && range_p.is_first_segment() && ip0.is_pi && !ip0.is_qi ) // !q0i prevents duplication
        {
            TurnInfo tp = tp_model;
            tp.operations[0].position = position_front;
            tp.operations[1].position = position_middle;

            if ( opposite ) // opposite -> collinear
            {
                tp.operations[0].operation = operation_continue;
                tp.operations[1].operation = operation_union;
                tp.method = ip0.is_qj ? method_touch : method_touch_interior;
            }
            else
            {
                auto const sides = strategy.side();

                // pi is the intersection point at qj or in the middle of q1
                // so consider segments
                // 1. pi at qj: qi-qj-pj and qi-qj-qk
                //    x: qi-qj, y: qj-qk, qz: qk
                // 2. pi in the middle of q1: qi-pi-pj and qi-pi-qj
                //    x: qi-pi, y: pi-qj, qz: qj
                //    qi-pi, side the same as WRT q1
                //    pi-qj, side the same as WRT q1
                //    qj WRT q1 is 0
                method_type replaced_method = method_none;
                int side_pj_y = 0, side_pj_x = 0, side_qz_x = 0;
                // 1. ip0 or pi at qj
                if ( ip0.is_qj )
                {
                    replaced_method = method_touch;
                    side_pj_y = sides.apply(range_q.at(1), range_q.at(2), range_p.at(1)); // pj wrt q2
                    side_pj_x = sides.apply(range_q.at(0), range_q.at(1), range_p.at(1)); // pj wrt q1
                    side_qz_x = sides.apply(range_q.at(0), range_q.at(1), range_q.at(2)); // qk wrt q1
                }
                // 2. ip0 or pi in the middle of q1
                else
                {
                    replaced_method = method_touch_interior;
                    side_pj_y = sides.apply(range_q.at(0), range_q.at(1), range_p.at(1)); // pj wrt q1
                    side_pj_x = side_pj_y; // pj wrt q1
                    side_qz_x = 0; // qj wrt q1
                }

                std::pair<operation_type, operation_type> operations
                    = get_info_e::operations_of_equal(side_pj_y, side_pj_x, side_qz_x);

                tp.operations[0].operation = operations.first;
                tp.operations[1].operation = operations.second;

                turn_transformer_ec<true> transformer(replaced_method);
                transformer(tp);
            }

            // equals<> or collinear<> will assign the second point,
            // we'd like to assign the first one
            base_turn_handler::assign_point(tp, tp.method, inters.i_info(), 0);

            // NOTE: is_collinear is not set for the first endpoint of L
            // for which there is no preceding segment
            // here is_p_first_ip == true
            tp.operations[0].is_collinear = false;

            *out++ = tp;

            was_first_point_handled = true;
        }

        // ANALYSE AND ASSIGN LAST

        // IP on the last point of Linear Geometry
        if ( BOOST_GEOMETRY_CONDITION(EnableLast)
          && range_p.is_last_segment()
          && ( ip_count > 1 ? (ip1.is_pj && !ip1.is_qi) : (ip0.is_pj && !ip0.is_qi) ) ) // prevents duplication
        {
            TurnInfo tp = tp_model;
            
            if ( inters.i_info().count > 1 )
            {
                //BOOST_GEOMETRY_ASSERT( result.template get<1>().dir_a == 0 && result.template get<1>().dir_b == 0 );
                tp.operations[0].is_collinear = true;
                tp.operations[1].operation = opposite ? operation_continue : operation_union;
            }
            else //if ( result.template get<0>().count == 1 )
            {
                auto const sides = strategy.side();

                // pj is the intersection point at qj or in the middle of q1
                // so consider segments
                // 1. pj at qj: qi-qj-pi and qi-qj-qk
                //    x: qi-qj, y: qj-qk, qz: qk
                // 2. pj in the middle of q1: qi-pj-pi and qi-pj-qj
                //    x: qi-pj, y: pj-qj, qz: qj
                //    qi-pj, the side is the same as WRT q1
                //    pj-qj, the side is the same as WRT q1
                //    side of qj WRT q1 is 0
                int side_pi_y = 0, side_pi_x = 0, side_qz_x = 0;
                // 1. ip0 or pj at qj
                if ( ip0.is_qj )
                {
                    side_pi_y = sides.apply(range_q.at(1), range_q.at(2), range_p.at(0)); // pi wrt q2
                    side_pi_x = sides.apply(range_q.at(0), range_q.at(1), range_p.at(0)); // pi wrt q1
                    side_qz_x = sides.apply(range_q.at(0), range_q.at(1), range_q.at(2)); // qk wrt q1   
                }
                // 2. ip0 or pj in the middle of q1
                else
                {
                    side_pi_y = sides.apply(range_q.at(0), range_q.at(1), range_p.at(0)); // pi wrt q1
                    side_pi_x = side_pi_y; // pi wrt q1
                    side_qz_x = 0; // qj wrt q1
                }

                std::pair<operation_type, operation_type> operations
                    = get_info_e::operations_of_equal(side_pi_y, side_pi_x, side_qz_x);

                tp.operations[0].operation = operations.first;
                tp.operations[1].operation = operations.second;

                turn_transformer_ec<false> transformer(method_none);
                transformer(tp);

                tp.operations[0].is_collinear = tp.both(operation_continue);
            }

            tp.method = ( ip_count > 1 ? ip1.is_qj : ip0.is_qj ) ? method_touch : method_touch_interior;
            tp.operations[0].operation = operation_blocked;
            tp.operations[0].position = position_back;
            tp.operations[1].position = position_middle;
            
            // equals<> or collinear<> will assign the second point,
            // we'd like to assign the first one
            unsigned int ip_index = ip_count > 1 ? 1 : 0;
            base_turn_handler::assign_point(tp, tp.method, inters.i_info(), ip_index);

            *out++ = tp;

            // don't ignore the first IP if the segment is opposite
            return !( opposite && ip_count > 1 ) || was_first_point_handled;
        }

        // don't ignore anything for now
        return false;
    }
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_LA_HPP
