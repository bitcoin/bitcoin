// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 1995, 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 1995 Maarten Hilferink, Amsterdam, the Netherlands

// This file was modified by Oracle on 2015-2021.
// Modifications copyright (c) 2015-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_AGNOSTIC_SIMPLIFY_DOUGLAS_PEUCKER_HPP
#define BOOST_GEOMETRY_STRATEGY_AGNOSTIC_SIMPLIFY_DOUGLAS_PEUCKER_HPP


#include <boost/geometry/strategies/distance.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace simplify
{


// NOTE: Left here for backward compatibility.


/*!
\brief Implements the simplify algorithm.
\ingroup strategies
\details The douglas_peucker strategy simplifies a linestring, ring or
    vector of points using the well-known Douglas-Peucker algorithm.
\tparam Point the point type
\tparam PointDistanceStrategy point-segment distance strategy to be used
\note This strategy uses itself a point-segment-distance strategy which
    can be specified
\author Barend and Maarten, 1995/1996
\author Barend, revised for Generic Geometry Library, 2008
*/

/*
For the algorithm, see for example:
 - http://en.wikipedia.org/wiki/Ramer-Douglas-Peucker_algorithm
 - http://www2.dcs.hull.ac.uk/CISRG/projects/Royal-Inst/demos/dp.html
*/
template
<
    typename Point,
    typename PointDistanceStrategy
>
class douglas_peucker
{
public :

    typedef PointDistanceStrategy distance_strategy_type;

    typedef typename strategy::distance::services::return_type
                     <
                         distance_strategy_type,
                         Point, Point
                     >::type distance_type;

    template <typename Range, typename OutputIterator>
    static inline OutputIterator apply(Range const& ,
                                       OutputIterator out,
                                       distance_type const& )
    {
        return out;
    }
};

}} // namespace strategy::simplify


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_AGNOSTIC_SIMPLIFY_DOUGLAS_PEUCKER_HPP
