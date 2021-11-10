// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_LINEAR_HPP

#include <cstddef>

#include <boost/range/begin.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/validity_failure_type.hpp>
#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_invalid_coordinate.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_spikes.hpp>
#include <boost/geometry/algorithms/detail/num_distinct_consecutive_points.hpp>

#include <boost/geometry/algorithms/dispatch/is_valid.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/condition.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{


template <typename Linestring>
struct is_valid_linestring
{
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(Linestring const& linestring,
                             VisitPolicy& visitor,
                             Strategy const& strategy)
    {
        // TODO: Consider checking coordinates based on coordinate system
        //       Right now they are only checked for infinity in all systems.
        if (has_invalid_coordinate<Linestring>::apply(linestring, visitor))
        {
            return false;
        }

        if (boost::size(linestring) < 2)
        {
            return visitor.template apply<failure_few_points>();
        }

        std::size_t num_distinct = detail::num_distinct_consecutive_points
            <
                Linestring, 3u, true
            >::apply(linestring, strategy);

        if (num_distinct < 2u)
        {
            return
                visitor.template apply<failure_wrong_topological_dimension>();
        }

        if (num_distinct == 2u)
        {
            return visitor.template apply<no_failure>();
        }

        // TODO: This algorithm iterates over the linestring until a spike is
        //   found and only then the decision about the validity is made. This
        //   is done regardless of VisitPolicy.
        //   An obvious improvement is to avoid calling the algorithm at all if
        //   spikes are allowed which is the default.
        return ! has_spikes<Linestring>::apply(linestring, visitor, strategy);
    }
};


}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// A linestring is a curve.
// A curve is 1-dimensional so it has to have at least two distinct
// points.
// A curve is simple if it does not pass through the same point twice,
// with the possible exception of its two endpoints
//
// There is an option here as to whether spikes are allowed for linestrings; 
// here we pass this as an additional template parameter: allow_spikes
// If allow_spikes is set to true, spikes are allowed, false otherwise.
// By default, spikes are disallowed
//
// Reference: OGC 06-103r4 (6.1.6.1)
template <typename Linestring, bool AllowEmptyMultiGeometries>
struct is_valid
    <
        Linestring, linestring_tag, AllowEmptyMultiGeometries
    > : detail::is_valid::is_valid_linestring<Linestring>
{};


// A MultiLinestring is a MultiCurve
// A MultiCurve is simple if all of its elements are simple and the
// only intersections between any two elements occur at Points that
// are on the boundaries of both elements.
//
// Reference: OGC 06-103r4 (6.1.8.1; Fig. 9)
template <typename MultiLinestring, bool AllowEmptyMultiGeometries>
class is_valid
    <
        MultiLinestring, multi_linestring_tag, AllowEmptyMultiGeometries
    >
{
private:
    template <typename VisitPolicy, typename Strategy>
    struct per_linestring
    {
        per_linestring(VisitPolicy& policy, Strategy const& strategy)
            : m_policy(policy)
            , m_strategy(strategy)
        {}

        template <typename Linestring>
        inline bool apply(Linestring const& linestring) const
        {
            return detail::is_valid::is_valid_linestring
                <
                    Linestring
                >::apply(linestring, m_policy, m_strategy);
        }

        VisitPolicy& m_policy;
        Strategy const& m_strategy;
    };

public:
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(MultiLinestring const& multilinestring,
                             VisitPolicy& visitor,
                             Strategy const& strategy)
    {
        if (BOOST_GEOMETRY_CONDITION(
                AllowEmptyMultiGeometries && boost::empty(multilinestring)))
        {
            return visitor.template apply<no_failure>();
        }

        typedef per_linestring<VisitPolicy, Strategy> per_ls;

        return detail::check_iterator_range
            <
                per_ls,
                false // do not check for empty multilinestring (done above)
            >::apply(boost::begin(multilinestring),
                     boost::end(multilinestring),
                     per_ls(visitor, strategy));
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_LINEAR_HPP
