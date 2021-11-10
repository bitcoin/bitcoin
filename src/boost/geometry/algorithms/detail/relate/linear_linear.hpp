// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013, 2014, 2015, 2017, 2018, 2019.
// Modifications copyright (c) 2013-2019 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_LINEAR_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_LINEAR_LINEAR_HPP

#include <algorithm>

#include <boost/core/ignore_unused.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/core/assert.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/range.hpp>

#include <boost/geometry/algorithms/detail/sub_range.hpp>
#include <boost/geometry/algorithms/detail/single_geometry.hpp>

#include <boost/geometry/algorithms/detail/relate/point_geometry.hpp>
#include <boost/geometry/algorithms/detail/relate/result.hpp>
#include <boost/geometry/algorithms/detail/relate/turns.hpp>
#include <boost/geometry/algorithms/detail/relate/boundary_checker.hpp>
#include <boost/geometry/algorithms/detail/relate/follow_helpers.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace relate {

template <typename Result, typename BoundaryChecker, bool TransposeResult>
class disjoint_linestring_pred
{
public:
    disjoint_linestring_pred(Result & res,
                             BoundaryChecker const& boundary_checker)
        : m_result(res)
        , m_boundary_checker(boundary_checker)
        , m_flags(0)
    {
        if ( ! may_update<interior, exterior, '1', TransposeResult>(m_result) )
        {
            m_flags |= 1;
        }
        if ( ! may_update<boundary, exterior, '0', TransposeResult>(m_result) )
        {
            m_flags |= 2;
        }
    }

    template <typename Linestring>
    bool operator()(Linestring const& linestring)
    {
        if ( m_flags == 3 )
        {
            return false;
        }

        std::size_t const count = boost::size(linestring);
        
        // invalid input
        if ( count < 2 )
        {
            // ignore
            // TODO: throw an exception?
            return true;
        }

        // point-like linestring
        if ( count == 2
          && equals::equals_point_point(range::front(linestring),
                                        range::back(linestring),
                                        m_boundary_checker.strategy()) )
        {
            update<interior, exterior, '0', TransposeResult>(m_result);
        }
        else
        {
            update<interior, exterior, '1', TransposeResult>(m_result);
            m_flags |= 1;

            // check if there is a boundary
            if ( m_flags < 2
              && ( m_boundary_checker.template
                    is_endpoint_boundary<boundary_front>(range::front(linestring))
                || m_boundary_checker.template
                    is_endpoint_boundary<boundary_back>(range::back(linestring)) ) )
            {
                update<boundary, exterior, '0', TransposeResult>(m_result);
                m_flags |= 2;
            }
        }

        return m_flags != 3
            && ! m_result.interrupt;
    }

private:
    Result & m_result;
    BoundaryChecker const& m_boundary_checker;
    unsigned m_flags;
};

template <typename Geometry1, typename Geometry2>
struct linear_linear
{
    static const bool interruption_enabled = true;

    typedef typename geometry::point_type<Geometry1>::type point1_type;
    typedef typename geometry::point_type<Geometry2>::type point2_type;

    template <typename Result, typename Strategy>
    static inline void apply(Geometry1 const& geometry1, Geometry2 const& geometry2,
                             Result & result,
                             Strategy const& strategy)
    {
        typedef typename Strategy::cs_tag cs_tag;

        // The result should be FFFFFFFFF
        relate::set<exterior, exterior, result_dimension<Geometry1>::value>(result);// FFFFFFFFd, d in [1,9] or T
        if ( BOOST_GEOMETRY_CONDITION( result.interrupt ) )
            return;

        // get and analyse turns
        typedef typename turns::get_turns
            <
                Geometry1, Geometry2
            >::template turn_info_type<Strategy>::type turn_type;
        std::vector<turn_type> turns;

        interrupt_policy_linear_linear<Result> interrupt_policy(result);

        turns::get_turns
            <
                Geometry1,
                Geometry2,
                detail::get_turns::get_turn_info_type<Geometry1, Geometry2, turns::assign_policy<true> >
            >::apply(turns, geometry1, geometry2, interrupt_policy, strategy);

        if ( BOOST_GEOMETRY_CONDITION( result.interrupt ) )
            return;

        typedef boundary_checker<Geometry1, Strategy> boundary_checker1_type;
        boundary_checker1_type boundary_checker1(geometry1, strategy);
        disjoint_linestring_pred<Result, boundary_checker1_type, false> pred1(result, boundary_checker1);
        for_each_disjoint_geometry_if<0, Geometry1>::apply(turns.begin(), turns.end(), geometry1, pred1);
        if ( BOOST_GEOMETRY_CONDITION( result.interrupt ) )
            return;

        typedef boundary_checker<Geometry2, Strategy> boundary_checker2_type;
        boundary_checker2_type boundary_checker2(geometry2, strategy);
        disjoint_linestring_pred<Result, boundary_checker2_type, true> pred2(result, boundary_checker2);
        for_each_disjoint_geometry_if<1, Geometry2>::apply(turns.begin(), turns.end(), geometry2, pred2);
        if ( BOOST_GEOMETRY_CONDITION( result.interrupt ) )
            return;
        
        if ( turns.empty() )
            return;

        // TODO: turns must be sorted and followed only if it's possible to go out and in on the same point
        // for linear geometries union operation must be detected which I guess would be quite often

        if ( may_update<interior, interior, '1'>(result)
          || may_update<interior, boundary, '0'>(result)
          || may_update<interior, exterior, '1'>(result)
          || may_update<boundary, interior, '0'>(result)
          || may_update<boundary, boundary, '0'>(result)
          || may_update<boundary, exterior, '0'>(result) )
        {
            typedef turns::less<0, turns::less_op_linear_linear<0>, cs_tag> less;
            std::sort(turns.begin(), turns.end(), less());

            turns_analyser<turn_type, 0> analyser;
            analyse_each_turn(result, analyser,
                              turns.begin(), turns.end(),
                              geometry1, geometry2,
                              boundary_checker1, boundary_checker2);
        }

        if ( BOOST_GEOMETRY_CONDITION( result.interrupt ) )
            return;
        
        if ( may_update<interior, interior, '1', true>(result)
          || may_update<interior, boundary, '0', true>(result)
          || may_update<interior, exterior, '1', true>(result)
          || may_update<boundary, interior, '0', true>(result)
          || may_update<boundary, boundary, '0', true>(result)
          || may_update<boundary, exterior, '0', true>(result) )
        {
            typedef turns::less<1, turns::less_op_linear_linear<1>, cs_tag> less;
            std::sort(turns.begin(), turns.end(), less());

            turns_analyser<turn_type, 1> analyser;
            analyse_each_turn(result, analyser,
                              turns.begin(), turns.end(),
                              geometry2, geometry1,
                              boundary_checker2, boundary_checker1);
        }
    }

    template <typename Result>
    class interrupt_policy_linear_linear
    {
    public:
        static bool const enabled = true;

        explicit interrupt_policy_linear_linear(Result & result)
            : m_result(result)
        {}

// TODO: since we update result for some operations here, we may not do it in the analyser!

        template <typename Range>
        inline bool apply(Range const& turns)
        {
            typedef typename boost::range_iterator<Range const>::type iterator;
            
            for (iterator it = boost::begin(turns) ; it != boost::end(turns) ; ++it)
            {
                if ( it->operations[0].operation == overlay::operation_intersection
                  || it->operations[1].operation == overlay::operation_intersection )
                {
                    update<interior, interior, '1'>(m_result);
                }
                else if ( ( it->operations[0].operation == overlay::operation_union
                         || it->operations[0].operation == overlay::operation_blocked
                         || it->operations[1].operation == overlay::operation_union
                         || it->operations[1].operation == overlay::operation_blocked )
                       && it->operations[0].position == overlay::position_middle
                       && it->operations[1].position == overlay::position_middle )
                {
// TODO: here we could also check the boundaries and set IB,BI,BB at this point
                    update<interior, interior, '0'>(m_result);
                }
            }

            return m_result.interrupt;
        }

    private:
        Result & m_result;
    };

    // This analyser should be used like Input or SinglePass Iterator
    template <typename TurnInfo, std::size_t OpId>
    class turns_analyser
    {
        typedef typename TurnInfo::point_type turn_point_type;

        static const std::size_t op_id = OpId;
        static const std::size_t other_op_id = (OpId + 1) % 2;
        static const bool transpose_result = OpId != 0;

    public:
        turns_analyser()
            : m_previous_turn_ptr(NULL)
            , m_previous_operation(overlay::operation_none)
            , m_degenerated_turn_ptr(NULL)
            , m_collinear_spike_exit(false)
        {}

        template <typename Result,
                  typename TurnIt,
                  typename Geometry,
                  typename OtherGeometry,
                  typename BoundaryChecker,
                  typename OtherBoundaryChecker>
        void apply(Result & res, TurnIt it,
                   Geometry const& geometry,
                   OtherGeometry const& other_geometry,
                   BoundaryChecker const& boundary_checker,
                   OtherBoundaryChecker const& other_boundary_checker)
        {
            overlay::operation_type const op = it->operations[op_id].operation;

            segment_identifier const& seg_id = it->operations[op_id].seg_id;
            segment_identifier const& other_id = it->operations[other_op_id].seg_id;

            bool const first_in_range = m_seg_watcher.update(seg_id);

            if ( op != overlay::operation_union
              && op != overlay::operation_intersection
              && op != overlay::operation_blocked )
            {
                // degenerated turn
                if ( op == overlay::operation_continue
                  && it->method == overlay::method_none
                  && m_exit_watcher.is_outside(*it) 
                  /*&& ( m_exit_watcher.get_exit_operation() == overlay::operation_none 
                    || ! turn_on_the_same_ip<op_id>(m_exit_watcher.get_exit_turn(), *it) )*/ )
                {
                    // TODO: rewrite the above condition

                    // WARNING! For spikes the above condition may be TRUE
                    // When degenerated turns are be marked in a different way than c,c/c
                    // different condition will be checked

                    handle_degenerated(res, *it,
                                       geometry, other_geometry,
                                       boundary_checker, other_boundary_checker,
                                       first_in_range);

                    // TODO: not elegant solution! should be rewritten.
                    if ( it->operations[op_id].position == overlay::position_back )
                    {
                        m_previous_operation = overlay::operation_blocked;
                        m_exit_watcher.reset_detected_exit();
                    }
                }

                return;
            }

            // reset
            m_degenerated_turn_ptr = NULL;

            // handle possible exit
            bool fake_enter_detected = false;
            if ( m_exit_watcher.get_exit_operation() == overlay::operation_union )
            {
                // real exit point - may be multiple
                // we know that we entered and now we exit
                if ( ! turn_on_the_same_ip<op_id>(m_exit_watcher.get_exit_turn(),
                                                  *it,
                                                  boundary_checker.strategy()) )
                {
                    m_exit_watcher.reset_detected_exit();
                    
                    // not the last IP
                    update<interior, exterior, '1', transpose_result>(res);
                }
                // fake exit point, reset state
                else if ( op == overlay::operation_intersection )
                {
                    m_exit_watcher.reset_detected_exit();
                    fake_enter_detected = true;
                }
            }
            else if ( m_exit_watcher.get_exit_operation() == overlay::operation_blocked )
            {
                // ignore multiple BLOCKs
                if ( op == overlay::operation_blocked )
                    return;

                if ( op == overlay::operation_intersection
                  && turn_on_the_same_ip<op_id>(m_exit_watcher.get_exit_turn(),
                                                *it,
                                                boundary_checker.strategy()) )
                {
                    fake_enter_detected = true;
                }

                m_exit_watcher.reset_detected_exit();
            }

            // i/i, i/x, i/u
            if ( op == overlay::operation_intersection )
            {
                bool const was_outside = m_exit_watcher.is_outside();
                m_exit_watcher.enter(*it);

                // interiors overlaps
                update<interior, interior, '1', transpose_result>(res);

                bool const this_b = it->operations[op_id].position == overlay::position_front // ignore spikes!
                                 && is_ip_on_boundary<boundary_front>(it->point,
                                                                      it->operations[op_id],
                                                                      boundary_checker,
                                                                      seg_id);

                // going inside on boundary point
                // may be front only
                if ( this_b )
                {
                    // may be front and back
                    bool const other_b = is_ip_on_boundary<boundary_any>(it->point,
                                                                         it->operations[other_op_id],
                                                                         other_boundary_checker,
                                                                         other_id);

                    // it's also the boundary of the other geometry
                    if ( other_b )
                    {
                        update<boundary, boundary, '0', transpose_result>(res);
                    }
                    else
                    {
                        update<boundary, interior, '0', transpose_result>(res);
                    }
                }
                // going inside on non-boundary point
                else
                {
                    // if we didn't enter in the past, we were outside
                    if ( was_outside
                      && ! fake_enter_detected
                      && it->operations[op_id].position != overlay::position_front
                      && ! m_collinear_spike_exit )
                    {
                        update<interior, exterior, '1', transpose_result>(res);

                        // if it's the first IP then the first point is outside
                        if ( first_in_range )
                        {
                            bool const front_b = is_endpoint_on_boundary<boundary_front>(
                                                    range::front(sub_range(geometry, seg_id)),
                                                    boundary_checker);

                            // if there is a boundary on the first point
                            if ( front_b )
                            {
                                update<boundary, exterior, '0', transpose_result>(res);
                            }
                        }
                    }
                }

                m_collinear_spike_exit = false;
            }
            // u/i, u/u, u/x, x/i, x/u, x/x
            else if ( op == overlay::operation_union || op == overlay::operation_blocked )
            {
                // TODO: is exit watcher still needed?
                // couldn't is_collinear and some going inside counter be used instead?

                bool const is_collinear = it->operations[op_id].is_collinear;
                bool const was_outside = m_exit_watcher.is_outside()
                                      && m_exit_watcher.get_exit_operation() == overlay::operation_none;
// TODO: move the above condition into the exit_watcher?

                // to exit we must be currently inside and the current segment must be collinear
                if ( !was_outside && is_collinear )
                {
                    m_exit_watcher.exit(*it, false);
                    // if the position is not set to back it must be a spike
                    if ( it->operations[op_id].position != overlay::position_back )
                    {
                        m_collinear_spike_exit = true;
                    }
                }

                bool const op_blocked = op == overlay::operation_blocked;

                // we're inside and going out from inside
                // possibly going out right now
                if ( ! was_outside && is_collinear )
                {
                    if ( op_blocked
                      && it->operations[op_id].position == overlay::position_back ) // ignore spikes!
                    {
                        // check if this is indeed the boundary point
                        // NOTE: is_ip_on_boundary<>() should be called here but the result will be the same
                        if ( is_endpoint_on_boundary<boundary_back>(it->point, boundary_checker) )
                        {
                            // may be front and back
                            bool const other_b = is_ip_on_boundary<boundary_any>(it->point,
                                                                                 it->operations[other_op_id],
                                                                                 other_boundary_checker,
                                                                                 other_id);
                            // it's also the boundary of the other geometry
                            if ( other_b )
                            {
                                update<boundary, boundary, '0', transpose_result>(res);
                            }
                            else
                            {
                                update<boundary, interior, '0', transpose_result>(res);
                            }
                        }
                    }
                }
                // we're outside or intersects some segment from the outside
                else
                {
                    // if we are truly outside
                    if ( was_outside
                      && it->operations[op_id].position != overlay::position_front
                      && ! m_collinear_spike_exit
                      /*&& !is_collinear*/ )
                    {
                        update<interior, exterior, '1', transpose_result>(res);
                    }

                    // boundaries don't overlap - just an optimization
                    if ( it->method == overlay::method_crosses )
                    {
                        // the L1 is going from one side of the L2 to the other through the point
                        update<interior, interior, '0', transpose_result>(res);

                        // it's the first point in range
                        if ( first_in_range )
                        {
                            bool const front_b = is_endpoint_on_boundary<boundary_front>(
                                                    range::front(sub_range(geometry, seg_id)),
                                                    boundary_checker);

                            // if there is a boundary on the first point
                            if ( front_b )
                            {
                                update<boundary, exterior, '0', transpose_result>(res);
                            }
                        }
                    }
                    // method other than crosses, check more conditions
                    else
                    {
                        bool const this_b = is_ip_on_boundary<boundary_any>(it->point,
                                                                            it->operations[op_id],
                                                                            boundary_checker,
                                                                            seg_id);

                        bool const other_b = is_ip_on_boundary<boundary_any>(it->point,
                                                                             it->operations[other_op_id],
                                                                             other_boundary_checker,
                                                                             other_id);
                        
                        // if current IP is on boundary of the geometry
                        if ( this_b )
                        {
                            // it's also the boundary of the other geometry
                            if ( other_b )
                            {
                                update<boundary, boundary, '0', transpose_result>(res);
                            }
                            else
                            {
                                update<boundary, interior, '0', transpose_result>(res);
                            }
                        }
                        // if current IP is not on boundary of the geometry
                        else
                        {
                            // it's also the boundary of the other geometry
                            if ( other_b )
                            {
                                update<interior, boundary, '0', transpose_result>(res);
                            }
                            else
                            {
                                update<interior, interior, '0', transpose_result>(res);
                            }
                        }

                        // first IP on the last segment point - this means that the first point is outside
                        if ( first_in_range
                          && ( !this_b || op_blocked )
                          && was_outside
                          && it->operations[op_id].position != overlay::position_front
                          && ! m_collinear_spike_exit
                          /*&& !is_collinear*/ )
                        {
                            bool const front_b = is_endpoint_on_boundary<boundary_front>(
                                                    range::front(sub_range(geometry, seg_id)),
                                                    boundary_checker);

                            // if there is a boundary on the first point
                            if ( front_b )
                            {
                                update<boundary, exterior, '0', transpose_result>(res);
                            }
                        }
                            
                    }
                }
            }

            // store ref to previously analysed (valid) turn
            m_previous_turn_ptr = boost::addressof(*it);
            // and previously analysed (valid) operation
            m_previous_operation = op;
        }

        // Called for last
        template <typename Result,
                  typename TurnIt,
                  typename Geometry,
                  typename OtherGeometry,
                  typename BoundaryChecker,
                  typename OtherBoundaryChecker>
        void apply(Result & res,
                   TurnIt first, TurnIt last,
                   Geometry const& geometry,
                   OtherGeometry const& /*other_geometry*/,
                   BoundaryChecker const& boundary_checker,
                   OtherBoundaryChecker const& /*other_boundary_checker*/)
        {
            boost::ignore_unused(first, last);
            //BOOST_GEOMETRY_ASSERT( first != last );

            // here, the possible exit is the real one
            // we know that we entered and now we exit
            if ( /*m_exit_watcher.get_exit_operation() == overlay::operation_union // THIS CHECK IS REDUNDANT
                ||*/ m_previous_operation == overlay::operation_union
                || m_degenerated_turn_ptr )
            {
                update<interior, exterior, '1', transpose_result>(res);

                BOOST_GEOMETRY_ASSERT(first != last);

                const TurnInfo * turn_ptr = NULL;
                if ( m_degenerated_turn_ptr )
                    turn_ptr = m_degenerated_turn_ptr;
                else if ( m_previous_turn_ptr )
                    turn_ptr = m_previous_turn_ptr;
                
                if ( turn_ptr )
                {
                    segment_identifier const& prev_seg_id = turn_ptr->operations[op_id].seg_id;

                    //BOOST_GEOMETRY_ASSERT(!boost::empty(sub_range(geometry, prev_seg_id)));
                    bool const prev_back_b = is_endpoint_on_boundary<boundary_back>(
                                                range::back(sub_range(geometry, prev_seg_id)),
                                                boundary_checker);

                    // if there is a boundary on the last point
                    if ( prev_back_b )
                    {
                        update<boundary, exterior, '0', transpose_result>(res);
                    }
                }
            }

            // Just in case,
            // reset exit watcher before the analysis of the next Linestring
            // note that if there are some enters stored there may be some error above
            m_exit_watcher.reset();

            m_previous_turn_ptr = NULL;
            m_previous_operation = overlay::operation_none;
            m_degenerated_turn_ptr = NULL;

            // actually if this is set to true here there is some error
            // in get_turns_ll or relate_ll, an assert could be checked here
            m_collinear_spike_exit = false;
        }

        template <typename Result,
                  typename Turn,
                  typename Geometry,
                  typename OtherGeometry,
                  typename BoundaryChecker,
                  typename OtherBoundaryChecker>
        void handle_degenerated(Result & res,
                                Turn const& turn,
                                Geometry const& geometry,
                                OtherGeometry const& other_geometry,
                                BoundaryChecker const& boundary_checker,
                                OtherBoundaryChecker const& other_boundary_checker,
                                bool first_in_range)
        {
            typename detail::single_geometry_return_type<Geometry const>::type
                ls1_ref = detail::single_geometry(geometry, turn.operations[op_id].seg_id);
            typename detail::single_geometry_return_type<OtherGeometry const>::type
                ls2_ref = detail::single_geometry(other_geometry, turn.operations[other_op_id].seg_id);

            // only one of those should be true:

            if ( turn.operations[op_id].position == overlay::position_front )
            {
                // valid, point-sized
                if ( boost::size(ls2_ref) == 2 )
                {
                    bool const front_b = is_endpoint_on_boundary<boundary_front>(turn.point, boundary_checker);

                    if ( front_b )
                    {
                        update<boundary, interior, '0', transpose_result>(res);
                    }
                    else
                    {
                        update<interior, interior, '0', transpose_result>(res);
                    }

                    // operation 'c' should be last for the same IP so we know that the next point won't be the same
                    update<interior, exterior, '1', transpose_result>(res);

                    m_degenerated_turn_ptr = boost::addressof(turn);
                }
            }
            else if ( turn.operations[op_id].position == overlay::position_back )
            {
                // valid, point-sized
                if ( boost::size(ls2_ref) == 2 )
                {
                    update<interior, exterior, '1', transpose_result>(res);

                    bool const back_b = is_endpoint_on_boundary<boundary_back>(turn.point, boundary_checker);

                    if ( back_b )
                    {
                        update<boundary, interior, '0', transpose_result>(res);
                    }
                    else
                    {
                        update<interior, interior, '0', transpose_result>(res);
                    }

                    if ( first_in_range )
                    {
                        //BOOST_GEOMETRY_ASSERT(!boost::empty(ls1_ref));
                        bool const front_b = is_endpoint_on_boundary<boundary_front>(
                                                range::front(ls1_ref), boundary_checker);
                        if ( front_b )
                        {
                            update<boundary, exterior, '0', transpose_result>(res);
                        }
                    }
                }
            }
            else if ( turn.operations[op_id].position == overlay::position_middle
                   && turn.operations[other_op_id].position == overlay::position_middle )
            {
                update<interior, interior, '0', transpose_result>(res);

                // here we don't know which one is degenerated

                bool const is_point1 = boost::size(ls1_ref) == 2
                                    && equals::equals_point_point(range::front(ls1_ref),
                                                                  range::back(ls1_ref),
                                                                  boundary_checker.strategy());
                bool const is_point2 = boost::size(ls2_ref) == 2
                                    && equals::equals_point_point(range::front(ls2_ref),
                                                                  range::back(ls2_ref),
                                                                  other_boundary_checker.strategy());

                // if the second one is degenerated
                if ( !is_point1 && is_point2 )
                {
                    update<interior, exterior, '1', transpose_result>(res);

                    if ( first_in_range )
                    {
                        //BOOST_GEOMETRY_ASSERT(!boost::empty(ls1_ref));
                        bool const front_b = is_endpoint_on_boundary<boundary_front>(
                                                range::front(ls1_ref), boundary_checker);
                        if ( front_b )
                        {
                            update<boundary, exterior, '0', transpose_result>(res);
                        }
                    }

                    m_degenerated_turn_ptr = boost::addressof(turn);
                }
            }

            // NOTE: other.position == front and other.position == back
            //       will be handled later, for the other geometry
        }

    private:
        exit_watcher<TurnInfo, OpId> m_exit_watcher;
        segment_watcher<same_single> m_seg_watcher;
        const TurnInfo * m_previous_turn_ptr;
        overlay::operation_type m_previous_operation;
        const TurnInfo * m_degenerated_turn_ptr;
        bool m_collinear_spike_exit;
    };

    template <typename Result,
              typename TurnIt,
              typename Analyser,
              typename Geometry,
              typename OtherGeometry,
              typename BoundaryChecker,
              typename OtherBoundaryChecker>
    static inline void analyse_each_turn(Result & res,
                                         Analyser & analyser,
                                         TurnIt first, TurnIt last,
                                         Geometry const& geometry,
                                         OtherGeometry const& other_geometry,
                                         BoundaryChecker const& boundary_checker,
                                         OtherBoundaryChecker const& other_boundary_checker)
    {
        if ( first == last )
            return;

        for ( TurnIt it = first ; it != last ; ++it )
        {
            analyser.apply(res, it,
                           geometry, other_geometry,
                           boundary_checker, other_boundary_checker);

            if ( BOOST_GEOMETRY_CONDITION( res.interrupt ) )
                return;
        }

        analyser.apply(res, first, last,
                       geometry, other_geometry,
                       boundary_checker, other_boundary_checker);
    }
};

}} // namespace detail::relate
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_LINEAR_LINEAR_HPP
