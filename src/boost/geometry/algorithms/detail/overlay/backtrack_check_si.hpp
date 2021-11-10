// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_BACKTRACK_CHECK_SI_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_BACKTRACK_CHECK_SI_HPP

#include <cstddef>
#include <string>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/has_self_intersections.hpp>
#if defined(BOOST_GEOMETRY_DEBUG_INTERSECTION) || defined(BOOST_GEOMETRY_OVERLAY_REPORT_WKT)
#  include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#  include <boost/geometry/io/wkt/wkt.hpp>
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template <typename Turns>
inline void clear_visit_info(Turns& turns)
{
    typedef typename boost::range_value<Turns>::type tp_type;

    for (typename boost::range_iterator<Turns>::type
        it = boost::begin(turns);
        it != boost::end(turns);
        ++it)
    {
        for (typename boost::range_iterator
            <
                typename tp_type::container_type
            >::type op_it = boost::begin(it->operations);
            op_it != boost::end(it->operations);
            ++op_it)
        {
            op_it->visited.clear();
        }
    }
}

struct backtrack_state
{
    bool m_good;

    inline backtrack_state() : m_good(true) {}
    inline void reset() { m_good = true; }
    inline bool good() const { return m_good; }
};


enum traverse_error_type
{
    traverse_error_none,
    traverse_error_no_next_ip_at_start,
    traverse_error_no_next_ip,
    traverse_error_dead_end_at_start,
    traverse_error_dead_end,
    traverse_error_visit_again,
    traverse_error_endless_loop
};

inline std::string traverse_error_string(traverse_error_type error)
{
    switch (error)
    {
        case traverse_error_none : return "";
        case traverse_error_no_next_ip_at_start : return "No next IP at start";
        case traverse_error_no_next_ip : return "No next IP";
        case traverse_error_dead_end_at_start : return "Dead end at start";
        case traverse_error_dead_end : return "Dead end";
        case traverse_error_visit_again : return "Visit again";
        case traverse_error_endless_loop : return "Endless loop";
        default : return "";
    }
    return "";
}


template
<
    typename Geometry1,
    typename Geometry2
>
class backtrack_check_self_intersections
{
    struct state : public backtrack_state
    {
        bool m_checked;
        inline state()
            : m_checked(true)
        {}
    };
public :
    typedef state state_type;

    template
    <
        typename Operation,
        typename Rings, typename Ring, typename Turns,
        typename Strategy,
        typename RobustPolicy,
        typename Visitor
    >
    static inline void apply(std::size_t size_at_start,
                             Rings& rings,
                             Ring& ring,
                             Turns& turns,
                             typename boost::range_value<Turns>::type const& turn,
                             Operation& operation,
                             traverse_error_type traverse_error,
                             Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Strategy const& strategy,
                             RobustPolicy const& robust_policy,
                             state_type& state,
                             Visitor& visitor)
    {
        visitor.visit_traverse_reject(turns, turn, operation, traverse_error);

        state.m_good = false;

        // Check self-intersections and throw exception if appropriate
        if (! state.m_checked)
        {
            state.m_checked = true;
            has_self_intersections(geometry1, strategy, robust_policy);
            has_self_intersections(geometry2, strategy, robust_policy);
        }

        // Make bad output clean
        rings.resize(size_at_start);
        geometry::traits::clear<typename boost::range_value<Rings>::type>::apply(ring);

        // Reject this as a starting point
        operation.visited.set_rejected();

        // And clear all visit info
        clear_visit_info(turns);
    }
};

#ifdef BOOST_GEOMETRY_OVERLAY_REPORT_WKT
template
<
    typename Geometry1,
    typename Geometry2
>
class backtrack_debug
{
public :
    typedef backtrack_state state_type;

    template <typename Operation, typename Rings, typename Turns>
    static inline void apply(std::size_t size_at_start,
                Rings& rings, typename boost::range_value<Rings>::type& ring,
                Turns& turns, Operation& operation,
                std::string const& reason,
                Geometry1 const& geometry1,
                Geometry2 const& geometry2,
                state_type& state
                )
    {
        std::cout << " REJECT " << reason << std::endl;

        state.m_good = false;

        rings.resize(size_at_start);
        ring.clear();
        operation.visited.set_rejected();
        clear_visit_info(turns);

        int c = 0;
        for (int i = 0; i < turns.size(); i++)
        {
            for (int j = 0; j < 2; j++)
            {
                if (turns[i].operations[j].visited.rejected())
                {
                    c++;
                }
            }
        }
        std::cout << "BACKTRACK (" << reason << " )"
            << " " << c << " of " << turns.size() << " rejected"
            << std::endl;
        std::cout
            << geometry::wkt(geometry1) << std::endl
            << geometry::wkt(geometry2) << std::endl;
    }
};
#endif // BOOST_GEOMETRY_OVERLAY_REPORT_WKT

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_BACKTRACK_CHECK_SI_HPP
