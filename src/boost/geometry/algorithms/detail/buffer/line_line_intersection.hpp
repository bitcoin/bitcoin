// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2020 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_LINE_LINE_INTERSECTION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_LINE_LINE_INTERSECTION_HPP

#include <boost/geometry/algorithms/detail/make/make.hpp>
#include <boost/geometry/arithmetic/infinite_line_functions.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{

struct line_line_intersection
{
    template <typename Point>
    static Point between_point(Point const& a, Point const& b)
    {
        Point result;
        geometry::set<0>(result, (geometry::get<0>(a) + geometry::get<0>(b)) / 2.0);
        geometry::set<1>(result, (geometry::get<1>(a) + geometry::get<1>(b)) / 2.0);
        return result;
    }

    template <typename Point>
    static bool
    apply(Point const& pi, Point const& pj, Point const& qi, Point const& qj,
          Point const& vertex, bool equidistant, Point& ip)
    {
        // Calculates ip (below) by either intersecting p (pi, pj)
        // with q (qi, qj) or by taking a point between pj and qi (b) and
        // intersecting r (b, v), where v is the original vertex, with p (or q).
        // The reason for dual approach: p might be nearly collinear with q,
        // and in that case the intersection points can lose precision
        // (or be plainly wrong).
        // Therefore it takes the most precise option (this is usually p, r)
        //
        //             /qj                     |
        //            /                        |
        //           /      /                  |
        //          /      /                   |
        //         /      /                    |
        //        /qi    /                     |
        //              /                      |
        //   ip *  + b * v                     |
        //              \                      |
        //        \pj    \                     |
        //         \      \                    |
        //          \      \                   |
        //           \      \                  |
        //            \pi    \                 |
        //
        // If generated sides along the segments can have an adapted distance,
        // in a custom strategy, then the calculation of the point in between
        // might be incorrect and the optimization is not used.

        using ct = typename coordinate_type<Point>::type;

        auto const p = detail::make::make_infinite_line<ct>(pi, pj);
        auto const q = detail::make::make_infinite_line<ct>(qi, qj);

        using line = decltype(p);
        using arithmetic::determinant;
        using arithmetic::assign_intersection_point;

        // The denominator is the determinant of (a,b) values of lines p q
        // | pa pa |
        // | qb qb |
        auto const denominator_pq = determinant<line, &line::a, &line::b>(p, q);
        constexpr decltype(denominator_pq) const zero = 0;

        if (equidistant)
        {
            auto const between = between_point(pj, qi);
            auto const r = detail::make::make_infinite_line<ct>(vertex, between);
            auto const denominator_pr = determinant<line, &line::a, &line::b>(p, r);

            if (math::equals(denominator_pq, zero)
                && math::equals(denominator_pr, zero))
            {
                // Degenerate case (for example when length results in <inf>)
                return false;
            }

            ip = geometry::math::abs(denominator_pq) > geometry::math::abs(denominator_pr)
                 ? assign_intersection_point<Point>(p, q, denominator_pq)
                 : assign_intersection_point<Point>(p, r, denominator_pr);
        }
        else
        {
            if (math::equals(denominator_pq, zero))
            {
                return false;
            }
            ip = assign_intersection_point<Point>(p, q, denominator_pq);
        }

        return true;
    }
};


}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_LINE_LINE_INTERSECTION_HPP
