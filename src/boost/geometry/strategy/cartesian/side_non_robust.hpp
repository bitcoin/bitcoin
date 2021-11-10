// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_SIDE_NON_ROBUST_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_SIDE_NON_ROBUST_HPP

#include <boost/geometry/util/select_most_precise.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/precise_math.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace side
{

/*!
\brief Adaptive precision predicate to check at which side of a segment a point lies:
    left of segment (>0), right of segment (< 0), on segment (0).
\ingroup strategies
\tparam CalculationType \tparam_calculation
\details This predicate determines at which side of a segment a point lies
*/
template
<
    typename CalculationType = void
>
struct side_non_robust
{
public:
    //! \brief Computes double the signed area of the CCW triangle p1, p2, p

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    template
    <
        typename P1,
        typename P2,
        typename P
    >
    static inline int apply(P1 const& p1, P2 const& p2, P const& p)
    {
        typedef typename select_calculation_type_alt
            <
                CalculationType,
                P1,
                P2,
                P
            >::type coordinate_type;
        typedef typename select_most_precise
            <
                coordinate_type,
                double
            >::type promoted_type;

        auto detleft = (promoted_type(get<0>(p1)) - promoted_type(get<0>(p)))
                * (promoted_type(get<1>(p2)) - promoted_type(get<1>(p)));
        auto detright = (promoted_type(get<1>(p1)) - promoted_type(get<1>(p)))
                * (promoted_type(get<0>(p2)) - promoted_type(get<0>(p)));
        return detleft > detright ? 1 : (detleft < detright ? -1 : 0 );

    }
#endif

};

}} // namespace strategy::side

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_SIDE_NON_ROBUST_HPP
