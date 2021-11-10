// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_DEBUG_TURN_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_DEBUG_TURN_HPP

#ifdef BOOST_GEOMETRY_DEBUG_TURNS
#include <iostream>
#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include <boost/geometry/io/wkt/write.hpp>
#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#endif // BOOST_GEOMETRY_DEBUG_TURNS


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace turns
{


#ifdef BOOST_GEOMETRY_DEBUG_TURNS
template <typename Turn, typename Operation>
inline void debug_turn(Turn const& turn, Operation op,
                       std::string const& header)
{
    std::cout << header
        << " at " << op.seg_id
        << " meth: " << method_char(turn.method)
        << " op: " << operation_char(op.operation)
        << " of: " << operation_char(turn.operations[0].operation)
        << operation_char(turn.operations[1].operation)
        << " " << geometry::wkt(turn.point)
        << std::endl;

    if (boost::contains(header, "Finished"))
    {
        std::cout << std::endl;
    }
}
#else
template <typename Turn, typename Operation>
inline void debug_turn(Turn const& , Operation, const char*)
{
}
#endif // BOOST_GEOMETRY_DEBUG_TURNS


}} // namespace detail::turns
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost:geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_DEBUG_TURN_HPP
