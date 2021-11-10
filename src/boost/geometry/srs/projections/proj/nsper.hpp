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

#ifndef BOOST_GEOMETRY_PROJECTIONS_NSPER_HPP
#define BOOST_GEOMETRY_PROJECTIONS_NSPER_HPP

#include <boost/config.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/math/special_functions/hypot.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace nsper
    {

            static const double epsilon10 = 1.e-10;
            enum mode_type {
                n_pole = 0,
                s_pole = 1,
                equit  = 2,
                obliq  = 3
            };

            template <typename T>
            struct par_nsper
            {
                T   height;
                T   sinph0;
                T   cosph0;
                T   p;
                T   rp;
                T   pn1;
                T   pfact;
                T   h;
                T   cg;
                T   sg;
                T   sw;
                T   cw;
                mode_type mode;
                bool tilt;
            };

            template <typename T, typename Parameters>
            struct base_nsper_spheroid
            {
                par_nsper<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T  coslam, cosphi, sinphi;

                    sinphi = sin(lp_lat);
                    cosphi = cos(lp_lat);
                    coslam = cos(lp_lon);
                    switch (this->m_proj_parm.mode) {
                    case obliq:
                        xy_y = this->m_proj_parm.sinph0 * sinphi + this->m_proj_parm.cosph0 * cosphi * coslam;
                        break;
                    case equit:
                        xy_y = cosphi * coslam;
                        break;
                    case s_pole:
                        xy_y = - sinphi;
                        break;
                    case n_pole:
                        xy_y = sinphi;
                        break;
                    }
                    if (xy_y < this->m_proj_parm.rp) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    xy_y = this->m_proj_parm.pn1 / (this->m_proj_parm.p - xy_y);
                    xy_x = xy_y * cosphi * sin(lp_lon);
                    switch (this->m_proj_parm.mode) {
                    case obliq:
                        xy_y *= (this->m_proj_parm.cosph0 * sinphi -
                           this->m_proj_parm.sinph0 * cosphi * coslam);
                        break;
                    case equit:
                        xy_y *= sinphi;
                        break;
                    case n_pole:
                        coslam = - coslam;
                        BOOST_FALLTHROUGH;
                    case s_pole:
                        xy_y *= cosphi * coslam;
                        break;
                    }
                    if (this->m_proj_parm.tilt) {
                        T yt, ba;

                        yt = xy_y * this->m_proj_parm.cg + xy_x * this->m_proj_parm.sg;
                        ba = 1. / (yt * this->m_proj_parm.sw * this->m_proj_parm.h + this->m_proj_parm.cw);
                        xy_x = (xy_x * this->m_proj_parm.cg - xy_y * this->m_proj_parm.sg) * this->m_proj_parm.cw * ba;
                        xy_y = yt * ba;
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    T  rh, cosz, sinz;

                    if (this->m_proj_parm.tilt) {
                        T bm, bq, yt;

                        yt = 1./(this->m_proj_parm.pn1 - xy_y * this->m_proj_parm.sw);
                        bm = this->m_proj_parm.pn1 * xy_x * yt;
                        bq = this->m_proj_parm.pn1 * xy_y * this->m_proj_parm.cw * yt;
                        xy_x = bm * this->m_proj_parm.cg + bq * this->m_proj_parm.sg;
                        xy_y = bq * this->m_proj_parm.cg - bm * this->m_proj_parm.sg;
                    }
                    rh = boost::math::hypot(xy_x, xy_y);
                    if ((sinz = 1. - rh * rh * this->m_proj_parm.pfact) < 0.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    sinz = (this->m_proj_parm.p - sqrt(sinz)) / (this->m_proj_parm.pn1 / rh + rh / this->m_proj_parm.pn1);
                    cosz = sqrt(1. - sinz * sinz);
                    if (fabs(rh) <= epsilon10) {
                        lp_lon = 0.;
                        lp_lat = par.phi0;
                    } else {
                        switch (this->m_proj_parm.mode) {
                        case obliq:
                            lp_lat = asin(cosz * this->m_proj_parm.sinph0 + xy_y * sinz * this->m_proj_parm.cosph0 / rh);
                            xy_y = (cosz - this->m_proj_parm.sinph0 * sin(lp_lat)) * rh;
                            xy_x *= sinz * this->m_proj_parm.cosph0;
                            break;
                        case equit:
                            lp_lat = asin(xy_y * sinz / rh);
                            xy_y = cosz * rh;
                            xy_x *= sinz;
                            break;
                        case n_pole:
                            lp_lat = asin(cosz);
                            xy_y = -xy_y;
                            break;
                        case s_pole:
                            lp_lat = - asin(cosz);
                            break;
                        }
                        lp_lon = atan2(xy_x, xy_y);
                    }
                }

                static inline std::string get_name()
                {
                    return "nsper_spheroid";
                }

            };

            template <typename Params, typename Parameters, typename T>
            inline void setup(Params const& params, Parameters& par, par_nsper<T>& proj_parm) 
            {
                proj_parm.height = pj_get_param_f<T, srs::spar::h>(params, "h", srs::dpar::h);
                if (proj_parm.height <= 0.)
                    BOOST_THROW_EXCEPTION( projection_exception(error_h_less_than_zero) );

                if (fabs(fabs(par.phi0) - geometry::math::half_pi<T>()) < epsilon10)
                    proj_parm.mode = par.phi0 < 0. ? s_pole : n_pole;
                else if (fabs(par.phi0) < epsilon10)
                    proj_parm.mode = equit;
                else {
                    proj_parm.mode = obliq;
                    proj_parm.sinph0 = sin(par.phi0);
                    proj_parm.cosph0 = cos(par.phi0);
                }
                proj_parm.pn1 = proj_parm.height / par.a; /* normalize by radius */
                proj_parm.p = 1. + proj_parm.pn1;
                proj_parm.rp = 1. / proj_parm.p;
                proj_parm.h = 1. / proj_parm.pn1;
                proj_parm.pfact = (proj_parm.p + 1.) * proj_parm.h;
                par.es = 0.;
            }


            // Near-sided perspective
            template <typename Params, typename Parameters, typename T>
            inline void setup_nsper(Params const& params, Parameters& par, par_nsper<T>& proj_parm)
            {
                proj_parm.tilt = false;

                setup(params, par, proj_parm);
            }

            // Tilted perspective
            template <typename Params, typename Parameters, typename T>
            inline void setup_tpers(Params const& params, Parameters& par, par_nsper<T>& proj_parm)
            {
                T const omega = pj_get_param_r<T, srs::spar::tilt>(params, "tilt", srs::dpar::tilt);
                T const gamma = pj_get_param_r<T, srs::spar::azi>(params, "azi", srs::dpar::azi);
                proj_parm.tilt = true;
                proj_parm.cg = cos(gamma); proj_parm.sg = sin(gamma);
                proj_parm.cw = cos(omega); proj_parm.sw = sin(omega);

                setup(params, par, proj_parm);
            }

    }} // namespace detail::nsper
    #endif // doxygen

    /*!
        \brief Near-sided perspective projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
        \par Projection parameters
         - h: Height
        \par Example
        \image html ex_nsper.gif
    */
    template <typename T, typename Parameters>
    struct nsper_spheroid : public detail::nsper::base_nsper_spheroid<T, Parameters>
    {
        template <typename Params>
        inline nsper_spheroid(Params const& params, Parameters & par)
        {
            detail::nsper::setup_nsper(params, par, this->m_proj_parm);
        }
    };

    /*!
        \brief Tilted perspective projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
        \par Projection parameters
         - tilt: Tilt, or Omega (real)
         - azi: Azimuth (or Gamma) (real)
         - h: Height
        \par Example
        \image html ex_tpers.gif
    */
    template <typename T, typename Parameters>
    struct tpers_spheroid : public detail::nsper::base_nsper_spheroid<T, Parameters>
    {
        template <typename Params>
        inline tpers_spheroid(Params const& params, Parameters & par)
        {
            detail::nsper::setup_tpers(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_nsper, nsper_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_tpers, tpers_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(nsper_entry, nsper_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(tpers_entry, tpers_spheroid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(nsper_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(nsper, nsper_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(tpers, tpers_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_NSPER_HPP

