// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_SPIKES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_SPIKES_HPP

#include <algorithm>

#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/rbegin.hpp>
#include <boost/range/rend.hpp>

#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/validity_failure_type.hpp>
#include <boost/geometry/algorithms/detail/point_is_spike_or_equal.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/io/dsv/write.hpp>

#include <boost/geometry/policies/is_valid/default_policy.hpp>

#include <boost/geometry/util/range.hpp>
#include <boost/geometry/util/type_traits.hpp>

#include <boost/geometry/views/closeable_view.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{


template <typename Range>
struct has_spikes
{
    template <typename Iterator, typename Strategy>
    static inline Iterator find_different_from_first(Iterator first,
                                                     Iterator last,
                                                     Strategy const& strategy)
    {
        if (first == last)
            return last;
        auto const& front = *first;
        ++first;
        return std::find_if(first, last, [&](auto const& pt) {
            return ! equals::equals_point_point(pt, front, strategy);
        });
    }

    template <typename View, typename VisitPolicy, typename Strategy>
    static inline bool apply_at_closure(View const& view, VisitPolicy& visitor,
                                        Strategy const& strategy,
                                        bool is_linear)
    {
        boost::ignore_unused(visitor);

        typedef typename boost::range_iterator<View const>::type iterator;

        iterator cur = boost::begin(view);
        typename boost::range_reverse_iterator
            <
                View const
            >::type prev = find_different_from_first(boost::rbegin(view),
                                                     boost::rend(view),
                                                     strategy);

        iterator next = find_different_from_first(cur, boost::end(view),
                                                  strategy);
        if (detail::is_spike_or_equal(*next, *cur, *prev, strategy.side()))
        {
            return ! visitor.template apply<failure_spikes>(is_linear, *cur);
        }
        else
        {
            return ! visitor.template apply<no_failure>();
        }
    }


    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(Range const& range, VisitPolicy& visitor,
                             Strategy const& strategy)
    {
        boost::ignore_unused(visitor);

        bool const is_linestring = util::is_linestring<Range>::value;

        detail::closed_view<Range const> const view(range);

        auto prev = boost::begin(view);
        auto const end = boost::end(view);

        auto cur = find_different_from_first(prev, boost::end(view), strategy);
        if (cur == end)
        {
            // the range has only one distinct point, so it
            // cannot have a spike
            return ! visitor.template apply<no_failure>();
        }

        auto next = find_different_from_first(cur, boost::end(view), strategy);
        if (next == end)
        {
            // the range has only two distinct points, so it
            // cannot have a spike
            return ! visitor.template apply<no_failure>();
        }

        while (next != end)
        {
            // Verify spike. TODO: this is a reverse order from expected
            // in is_spike_or_equal, but this order calls the side
            // strategy in the way to correctly detect the spikes,
            // also in geographic cases going over the pole
            if (detail::is_spike_or_equal(*next, *cur, *prev, strategy.side()))
            {
                return
                    ! visitor.template apply<failure_spikes>(is_linestring, *cur);
            }
            prev = cur;
            cur = next;
            next = find_different_from_first(cur, boost::end(view), strategy);
        }

        if (equals::equals_point_point(range::front(view), range::back(view),
                                       strategy))
        {
            return apply_at_closure(view, visitor, strategy, is_linestring);
        }

        return ! visitor.template apply<no_failure>();
    }
};



}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_SPIKES_HPP
