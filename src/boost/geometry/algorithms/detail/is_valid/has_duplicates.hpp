// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_DUPLICATES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_DUPLICATES_HPP

#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/core/closure.hpp>

#include <boost/geometry/policies/compare.hpp>
#include <boost/geometry/policies/is_valid/default_policy.hpp>

#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/algorithms/validity_failure_type.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{

template <typename Range>
struct has_duplicates
{
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(Range const& range, VisitPolicy& visitor,
                             Strategy const& )
    {
        boost::ignore_unused(visitor);

        detail::closed_view<Range const> const view(range);

        if ( boost::size(view) < 2 )
        {
            return ! visitor.template apply<no_failure>();
        }

        geometry::equal_to
            <
                typename boost::range_value<Range>::type,
                -1,
                typename Strategy::cs_tag
            > equal;

        auto it = boost::begin(view);
        auto const end = boost::end(view);
        auto next = it;
        for (++next; next != end; ++it, ++next)
        {
            if ( equal(*it, *next) )
            {
                return ! visitor.template apply<failure_duplicate_points>(*it);
            }
        }
        return ! visitor.template apply<no_failure>();
    }
};



}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry



#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_DUPLICATES_HPP
