// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2021.
// Modifications copyright (c) 2015-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_CENTROID_BASHEIN_DETMER_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_CENTROID_BASHEIN_DETMER_HPP


#include <cstddef>

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/strategies/centroid.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

// Note: when calling the namespace "centroid", it sometimes,
// somehow, in gcc, gives compilation problems (confusion with function centroid).

namespace strategy { namespace centroid
{



/*!
\brief Centroid calculation using algorithm Bashein / Detmer
\ingroup strategies
\details Calculates centroid using triangulation method published by
    Bashein / Detmer
\tparam Point point type of centroid to calculate
\tparam PointOfSegment point type of segments, defaults to Point
\tparam CalculationType \tparam_calculation

\author Adapted from  "Centroid of a Polygon" by
    Gerard Bashein and Paul R. Detmer<em>,
in "Graphics Gems IV", Academic Press, 1994</em>


\qbk{
[heading See also]
[link geometry.reference.algorithms.centroid.centroid_3_with_strategy centroid (with strategy)]
}
*/

/*
\par Research notes
The algorithm gives the same results as Oracle and PostGIS but
    differs from MySQL
(tried 5.0.21 / 5.0.45 / 5.0.51a / 5.1.23).

Without holes:
- this:       POINT(4.06923363095238 1.65055803571429)
- geolib:     POINT(4.07254 1.66819)
- MySQL:      POINT(3.6636363636364  1.6272727272727)'
- PostGIS:    POINT(4.06923363095238 1.65055803571429)
- Oracle:           4.06923363095238 1.65055803571429
- SQL Server: POINT(4.06923362245959 1.65055804168294)

Statements:
- \b MySQL/PostGIS: select AsText(Centroid(GeomFromText(
    'POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6
        ,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3))')))
- \b Oracle: select sdo_geom.sdo_centroid(sdo_geometry(2003, null, null,
        sdo_elem_info_array(1, 1003, 1), sdo_ordinate_array(
            2,1.3,2.4,1.7,2.8,1.8,3.4,1.2,3.7,1.6,3.4,2,4.1,3,5.3,2.6
            ,5.4,1.2,4.9,0.8,2.9,0.7,2,1.3))
        , mdsys.sdo_dim_array(mdsys.sdo_dim_element('x',0,10,.00000005)
        ,mdsys.sdo_dim_element('y',0,10,.00000005)))
        from dual
- \b SQL Server 2008: select geometry::STGeomFromText(
    'POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6
        ,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3))',0)
                .STCentroid()
                .STAsText()

With holes:
- this:       POINT(4.04663 1.6349)
- geolib:     POINT(4.04675 1.65735)
- MySQL:      POINT(3.6090580503834 1.607573932092)
- PostGIS:    POINT(4.0466265060241 1.63489959839357)
- Oracle:           4.0466265060241 1.63489959839357
- SQL Server: POINT(4.0466264962959677 1.6348996057331333)

Statements:
- \b MySQL/PostGIS: select AsText(Centroid(GeomFromText(
    'POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2
        ,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)
        ,(4 2,4.2 1.4,4.8 1.9,4.4 2.2,4 2))')));
- \b Oracle: select sdo_geom.sdo_centroid(sdo_geometry(2003, null, null
        , sdo_elem_info_array(1, 1003, 1, 25, 2003, 1)
        , sdo_ordinate_array(2,1.3,2.4,1.7,2.8,1.8,3.4,1.2,3.7,1.6,3.4,
        2,4.1,3,5.3,2.6,5.4,1.2,4.9,0.8,2.9,0.7,2,1.3,4,2, 4.2,1.4,
        4.8,1.9, 4.4,2.2, 4,2))
        , mdsys.sdo_dim_array(mdsys.sdo_dim_element('x',0,10,.00000005)
        ,mdsys.sdo_dim_element('y',0,10,.00000005)))
        from dual
 */
template
<
    typename Ignored1 = void,
    typename Ignored2 = void,
    typename CalculationType = void
>
class bashein_detmer
{
private :
    // If user specified a calculation type, use that type,
    //   whatever it is and whatever the point-type(s) are.
    // Else, use the most appropriate coordinate type
    //    of the two points, but at least double
    template <typename GeometryPoint, typename ResultPoint>
    struct calculation_type
        : std::conditional
            <
                std::is_void<CalculationType>::value,
                typename select_most_precise
                    <
                        typename coordinate_type<GeometryPoint>::type,
                        typename coordinate_type<ResultPoint>::type,
                        double
                    >::type,
                CalculationType
            >
    {};

    /*! subclass to keep state */
    template <typename GeometryPoint, typename ResultPoint>
    class sums
    {
        typedef typename calculation_type<GeometryPoint, ResultPoint>::type calc_type;

        friend class bashein_detmer;
        std::size_t count;
        calc_type sum_a2;
        calc_type sum_x;
        calc_type sum_y;

    public :
        inline sums()
            : count(0)
            , sum_a2(calc_type())
            , sum_x(calc_type())
            , sum_y(calc_type())
        {}
    };

public :
    template <typename GeometryPoint, typename ResultPoint>
    struct state_type
    {
        typedef sums<GeometryPoint, ResultPoint> type;
    };

    template <typename GeometryPoint, typename ResultPoint>
    static inline void apply(GeometryPoint const& p1, GeometryPoint const& p2,
                             sums<GeometryPoint, ResultPoint>& state)
    {
        /* Algorithm:
        For each segment:
        begin
            ai = x1 * y2 - x2 * y1;
            sum_a2 += ai;
            sum_x += ai * (x1 + x2);
            sum_y += ai * (y1 + y2);
        end
        return POINT(sum_x / (3 * sum_a2), sum_y / (3 * sum_a2) )
        */

        typedef typename calculation_type<GeometryPoint, ResultPoint>::type calc_type;

        // Get coordinates and promote them to calculation_type
        calc_type const x1 = boost::numeric_cast<calc_type>(get<0>(p1));
        calc_type const y1 = boost::numeric_cast<calc_type>(get<1>(p1));
        calc_type const x2 = boost::numeric_cast<calc_type>(get<0>(p2));
        calc_type const y2 = boost::numeric_cast<calc_type>(get<1>(p2));
        calc_type const ai = geometry::detail::determinant<calc_type>(p1, p2);
        state.count++;
        state.sum_a2 += ai;
        state.sum_x += ai * (x1 + x2);
        state.sum_y += ai * (y1 + y2);
    }

    template <typename GeometryPoint, typename ResultPoint>
    static inline bool result(sums<GeometryPoint, ResultPoint> const& state,
                              ResultPoint& centroid)
    {
        typedef typename calculation_type<GeometryPoint, ResultPoint>::type calc_type;

        calc_type const zero = calc_type();
        if (state.count > 0 && ! math::equals(state.sum_a2, zero))
        {
            calc_type const v3 = 3;
            calc_type const a3 = v3 * state.sum_a2;

            typedef typename geometry::coordinate_type
                <
                    ResultPoint
                >::type coordinate_type;

            // Prevent NaN centroid coordinates
            if (boost::math::isfinite(a3))
            {
                // NOTE: above calculation_type is checked, not the centroid coordinate_type
                // which means that the centroid can still be filled with INF
                // if e.g. calculation_type is double and centroid contains floats
                set<0>(centroid,
                    boost::numeric_cast<coordinate_type>(state.sum_x / a3));
                set<1>(centroid,
                    boost::numeric_cast<coordinate_type>(state.sum_y / a3));
                return true;
            }
        }

        return false;
    }

};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

// Register this strategy for rings and (multi)polygons, in two dimensions
template <typename Point, typename Geometry>
struct default_strategy<cartesian_tag, areal_tag, 2, Point, Geometry>
{
    typedef bashein_detmer
        <
            Point,
            typename point_type<Geometry>::type
        > type;
};


} // namespace services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::centroid


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_CENTROID_BASHEIN_DETMER_HPP
