// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2018.
// Modifications copyright (c) 2018 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CHECK_ENRICH_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CHECK_ENRICH_HPP

#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
#include <iostream>
#endif // BOOST_GEOMETRY_DEBUG_ENRICH

#include <cstddef>
#include <vector>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template<typename Turn>
struct meta_turn
{
    int index;
    Turn const* turn;
    bool handled[2];

    inline meta_turn(int i, Turn const& t)
        : index(i), turn(&t)
    {
        handled[0] = false;
        handled[1] = false;
    }
};


template <typename MetaTurn>
inline void display(MetaTurn const& meta_turn, const char* reason = "")
{
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    std::cout << meta_turn.index
        << "\tMethods: " << method_char(meta_turn.turn->method)
        << " operations: "  << operation_char(meta_turn.turn->operations[0].operation)
                << operation_char(meta_turn.turn->operations[1].operation)
        << " travels to " << meta_turn.turn->operations[0].enriched.travels_to_ip_index
        << " and " << meta_turn.turn->operations[1].enriched.travels_to_ip_index
        //<< " -> " << op_index
        << " " << reason
        << std::endl;
#endif
}


template <typename MetaTurns, typename MetaTurn>
inline void check_detailed(MetaTurns& meta_turns, MetaTurn const& meta_turn,
            int op_index, int cycle, int start, operation_type for_operation,
            bool& error)
{
    display(meta_turn);
    int const ip_index = meta_turn.turn->operations[op_index].enriched.travels_to_ip_index;
    if (ip_index >= 0)
    {
        bool found = false;

        if (ip_index == start)
        {
            display(meta_turns[ip_index], " FINISH");
            return;
        }

        // check on continuing, or on same-operation-on-same-geometry
        if (! meta_turns[ip_index].handled[op_index]
            && (meta_turns[ip_index].turn->operations[op_index].operation == operation_continue
                || meta_turns[ip_index].turn->operations[op_index].operation == for_operation)
            )
        {
            meta_turns[ip_index].handled[op_index] = true;
            check_detailed(meta_turns, meta_turns[ip_index], op_index, cycle, start, for_operation, error);
            found = true;
        }
        // check on other geometry
        if (! found)
        {
            int const other_index = 1 - op_index;
            if (! meta_turns[ip_index].handled[other_index]
                && meta_turns[ip_index].turn->operations[other_index].operation == for_operation)
            {
                meta_turns[ip_index].handled[other_index] = true;
                check_detailed(meta_turns, meta_turns[ip_index], other_index, cycle, start, for_operation, error);
                found = true;
            }
        }

        if (! found)
        {
            display(meta_turns[ip_index], " STOP");
            error = true;
#ifndef BOOST_GEOMETRY_DEBUG_ENRICH
            //std::cout << " STOP";
#endif
        }
    }
}


template <typename TurnPoints>
inline bool check_graph(TurnPoints& turn_points, operation_type for_operation)
{
    typedef typename boost::range_value<TurnPoints>::type turn_point_type;

    bool error = false;
    int index = 0;

    std::vector<meta_turn<turn_point_type> > meta_turns;
    for (typename boost::range_iterator<TurnPoints const>::type
            it = boost::begin(turn_points);
         it != boost::end(turn_points);
         ++it, ++index)
    {
        meta_turns.push_back(meta_turn<turn_point_type>(index, *it));
    }

    int cycle = 0;
    for (typename boost::range_iterator<std::vector<meta_turn<turn_point_type> > > ::type
            it = boost::begin(meta_turns);
         it != boost::end(meta_turns);
         ++it)
    {
        if (! (it->turn->blocked() || it->turn->discarded))
        {
            for (int i = 0 ; i < 2; i++)
            {
                if (! it->handled[i]
                    && it->turn->operations[i].operation == for_operation)
                {
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
                    std::cout << "CYCLE " << cycle << std::endl;
#endif
                    it->handled[i] = true;
                    check_detailed(meta_turns, *it, i, cycle++, it->index, for_operation, error);
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
                    std::cout <<" END CYCLE " << it->index << std::endl;
#endif
                }
            }
        }
    }
    return error;
}



}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CHECK_ENRICH_HPP
