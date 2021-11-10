// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PROJECTED_POINT_AX_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PROJECTED_POINT_AX_HPP


#include <algorithm>

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
#include <boost/geometry/strategies/cartesian/distance_projected_point.hpp>

#include <boost/geometry/util/select_coordinate_type.hpp>

// Helper geometry (projected point on line)
#include <boost/geometry/geometries/point.hpp>


namespace boost { namespace geometry
{


namespace strategy { namespace distance
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename T>
struct projected_point_ax_result
{
    typedef T value_type;

    projected_point_ax_result(T const& c = T(0))
        : atd(c), xtd(c)
    {}

    projected_point_ax_result(T const& a, T const& x)
        : atd(a), xtd(x)
    {}

    friend inline bool operator<(projected_point_ax_result const& left,
                                 projected_point_ax_result const& right)
    {
        return left.xtd < right.xtd || left.atd < right.atd;
    }

    T atd, xtd;
};

// This less-comparator may be used as a parameter of detail::douglas_peucker.
// In this simplify strategy distances are compared in 2 places
// 1. to choose the furthest candidate (md < dist)
// 2. to check if the candidate is further than max_distance (max_distance < md)
template <typename Distance>
class projected_point_ax_less
{
public:
    projected_point_ax_less(Distance const& max_distance)
        : m_max_distance(max_distance)
    {}

    inline bool operator()(Distance const& left, Distance const& right) const
    {
        //return left.xtd < right.xtd && right.atd < m_max_distance.atd;

        typedef typename Distance::value_type value_type;

        value_type const lx = left.xtd > m_max_distance.xtd ? left.xtd - m_max_distance.xtd : 0;
        value_type const rx = right.xtd > m_max_distance.xtd ? right.xtd - m_max_distance.xtd : 0;
        value_type const la = left.atd > m_max_distance.atd ? left.atd - m_max_distance.atd : 0;
        value_type const ra = right.atd > m_max_distance.atd ? right.atd - m_max_distance.atd : 0;

        value_type const l = (std::max)(lx, la);
        value_type const r = (std::max)(rx, ra);

        return l < r;
    }
private:
    Distance const& m_max_distance;
};

// This strategy returns 2-component Point/Segment distance.
// The ATD (along track distance) is parallel to the Segment
// and is a distance between Point projected into a line defined by a Segment and the nearest Segment's endpoint.
// If the projected Point intersects the Segment the ATD is equal to 0.
// The XTD (cross track distance) is perpendicular to the Segment
// and is a distance between input Point and its projection.
// If the Segment has length equal to 0, ATD and XTD has value equal
// to the distance between the input Point and one of the Segment's endpoints.
//
//          p3         p4
//          ^         7
//          |        /
// p1<-----e========e----->p2
//
// p1: atd=D,   xtd=0
// p2: atd=D,   xtd=0
// p3: atd=0,   xtd=D
// p4: atd=D/2, xtd=D
template
<
    typename CalculationType = void,
    typename Strategy = pythagoras<CalculationType>
>
class projected_point_ax
{
public :
    template <typename Point, typename PointOfSegment>
    struct calculation_type
        : public projected_point<CalculationType, Strategy>
            ::template calculation_type<Point, PointOfSegment>
    {};

    template <typename Point, typename PointOfSegment>
    struct result_type
    {
        typedef projected_point_ax_result
                    <
                        typename calculation_type<Point, PointOfSegment>::type
                    > type;
    };

public :

    template <typename Point, typename PointOfSegment>
    inline typename result_type<Point, PointOfSegment>::type
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

        typename result_type<Point, PointOfSegment>::type result;

        calculation_type const zero = calculation_type();
        calculation_type const c2 = dot_product(v, v);
        if ( math::equals(c2, zero) )
        {
            result.xtd = strategy.apply(p, projected);
            // assume that the 0-length segment is perpendicular to the Pt->ProjPt vector
            result.atd = 0;
            return result;
        }

        calculation_type const c1 = dot_product(w, v);
        calculation_type const b = c1 / c2;
        multiply_value(v, b);
        add_point(projected, v);

        result.xtd = strategy.apply(p, projected);

        if (c1 <= zero)
        {
            result.atd = strategy.apply(p1, projected);
        }
        else if (c2 <= c1)
        {
            result.atd = strategy.apply(p2, projected);
        }
        else
        {
            result.atd = 0;
        }

        return result;
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{


template <typename CalculationType, typename Strategy>
struct tag<detail::projected_point_ax<CalculationType, Strategy> >
{
    typedef strategy_tag_distance_point_segment type;
};


template <typename CalculationType, typename Strategy, typename P, typename PS>
struct return_type<detail::projected_point_ax<CalculationType, Strategy>, P, PS>
{
    typedef typename detail::projected_point_ax<CalculationType, Strategy>
                        ::template result_type<P, PS>::type type;
};


template <typename CalculationType, typename Strategy>
struct comparable_type<detail::projected_point_ax<CalculationType, Strategy> >
{
    // Define a projected_point strategy with its underlying point-point-strategy
    // being comparable
    typedef detail::projected_point_ax
        <
            CalculationType,
            typename comparable_type<Strategy>::type
        > type;
};


template <typename CalculationType, typename Strategy>
struct get_comparable<detail::projected_point_ax<CalculationType, Strategy> >
{
    typedef typename comparable_type
        <
            detail::projected_point_ax<CalculationType, Strategy>
        >::type comparable_type;
public :
    static inline comparable_type apply(detail::projected_point_ax<CalculationType, Strategy> const& )
    {
        return comparable_type();
    }
};


template <typename CalculationType, typename Strategy, typename P, typename PS>
struct result_from_distance<detail::projected_point_ax<CalculationType, Strategy>, P, PS>
{
private :
    typedef typename return_type<detail::projected_point_ax<CalculationType, Strategy>, P, PS>::type return_type;
public :
    template <typename T>
    static inline return_type apply(detail::projected_point_ax<CalculationType, Strategy> const& , T const& value)
    {
        Strategy s;
        return_type ret;
        ret.atd = result_from_distance<Strategy, P, PS>::apply(s, value.atd);
        ret.xtd = result_from_distance<Strategy, P, PS>::apply(s, value.xtd);
        return ret;
    }
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PROJECTED_POINT_AX_HPP
