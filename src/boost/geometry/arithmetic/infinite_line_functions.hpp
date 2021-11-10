// Boost.Geometry

// Copyright (c) 2018-2019 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ARITHMETIC_LINE_FUNCTIONS_HPP
#define BOOST_GEOMETRY_ARITHMETIC_LINE_FUNCTIONS_HPP

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/config.hpp>
#include <boost/geometry/geometries/infinite_line.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

namespace arithmetic
{

template <typename Line, typename Line::type Line::* member1, typename Line::type Line::* member2>
inline auto determinant(Line const& p, Line const& q)
{
    return geometry::detail::determinant<typename Line::type>(p.*member1, p.*member2,
                                                              q.*member1, q.*member2);
}

template <typename Point, typename Line, typename Type>
inline Point assign_intersection_point(Line const& p, Line const& q, Type const& denominator)
{
    BOOST_ASSERT(denominator != Type(0));

    // x = | pb pc | / d  and y = | pc pa | / d
    //     | qb qc |              | qc qa |

    Point result;
    geometry::set<0>(result, determinant<Line, &Line::b, &Line::c>(p, q) / denominator);
    geometry::set<1>(result, determinant<Line, &Line::c, &Line::a>(p, q) / denominator);
    return result;
}

// Calculates intersection point of two infinite lines.
// Returns true if the lines intersect.
// Returns false if lines are parallel (or collinear, possibly opposite)
template <typename Line, typename Point>
inline bool intersection_point(Line const& p, Line const& q, Point& ip)
{
    auto const denominator = determinant<Line, &Line::a, &Line::b>(p, q);
    constexpr decltype(denominator) const zero = 0;

    if (math::equals(denominator, zero))
    {
        // Lines are parallel
        return false;
    }

    ip = assign_intersection_point<Point>(p, q, denominator);

    return true;
}

//! Return a distance-side-measure for a point to a line
//! Point is located left of the line if value is positive,
//! right of the line is value is negative, and on the line if the value
//! is exactly zero
template <typename Type, typename CoordinateType>
inline
typename select_most_precise<Type, CoordinateType>::type
side_value(model::infinite_line<Type> const& line,
    CoordinateType const& x, CoordinateType const& y)
{
    // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line#Line_defined_by_an_equation
    // Distance from point to line in general form is given as:
    // (a * x + b * y + c) / sqrt(a * a + b * b);
    // In most use cases comparisons are enough, saving the sqrt
    // and often even the division.
    // Also, this gives positive values for points left to the line,
    // and negative values for points right to the line.
    return line.a * x + line.b * y + line.c;
}

template <typename Type, typename Point>
inline
typename select_most_precise
<
    Type,
    typename geometry::coordinate_type<Point>::type
>::type
side_value(model::infinite_line<Type> const& line, Point const& p)
{
    return side_value(line, geometry::get<0>(p), geometry::get<1>(p));
}

template <typename Type>
inline bool is_degenerate(const model::infinite_line<Type>& line)
{
    static Type const zero = 0;
    return math::equals(line.a, zero) && math::equals(line.b, zero);
}


} // namespace arithmetic


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ARITHMETIC_LINE_FUNCTIONS_HPP
