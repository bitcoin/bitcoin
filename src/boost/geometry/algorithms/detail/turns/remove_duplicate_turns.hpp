// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle


#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_REMOVE_DUPLICATE_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_REMOVE_DUPLICATE_TURNS_HPP

#include <algorithm>
#include <boost/geometry/algorithms/equals.hpp>

namespace boost { namespace geometry
{

namespace detail { namespace turns
{

template <typename Turns, bool Enable>
struct remove_duplicate_turns
{
    static inline void apply(Turns&) {}
};



template <typename Turns>
class remove_duplicate_turns<Turns, true>
{
private:
    struct TurnEqualsTo
    {
        template <typename Turn>
        bool operator()(Turn const& t1, Turn const& t2) const
        {
            return geometry::equals(t1.point, t2.point)
                && t1.operations[0].seg_id == t2.operations[0].seg_id
                && t1.operations[1].seg_id == t2.operations[1].seg_id;
        }
    };

public:
    static inline void apply(Turns& turns)
    {
        turns.erase( std::unique(turns.begin(), turns.end(),
                                 TurnEqualsTo()),
                     turns.end()
                     );
    }
};



}} // namespace detail::turns

}} // namespect boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_REMOVE_DUPLICATE_TURNS_HPP
