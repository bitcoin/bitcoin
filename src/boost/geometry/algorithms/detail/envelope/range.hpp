// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_HPP

#include <iterator>
#include <vector>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/algorithms/is_empty.hpp>

#include <boost/geometry/algorithms/detail/envelope/initialize.hpp>
#include <boost/geometry/algorithms/detail/expand/box.hpp>
#include <boost/geometry/algorithms/detail/expand/point.hpp>
#include <boost/geometry/algorithms/detail/expand/segment.hpp>

#include <boost/geometry/core/coordinate_dimension.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


// implementation for simple ranges
struct envelope_range
{
    template <typename Iterator, typename Box, typename Strategy>
    static inline void apply(Iterator it,
                             Iterator last,
                             Box& mbr,
                             Strategy const& strategy)
    {
        typedef typename std::iterator_traits<Iterator>::value_type value_type;

        // initialize MBR
        initialize<Box, 0, dimension<Box>::value>::apply(mbr);

        if (it != last)
        {
            // initialize box with first element in range
            dispatch::envelope
                <
                    value_type
                >::apply(*it, mbr, strategy);

            // consider now the remaining elements in the range (if any)
            for (++it; it != last; ++it)
            {
                dispatch::expand
                    <
                        Box, value_type
                    >::apply(mbr, *it, strategy);
            }
        }
    }

    template <typename Range, typename Box, typename Strategy>
    static inline void apply(Range const& range, Box& mbr, Strategy const& strategy)
    {
        using strategy_t = decltype(strategy.envelope(range, mbr));
        return apply(strategy_t::begin(range), strategy_t::end(range),
                     mbr, strategy);
    }
};


// implementation for multi-ranges
template <typename EnvelopePolicy>
struct envelope_multi_range
{
    template <typename MultiRange, typename Box, typename Strategy>
    static inline void apply(MultiRange const& multirange,
                             Box& mbr,
                             Strategy const& strategy)
    {
        using range_t = typename boost::range_value<MultiRange>::type;
        using strategy_t = decltype(strategy.envelope(std::declval<range_t>(), mbr));
        using state_t = typename strategy_t::template multi_state<Box>;
        apply<state_t>(boost::begin(multirange), boost::end(multirange), mbr, strategy);
    }

private:
    template <typename State, typename Iter, typename Box, typename Strategy>
    static inline void apply(Iter it,
                             Iter last,
                             Box& mbr,
                             Strategy const& strategy)
    {
        State state;
        for (; it != last; ++it)
        {
            if (! geometry::is_empty(*it))
            {
                Box helper_mbr;
                EnvelopePolicy::apply(*it, helper_mbr, strategy);
                state.apply(helper_mbr);
            }
        }
        state.result(mbr);
    }
};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_HPP
