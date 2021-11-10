// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013, 2014, 2015, 2017, 2018, 2019.
// Modifications copyright (c) 2013-2019 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_AREAL_AREAL_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_AREAL_AREAL_HPP

#include <boost/geometry/core/topological_dimension.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/range.hpp>

#include <boost/geometry/algorithms/num_interior_rings.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/sub_range.hpp>
#include <boost/geometry/algorithms/detail/single_geometry.hpp>

#include <boost/geometry/algorithms/detail/relate/point_geometry.hpp>
#include <boost/geometry/algorithms/detail/relate/turns.hpp>
#include <boost/geometry/algorithms/detail/relate/boundary_checker.hpp>
#include <boost/geometry/algorithms/detail/relate/follow_helpers.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace relate {
    
// WARNING!
// TODO: In the worst case calling this Pred in a loop for MultiPolygon/MultiPolygon may take O(NM)
// Use the rtree in this case!

// may be used to set EI and EB for an Areal geometry for which no turns were generated
template
<
    typename OtherAreal,
    typename Result,
    typename PointInArealStrategy,
    bool TransposeResult
>
class no_turns_aa_pred
{
public:
    no_turns_aa_pred(OtherAreal const& other_areal,
                     Result & res,
                     PointInArealStrategy const& point_in_areal_strategy)
        : m_result(res)
        , m_point_in_areal_strategy(point_in_areal_strategy)
        , m_other_areal(other_areal)
        , m_flags(0)
    {
        // check which relations must be analysed

        if ( ! may_update<interior, interior, '2', TransposeResult>(m_result)
          && ! may_update<boundary, interior, '1', TransposeResult>(m_result)
          && ! may_update<exterior, interior, '2', TransposeResult>(m_result) )
        {
            m_flags |= 1;
        }

        if ( ! may_update<interior, exterior, '2', TransposeResult>(m_result)
          && ! may_update<boundary, exterior, '1', TransposeResult>(m_result) )
        {
            m_flags |= 2;
        }
    }

    template <typename Areal>
    bool operator()(Areal const& areal)
    {
        using detail::within::point_in_geometry;

        // if those flags are set nothing will change
        if ( m_flags == 3 )
        {
            return false;
        }

        typedef typename geometry::point_type<Areal>::type point_type;
        point_type pt;
        bool const ok = boost::geometry::point_on_border(pt, areal);

        // TODO: for now ignore, later throw an exception?
        if ( !ok )
        {
            return true;
        }

        // check if the areal is inside the other_areal
        // TODO: This is O(N)
        // Run in a loop O(NM) - optimize!
        int const pig = point_in_geometry(pt,
                                          m_other_areal,
                                          m_point_in_areal_strategy);
        //BOOST_GEOMETRY_ASSERT( pig != 0 );
        
        // inside
        if ( pig > 0 )
        {
            update<interior, interior, '2', TransposeResult>(m_result);
            update<boundary, interior, '1', TransposeResult>(m_result);
            update<exterior, interior, '2', TransposeResult>(m_result);
            m_flags |= 1;

            // TODO: OPTIMIZE!
            // Only the interior rings of other ONE single geometry must be checked
            // NOT all geometries

            // Check if any interior ring is outside
            ring_identifier ring_id(0, -1, 0);
            std::size_t const irings_count = geometry::num_interior_rings(areal);
            for ( ; static_cast<std::size_t>(ring_id.ring_index) < irings_count ;
                    ++ring_id.ring_index )
            {
                typename detail::sub_range_return_type<Areal const>::type
                    range_ref = detail::sub_range(areal, ring_id);

                if ( boost::empty(range_ref) )
                {
                    // TODO: throw exception?
                    continue; // ignore
                }

                // TODO: O(N)
                // Optimize!
                int const hpig = point_in_geometry(range::front(range_ref),
                                                   m_other_areal,
                                                   m_point_in_areal_strategy);

                // hole outside
                if ( hpig < 0 )
                {
                    update<interior, exterior, '2', TransposeResult>(m_result);
                    update<boundary, exterior, '1', TransposeResult>(m_result);
                    m_flags |= 2;
                    break;
                }
            }
        }
        // outside
        else
        {
            update<interior, exterior, '2', TransposeResult>(m_result);
            update<boundary, exterior, '1', TransposeResult>(m_result);
            m_flags |= 2;

            // Check if any interior ring is inside
            ring_identifier ring_id(0, -1, 0);
            std::size_t const irings_count = geometry::num_interior_rings(areal);
            for ( ; static_cast<std::size_t>(ring_id.ring_index) < irings_count ;
                    ++ring_id.ring_index )
            {
                typename detail::sub_range_return_type<Areal const>::type
                    range_ref = detail::sub_range(areal, ring_id);

                if ( boost::empty(range_ref) )
                {
                    // TODO: throw exception?
                    continue; // ignore
                }

                // TODO: O(N)
                // Optimize!
                int const hpig = point_in_geometry(range::front(range_ref),
                                                   m_other_areal,
                                                   m_point_in_areal_strategy);

                // hole inside
                if ( hpig > 0 )
                {
                    update<interior, interior, '2', TransposeResult>(m_result);
                    update<boundary, interior, '1', TransposeResult>(m_result);
                    update<exterior, interior, '2', TransposeResult>(m_result);
                    m_flags |= 1;
                    break;
                }
            }
        }
                    
        return m_flags != 3 && !m_result.interrupt;
    }

private:
    Result & m_result;
    PointInArealStrategy const& m_point_in_areal_strategy;
    OtherAreal const& m_other_areal;
    int m_flags;
};

// The implementation of an algorithm calculating relate() for A/A
template <typename Geometry1, typename Geometry2>
struct areal_areal
{
    // check Linear / Areal
    BOOST_STATIC_ASSERT(topological_dimension<Geometry1>::value == 2
                     && topological_dimension<Geometry2>::value == 2);

    static const bool interruption_enabled = true;

    typedef typename geometry::point_type<Geometry1>::type point1_type;
    typedef typename geometry::point_type<Geometry2>::type point2_type;
    
    template <typename Result, typename Strategy>
    static inline void apply(Geometry1 const& geometry1, Geometry2 const& geometry2,
                             Result & result,
                             Strategy const& strategy)
    {
// TODO: If Areal geometry may have infinite size, change the following line:

        // The result should be FFFFFFFFF
        relate::set<exterior, exterior, result_dimension<Geometry2>::value>(result);// FFFFFFFFd, d in [1,9] or T

        if ( BOOST_GEOMETRY_CONDITION(result.interrupt) )
            return;

        // get and analyse turns
        typedef typename turns::get_turns
            <
                Geometry1, Geometry2
            >::template turn_info_type<Strategy>::type turn_type;
        std::vector<turn_type> turns;

        interrupt_policy_areal_areal<Result> interrupt_policy(geometry1, geometry2, result);

        turns::get_turns<Geometry1, Geometry2>::apply(turns, geometry1, geometry2, interrupt_policy, strategy);
        if ( BOOST_GEOMETRY_CONDITION(result.interrupt) )
            return;

        typedef typename Strategy::cs_tag cs_tag;

        no_turns_aa_pred<Geometry2, Result, Strategy, false>
            pred1(geometry2, result, strategy);
        for_each_disjoint_geometry_if<0, Geometry1>::apply(turns.begin(), turns.end(), geometry1, pred1);
        if ( BOOST_GEOMETRY_CONDITION(result.interrupt) )
            return;

        no_turns_aa_pred<Geometry1, Result, Strategy, true>
            pred2(geometry1, result, strategy);
        for_each_disjoint_geometry_if<1, Geometry2>::apply(turns.begin(), turns.end(), geometry2, pred2);
        if ( BOOST_GEOMETRY_CONDITION(result.interrupt) )
            return;
        
        if ( turns.empty() )
            return;

        if ( may_update<interior, interior, '2'>(result)
          || may_update<interior, exterior, '2'>(result)
          || may_update<boundary, interior, '1'>(result)
          || may_update<boundary, exterior, '1'>(result)
          || may_update<exterior, interior, '2'>(result) )
        {
            // sort turns
            typedef turns::less<0, turns::less_op_areal_areal<0>, cs_tag> less;
            std::sort(turns.begin(), turns.end(), less());

            /*if ( may_update<interior, exterior, '2'>(result)
              || may_update<boundary, exterior, '1'>(result)
              || may_update<boundary, interior, '1'>(result)
              || may_update<exterior, interior, '2'>(result) )*/
            {
                // analyse sorted turns
                turns_analyser<turn_type, 0> analyser;
                analyse_each_turn(result, analyser, turns.begin(), turns.end(), strategy);

                if ( BOOST_GEOMETRY_CONDITION(result.interrupt) )
                    return;
            }

            if ( may_update<interior, interior, '2'>(result)
              || may_update<interior, exterior, '2'>(result)
              || may_update<boundary, interior, '1'>(result)
              || may_update<boundary, exterior, '1'>(result)
              || may_update<exterior, interior, '2'>(result) )
            {
                // analyse rings for which turns were not generated
                // or only i/i or u/u was generated
                uncertain_rings_analyser<0, Result, Geometry1, Geometry2, Strategy>
                    rings_analyser(result, geometry1, geometry2, strategy);
                analyse_uncertain_rings<0>::apply(rings_analyser, turns.begin(), turns.end());

                if ( BOOST_GEOMETRY_CONDITION(result.interrupt) )
                    return;
            }
        }

        if ( may_update<interior, interior, '2', true>(result)
          || may_update<interior, exterior, '2', true>(result)
          || may_update<boundary, interior, '1', true>(result)
          || may_update<boundary, exterior, '1', true>(result)
          || may_update<exterior, interior, '2', true>(result) )
        {
            // sort turns
            typedef turns::less<1, turns::less_op_areal_areal<1>, cs_tag> less;
            std::sort(turns.begin(), turns.end(), less());

            /*if ( may_update<interior, exterior, '2', true>(result)
              || may_update<boundary, exterior, '1', true>(result)
              || may_update<boundary, interior, '1', true>(result)
              || may_update<exterior, interior, '2', true>(result) )*/
            {
                // analyse sorted turns
                turns_analyser<turn_type, 1> analyser;
                analyse_each_turn(result, analyser, turns.begin(), turns.end(), strategy);

                if ( BOOST_GEOMETRY_CONDITION(result.interrupt) )
                    return;
            }

            if ( may_update<interior, interior, '2', true>(result)
              || may_update<interior, exterior, '2', true>(result)
              || may_update<boundary, interior, '1', true>(result)
              || may_update<boundary, exterior, '1', true>(result)
              || may_update<exterior, interior, '2', true>(result) )
            {
                // analyse rings for which turns were not generated
                // or only i/i or u/u was generated
                uncertain_rings_analyser<1, Result, Geometry2, Geometry1, Strategy>
                    rings_analyser(result, geometry2, geometry1, strategy);
                analyse_uncertain_rings<1>::apply(rings_analyser, turns.begin(), turns.end());

                //if ( result.interrupt )
                //    return;
            }
        }
    }

    // interrupt policy which may be passed to get_turns to interrupt the analysis
    // based on the info in the passed result/mask
    template <typename Result>
    class interrupt_policy_areal_areal
    {
    public:
        static bool const enabled = true;

        interrupt_policy_areal_areal(Geometry1 const& geometry1,
                                     Geometry2 const& geometry2,
                                     Result & result)
            : m_result(result)
            , m_geometry1(geometry1)
            , m_geometry2(geometry2)
        {}

        template <typename Range>
        inline bool apply(Range const& turns)
        {
            typedef typename boost::range_iterator<Range const>::type iterator;
            
            for (iterator it = boost::begin(turns) ; it != boost::end(turns) ; ++it)
            {
                per_turn<0>(*it);
                per_turn<1>(*it);
            }

            return m_result.interrupt;
        }

    private:
        template <std::size_t OpId, typename Turn>
        inline void per_turn(Turn const& turn)
        {
            //static const std::size_t other_op_id = (OpId + 1) % 2;
            static const bool transpose_result = OpId != 0;

            overlay::operation_type const op = turn.operations[OpId].operation;

            if ( op == overlay::operation_union )
            {
                // ignore u/u
                /*if ( turn.operations[other_op_id].operation != overlay::operation_union )
                {
                    update<interior, exterior, '2', transpose_result>(m_result);
                    update<boundary, exterior, '1', transpose_result>(m_result);
                }*/

                update<boundary, boundary, '0', transpose_result>(m_result);
            }
            else if ( op == overlay::operation_intersection )
            {
                // ignore i/i
                /*if ( turn.operations[other_op_id].operation != overlay::operation_intersection )
                {
                    // not correct e.g. for G1 touching G2 in a point where a hole is touching the exterior ring
                    // in this case 2 turns i/... and u/u will be generated for this IP
                    //update<interior, interior, '2', transpose_result>(m_result);

                    //update<boundary, interior, '1', transpose_result>(m_result);
                }*/

                update<boundary, boundary, '0', transpose_result>(m_result);
            }
            else if ( op == overlay::operation_continue )
            {
                update<boundary, boundary, '1', transpose_result>(m_result);
                update<interior, interior, '2', transpose_result>(m_result);
            }
            else if ( op == overlay::operation_blocked )
            {
                update<boundary, boundary, '1', transpose_result>(m_result);
                update<interior, exterior, '2', transpose_result>(m_result);
            }
        }

        Result & m_result;
        Geometry1 const& m_geometry1;
        Geometry2 const& m_geometry2;
    };

    // This analyser should be used like Input or SinglePass Iterator
    // IMPORTANT! It should be called also for the end iterator - last
    template <typename TurnInfo, std::size_t OpId>
    class turns_analyser
    {
        typedef typename TurnInfo::point_type turn_point_type;

        static const std::size_t op_id = OpId;
        static const std::size_t other_op_id = (OpId + 1) % 2;
        static const bool transpose_result = OpId != 0;

    public:
        turns_analyser()
            : m_previous_turn_ptr(0)
            , m_previous_operation(overlay::operation_none)
            , m_enter_detected(false)
            , m_exit_detected(false)
        {}

        template <typename Result, typename TurnIt, typename EqPPStrategy>
        void apply(Result & result, TurnIt it, EqPPStrategy const& strategy)
        {
            //BOOST_GEOMETRY_ASSERT( it != last );

            overlay::operation_type const op = it->operations[op_id].operation;

            if ( op != overlay::operation_union
              && op != overlay::operation_intersection
              && op != overlay::operation_blocked
              && op != overlay::operation_continue )
            {
                return;
            }

            segment_identifier const& seg_id = it->operations[op_id].seg_id;
            //segment_identifier const& other_id = it->operations[other_op_id].seg_id;

            const bool first_in_range = m_seg_watcher.update(seg_id);

            if ( m_previous_turn_ptr )
            {
                if ( m_exit_detected /*m_previous_operation == overlay::operation_union*/ )
                {
                    // real exit point - may be multiple
                    if ( first_in_range
                      || ! turn_on_the_same_ip<op_id>(*m_previous_turn_ptr, *it, strategy) )
                    {
                        update_exit(result);
                        m_exit_detected = false;
                    }
                    // fake exit point, reset state
                    else if ( op != overlay::operation_union )
                    {
                        m_exit_detected = false;
                    }
                }                
                /*else*/
                if ( m_enter_detected /*m_previous_operation == overlay::operation_intersection*/ )
                {
                    // real entry point
                    if ( first_in_range
                      || ! turn_on_the_same_ip<op_id>(*m_previous_turn_ptr, *it, strategy) )
                    {
                        update_enter(result);
                        m_enter_detected = false;
                    }
                    // fake entry point, reset state
                    else if ( op != overlay::operation_intersection )
                    {
                        m_enter_detected = false;
                    }
                }
            }

            if ( op == overlay::operation_union )
            {
                // already set in interrupt policy
                //update<boundary, boundary, '0', transpose_result>(m_result);

                // ignore u/u
                //if ( it->operations[other_op_id].operation != overlay::operation_union )
                {
                    m_exit_detected = true;
                }
            }
            else if ( op == overlay::operation_intersection )
            {
                // ignore i/i
                if ( it->operations[other_op_id].operation != overlay::operation_intersection )
                {
                    // this was set in the interrupt policy but it was wrong
                    // also here it's wrong since it may be a fake entry point
                    //update<interior, interior, '2', transpose_result>(result);

                    // already set in interrupt policy
                    //update<boundary, boundary, '0', transpose_result>(result);
                    m_enter_detected = true;
                }
            }
            else if ( op == overlay::operation_blocked )
            {
                // already set in interrupt policy
            }
            else // if ( op == overlay::operation_continue )
            {
                // already set in interrupt policy
            }

            // store ref to previously analysed (valid) turn
            m_previous_turn_ptr = boost::addressof(*it);
            // and previously analysed (valid) operation
            m_previous_operation = op;
        }

        // it == last
        template <typename Result>
        void apply(Result & result)
        {
            //BOOST_GEOMETRY_ASSERT( first != last );

            if ( m_exit_detected /*m_previous_operation == overlay::operation_union*/ )
            {
                update_exit(result);
                m_exit_detected = false;
            }

            if ( m_enter_detected /*m_previous_operation == overlay::operation_intersection*/ )
            {
                update_enter(result);
                m_enter_detected = false;
            }
        }

        template <typename Result>
        static inline void update_exit(Result & result)
        {
            update<interior, exterior, '2', transpose_result>(result);
            update<boundary, exterior, '1', transpose_result>(result);
        }

        template <typename Result>
        static inline void update_enter(Result & result)
        {
            update<interior, interior, '2', transpose_result>(result);
            update<boundary, interior, '1', transpose_result>(result);
            update<exterior, interior, '2', transpose_result>(result);
        }

    private:
        segment_watcher<same_ring> m_seg_watcher;
        TurnInfo * m_previous_turn_ptr;
        overlay::operation_type m_previous_operation;
        bool m_enter_detected;
        bool m_exit_detected;
    };

    // call analyser.apply() for each turn in range
    // IMPORTANT! The analyser is also called for the end iterator - last
    template
    <
        typename Result,
        typename Analyser,
        typename TurnIt,
        typename EqPPStrategy
    >
    static inline void analyse_each_turn(Result & res,
                                         Analyser & analyser,
                                         TurnIt first, TurnIt last,
                                         EqPPStrategy const& strategy)
    {
        if ( first == last )
            return;

        for ( TurnIt it = first ; it != last ; ++it )
        {
            analyser.apply(res, it, strategy);

            if ( BOOST_GEOMETRY_CONDITION(res.interrupt) )
                return;
        }

        analyser.apply(res);
    }

    template
    <
        std::size_t OpId,
        typename Result,
        typename Geometry,
        typename OtherGeometry,
        typename PointInArealStrategy
    >
    class uncertain_rings_analyser
    {
        static const bool transpose_result = OpId != 0;
        static const int other_id = (OpId + 1) % 2;

    public:
        inline uncertain_rings_analyser(Result & result,
                                        Geometry const& geom,
                                        OtherGeometry const& other_geom,
                                        PointInArealStrategy const& point_in_areal_strategy)
            : geometry(geom)
            , other_geometry(other_geom)
            , interrupt(result.interrupt) // just in case, could be false as well
            , m_result(result)
            , m_point_in_areal_strategy(point_in_areal_strategy)
            , m_flags(0)
        {
            // check which relations must be analysed
            // NOTE: 1 and 4 could probably be connected

            if ( ! may_update<interior, interior, '2', transpose_result>(m_result) )
            {
                m_flags |= 1;
            }

            if ( ! may_update<interior, exterior, '2', transpose_result>(m_result)
              && ! may_update<boundary, exterior, '1', transpose_result>(m_result) )
            {
                m_flags |= 2;
            }

            if ( ! may_update<boundary, interior, '1', transpose_result>(m_result)
              && ! may_update<exterior, interior, '2', transpose_result>(m_result) )
            {
                m_flags |= 4;
            }
        }

        inline void no_turns(segment_identifier const& seg_id)
        {
            // if those flags are set nothing will change
            if ( m_flags == 7 )
            {
                return;
            }

            typename detail::sub_range_return_type<Geometry const>::type
                range_ref = detail::sub_range(geometry, seg_id);

            if ( boost::empty(range_ref) )
            {
                // TODO: throw an exception?
                return; // ignore
            }
                
            // TODO: possible optimization
            // if the range is an interior ring we may use other IPs generated for this single geometry
            // to know which other single geometries should be checked

            // TODO: optimize! e.g. use spatial index
            // O(N) - running it in a loop gives O(NM)
            using detail::within::point_in_geometry;
            int const pig = point_in_geometry(range::front(range_ref),
                                              other_geometry,
                                              m_point_in_areal_strategy);

            //BOOST_GEOMETRY_ASSERT(pig != 0);
            if ( pig > 0 )
            {
                update<interior, interior, '2', transpose_result>(m_result);
                m_flags |= 1;

                update<boundary, interior, '1', transpose_result>(m_result);
                update<exterior, interior, '2', transpose_result>(m_result);
                m_flags |= 4;
            }
            else
            {
                update<boundary, exterior, '1', transpose_result>(m_result);
                update<interior, exterior, '2', transpose_result>(m_result);
                m_flags |= 2;
            }

// TODO: break if all things are set
// also some of them could be checked outside, before the analysis
// In this case we shouldn't relay just on dummy flags
// Flags should be initialized with proper values
// or the result should be checked directly
// THIS IS ALSO TRUE FOR OTHER ANALYSERS! in L/L and L/A

            interrupt = m_flags == 7 || m_result.interrupt;
        }

        template <typename TurnIt>
        inline void turns(TurnIt first, TurnIt last)
        {
            // if those flags are set nothing will change
            if ( (m_flags & 6) == 6 )
            {
                return;
            }

            bool found_ii = false;
            bool found_uu = false;

            for ( TurnIt it = first ; it != last ; ++it )
            {
                if ( it->operations[0].operation == overlay::operation_intersection 
                  && it->operations[1].operation == overlay::operation_intersection )
                {
                    found_ii = true;
                }
                else if ( it->operations[0].operation == overlay::operation_union 
                       && it->operations[1].operation == overlay::operation_union )
                {
                    found_uu = true;
                }
                else // ignore
                {
                    return; // don't interrupt
                }
            }

            // only i/i was generated for this ring
            if ( found_ii )
            {
                update<interior, interior, '2', transpose_result>(m_result);
                m_flags |= 1;

                //update<boundary, boundary, '0', transpose_result>(m_result);                

                update<boundary, interior, '1', transpose_result>(m_result);
                update<exterior, interior, '2', transpose_result>(m_result);
                m_flags |= 4;
            }

            // only u/u was generated for this ring
            if ( found_uu )
            {
                update<boundary, exterior, '1', transpose_result>(m_result);
                update<interior, exterior, '2', transpose_result>(m_result);
                m_flags |= 2;
            }

            interrupt = m_flags == 7 || m_result.interrupt; // interrupt if the result won't be changed in the future
        }

        Geometry const& geometry;
        OtherGeometry const& other_geometry;
        bool interrupt;

    private:
        Result & m_result;
        PointInArealStrategy const& m_point_in_areal_strategy;
        int m_flags;
    };

    template <std::size_t OpId>
    class analyse_uncertain_rings
    {
    public:
        template <typename Analyser, typename TurnIt>
        static inline void apply(Analyser & analyser, TurnIt first, TurnIt last)
        {
            if ( first == last )
                return;

            for_preceding_rings(analyser, *first);
            //analyser.per_turn(*first);

            TurnIt prev = first;
            for ( ++first ; first != last ; ++first, ++prev )
            {
                // same multi
                if ( prev->operations[OpId].seg_id.multi_index
                  == first->operations[OpId].seg_id.multi_index )
                {
                    // same ring
                    if ( prev->operations[OpId].seg_id.ring_index
                      == first->operations[OpId].seg_id.ring_index )
                    {
                        //analyser.per_turn(*first);
                    }
                    // same multi, next ring
                    else
                    {
                        //analyser.end_ring(*prev);
                        analyser.turns(prev, first);

                        //if ( prev->operations[OpId].seg_id.ring_index + 1
                        //   < first->operations[OpId].seg_id.ring_index)
                        {
                            for_no_turns_rings(analyser,
                                               *first,
                                               prev->operations[OpId].seg_id.ring_index + 1,
                                               first->operations[OpId].seg_id.ring_index);
                        }

                        //analyser.per_turn(*first);
                    }
                }
                // next multi
                else
                {
                    //analyser.end_ring(*prev);
                    analyser.turns(prev, first);
                    for_following_rings(analyser, *prev);
                    for_preceding_rings(analyser, *first);
                    //analyser.per_turn(*first);
                }

                if ( analyser.interrupt )
                {
                    return;
                }
            }

            //analyser.end_ring(*prev);
            analyser.turns(prev, first); // first == last
            for_following_rings(analyser, *prev);
        }

    private:
        template <typename Analyser, typename Turn>
        static inline void for_preceding_rings(Analyser & analyser, Turn const& turn)
        {
            segment_identifier const& seg_id = turn.operations[OpId].seg_id;

            for_no_turns_rings(analyser, turn, -1, seg_id.ring_index);
        }

        template <typename Analyser, typename Turn>
        static inline void for_following_rings(Analyser & analyser, Turn const& turn)
        {
            segment_identifier const& seg_id = turn.operations[OpId].seg_id;

            signed_size_type
                count = boost::numeric_cast<signed_size_type>(
                            geometry::num_interior_rings(
                                detail::single_geometry(analyser.geometry, seg_id)));
            
            for_no_turns_rings(analyser, turn, seg_id.ring_index + 1, count);
        }

        template <typename Analyser, typename Turn>
        static inline void for_no_turns_rings(Analyser & analyser,
                                              Turn const& turn,
                                              signed_size_type first,
                                              signed_size_type last)
        {
            segment_identifier seg_id = turn.operations[OpId].seg_id;

            for ( seg_id.ring_index = first ; seg_id.ring_index < last ; ++seg_id.ring_index )
            {
                analyser.no_turns(seg_id);
            }
        }
    };
};

}} // namespace detail::relate
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_AREAL_AREAL_HPP
