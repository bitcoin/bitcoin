// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PROJECTED_POINT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PROJECTED_POINT_HPP


#include <type_traits>

#include <boost/concept_check.hpp>
#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/arithmetic/dot_product.hpp>

#include <boost/geometry/strategies/tags.hpp>
#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/default_distance_result.hpp>
#include <boost/geometry/strategies/cartesian/distance_pythagoras.hpp>
#include <boost/geometry/strategies/cartesian/point_in_point.hpp>
#include <boost/geometry/strategies/cartesian/intersection.hpp>

// Helper geometry (projected point on line)
#include <boost/geometry/geometries/point.hpp>


namespace boost { namespace geometry
{


namespace strategy { namespace distance
{

/*!
\brief Strategy for distance point to segment
\ingroup strategies
\details Calculates distance using projected-point method, and (optionally) Pythagoras
\author Adapted from: http://geometryalgorithms.com/Archive/algorithm_0102/algorithm_0102.htm
\tparam CalculationType \tparam_calculation
\tparam Strategy underlying point-point distance strategy
\par Concepts for Strategy:
- cartesian_distance operator(Point,Point)
\note If the Strategy is a "comparable::pythagoras", this strategy
    automatically is a comparable projected_point strategy (so without sqrt)

\qbk{
[heading See also]
[link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
}

*/
template
<
    typename CalculationType = void,
    typename Strategy = pythagoras<CalculationType>
>
class projected_point
{
public:
    // The three typedefs below are necessary to calculate distances
    // from segments defined in integer coordinates.

    // Integer coordinates can still result in FP distances.
    // There is a division, which must be represented in FP.
    // So promote.
    template <typename Point, typename PointOfSegment>
    struct calculation_type
        : promote_floating_point
          <
              typename strategy::distance::services::return_type
                  <
                      Strategy,
                      Point,
                      PointOfSegment
                  >::type
          >
    {};

    template <typename Point, typename PointOfSegment>
    inline typename calculation_type<Point, PointOfSegment>::type
    apply(Point const& p, PointOfSegment const& p1, PointOfSegment const& p2) const
    {
        assert_dimension_equal<Point, PointOfSegment>();

        typedef typename calculation_type<Point, PointOfSegment>::type calculation_type;

        // A projected point of points in Integer coordinates must be able to be
        // represented in FP.
        typedef model::point
            <
                calculation_type,
                dimension<PointOfSegment>::value,
                typename coordinate_system<PointOfSegment>::type
            > fp_point_type;

        // For convenience
        typedef fp_point_type fp_vector_type;

        /*
            Algorithm [p: (px,py), p1: (x1,y1), p2: (x2,y2)]
            VECTOR v(x2 - x1, y2 - y1)
            VECTOR w(px - x1, py - y1)
            c1 = w . v
            c2 = v . v
            b = c1 / c2
            RETURN POINT(x1 + b * vx, y1 + b * vy)
        */

        // v is multiplied below with a (possibly) FP-value, so should be in FP
        // For consistency we define w also in FP
        fp_vector_type v, w, projected;

        geometry::convert(p2, v);
        geometry::convert(p, w);
        geometry::convert(p1, projected);
        subtract_point(v, projected);
        subtract_point(w, projected);

        Strategy strategy;
        boost::ignore_unused(strategy);

        calculation_type const zero = calculation_type();
        calculation_type const c1 = dot_product(w, v);
        if (c1 <= zero)
        {
            return strategy.apply(p, p1);
        }
        calculation_type const c2 = dot_product(v, v);
        if (c2 <= c1)
        {
            return strategy.apply(p, p2);
        }

        // See above, c1 > 0 AND c2 > c1 so: c2 != 0
        calculation_type const b = c1 / c2;

        multiply_value(v, b);
        add_point(projected, v);

        return strategy.apply(p, projected);
    }

    template <typename CT>
    inline CT vertical_or_meridian(CT const& lat1, CT const& lat2) const
    {
        return lat1 - lat2;
    }

};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType, typename Strategy>
struct tag<projected_point<CalculationType, Strategy> >
{
    typedef strategy_tag_distance_point_segment type;
};


template <typename CalculationType, typename Strategy, typename P, typename PS>
struct return_type<projected_point<CalculationType, Strategy>, P, PS>
    : projected_point<CalculationType, Strategy>::template calculation_type<P, PS>
{};



template <typename CalculationType, typename Strategy>
struct comparable_type<projected_point<CalculationType, Strategy> >
{
    // Define a projected_point strategy with its underlying point-point-strategy
    // being comparable
    typedef projected_point
        <
            CalculationType,
            typename comparable_type<Strategy>::type
        > type;
};


template <typename CalculationType, typename Strategy>
struct get_comparable<projected_point<CalculationType, Strategy> >
{
    typedef typename comparable_type
        <
            projected_point<CalculationType, Strategy>
        >::type comparable_type;
public :
    static inline comparable_type apply(projected_point<CalculationType, Strategy> const& )
    {
        return comparable_type();
    }
};


template <typename CalculationType, typename Strategy, typename P, typename PS>
struct result_from_distance<projected_point<CalculationType, Strategy>, P, PS>
{
private :
    typedef typename return_type<projected_point<CalculationType, Strategy>, P, PS>::type return_type;
public :
    template <typename T>
    static inline return_type apply(projected_point<CalculationType, Strategy> const& , T const& value)
    {
        Strategy s;
        return result_from_distance<Strategy, P, PS>::apply(s, value);
    }
};


// Get default-strategy for point-segment distance calculation
// while still have the possibility to specify point-point distance strategy (PPS)
// It is used in algorithms/distance.hpp where users specify PPS for distance
// of point-to-segment or point-to-linestring.
// Convenient for geographic coordinate systems especially.
template <typename Point, typename PointOfSegment, typename Strategy>
struct default_strategy
    <
        point_tag, segment_tag, Point, PointOfSegment,
        cartesian_tag, cartesian_tag, Strategy
    >
{
    typedef strategy::distance::projected_point
        <
            void,
            std::conditional_t
                <
                    std::is_void<Strategy>::value,
                    typename default_strategy
                        <
                            point_tag, point_tag, Point, PointOfSegment,
                            cartesian_tag, cartesian_tag
                        >::type,
                    Strategy
                >
        > type;
};

template <typename PointOfSegment, typename Point, typename Strategy>
struct default_strategy
    <
        segment_tag, point_tag, PointOfSegment, Point,
        cartesian_tag, cartesian_tag, Strategy
    >
{
    typedef typename default_strategy
        <
            point_tag, segment_tag, Point, PointOfSegment,
            cartesian_tag, cartesian_tag, Strategy
        >::type type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PROJECTED_POINT_HPP
