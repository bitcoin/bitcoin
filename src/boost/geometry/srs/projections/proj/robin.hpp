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

#ifndef BOOST_GEOMETRY_PROJECTIONS_ROBIN_HPP
#define BOOST_GEOMETRY_PROJECTIONS_ROBIN_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/function_overloads.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace robin
    {

            static const double FXC = 0.8487;
            static const double FYC = 1.3523;
            static const double C1 = 11.45915590261646417544;
            static const double RC1 = 0.08726646259971647884;
            static const int n_nodes = 18;
            static const double one_plus_eps = 1.000001;
            static const double epsilon = 1e-8;
            /* Not sure at all of the appropriate number for max_iter... */
            static const int max_iter = 100;

            /*
            note: following terms based upon 5 deg. intervals in degrees.

            Some background on these coefficients is available at:

            http://article.gmane.org/gmane.comp.gis.proj-4.devel/6039
            http://trac.osgeo.org/proj/ticket/113
            */

            template <typename T>
            struct coefs {
                T c0, c1, c2, c3;
            };

            template <typename T>
            inline const coefs<T> * coefs_x()
            {
                static const coefs<T> result[] = {
                    {1.0, 2.2199e-17, -7.15515e-05, 3.1103e-06},
                    {0.9986, -0.000482243, -2.4897e-05, -1.3309e-06},
                    {0.9954, -0.00083103, -4.48605e-05, -9.86701e-07},
                    {0.99, -0.00135364, -5.9661e-05, 3.6777e-06},
                    {0.9822, -0.00167442, -4.49547e-06, -5.72411e-06},
                    {0.973, -0.00214868, -9.03571e-05, 1.8736e-08},
                    {0.96, -0.00305085, -9.00761e-05, 1.64917e-06},
                    {0.9427, -0.00382792, -6.53386e-05, -2.6154e-06},
                    {0.9216, -0.00467746, -0.00010457, 4.81243e-06},
                    {0.8962, -0.00536223, -3.23831e-05, -5.43432e-06},
                    {0.8679, -0.00609363, -0.000113898, 3.32484e-06},
                    {0.835, -0.00698325, -6.40253e-05, 9.34959e-07},
                    {0.7986, -0.00755338, -5.00009e-05, 9.35324e-07},
                    {0.7597, -0.00798324, -3.5971e-05, -2.27626e-06},
                    {0.7186, -0.00851367, -7.01149e-05, -8.6303e-06},
                    {0.6732, -0.00986209, -0.000199569, 1.91974e-05},
                    {0.6213, -0.010418, 8.83923e-05, 6.24051e-06},
                    {0.5722, -0.00906601, 0.000182, 6.24051e-06},
                    {0.5322, -0.00677797, 0.000275608, 6.24051e-06}
                };
                return result;
            }

            template <typename T>
            inline const coefs<T> * coefs_y()
            {
                static const coefs<T> result[] = {
                    {-5.20417e-18, 0.0124, 1.21431e-18, -8.45284e-11},
                    {0.062, 0.0124, -1.26793e-09, 4.22642e-10},
                    {0.124, 0.0124, 5.07171e-09, -1.60604e-09},
                    {0.186, 0.0123999, -1.90189e-08, 6.00152e-09},
                    {0.248, 0.0124002, 7.10039e-08, -2.24e-08},
                    {0.31, 0.0123992, -2.64997e-07, 8.35986e-08},
                    {0.372, 0.0124029, 9.88983e-07, -3.11994e-07},
                    {0.434, 0.0123893, -3.69093e-06, -4.35621e-07},
                    {0.4958, 0.0123198, -1.02252e-05, -3.45523e-07},
                    {0.5571, 0.0121916, -1.54081e-05, -5.82288e-07},
                    {0.6176, 0.0119938, -2.41424e-05, -5.25327e-07},
                    {0.6769, 0.011713, -3.20223e-05, -5.16405e-07},
                    {0.7346, 0.0113541, -3.97684e-05, -6.09052e-07},
                    {0.7903, 0.0109107, -4.89042e-05, -1.04739e-06},
                    {0.8435, 0.0103431, -6.4615e-05, -1.40374e-09},
                    {0.8936, 0.00969686, -6.4636e-05, -8.547e-06},
                    {0.9394, 0.00840947, -0.000192841, -4.2106e-06},
                    {0.9761, 0.00616527, -0.000256, -4.2106e-06},
                    {1.0, 0.00328947, -0.000319159, -4.2106e-06}
                };
                return result;
            }

            template <typename T, typename Parameters>
            struct base_robin_spheroid
            {
                inline T v(coefs<T> const& c, T const& z) const
                { return (c.c0 + z * (c.c1 + z * (c.c2 + z * c.c3))); }
                inline T dv(coefs<T> const& c, T const&  z) const
                { return (c.c1 + z * (c.c2 + c.c2 + z * 3. * c.c3)); }

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    int i;
                    T dphi;

                    i = int_floor((dphi = fabs(lp_lat)) * C1);
                    if (i < 0) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    if (i >= n_nodes) i = n_nodes - 1;
                    dphi = geometry::math::r2d<T>() * (dphi - RC1 * i);
                    xy_x = v(coefs_x<T>()[i], dphi) * FXC * lp_lon;
                    xy_y = v(coefs_y<T>()[i], dphi) * FYC;
                    if (lp_lat < 0.) xy_y = -xy_y;
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    const coefs<T> * coefs_x = robin::coefs_x<T>();
                    const coefs<T> * coefs_y = robin::coefs_y<T>();

                    int i;
                    T t, t1;
                    coefs<T> coefs_t;
                    int iters;

                    lp_lon = xy_x / FXC;
                    lp_lat = fabs(xy_y / FYC);
                    if (lp_lat >= 1.) { /* simple pathologic cases */
                        if (lp_lat > one_plus_eps) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        } else {
                            lp_lat = xy_y < 0. ? -half_pi : half_pi;
                            lp_lon /= coefs_x[n_nodes].c0;
                        }
                    } else { /* general problem */
                        /* in Y space, reduce to table interval */
                        i = int_floor(lp_lat * n_nodes);
                        if( i < 0 || i >= n_nodes ) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        for (;;) {
                            if (coefs_y[i].c0 > lp_lat) --i;
                            else if (coefs_y[i+1].c0 <= lp_lat) ++i;
                            else break;
                        }
                        coefs_t = coefs_y[i];
                        /* first guess, linear interp */
                        t = 5. * (lp_lat - coefs_t.c0)/(coefs_y[i+1].c0 - coefs_t.c0);
                        /* make into root */
                        coefs_t.c0 = (T)(coefs_t.c0 - lp_lat);
                        for (iters = max_iter; iters ; --iters) { /* Newton-Raphson */
                            t -= t1 = v(coefs_t,t) / dv(coefs_t,t);
                            if (fabs(t1) < epsilon)
                                break;
                        }
                        if( iters == 0 )
                            BOOST_THROW_EXCEPTION( projection_exception(error_non_convergent) );
                        lp_lat = (5 * i + t) * geometry::math::d2r<T>();
                        if (xy_y < 0.) lp_lat = -lp_lat;
                        lp_lon /= v(coefs_x[i], t);
                    }
                }

                static inline std::string get_name()
                {
                    return "robin_spheroid";
                }

            };

            // Robinson
            template <typename Parameters>
            inline void setup_robin(Parameters& par)
            {
                par.es = 0.;
            }

    }} // namespace detail::robin
    #endif // doxygen

    /*!
        \brief Robinson projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_robin.gif
    */
    template <typename T, typename Parameters>
    struct robin_spheroid : public detail::robin::base_robin_spheroid<T, Parameters>
    {
        template <typename Params>
        inline robin_spheroid(Params const& , Parameters & par)
        {
            detail::robin::setup_robin(par);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_robin, robin_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(robin_entry, robin_spheroid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(robin_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(robin, robin_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_ROBIN_HPP

