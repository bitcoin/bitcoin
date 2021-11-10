// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POINTLIKE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POINTLIKE_HPP

#include <boost/core/ignore_unused.hpp>
#include <boost/range/empty.hpp>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/validity_failure_type.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_invalid_coordinate.hpp>
#include <boost/geometry/algorithms/dispatch/is_valid.hpp>

#include <boost/geometry/util/condition.hpp>


namespace boost { namespace geometry
{



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// A point is always simple
template <typename Point>
struct is_valid<Point, point_tag>
{
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(Point const& point, VisitPolicy& visitor, Strategy const&)
    {
        boost::ignore_unused(visitor);
        return ! detail::is_valid::has_invalid_coordinate
            <
                Point
            >::apply(point, visitor);
    }
};



// A MultiPoint is simple if no two Points in the MultiPoint are equal
// (have identical coordinate values in X and Y)
//
// Reference: OGC 06-103r4 (6.1.5)
template <typename MultiPoint, bool AllowEmptyMultiGeometries>
struct is_valid<MultiPoint, multi_point_tag, AllowEmptyMultiGeometries>
{
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(MultiPoint const& multipoint,
                             VisitPolicy& visitor,
                             Strategy const&)
    {
        boost::ignore_unused(multipoint, visitor);

        if (BOOST_GEOMETRY_CONDITION(
                AllowEmptyMultiGeometries || !boost::empty(multipoint)))
        {
            // we allow empty multi-geometries, so an empty multipoint
            // is considered valid
            return ! detail::is_valid::has_invalid_coordinate
                <
                    MultiPoint
                >::apply(multipoint, visitor);
        }
        else
        {
            // we do not allow an empty multipoint
            return visitor.template apply<failure_few_points>();
        }
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POINTLIKE_HPP
