// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2019 Tinko Bartels, Berlin, Germany.

// Contributed and/or modified by Tinko Bartels,
//   as part of Google Summer of Code 2019 program.

// This file was modified by Oracle on 2021.
// Modifications copyright (c) 2021, Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_SIDE_ROBUST_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_SIDE_ROBUST_HPP

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
\tparam CalculationType \tparam_calculation (numeric_limits<ct>::epsilon() and numeric_limits<ct>::digits must be supported for calculation type ct)
\tparam Robustness std::size_t value from 0 (fastest) to 3 (default, guarantees correct results).
\details This predicate determines at which side of a segment a point lies using an algorithm that is adapted from orient2d as described in "Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates" by Jonathan Richard Shewchuk ( https://dl.acm.org/citation.cfm?doid=237218.237337 ). More information and copies of the paper can also be found at https://www.cs.cmu.edu/~quake/robust.html . It is designed to be adaptive in the sense that it should be fast for inputs that lead to correct results with plain float operations but robust for inputs that require higher precision arithmetics.
 */
template
<
    typename CalculationType = void,
    std::size_t Robustness = 3
>
struct side_robust
{
public:
    //! \brief Computes double the signed area of the CCW triangle p1, p2, p
    template
    <
        typename PromotedType,
        typename P1,
        typename P2,
        typename P
    >
    static inline PromotedType side_value(P1 const& p1, P2 const& p2,
        P const& p)
    {
        using vec2d = ::boost::geometry::detail::precise_math::vec2d<PromotedType>;
        vec2d pa;
        pa.x = get<0>(p1);
        pa.y = get<1>(p1);
        vec2d pb;
        pb.x = get<0>(p2);
        pb.y = get<1>(p2);
        vec2d pc;
        pc.x = get<0>(p);
        pc.y = get<1>(p);
        return ::boost::geometry::detail::precise_math::orient2d
            <PromotedType, Robustness>(pa, pb, pc);
    }

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

        promoted_type sv = side_value<promoted_type>(p1, p2, p);
        return sv > 0 ? 1
            : sv < 0 ? -1
            : 0;
    }
#endif

};

}} // namespace strategy::side

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_SIDE_ROBUST_HPP
