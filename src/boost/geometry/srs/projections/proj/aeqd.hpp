// Boost.Geometry - gis-projections (based on PROJ4)

// Copyright (c) 2008-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.
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

// Purpose:  Implementation of the aeqd (Azimuthal Equidistant) projection.
// Author:   Gerald Evenden
// Copyright (c) 1995, Gerald Evenden

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_AEQD_HPP
#define BOOST_GEOMETRY_PROJECTIONS_AEQD_HPP


#include <type_traits>

#include <boost/config.hpp>

#include <boost/geometry/formulas/vincenty_direct.hpp>
#include <boost/geometry/formulas/vincenty_inverse.hpp>

#include <boost/geometry/srs/projections/impl/aasincos.hpp>
#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_mlfn.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/math/special_functions/hypot.hpp>


namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace aeqd
    {

            static const double epsilon10 = 1.e-10;
            static const double tolerance = 1.e-14;
            enum mode_type {
                n_pole = 0,
                s_pole = 1,
                equit  = 2,
                obliq  = 3
            };

            template <typename T>
            struct par_aeqd
            {
                T    sinph0;
                T    cosph0;
                detail::en<T> en;
                T    M1;
                //T    N1;
                T    Mp;
                //T    He;
                //T    G;
                T    b;
                mode_type mode;
            };

            template <typename T, typename Par, typename ProjParm>
            inline void e_forward(T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y, Par const& par, ProjParm const& proj_parm)
            {
                T coslam, cosphi, sinphi, rho;
                //T azi1, s12;
                //T lam1, phi1, lam2, phi2;

                coslam = cos(lp_lon);
                cosphi = cos(lp_lat);
                sinphi = sin(lp_lat);
                switch (proj_parm.mode) {
                case n_pole:
                    coslam = - coslam;
                    BOOST_FALLTHROUGH;
                case s_pole:
                    xy_x = (rho = fabs(proj_parm.Mp - pj_mlfn(lp_lat, sinphi, cosphi, proj_parm.en))) *
                        sin(lp_lon);
                    xy_y = rho * coslam;
                    break;
                case equit:
                case obliq:
                    if (fabs(lp_lon) < epsilon10 && fabs(lp_lat - par.phi0) < epsilon10) {
                        xy_x = xy_y = 0.;
                        break;
                    }

                    //phi1 = par.phi0; lam1 = par.lam0;
                    //phi2 = lp_lat;  lam2 = lp_lon + par.lam0;

                    formula::result_inverse<T> const inv =
                        formula::vincenty_inverse
                            <
                                T, true, true
                            >::apply(par.lam0, par.phi0, lp_lon + par.lam0, lp_lat, srs::spheroid<T>(par.a, proj_parm.b));
                    //azi1 = inv.azimuth; s12 = inv.distance;
                    xy_x = inv.distance * sin(inv.azimuth) / par.a;
                    xy_y = inv.distance * cos(inv.azimuth) / par.a;
                    break;
                }
            }

            template <typename T, typename Par, typename ProjParm>
            inline void e_inverse(T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat, Par const& par, ProjParm const& proj_parm)
            {
                T c;

                if ((c = boost::math::hypot(xy_x, xy_y)) < epsilon10) {
                    lp_lat = par.phi0;
                    lp_lon = 0.;
                        return;
                }
                if (proj_parm.mode == obliq || proj_parm.mode == equit) {
                    T const x2 = xy_x * par.a;
                    T const y2 = xy_y * par.a;
                    //T const lat1 = par.phi0;
                    //T const lon1 = par.lam0;
                    T const azi1 = atan2(x2, y2);
                    T const s12 = sqrt(x2 * x2 + y2 * y2);
                    formula::result_direct<T> const dir =
                        formula::vincenty_direct
                            <
                                T, true
                            >::apply(par.lam0, par.phi0, s12, azi1, srs::spheroid<T>(par.a, proj_parm.b));
                    lp_lat = dir.lat2;
                    lp_lon = dir.lon2;
                    lp_lon -= par.lam0;
                } else { /* Polar */
                    lp_lat = pj_inv_mlfn(proj_parm.mode == n_pole ? proj_parm.Mp - c : proj_parm.Mp + c,
                        par.es, proj_parm.en);
                    lp_lon = atan2(xy_x, proj_parm.mode == n_pole ? -xy_y : xy_y);
                }
            }

            template <typename T, typename Par, typename ProjParm>
            inline void e_guam_fwd(T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y, Par const& par, ProjParm const& proj_parm)
            {
                T cosphi, sinphi, t;

                cosphi = cos(lp_lat);
                sinphi = sin(lp_lat);
                t = 1. / sqrt(1. - par.es * sinphi * sinphi);
                xy_x = lp_lon * cosphi * t;
                xy_y = pj_mlfn(lp_lat, sinphi, cosphi, proj_parm.en) - proj_parm.M1 +
                    .5 * lp_lon * lp_lon * cosphi * sinphi * t;
            }

            template <typename T, typename Par, typename ProjParm>
            inline void e_guam_inv(T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat, Par const& par, ProjParm const& proj_parm)
            {
                T x2, t = 0.0;
                int i;

                x2 = 0.5 * xy_x * xy_x;
                lp_lat = par.phi0;
                for (i = 0; i < 3; ++i) {
                    t = par.e * sin(lp_lat);
                    lp_lat = pj_inv_mlfn(proj_parm.M1 + xy_y -
                        x2 * tan(lp_lat) * (t = sqrt(1. - t * t)), par.es, proj_parm.en);
                }
                lp_lon = xy_x * t / cos(lp_lat);
            }

            template <typename T, typename Par, typename ProjParm>
            inline void s_forward(T const& lp_lon, T lp_lat, T& xy_x, T& xy_y, Par const& /*par*/, ProjParm const& proj_parm)
            {
                static const T half_pi = detail::half_pi<T>();
                    
                T coslam, cosphi, sinphi;

                sinphi = sin(lp_lat);
                cosphi = cos(lp_lat);
                coslam = cos(lp_lon);
                switch (proj_parm.mode) {
                case equit:
                    xy_y = cosphi * coslam;
                    goto oblcon;
                case obliq:
                    xy_y = proj_parm.sinph0 * sinphi + proj_parm.cosph0 * cosphi * coslam;
            oblcon:
                    if (fabs(fabs(xy_y) - 1.) < tolerance)
                        if (xy_y < 0.)
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        else
                            xy_x = xy_y = 0.;
                    else {
                        xy_y = acos(xy_y);
                        xy_y /= sin(xy_y);
                        xy_x = xy_y * cosphi * sin(lp_lon);
                        xy_y *= (proj_parm.mode == equit) ? sinphi :
                                proj_parm.cosph0 * sinphi - proj_parm.sinph0 * cosphi * coslam;
                    }
                    break;
                case n_pole:
                    lp_lat = -lp_lat;
                    coslam = -coslam;
                    BOOST_FALLTHROUGH;
                case s_pole:
                    if (fabs(lp_lat - half_pi) < epsilon10)
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    xy_x = (xy_y = (half_pi + lp_lat)) * sin(lp_lon);
                    xy_y *= coslam;
                    break;
                }
            }

            template <typename T, typename Par, typename ProjParm>
            inline void s_inverse(T xy_x, T xy_y, T& lp_lon, T& lp_lat, Par const& par, ProjParm const& proj_parm)
            {
                static const T pi = detail::pi<T>();
                static const T half_pi = detail::half_pi<T>();
                    
                T cosc, c_rh, sinc;

                if ((c_rh = boost::math::hypot(xy_x, xy_y)) > pi) {
                    if (c_rh - epsilon10 > pi)
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    c_rh = pi;
                } else if (c_rh < epsilon10) {
                    lp_lat = par.phi0;
                    lp_lon = 0.;
                        return;
                }
                if (proj_parm.mode == obliq || proj_parm.mode == equit) {
                    sinc = sin(c_rh);
                    cosc = cos(c_rh);
                    if (proj_parm.mode == equit) {
                                    lp_lat = aasin(xy_y * sinc / c_rh);
                        xy_x *= sinc;
                        xy_y = cosc * c_rh;
                    } else {
                        lp_lat = aasin(cosc * proj_parm.sinph0 + xy_y * sinc * proj_parm.cosph0 /
                            c_rh);
                        xy_y = (cosc - proj_parm.sinph0 * sin(lp_lat)) * c_rh;
                        xy_x *= sinc * proj_parm.cosph0;
                    }
                    lp_lon = xy_y == 0. ? 0. : atan2(xy_x, xy_y);
                } else if (proj_parm.mode == n_pole) {
                    lp_lat = half_pi - c_rh;
                    lp_lon = atan2(xy_x, -xy_y);
                } else {
                    lp_lat = c_rh - half_pi;
                    lp_lon = atan2(xy_x, xy_y);
                }
            }

            // Azimuthal Equidistant
            template <typename Params, typename Parameters, typename T>
            inline void setup_aeqd(Params const& params, Parameters& par, par_aeqd<T>& proj_parm, bool is_sphere, bool is_guam)
            {
                static const T half_pi = detail::half_pi<T>();

                par.phi0 = pj_get_param_r<T, srs::spar::lat_0>(params, "lat_0", srs::dpar::lat_0);
                if (fabs(fabs(par.phi0) - half_pi) < epsilon10) {
                    proj_parm.mode = par.phi0 < 0. ? s_pole : n_pole;
                    proj_parm.sinph0 = par.phi0 < 0. ? -1. : 1.;
                    proj_parm.cosph0 = 0.;
                } else if (fabs(par.phi0) < epsilon10) {
                    proj_parm.mode = equit;
                    proj_parm.sinph0 = 0.;
                    proj_parm.cosph0 = 1.;
                } else {
                    proj_parm.mode = obliq;
                    proj_parm.sinph0 = sin(par.phi0);
                    proj_parm.cosph0 = cos(par.phi0);
                }
                if (is_sphere) {
                    /* empty */
                } else {
                    proj_parm.en = pj_enfn<T>(par.es);
                    if (is_guam) {
                        proj_parm.M1 = pj_mlfn(par.phi0, proj_parm.sinph0, proj_parm.cosph0, proj_parm.en);
                    } else {
                        switch (proj_parm.mode) {
                        case n_pole:
                            proj_parm.Mp = pj_mlfn<T>(half_pi, 1., 0., proj_parm.en);
                            break;
                        case s_pole:
                            proj_parm.Mp = pj_mlfn<T>(-half_pi, -1., 0., proj_parm.en);
                            break;
                        case equit:
                        case obliq:
                            //proj_parm.N1 = 1. / sqrt(1. - par.es * proj_parm.sinph0 * proj_parm.sinph0);
                            //proj_parm.G = proj_parm.sinph0 * (proj_parm.He = par.e / sqrt(par.one_es));
                            //proj_parm.He *= proj_parm.cosph0;
                            break;
                        }
                        // Boost.Geometry specific, in proj4 geodesic is initialized at the beginning
                        proj_parm.b = math::sqrt(math::sqr(par.a) * (1. - par.es));
                    }
                }
            }

            template <typename T, typename Parameters>
            struct base_aeqd_e
            {
                par_aeqd<T> m_proj_parm;

                // FORWARD(e_forward)  elliptical
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    e_forward(lp_lon, lp_lat, xy_x, xy_y, par, this->m_proj_parm);
                }

                // INVERSE(e_inverse)  elliptical
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    e_inverse(xy_x, xy_y, lp_lon, lp_lat, par, this->m_proj_parm);
                }

                static inline std::string get_name()
                {
                    return "aeqd_e";
                }

            };

            template <typename T, typename Parameters>
            struct base_aeqd_e_guam
            {
                par_aeqd<T> m_proj_parm;

                // FORWARD(e_guam_fwd)  Guam elliptical
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    e_guam_fwd(lp_lon, lp_lat, xy_x, xy_y, par, this->m_proj_parm);
                }

                // INVERSE(e_guam_inv)  Guam elliptical
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    e_guam_inv(xy_x, xy_y, lp_lon, lp_lat, par, this->m_proj_parm);
                }

                static inline std::string get_name()
                {
                    return "aeqd_e_guam";
                }

            };

            template <typename T, typename Parameters>
            struct base_aeqd_s
            {
                par_aeqd<T> m_proj_parm;

                // FORWARD(s_forward)  spherical
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    s_forward(lp_lon, lp_lat, xy_x, xy_y, par, this->m_proj_parm);
                }

                // INVERSE(s_inverse)  spherical
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    s_inverse(xy_x, xy_y, lp_lon, lp_lat, par, this->m_proj_parm);
                }

                static inline std::string get_name()
                {
                    return "aeqd_s";
                }

            };

    }} // namespace detail::aeqd
    #endif // doxygen

    /*!
        \brief Azimuthal Equidistant projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - lat_0: Latitude of origin (degrees)
         - guam (boolean)
        \par Example
        \image html ex_aeqd.gif
    */
    template <typename T, typename Parameters>
    struct aeqd_e : public detail::aeqd::base_aeqd_e<T, Parameters>
    {
        template <typename Params>
        inline aeqd_e(Params const& params, Parameters & par)
        {
            detail::aeqd::setup_aeqd(params, par, this->m_proj_parm, false, false);
        }
    };

    /*!
        \brief Azimuthal Equidistant projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - lat_0: Latitude of origin (degrees)
         - guam (boolean)
        \par Example
        \image html ex_aeqd.gif
    */
    template <typename T, typename Parameters>
    struct aeqd_e_guam : public detail::aeqd::base_aeqd_e_guam<T, Parameters>
    {
        template <typename Params>
        inline aeqd_e_guam(Params const& params, Parameters & par)
        {
            detail::aeqd::setup_aeqd(params, par, this->m_proj_parm, false, true);
        }
    };

    /*!
        \brief Azimuthal Equidistant projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - lat_0: Latitude of origin (degrees)
         - guam (boolean)
        \par Example
        \image html ex_aeqd.gif
    */
    template <typename T, typename Parameters>
    struct aeqd_s : public detail::aeqd::base_aeqd_s<T, Parameters>
    {
        template <typename Params>
        inline aeqd_s(Params const& params, Parameters & par)
        {
            detail::aeqd::setup_aeqd(params, par, this->m_proj_parm, true, false);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        template <typename BGP, typename CT, typename P>
        struct static_projection_type<srs::spar::proj_aeqd, srs_sphere_tag, BGP, CT, P>
        {
            typedef static_wrapper_fi<aeqd_s<CT, P>, P> type;
        };
        template <typename BGP, typename CT, typename P>
        struct static_projection_type<srs::spar::proj_aeqd, srs_spheroid_tag, BGP, CT, P>
        {
            typedef static_wrapper_fi
                <
                    std::conditional_t
                        <
                            std::is_void
                                <
                                    typename geometry::tuples::find_if
                                        <
                                            BGP,
                                            //srs::par4::detail::is_guam
                                            srs::spar::detail::is_param<srs::spar::guam>::pred
                                        >::type
                                >::value,
                            aeqd_e<CT, P>,
                            aeqd_e_guam<CT, P>
                        >
                    , P
                > type;
        };

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_BEGIN(aeqd_entry)
        {
            bool const guam = pj_get_param_b<srs::spar::guam>(params, "guam", srs::dpar::guam);

            if (parameters.es && ! guam)
                return new dynamic_wrapper_fi<aeqd_e<T, Parameters>, T, Parameters>(params, parameters);
            else if (parameters.es && guam)
                return new dynamic_wrapper_fi<aeqd_e_guam<T, Parameters>, T, Parameters>(params, parameters);
            else
                return new dynamic_wrapper_fi<aeqd_s<T, Parameters>, T, Parameters>(params, parameters);
        }
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_END

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(aeqd_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(aeqd, aeqd_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_AEQD_HPP

