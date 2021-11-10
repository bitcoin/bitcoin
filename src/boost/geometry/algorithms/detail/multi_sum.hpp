// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_MULTI_SUM_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_MULTI_SUM_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>


namespace boost { namespace geometry
{
#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct multi_sum
{
    template <typename ReturnType, typename Policy, typename MultiGeometry, typename Strategy>
    static inline ReturnType apply(MultiGeometry const& geometry, Strategy const& strategy)
    {
        ReturnType sum = ReturnType();
        for (typename boost::range_iterator
                <
                    MultiGeometry const
                >::type it = boost::begin(geometry);
            it != boost::end(geometry);
            ++it)
        {
            sum += Policy::apply(*it, strategy);
        }
        return sum;
    }
};


} // namespace detail
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_MULTI_SUM_HPP
