// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2019, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_SEGMENT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_SEGMENT_HPP

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/validity_failure_type.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_invalid_coordinate.hpp>
#include <boost/geometry/algorithms/dispatch/is_valid.hpp>


namespace boost { namespace geometry
{



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// A segment is a curve.
// A curve is simple if it does not pass through the same point twice,
// with the possible exception of its two endpoints
// A curve is 1-dimensional, hence we have to check is the two
// endpoints of the segment coincide, since in this case it is
// 0-dimensional.
//
// Reference: OGC 06-103r4 (6.1.6.1)
template <typename Segment>
struct is_valid<Segment, segment_tag>
{
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(Segment const& segment, VisitPolicy& visitor, Strategy const& strategy)
    {
        boost::ignore_unused(visitor);

        typename point_type<Segment>::type p[2];
        detail::assign_point_from_index<0>(segment, p[0]);
        detail::assign_point_from_index<1>(segment, p[1]);

        if (detail::is_valid::has_invalid_coordinate
                <
                    Segment
                >::apply(segment, visitor))
        {
            return false;
        }
        else if (! detail::equals::equals_point_point(p[0], p[1], strategy))
        {
            return visitor.template apply<no_failure>();
        }
        else
        {
            return
                visitor.template apply<failure_wrong_topological_dimension>();
        }
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_SEGMENT_HPP
