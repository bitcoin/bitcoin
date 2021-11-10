// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_MULTIPOINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_MULTIPOINT_HPP

#include <algorithm>

#include <boost/range/begin.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/policies/compare.hpp>

#include <boost/geometry/algorithms/detail/is_valid/has_duplicates.hpp>
#include <boost/geometry/algorithms/detail/is_simple/failure_policy.hpp>

#include <boost/geometry/algorithms/dispatch/is_simple.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_simple
{


template <typename MultiPoint>
struct is_simple_multipoint
{
    template <typename Strategy>
    static inline bool apply(MultiPoint const& multipoint, Strategy const& strategy)
    {
        typedef typename Strategy::cs_tag cs_tag;
        typedef geometry::less
            <
                typename point_type<MultiPoint>::type,
                -1,
                cs_tag
            > less_type;

        if (boost::empty(multipoint))
        {
            return true;
        }

        MultiPoint mp(multipoint);
        std::sort(boost::begin(mp), boost::end(mp), less_type());

        simplicity_failure_policy policy;
        return ! detail::is_valid::has_duplicates<MultiPoint>::apply(mp, policy, strategy);
    }
};


}} // namespace detail::is_simple
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// A MultiPoint is simple if no two Points in the MultiPoint are equal
// (have identical coordinate values in X and Y)
//
// Reference: OGC 06-103r4 (6.1.5)
template <typename MultiPoint>
struct is_simple<MultiPoint, multi_point_tag>
    : detail::is_simple::is_simple_multipoint<MultiPoint>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_MULTIPOINT_HPP
