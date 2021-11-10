// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2021.
// Modifications copyright (c) 2015-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_SIDE_BY_TRIANGLE_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_SIDE_BY_TRIANGLE_HPP


#include <type_traits>

#include <boost/geometry/arithmetic/determinant.hpp>

#include <boost/geometry/core/access.hpp>

#include <boost/geometry/strategies/cartesian/disjoint_segment_box.hpp>
#include <boost/geometry/strategies/cartesian/point_in_point.hpp>
#include <boost/geometry/strategies/compare.hpp>
#include <boost/geometry/strategies/side.hpp>

#include <boost/geometry/strategy/cartesian/envelope.hpp>

#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace side
{

/*!
\brief Check at which side of a segment a point lies:
    left of segment (> 0), right of segment (< 0), on segment (0)
\ingroup strategies
\tparam CalculationType \tparam_calculation
 */
template <typename CalculationType = void>
class side_by_triangle
{
    template <typename Policy>
    struct eps_policy
    {
        eps_policy() {}
        template <typename Type>
        eps_policy(Type const& a, Type const& b, Type const& c, Type const& d)
            : policy(a, b, c, d)
        {}
        Policy policy;
    };

    struct eps_empty
    {
        eps_empty() {}
        template <typename Type>
        eps_empty(Type const&, Type const&, Type const&, Type const&) {}
    };

public :
    typedef cartesian_tag cs_tag;

    // Template member function, because it is not always trivial
    // or convenient to explicitly mention the typenames in the
    // strategy-struct itself.

    // Types can be all three different. Therefore it is
    // not implemented (anymore) as "segment"

    template
    <
        typename CoordinateType,
        typename PromotedType,
        typename P1,
        typename P2,
        typename P,
        typename EpsPolicy
    >
    static inline
    PromotedType side_value(P1 const& p1, P2 const& p2, P const& p, EpsPolicy & eps_policy)
    {
        CoordinateType const x = get<0>(p);
        CoordinateType const y = get<1>(p);

        CoordinateType const sx1 = get<0>(p1);
        CoordinateType const sy1 = get<1>(p1);
        CoordinateType const sx2 = get<0>(p2);
        CoordinateType const sy2 = get<1>(p2);

        PromotedType const dx = sx2 - sx1;
        PromotedType const dy = sy2 - sy1;
        PromotedType const dpx = x - sx1;
        PromotedType const dpy = y - sy1;

        eps_policy = EpsPolicy(dx, dy, dpx, dpy);

        return geometry::detail::determinant<PromotedType>
                (
                    dx, dy,
                    dpx, dpy
                );

    }

    template
    <
        typename CoordinateType,
        typename PromotedType,
        typename P1,
        typename P2,
        typename P
    >
    static inline
    PromotedType side_value(P1 const& p1, P2 const& p2, P const& p)
    {
        eps_empty dummy;
        return side_value<CoordinateType, PromotedType>(p1, p2, p, dummy);
    }


    template
    <
        typename CoordinateType,
        typename PromotedType,
        bool AreAllIntegralCoordinates
    >
    struct compute_side_value
    {
        template <typename P1, typename P2, typename P, typename EpsPolicy>
        static inline PromotedType apply(P1 const& p1, P2 const& p2, P const& p, EpsPolicy & epsp)
        {
            return side_value<CoordinateType, PromotedType>(p1, p2, p, epsp);
        }
    };

    template <typename CoordinateType, typename PromotedType>
    struct compute_side_value<CoordinateType, PromotedType, false>
    {
        template <typename P1, typename P2, typename P, typename EpsPolicy>
        static inline PromotedType apply(P1 const& p1, P2 const& p2, P const& p, EpsPolicy & epsp)
        {
            // For robustness purposes, first check if any two points are
            // the same; in this case simply return that the points are
            // collinear
            if (equals_point_point(p1, p2)
                || equals_point_point(p1, p)
                || equals_point_point(p2, p))
            {
                return PromotedType(0);
            }

            // The side_by_triangle strategy computes the signed area of
            // the point triplet (p1, p2, p); as such it is (in theory)
            // invariant under cyclic permutations of its three arguments.
            //
            // In the context of numerical errors that arise in
            // floating-point computations, and in order to make the strategy
            // consistent with respect to cyclic permutations of its three
            // arguments, we cyclically permute them so that the first
            // argument is always the lexicographically smallest point.

            typedef compare::cartesian<compare::less> less;

            if (less::apply(p, p1))
            {
                if (less::apply(p, p2))
                {
                    // p is the lexicographically smallest
                    return side_value<CoordinateType, PromotedType>(p, p1, p2, epsp);
                }
                else
                {
                    // p2 is the lexicographically smallest
                    return side_value<CoordinateType, PromotedType>(p2, p, p1, epsp);
                }
            }

            if (less::apply(p1, p2))
            {
                // p1 is the lexicographically smallest
                return side_value<CoordinateType, PromotedType>(p1, p2, p, epsp);
            }
            else
            {
                // p2 is the lexicographically smallest
                return side_value<CoordinateType, PromotedType>(p2, p, p1, epsp);
            }
        }
    };


    template <typename P1, typename P2, typename P>
    static inline int apply(P1 const& p1, P2 const& p2, P const& p)
    {
        typedef typename coordinate_type<P1>::type coordinate_type1;
        typedef typename coordinate_type<P2>::type coordinate_type2;
        typedef typename coordinate_type<P>::type coordinate_type3;

        typedef std::conditional_t
            <
                std::is_void<CalculationType>::value,
                typename select_most_precise
                    <
                        coordinate_type1,
                        coordinate_type2,
                        coordinate_type3
                    >::type,
                CalculationType
            > coordinate_type;

        // Promote float->double, small int->int
        typedef typename select_most_precise
            <
                coordinate_type,
                double
            >::type promoted_type;

        bool const are_all_integral_coordinates =
            std::is_integral<coordinate_type1>::value
            && std::is_integral<coordinate_type2>::value
            && std::is_integral<coordinate_type3>::value;

        eps_policy< math::detail::equals_factor_policy<promoted_type> > epsp;
        promoted_type s = compute_side_value
            <
                coordinate_type, promoted_type, are_all_integral_coordinates
            >::apply(p1, p2, p, epsp);

        promoted_type const zero = promoted_type();
        return math::detail::equals_by_policy(s, zero, epsp.policy) ? 0
            : s > zero ? 1
            : -1;
    }

private:
    template <typename P1, typename P2>
    static inline bool equals_point_point(P1 const& p1, P2 const& p2)
    {
        return strategy::within::cartesian_point_point::apply(p1, p2);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType>
struct default_strategy<cartesian_tag, CalculationType>
{
    typedef side_by_triangle<CalculationType> type;
};

}
#endif

}} // namespace strategy::side

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_SIDE_BY_TRIANGLE_HPP
