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

#ifndef BOOST_GEOMETRY_PROJECTIONS_STERE_HPP
#define BOOST_GEOMETRY_PROJECTIONS_STERE_HPP

#include <boost/config.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/math/special_functions/hypot.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/pj_tsfn.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace stere
    {
            static const double epsilon10 = 1.e-10;
            static const double tolerance = 1.e-8;
            static const int n_iter = 8;
            static const double conv_tolerance = 1.e-10;

            enum mode_type {
                s_pole = 0,
                n_pole = 1,
                obliq  = 2,
                equit  = 3
            };

            template <typename T>
            struct par_stere
            {
                T   phits;
                T   sinX1;
                T   cosX1;
                T   akm1;
                mode_type mode;
            };

            template <typename T>
            inline T ssfn_(T const& phit, T sinphi, T const& eccen)
            {
                static const T half_pi = detail::half_pi<T>();

                sinphi *= eccen;
                return (tan (.5 * (half_pi + phit)) *
                   math::pow((T(1) - sinphi) / (T(1) + sinphi), T(0.5) * eccen));
            }

            template <typename T, typename Parameters>
            struct base_stere_ellipsoid
            {
                par_stere<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T coslam, sinlam, sinX=0.0, cosX=0.0, X, A = 0.0, sinphi;

                    coslam = cos(lp_lon);
                    sinlam = sin(lp_lon);
                    sinphi = sin(lp_lat);
                    if (this->m_proj_parm.mode == obliq || this->m_proj_parm.mode == equit) {
                        sinX = sin(X = 2. * atan(ssfn_(lp_lat, sinphi, par.e)) - half_pi);
                        cosX = cos(X);
                    }
                    switch (this->m_proj_parm.mode) {
                    case obliq:
                        A = this->m_proj_parm.akm1 / (this->m_proj_parm.cosX1 * (1. + this->m_proj_parm.sinX1 * sinX +
                           this->m_proj_parm.cosX1 * cosX * coslam));
                        xy_y = A * (this->m_proj_parm.cosX1 * sinX - this->m_proj_parm.sinX1 * cosX * coslam);
                        goto xmul; /* but why not just  xy.x = A * cosX; break;  ? */

                    case equit:
                        // TODO: calculate denominator once
                        /* avoid zero division */
                        if (1. + cosX * coslam == 0.0) {
                            xy_y = HUGE_VAL;
                        } else {
                            A = this->m_proj_parm.akm1 / (1. + cosX * coslam);
                            xy_y = A * sinX;
                        }
                xmul:
                        xy_x = A * cosX;
                        break;

                    case s_pole:
                        lp_lat = -lp_lat;
                        coslam = - coslam;
                        sinphi = -sinphi;
                        BOOST_FALLTHROUGH;
                    case n_pole:
                        xy_x = this->m_proj_parm.akm1 * pj_tsfn(lp_lat, sinphi, par.e);
                        xy_y = - xy_x * coslam;
                        break;
                    }

                    xy_x = xy_x * sinlam;
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T cosphi, sinphi, tp=0.0, phi_l=0.0, rho, halfe=0.0, halfpi=0.0;
                    int i;

                    rho = boost::math::hypot(xy_x, xy_y);
                    switch (this->m_proj_parm.mode) {
                    case obliq:
                    case equit:
                        cosphi = cos( tp = 2. * atan2(rho * this->m_proj_parm.cosX1 , this->m_proj_parm.akm1) );
                        sinphi = sin(tp);
                        if( rho == 0.0 )
                            phi_l = asin(cosphi * this->m_proj_parm.sinX1);
                        else
                            phi_l = asin(cosphi * this->m_proj_parm.sinX1 + (xy_y * sinphi * this->m_proj_parm.cosX1 / rho));

                        tp = tan(.5 * (half_pi + phi_l));
                        xy_x *= sinphi;
                        xy_y = rho * this->m_proj_parm.cosX1 * cosphi - xy_y * this->m_proj_parm.sinX1* sinphi;
                        halfpi = half_pi;
                        halfe = .5 * par.e;
                        break;
                    case n_pole:
                        xy_y = -xy_y;
                        BOOST_FALLTHROUGH;
                    case s_pole:
                        phi_l = half_pi - 2. * atan(tp = - rho / this->m_proj_parm.akm1);
                        halfpi = -half_pi;
                        halfe = -.5 * par.e;
                        break;
                    }
                    for (i = n_iter; i--; phi_l = lp_lat) {
                        sinphi = par.e * sin(phi_l);
                        lp_lat = T(2) * atan(tp * math::pow((T(1)+sinphi)/(T(1)-sinphi), halfe)) - halfpi;
                        if (fabs(phi_l - lp_lat) < conv_tolerance) {
                            if (this->m_proj_parm.mode == s_pole)
                                lp_lat = -lp_lat;
                            lp_lon = (xy_x == 0. && xy_y == 0.) ? 0. : atan2(xy_x, xy_y);
                            return;
                        }
                    }
                    BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                }

                static inline std::string get_name()
                {
                    return "stere_ellipsoid";
                }

            };

            template <typename T, typename Parameters>
            struct base_stere_spheroid
            {
                par_stere<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T fourth_pi = detail::fourth_pi<T>();
                    static const T half_pi = detail::half_pi<T>();

                    T  sinphi, cosphi, coslam, sinlam;

                    sinphi = sin(lp_lat);
                    cosphi = cos(lp_lat);
                    coslam = cos(lp_lon);
                    sinlam = sin(lp_lon);
                    switch (this->m_proj_parm.mode) {
                    case equit:
                        xy_y = 1. + cosphi * coslam;
                        goto oblcon;
                    case obliq:
                        xy_y = 1. + this->m_proj_parm.sinX1 * sinphi + this->m_proj_parm.cosX1 * cosphi * coslam;
                oblcon:
                        if (xy_y <= epsilon10) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        xy_x = (xy_y = this->m_proj_parm.akm1 / xy_y) * cosphi * sinlam;
                        xy_y *= (this->m_proj_parm.mode == equit) ? sinphi :
                           this->m_proj_parm.cosX1 * sinphi - this->m_proj_parm.sinX1 * cosphi * coslam;
                        break;
                    case n_pole:
                        coslam = - coslam;
                        lp_lat = - lp_lat;
                        BOOST_FALLTHROUGH;
                    case s_pole:
                        if (fabs(lp_lat - half_pi) < tolerance) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        xy_x = sinlam * ( xy_y = this->m_proj_parm.akm1 * tan(fourth_pi + .5 * lp_lat) );
                        xy_y *= coslam;
                        break;
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    T  c, rh, sinc, cosc;

                    sinc = sin(c = 2. * atan((rh = boost::math::hypot(xy_x, xy_y)) / this->m_proj_parm.akm1));
                    cosc = cos(c);
                    lp_lon = 0.;

                    switch (this->m_proj_parm.mode) {
                    case equit:
                        if (fabs(rh) <= epsilon10)
                            lp_lat = 0.;
                        else
                            lp_lat = asin(xy_y * sinc / rh);
                        if (cosc != 0. || xy_x != 0.)
                            lp_lon = atan2(xy_x * sinc, cosc * rh);
                        break;
                    case obliq:
                        if (fabs(rh) <= epsilon10)
                            lp_lat = par.phi0;
                        else
                            lp_lat = asin(cosc * this->m_proj_parm.sinX1 + xy_y * sinc * this->m_proj_parm.cosX1 / rh);
                        if ((c = cosc - this->m_proj_parm.sinX1 * sin(lp_lat)) != 0. || xy_x != 0.)
                            lp_lon = atan2(xy_x * sinc * this->m_proj_parm.cosX1, c * rh);
                        break;
                    case n_pole:
                        xy_y = -xy_y;
                        BOOST_FALLTHROUGH;
                    case s_pole:
                        if (fabs(rh) <= epsilon10)
                            lp_lat = par.phi0;
                        else
                            lp_lat = asin(this->m_proj_parm.mode == s_pole ? - cosc : cosc);
                        lp_lon = (xy_x == 0. && xy_y == 0.) ? 0. : atan2(xy_x, xy_y);
                        break;
                    }
                }

                static inline std::string get_name()
                {
                    return "stere_spheroid";
                }

            };

            template <typename Parameters, typename T>
            inline void setup(Parameters const& par, par_stere<T>& proj_parm)  /* general initialization */
            {
                static const T fourth_pi = detail::fourth_pi<T>();
                static const T half_pi = detail::half_pi<T>();

                T t;

                if (fabs((t = fabs(par.phi0)) - half_pi) < epsilon10)
                    proj_parm.mode = par.phi0 < 0. ? s_pole : n_pole;
                else
                    proj_parm.mode = t > epsilon10 ? obliq : equit;
                proj_parm.phits = fabs(proj_parm.phits);

                if (par.es != 0.0) {
                    T X;

                    switch (proj_parm.mode) {
                    case n_pole:
                    case s_pole:
                        if (fabs(proj_parm.phits - half_pi) < epsilon10)
                            proj_parm.akm1 = 2. * par.k0 /
                               sqrt(math::pow(T(1)+par.e,T(1)+par.e)*math::pow(T(1)-par.e,T(1)-par.e));
                        else {
                            proj_parm.akm1 = cos(proj_parm.phits) /
                               pj_tsfn(proj_parm.phits, t = sin(proj_parm.phits), par.e);
                            t *= par.e;
                            proj_parm.akm1 /= sqrt(1. - t * t);
                        }
                        break;
                    case equit:
                    case obliq:
                        t = sin(par.phi0);
                        X = 2. * atan(ssfn_(par.phi0, t, par.e)) - half_pi;
                        t *= par.e;
                        proj_parm.akm1 = 2. * par.k0 * cos(par.phi0) / sqrt(1. - t * t);
                        proj_parm.sinX1 = sin(X);
                        proj_parm.cosX1 = cos(X);
                        break;
                    }
                } else {
                    switch (proj_parm.mode) {
                    case obliq:
                        proj_parm.sinX1 = sin(par.phi0);
                        proj_parm.cosX1 = cos(par.phi0);
                        BOOST_FALLTHROUGH;
                    case equit:
                        proj_parm.akm1 = 2. * par.k0;
                        break;
                    case s_pole:
                    case n_pole:
                        proj_parm.akm1 = fabs(proj_parm.phits - half_pi) >= epsilon10 ?
                           cos(proj_parm.phits) / tan(fourth_pi - .5 * proj_parm.phits) :
                           2. * par.k0 ;
                        break;
                    }
                }
            }


            // Stereographic
            template <typename Params, typename Parameters, typename T>
            inline void setup_stere(Params const& params, Parameters const& par, par_stere<T>& proj_parm)
            {
                static const T half_pi = detail::half_pi<T>();

                if (! pj_param_r<srs::spar::lat_ts>(params, "lat_ts", srs::dpar::lat_ts, proj_parm.phits))
                    proj_parm.phits = half_pi;

                setup(par, proj_parm);
            }

            // Universal Polar Stereographic
            template <typename Params, typename Parameters, typename T>
            inline void setup_ups(Params const& params, Parameters& par, par_stere<T>& proj_parm)
            {
                static const T half_pi = detail::half_pi<T>();

                /* International Ellipsoid */
                par.phi0 = pj_get_param_b<srs::spar::south>(params, "south", srs::dpar::south) ? -half_pi: half_pi;
                if (par.es == 0.0) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_ellipsoid_use_required) );
                }
                par.k0 = .994;
                par.x0 = 2000000.;
                par.y0 = 2000000.;
                proj_parm.phits = half_pi;
                par.lam0 = 0.;

                setup(par, proj_parm);
            }

    }} // namespace detail::stere
    #endif // doxygen

    /*!
        \brief Stereographic projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - lat_ts: Latitude of true scale (degrees)
        \par Example
        \image html ex_stere.gif
    */
    template <typename T, typename Parameters>
    struct stere_ellipsoid : public detail::stere::base_stere_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline stere_ellipsoid(Params const& params, Parameters const& par)
        {
            detail::stere::setup_stere(params, par, this->m_proj_parm);
        }
    };

    /*!
        \brief Stereographic projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - lat_ts: Latitude of true scale (degrees)
        \par Example
        \image html ex_stere.gif
    */
    template <typename T, typename Parameters>
    struct stere_spheroid : public detail::stere::base_stere_spheroid<T, Parameters>
    {
        template <typename Params>
        inline stere_spheroid(Params const& params, Parameters const& par)
        {
            detail::stere::setup_stere(params, par, this->m_proj_parm);
        }
    };

    /*!
        \brief Universal Polar Stereographic projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - south: Denotes southern hemisphere UTM zone (boolean)
        \par Example
        \image html ex_ups.gif
    */
    template <typename T, typename Parameters>
    struct ups_ellipsoid : public detail::stere::base_stere_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline ups_ellipsoid(Params const& params, Parameters & par)
        {
            detail::stere::setup_ups(params, par, this->m_proj_parm);
        }
    };

    /*!
        \brief Universal Polar Stereographic projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - south: Denotes southern hemisphere UTM zone (boolean)
        \par Example
        \image html ex_ups.gif
    */
    template <typename T, typename Parameters>
    struct ups_spheroid : public detail::stere::base_stere_spheroid<T, Parameters>
    {
        template <typename Params>
        inline ups_spheroid(Params const& params, Parameters & par)
        {
            detail::stere::setup_ups(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI2(srs::spar::proj_stere, stere_spheroid, stere_ellipsoid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI2(srs::spar::proj_ups, ups_spheroid, ups_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI2(stere_entry, stere_spheroid, stere_ellipsoid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI2(ups_entry, ups_spheroid, ups_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(stere_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(stere, stere_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(ups, ups_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_STERE_HPP

