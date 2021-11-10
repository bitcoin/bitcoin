// Boost.Geometry

// Copyright (c) 2016 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_GNOMONIC_INTERSECTION_HPP
#define BOOST_GEOMETRY_FORMULAS_GNOMONIC_INTERSECTION_HPP

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/cs.hpp>

#include <boost/geometry/arithmetic/cross_product.hpp>
#include <boost/geometry/formulas/gnomonic_spheroid.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry { namespace formula
{

/*!
\brief The intersection of two geodesics using spheroidal gnomonic projection
       as proposed by Karney.
\author See
    - Charles F.F Karney, Algorithms for geodesics, 2011
      https://arxiv.org/pdf/1109.4448.pdf
    - GeographicLib forum thread: Intersection between two geodesic lines
      https://sourceforge.net/p/geographiclib/discussion/1026621/thread/21aaff9f/
*/
template
<
    typename CT,
    template <typename, bool, bool, bool, bool, bool> class Inverse,
    template <typename, bool, bool, bool, bool> class Direct
>
class gnomonic_intersection
{
public:
    template <typename T1, typename T2, typename Spheroid>
    static inline bool apply(T1 const& lona1, T1 const& lata1,
                             T1 const& lona2, T1 const& lata2,
                             T2 const& lonb1, T2 const& latb1,
                             T2 const& lonb2, T2 const& latb2,
                             CT & lon, CT & lat,
                             Spheroid const& spheroid)
    {
        CT const lon_a1 = lona1;
        CT const lat_a1 = lata1;
        CT const lon_a2 = lona2;
        CT const lat_a2 = lata2;
        CT const lon_b1 = lonb1;
        CT const lat_b1 = latb1;
        CT const lon_b2 = lonb2;
        CT const lat_b2 = latb2;

        return apply(lon_a1, lat_a1, lon_a2, lat_a2, lon_b1, lat_b1, lon_b2, lat_b2, lon, lat, spheroid);
    }
    
    template <typename Spheroid>
    static inline bool apply(CT const& lona1, CT const& lata1,
                             CT const& lona2, CT const& lata2,
                             CT const& lonb1, CT const& latb1,
                             CT const& lonb2, CT const& latb2,
                             CT & lon, CT & lat,
                             Spheroid const& spheroid)
    {
        typedef gnomonic_spheroid<CT, Inverse, Direct> gnom_t;

        lon = (lona1 + lona2 + lonb1 + lonb2) / 4;
        lat = (lata1 + lata2 + latb1 + latb2) / 4;
        // TODO: consider normalizing lon

        for (int i = 0; i < 10; ++i)
        {
            CT xa1, ya1, xa2, ya2;
            CT xb1, yb1, xb2, yb2;
            CT x, y;
            double lat1, lon1;

            bool ok = gnom_t::forward(lon, lat, lona1, lata1, xa1, ya1, spheroid)
                   && gnom_t::forward(lon, lat, lona2, lata2, xa2, ya2, spheroid)
                   && gnom_t::forward(lon, lat, lonb1, latb1, xb1, yb1, spheroid)
                   && gnom_t::forward(lon, lat, lonb2, latb2, xb2, yb2, spheroid)
                   && intersect(xa1, ya1, xa2, ya2, xb1, yb1, xb2, yb2, x, y)
                   && gnom_t::inverse(lon, lat, x, y, lon1, lat1, spheroid);

            if (! ok)
            {
                return false;
            }
            
            if (math::equals(lat1, lat) && math::equals(lon1, lon))
            {
                break;
            }

            lat = lat1;
            lon = lon1;
        }

        // NOTE: true is also returned if the number of iterations is too great
        //       which means that the accuracy of the result is low
        return true;
    }

private:
    static inline bool intersect(CT const& xa1, CT const& ya1, CT const& xa2, CT const& ya2,
                                 CT const& xb1, CT const& yb1, CT const& xb2, CT const& yb2,
                                 CT & x, CT & y)
    {
        typedef model::point<CT, 3, cs::cartesian> v3d_t;

        CT const c0 = 0;
        CT const c1 = 1;

        v3d_t const va1(xa1, ya1, c1);
        v3d_t const va2(xa2, ya2, c1);
        v3d_t const vb1(xb1, yb1, c1);
        v3d_t const vb2(xb2, yb2, c1);

        v3d_t const la = cross_product(va1, va2);
        v3d_t const lb = cross_product(vb1, vb2);
        v3d_t const p = cross_product(la, lb);

        CT const z = get<2>(p);

        if (math::equals(z, c0))
        {
            // degenerated or collinear segments
            return false;
        }

        x = get<0>(p) / z;
        y = get<1>(p) / z;
        
        return true;
    }
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_GNOMONIC_INTERSECTION_HPP
