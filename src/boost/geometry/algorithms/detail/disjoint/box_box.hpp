// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2015 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_BOX_BOX_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_BOX_BOX_HPP

#include <cstddef>

#include <boost/geometry/algorithms/dispatch/disjoint.hpp>

#include <boost/geometry/strategies/detail.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{


/*!
    \brief Internal utility function to detect if boxes are disjoint
    \note Is used from other algorithms, declared separately
        to avoid circular references
 */
template
<
    typename Box1, typename Box2, typename Strategy,
    std::enable_if_t<strategies::detail::is_umbrella_strategy<Strategy>::value, int> = 0
>
inline bool disjoint_box_box(Box1 const& box1, Box2 const& box2, Strategy const& strategy)
{
    typedef decltype(strategy.disjoint(box1, box2)) strategy_type;
    return strategy_type::apply(box1, box2);
}

template
<
    typename Box1, typename Box2, typename Strategy,
    std::enable_if_t<! strategies::detail::is_umbrella_strategy<Strategy>::value, int> = 0
>
inline bool disjoint_box_box(Box1 const& box1, Box2 const& box2, Strategy const& )
{
    return Strategy::apply(box1, box2);
}


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Box1, typename Box2, std::size_t DimensionCount>
struct disjoint<Box1, Box2, DimensionCount, box_tag, box_tag, false>
{
    template <typename Strategy>
    static inline bool apply(Box1 const& box1, Box2 const& box2, Strategy const& strategy)
    {
        typedef decltype(strategy.disjoint(box1, box2)) strategy_type;
        return strategy_type::apply(box1, box2);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_BOX_BOX_HPP
