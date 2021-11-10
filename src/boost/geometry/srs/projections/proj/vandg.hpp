// Boost.Geometry - gis-projections (based on PROJ4)

// Copyright (c) 2008-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017, 2018, 2019.
// Modifications copyright (c) 2017-2019, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// PROJ4 is converted to Boost.Geometry by Barend Gehrels

// Last updated version of proj: 5.0.0

// Original copyright notice:

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef BOOST_GEOMETRY_PROJECTIONS_VANDG_HPP
#define BOOST_GEOMETRY_PROJECTIONS_VANDG_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace vandg
    {

            static const double tolerance = 1.e-10;

            template <typename T>
            inline T C2_27() { return .07407407407407407407407407407407; }
            template <typename T>
            inline T PI4_3() { return boost::math::constants::four_thirds_pi<T>(); }
            template <typename T>
            inline T TPISQ() { return 19.739208802178717237668981999752; }
            template <typename T>
            inline T HPISQ() { return 4.9348022005446793094172454999381; }

            template <typename T, typename Parameters>
            struct base_vandg_spheroid
            {
                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T pi = detail::pi<T>();

                    T  al, al2, g, g2, p2;

                    p2 = fabs(lp_lat / half_pi);
                    if ((p2 - tolerance) > 1.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    if (p2 > 1.)
                        p2 = 1.;
                    if (fabs(lp_lat) <= tolerance) {
                        xy_x = lp_lon;
                        xy_y = 0.;
                    } else if (fabs(lp_lon) <= tolerance || fabs(p2 - 1.) < tolerance) {
                        xy_x = 0.;
                        xy_y = pi * tan(.5 * asin(p2));
                        if (lp_lat < 0.) xy_y = -xy_y;
                    } else {
                        al = .5 * fabs(pi / lp_lon - lp_lon / pi);
                        al2 = al * al;
                        g = sqrt(1. - p2 * p2);
                        g = g / (p2 + g - 1.);
                        g2 = g * g;
                        p2 = g * (2. / p2 - 1.);
                        p2 = p2 * p2;
                        xy_x = g - p2; g = p2 + al2;
                        xy_x = pi * (al * xy_x + sqrt(al2 * xy_x * xy_x - g * (g2 - p2))) / g;
                        if (lp_lon < 0.) xy_x = -xy_x;
                        xy_y = fabs(xy_x / pi);
                        xy_y = 1. - xy_y * (xy_y + 2. * al);
                        if (xy_y < -tolerance) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        if (xy_y < 0.)
                            xy_y = 0.;
                        else
                            xy_y = sqrt(xy_y) * (lp_lat < 0. ? -pi : pi);
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T pi = detail::pi<T>();
                    static const T pi_sqr = detail::pi_sqr<T>();
                    static const T third = detail::third<T>();
                    static const T two_pi = detail::two_pi<T>();

                    static const T C2_27 = vandg::C2_27<T>();
                    static const T PI4_3 = vandg::PI4_3<T>();                    
                    static const T TPISQ = vandg::TPISQ<T>();
                    static const T HPISQ = vandg::HPISQ<T>();
                    
                    T t, c0, c1, c2, c3, al, r2, r, m, d, ay, x2, y2;

                    x2 = xy_x * xy_x;
                    if ((ay = fabs(xy_y)) < tolerance) {
                        lp_lat = 0.;
                        t = x2 * x2 + TPISQ * (x2 + HPISQ);
                        lp_lon = fabs(xy_x) <= tolerance ? 0. :
                           .5 * (x2 - pi_sqr + sqrt(t)) / xy_x;
                            return;
                    }
                    y2 = xy_y * xy_y;
                    r = x2 + y2;    r2 = r * r;
                    c1 = - pi * ay * (r + pi_sqr);
                    c3 = r2 + two_pi * (ay * r + pi * (y2 + pi * (ay + half_pi)));
                    c2 = c1 + pi_sqr * (r - 3. *  y2);
                    c0 = pi * ay;
                    c2 /= c3;
                    al = c1 / c3 - third * c2 * c2;
                    m = 2. * sqrt(-third * al);
                    d = C2_27 * c2 * c2 * c2 + (c0 * c0 - third * c2 * c1) / c3;
                    if (((t = fabs(d = 3. * d / (al * m))) - tolerance) <= 1.) {
                        d = t > 1. ? (d > 0. ? 0. : pi) : acos(d);
                        lp_lat = pi * (m * cos(d * third + PI4_3) - third * c2);
                        if (xy_y < 0.) lp_lat = -lp_lat;
                        t = r2 + TPISQ * (x2 - y2 + HPISQ);
                        lp_lon = fabs(xy_x) <= tolerance ? 0. :
                           .5 * (r - pi_sqr + (t <= 0. ? 0. : sqrt(t))) / xy_x;
                    } else {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                }

                static inline std::string get_name()
                {
                    return "vandg_spheroid";
                }

            };

            // van der Grinten (I)
            template <typename Parameters>
            inline void setup_vandg(Parameters& par)
            {
                par.es = 0.;
            }

    }} // namespace detail::vandg
    #endif // doxygen

    /*!
        \brief van der Grinten (I) projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Example
        \image html ex_vandg.gif
    */
    template <typename T, typename Parameters>
    struct vandg_spheroid : public detail::vandg::base_vandg_spheroid<T, Parameters>
    {
        template <typename Params>
        inline vandg_spheroid(Params const& , Parameters & par)
        {
            detail::vandg::setup_vandg(par);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_vandg, vandg_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(vandg_entry, vandg_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(vandg_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(vandg, vandg_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_VANDG_HPP

