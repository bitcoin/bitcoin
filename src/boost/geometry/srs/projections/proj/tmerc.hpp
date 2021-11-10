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

#ifndef BOOST_GEOMETRY_PROJECTIONS_TMERC_HPP
#define BOOST_GEOMETRY_PROJECTIONS_TMERC_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/function_overloads.hpp>
#include <boost/geometry/srs/projections/impl/pj_mlfn.hpp>


namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace tmerc
    {

            static const double epsilon10 = 1.e-10;

            template <typename T>
            inline T FC1() { return 1.; }
            template <typename T>
            inline T FC2() { return .5; }
            template <typename T>
            inline T FC3() { return .16666666666666666666666666666666666666; }
            template <typename T>
            inline T FC4() { return .08333333333333333333333333333333333333; }
            template <typename T>
            inline T FC5() { return .05; }
            template <typename T>
            inline T FC6() { return .03333333333333333333333333333333333333; }
            template <typename T>
            inline T FC7() { return .02380952380952380952380952380952380952; }
            template <typename T>
            inline T FC8() { return .01785714285714285714285714285714285714; }

            template <typename T>
            struct par_tmerc
            {
                T    esp;
                T    ml0;
                detail::en<T> en;
            };

            template <typename T, typename Parameters>
            struct base_tmerc_ellipsoid
            {
                par_tmerc<T> m_proj_parm;

                // FORWARD(e_forward)  ellipse
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T FC1 = tmerc::FC1<T>();
                    static const T FC2 = tmerc::FC2<T>();
                    static const T FC3 = tmerc::FC3<T>();
                    static const T FC4 = tmerc::FC4<T>();
                    static const T FC5 = tmerc::FC5<T>();
                    static const T FC6 = tmerc::FC6<T>();
                    static const T FC7 = tmerc::FC7<T>();
                    static const T FC8 = tmerc::FC8<T>();

                    T al, als, n, cosphi, sinphi, t;

                    /*
                     * Fail if our longitude is more than 90 degrees from the
                     * central meridian since the results are essentially garbage.
                     * Is error -20 really an appropriate return value?
                     *
                     *  http://trac.osgeo.org/proj/ticket/5
                     */
                    if( lp_lon < -half_pi || lp_lon > half_pi )
                    {
                        xy_x = HUGE_VAL;
                        xy_y = HUGE_VAL;
                        BOOST_THROW_EXCEPTION( projection_exception(error_lat_or_lon_exceed_limit) );
                        return;
                    }

                    sinphi = sin(lp_lat);
                    cosphi = cos(lp_lat);
                    t = fabs(cosphi) > 1e-10 ? sinphi/cosphi : 0.;
                    t *= t;
                    al = cosphi * lp_lon;
                    als = al * al;
                    al /= sqrt(1. - par.es * sinphi * sinphi);
                    n = this->m_proj_parm.esp * cosphi * cosphi;
                    xy_x = par.k0 * al * (FC1 +
                        FC3 * als * (1. - t + n +
                        FC5 * als * (5. + t * (t - 18.) + n * (14. - 58. * t)
                        + FC7 * als * (61. + t * ( t * (179. - t) - 479. ) )
                        )));
                    xy_y = par.k0 * (pj_mlfn(lp_lat, sinphi, cosphi, this->m_proj_parm.en) - this->m_proj_parm.ml0 +
                        sinphi * al * lp_lon * FC2 * ( 1. +
                        FC4 * als * (5. - t + n * (9. + 4. * n) +
                        FC6 * als * (61. + t * (t - 58.) + n * (270. - 330 * t)
                        + FC8 * als * (1385. + t * ( t * (543. - t) - 3111.) )
                        ))));
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T FC1 = tmerc::FC1<T>();
                    static const T FC2 = tmerc::FC2<T>();
                    static const T FC3 = tmerc::FC3<T>();
                    static const T FC4 = tmerc::FC4<T>();
                    static const T FC5 = tmerc::FC5<T>();
                    static const T FC6 = tmerc::FC6<T>();
                    static const T FC7 = tmerc::FC7<T>();
                    static const T FC8 = tmerc::FC8<T>();

                    T n, con, cosphi, d, ds, sinphi, t;

                    lp_lat = pj_inv_mlfn(this->m_proj_parm.ml0 + xy_y / par.k0, par.es, this->m_proj_parm.en);
                    if (fabs(lp_lat) >= half_pi) {
                        lp_lat = xy_y < 0. ? -half_pi : half_pi;
                        lp_lon = 0.;
                    } else {
                        sinphi = sin(lp_lat);
                        cosphi = cos(lp_lat);
                        t = fabs(cosphi) > 1e-10 ? sinphi/cosphi : 0.;
                        n = this->m_proj_parm.esp * cosphi * cosphi;
                        d = xy_x * sqrt(con = 1. - par.es * sinphi * sinphi) / par.k0;
                        con *= t;
                        t *= t;
                        ds = d * d;
                        lp_lat -= (con * ds / (1.-par.es)) * FC2 * (1. -
                            ds * FC4 * (5. + t * (3. - 9. *  n) + n * (1. - 4 * n) -
                            ds * FC6 * (61. + t * (90. - 252. * n +
                                45. * t) + 46. * n
                           - ds * FC8 * (1385. + t * (3633. + t * (4095. + 1574. * t)) )
                            )));
                        lp_lon = d*(FC1 -
                            ds*FC3*( 1. + 2.*t + n -
                            ds*FC5*(5. + t*(28. + 24.*t + 8.*n) + 6.*n
                           - ds * FC7 * (61. + t * (662. + t * (1320. + 720. * t)) )
                        ))) / cosphi;
                    }
                }

                static inline std::string get_name()
                {
                    return "tmerc_ellipsoid";
                }

            };

            template <typename T, typename Parameters>
            struct base_tmerc_spheroid
            {
                par_tmerc<T> m_proj_parm;

                // FORWARD(s_forward)  sphere
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T b, cosphi;

                    /*
                     * Fail if our longitude is more than 90 degrees from the
                     * central meridian since the results are essentially garbage.
                     * Is error -20 really an appropriate return value?
                     *
                     *  http://trac.osgeo.org/proj/ticket/5
                     */
                    if( lp_lon < -half_pi || lp_lon > half_pi )
                    {
                        xy_x = HUGE_VAL;
                        xy_y = HUGE_VAL;
                        BOOST_THROW_EXCEPTION( projection_exception(error_lat_or_lon_exceed_limit) );
                        return;
                    }

                    cosphi = cos(lp_lat);
                    b = cosphi * sin(lp_lon);
                    if (fabs(fabs(b) - 1.) <= epsilon10)
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );

                    xy_x = this->m_proj_parm.ml0 * log((1. + b) / (1. - b));
                    xy_y = cosphi * cos(lp_lon) / sqrt(1. - b * b);

                    b = fabs( xy_y );
                    if (b >= 1.) {
                        if ((b - 1.) > epsilon10)
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        else xy_y = 0.;
                    } else
                        xy_y = acos(xy_y);

                    if (lp_lat < 0.)
                        xy_y = -xy_y;
                    xy_y = this->m_proj_parm.esp * (xy_y - par.phi0);
                }

                // INVERSE(s_inverse)  sphere
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    T h, g;

                    h = exp(xy_x / this->m_proj_parm.esp);
                    g = .5 * (h - 1. / h);
                    h = cos(par.phi0 + xy_y / this->m_proj_parm.esp);
                    lp_lat = asin(sqrt((1. - h * h) / (1. + g * g)));

                    /* Make sure that phi is on the correct hemisphere when false northing is used */
                    if (xy_y < 0. && -lp_lat+par.phi0 < 0.0) lp_lat = -lp_lat;

                    lp_lon = (g != 0.0 || h != 0.0) ? atan2(g, h) : 0.;
                }

                static inline std::string get_name()
                {
                    return "tmerc_spheroid";
                }

            };

            template <typename Parameters, typename T>
            inline void setup(Parameters const& par, par_tmerc<T>& proj_parm)
            {
                if (par.es != 0.0) {
                    proj_parm.en = pj_enfn<T>(par.es);
                    proj_parm.ml0 = pj_mlfn(par.phi0, sin(par.phi0), cos(par.phi0), proj_parm.en);
                    proj_parm.esp = par.es / (1. - par.es);
                } else {
                    proj_parm.esp = par.k0;
                    proj_parm.ml0 = .5 * proj_parm.esp;
                }
            }

    }} // namespace detail::tmerc
    #endif // doxygen

    /*!
        \brief Transverse Mercator projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Cylindrical
         - Spheroid
         - Ellipsoid
        \par Example
        \image html ex_tmerc.gif
    */
    template <typename T, typename Parameters>
    struct tmerc_ellipsoid : public detail::tmerc::base_tmerc_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline tmerc_ellipsoid(Params const&, Parameters const& par)
        {
            detail::tmerc::setup(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Transverse Mercator projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Cylindrical
         - Spheroid
         - Ellipsoid
        \par Example
        \image html ex_tmerc.gif
    */
    template <typename T, typename Parameters>
    struct tmerc_spheroid : public detail::tmerc::base_tmerc_spheroid<T, Parameters>
    {
        template <typename Params>
        inline tmerc_spheroid(Params const&, Parameters const& par)
        {
            detail::tmerc::setup(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI2(srs::spar::proj_tmerc, tmerc_spheroid, tmerc_ellipsoid)
        
        // Factory entry(s) - dynamic projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI2(tmerc_entry, tmerc_spheroid, tmerc_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(tmerc_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(tmerc, tmerc_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_TMERC_HPP

