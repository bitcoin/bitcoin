// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2019 Tinko Bartels, Berlin, Germany.

// Contributed and/or modified by Tinko Bartels,
//   as part of Google Summer of Code 2019 program.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_IN_CIRCLE_ROBUST_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_IN_CIRCLE_ROBUST_HPP

#include<boost/geometry/util/precise_math.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace in_circle
{

/*!
\brief Adaptive precision predicate to check whether a fourth point lies inside the circumcircle of the first three points:
    inside (>0), outside (< 0), on the boundary (0).
\ingroup strategies
\tparam CalculationType \tparam_calculation (numeric_limits<ct>::epsilon() and numeric_limits<ct>::digits must be supported for calculation type ct)
\tparam Robustness std::size_t value from 0 (fastest) to 2 (default, most precise).
\details This predicate determines whether a fourth point lies inside the circumcircle of the first three points using an algorithm that is adapted from incircle as described in "Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates" by Jonathan Richard Shewchuk ( https://dl.acm.org/citation.cfm?doid=237218.237337 ). More information and copies of the paper can also be found at https://www.cs.cmu.edu/~quake/robust.html . It is designed to be adaptive in the sense that it should be fast for inputs that lead to correct results with plain float operations but robust for inputs that require higher precision arithmetics.
 */
template <typename CalculationType = double, std::size_t Robustness = 2>
class in_circle_robust
{
public:
    template <typename P1, typename P2, typename P3, typename P>
    static inline int apply(P1 const& p1, P2 const& p2, P3 const& p3, P const& p)
    {
        std::array<CalculationType, 2> pa {
            { boost::geometry::get<0>(p1), boost::geometry::get<1>(p1) }};
        std::array<CalculationType, 2> pb {
            { boost::geometry::get<0>(p2), boost::geometry::get<1>(p2) }};
        std::array<CalculationType, 2> pc {
            { boost::geometry::get<0>(p3), boost::geometry::get<1>(p3) }};
        std::array<CalculationType, 2> pd {
            { boost::geometry::get<0>(p), boost::geometry::get<1>(p) }};
        CalculationType det =
            boost::geometry::detail::precise_math::incircle
                <
                    CalculationType,
                    Robustness
                >(pa, pb, pc, pd);
        return det > 0 ? 1
                       : det < 0 ? -1 : 0;
    }

};

} // namespace in_circle

} // namespace strategy

}} // namespace boost::geometry::strategy

#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_IN_CIRCLE_ROBUST_HPP
