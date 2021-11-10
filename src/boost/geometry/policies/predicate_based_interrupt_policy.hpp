// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_POLICIES_PREDICATE_BASED_INTERRUPT_POLICY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_POLICIES_PREDICATE_BASED_INTERRUPT_POLICY_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template
<
    typename IsAcceptableTurnPredicate,
    bool AllowEmptyTurnRange = true // by default, allow an empty turn range
>
struct stateless_predicate_based_interrupt_policy
{
    static bool const enabled = true;
    bool has_intersections; // set to true if there is at least one
                            // unacceptable turn

    inline stateless_predicate_based_interrupt_policy()
        : has_intersections(false)
    {}

    template <typename Range>
    inline bool apply(Range const& range)
    {
        // if there is at least one unacceptable turn in the range, return false
        has_intersections = !detail::check_iterator_range
            <
                IsAcceptableTurnPredicate,
                AllowEmptyTurnRange
            >::apply(boost::begin(range), boost::end(range));

        return has_intersections;
    }
};




template
<
    typename IsAcceptableTurnPredicate,
    bool AllowEmptyTurnRange = true // by default, allow an empty turn range
>
struct predicate_based_interrupt_policy
{
    static bool const enabled = true;
    bool has_intersections; // set to true if there is at least one
                            // unacceptable turn
    IsAcceptableTurnPredicate const& m_predicate;

    inline
    predicate_based_interrupt_policy(IsAcceptableTurnPredicate const& predicate)
        : has_intersections(false)
        , m_predicate(predicate)
    {}

    template <typename Range>
    inline bool apply(Range const& range)
    {
        // if there is at least one unacceptable turn in the range, return false
        has_intersections = !detail::check_iterator_range
            <
                IsAcceptableTurnPredicate,
                AllowEmptyTurnRange
            >::apply(boost::begin(range), boost::end(range), m_predicate);

        return has_intersections;
    }
};




}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_POLICIES_PREDICATE_BASED_INTERRUPT_POLICY_HPP
