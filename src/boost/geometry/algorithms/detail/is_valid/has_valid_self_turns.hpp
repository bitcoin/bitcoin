// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_VALID_SELF_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_VALID_SELF_TURNS_HPP

#include <vector>

#include <boost/core/ignore_unused.hpp>
#include <boost/range/empty.hpp>

#include <boost/geometry/algorithms/detail/is_valid/is_acceptable_turn.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>
#include <boost/geometry/algorithms/validity_failure_type.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/policies/predicate_based_interrupt_policy.hpp>
#include <boost/geometry/policies/robustness/segment_ratio_type.hpp>
#include <boost/geometry/policies/robustness/get_rescale_policy.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{


template
<
    typename Geometry,
    typename CSTag // TODO: remove
>
class has_valid_self_turns
{
private:
    typedef typename point_type<Geometry>::type point_type;

    typedef typename geometry::rescale_policy_type
        <
            point_type,
            CSTag
        >::type rescale_policy_type;

    typedef detail::overlay::get_turn_info
        <
            detail::overlay::assign_null_policy
        > turn_policy;

public:
    typedef detail::overlay::turn_info
        <
            point_type,
            typename segment_ratio_type
                <
                    point_type,
                    rescale_policy_type
                >::type
        > turn_type;

    // returns true if all turns are valid
    template <typename Turns, typename VisitPolicy, typename Strategy>
    static inline bool apply(Geometry const& geometry,
                             Turns& turns,
                             VisitPolicy& visitor,
                             Strategy const& strategy)
    {
        boost::ignore_unused(visitor);

        rescale_policy_type robust_policy
            = geometry::get_rescale_policy<rescale_policy_type>(geometry, strategy);

        detail::overlay::stateless_predicate_based_interrupt_policy
            <
                is_acceptable_turn<Geometry>
            > interrupt_policy;

        // Calculate self-turns, skipping adjacent segments
        detail::self_get_turn_points::self_turns<false, turn_policy>(geometry,
                                          strategy,
                                          robust_policy,
                                          turns,
                                          interrupt_policy,
                                          0, true);

        if (interrupt_policy.has_intersections)
        {
            BOOST_GEOMETRY_ASSERT(! boost::empty(turns));
            return visitor.template apply<failure_self_intersections>(turns);
        }
        else
        {
            return visitor.template apply<no_failure>();
        }
    }

    // returns true if all turns are valid
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(Geometry const& geometry, VisitPolicy& visitor, Strategy const& strategy)
    {
        std::vector<turn_type> turns;
        return apply(geometry, turns, visitor, strategy);
    }
};


}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_VALID_SELF_TURNS_HPP
