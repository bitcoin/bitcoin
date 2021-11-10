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

#ifndef BOOST_GEOMETRY_PROJECTIONS_LAEA_HPP
#define BOOST_GEOMETRY_PROJECTIONS_LAEA_HPP

#include <boost/config.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/math/special_functions/hypot.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_auth.hpp>
#include <boost/geometry/srs/projections/impl/pj_qsfn.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace laea
    {
            static const double epsilon10 = 1.e-10;

            enum mode_type {
                n_pole = 0,
                s_pole = 1,
                equit  = 2,
                obliq  = 3
            };

            template <typename T>
            struct par_laea
            {
                T   sinb1;
                T   cosb1;
                T   xmf;
                T   ymf;
                T   mmf;
                T   qp;
                T   dd;
                T   rq;
                detail::apa<T> apa;
                mode_type mode;
            };

            template <typename T, typename Parameters>
            struct base_laea_ellipsoid
            {
                par_laea<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T coslam, sinlam, sinphi, q, sinb=0.0, cosb=0.0, b=0.0;

                    coslam = cos(lp_lon);
                    sinlam = sin(lp_lon);
                    sinphi = sin(lp_lat);
                    q = pj_qsfn(sinphi, par.e, par.one_es);

                    if (this->m_proj_parm.mode == obliq || this->m_proj_parm.mode == equit) {
                        sinb = q / this->m_proj_parm.qp;
                        cosb = sqrt(1. - sinb * sinb);
                    }

                    switch (this->m_proj_parm.mode) {
                    case obliq:
                        b = 1. + this->m_proj_parm.sinb1 * sinb + this->m_proj_parm.cosb1 * cosb * coslam;
                        break;
                    case equit:
                        b = 1. + cosb * coslam;
                        break;
                    case n_pole:
                        b = half_pi + lp_lat;
                        q = this->m_proj_parm.qp - q;
                        break;
                    case s_pole:
                        b = lp_lat - half_pi;
                        q = this->m_proj_parm.qp + q;
                        break;
                    }
                    if (fabs(b) < epsilon10) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }

                    switch (this->m_proj_parm.mode) {
                    case obliq:
                        b = sqrt(2. / b);
                        xy_y = this->m_proj_parm.ymf * b * (this->m_proj_parm.cosb1 * sinb - this->m_proj_parm.sinb1 * cosb * coslam);
                        goto eqcon;
                        break;
                    case equit:
                        b = sqrt(2. / (1. + cosb * coslam));
                        xy_y = b * sinb * this->m_proj_parm.ymf;
                eqcon:
                        xy_x = this->m_proj_parm.xmf * b * cosb * sinlam;
                        break;
                    case n_pole:
                    case s_pole:
                        if (q >= 0.) {
                            b = sqrt(q);
                            xy_x = b * sinlam;
                            xy_y = coslam * (this->m_proj_parm.mode == s_pole ? b : -b);
                        } else
                            xy_x = xy_y = 0.;
                        break;
                    }
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    T cCe, sCe, q, rho, ab=0.0;

                    switch (this->m_proj_parm.mode) {
                    case equit:
                    case obliq:
                        xy_x /= this->m_proj_parm.dd;
                        xy_y *=  this->m_proj_parm.dd;
                        rho = boost::math::hypot(xy_x, xy_y);
                        if (rho < epsilon10) {
                            lp_lon = 0.;
                            lp_lat = par.phi0;
                            return;
                        }
                        sCe = 2. * asin(.5 * rho / this->m_proj_parm.rq);
                        cCe = cos(sCe);
                        sCe = sin(sCe);
                        xy_x *= sCe;
                        if (this->m_proj_parm.mode == obliq) {
                            ab = cCe * this->m_proj_parm.sinb1 + xy_y * sCe * this->m_proj_parm.cosb1 / rho;
                            xy_y = rho * this->m_proj_parm.cosb1 * cCe - xy_y * this->m_proj_parm.sinb1 * sCe;
                        } else {
                            ab = xy_y * sCe / rho;
                            xy_y = rho * cCe;
                        }
                        break;
                    case n_pole:
                        xy_y = -xy_y;
                        BOOST_FALLTHROUGH;
                    case s_pole:
                        q = (xy_x * xy_x + xy_y * xy_y);
                        if (q == 0.0) {
                            lp_lon = 0.;
                            lp_lat = par.phi0;
                            return;
                        }
                        ab = 1. - q / this->m_proj_parm.qp;
                        if (this->m_proj_parm.mode == s_pole)
                            ab = - ab;
                        break;
                    }
                    lp_lon = atan2(xy_x, xy_y);
                    lp_lat = pj_authlat(asin(ab), this->m_proj_parm.apa);
                }

                static inline std::string get_name()
                {
                    return "laea_ellipsoid";
                }

            };

            template <typename T, typename Parameters>
            struct base_laea_spheroid
            {
                par_laea<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T fourth_pi = detail::fourth_pi<T>();

                    T  coslam, cosphi, sinphi;

                    sinphi = sin(lp_lat);
                    cosphi = cos(lp_lat);
                    coslam = cos(lp_lon);
                    switch (this->m_proj_parm.mode) {
                    case equit:
                        xy_y = 1. + cosphi * coslam;
                        goto oblcon;
                    case obliq:
                        xy_y = 1. + this->m_proj_parm.sinb1 * sinphi + this->m_proj_parm.cosb1 * cosphi * coslam;
                oblcon:
                        if (xy_y <= epsilon10) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        xy_y = sqrt(2. / xy_y);
                        xy_x = xy_y * cosphi * sin(lp_lon);
                        xy_y *= this->m_proj_parm.mode == equit ? sinphi :
                           this->m_proj_parm.cosb1 * sinphi - this->m_proj_parm.sinb1 * cosphi * coslam;
                        break;
                    case n_pole:
                        coslam = -coslam;
                        BOOST_FALLTHROUGH;
                    case s_pole:
                        if (fabs(lp_lat + par.phi0) < epsilon10) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        xy_y = fourth_pi - lp_lat * .5;
                        xy_y = 2. * (this->m_proj_parm.mode == s_pole ? cos(xy_y) : sin(xy_y));
                        xy_x = xy_y * sin(lp_lon);
                        xy_y *= coslam;
                        break;
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T  cosz=0.0, rh, sinz=0.0;

                    rh = boost::math::hypot(xy_x, xy_y);
                    if ((lp_lat = rh * .5 ) > 1.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    lp_lat = 2. * asin(lp_lat);
                    if (this->m_proj_parm.mode == obliq || this->m_proj_parm.mode == equit) {
                        sinz = sin(lp_lat);
                        cosz = cos(lp_lat);
                    }
                    switch (this->m_proj_parm.mode) {
                    case equit:
                        lp_lat = fabs(rh) <= epsilon10 ? 0. : asin(xy_y * sinz / rh);
                        xy_x *= sinz;
                        xy_y = cosz * rh;
                        break;
                    case obliq:
                        lp_lat = fabs(rh) <= epsilon10 ? par.phi0 :
                           asin(cosz * this->m_proj_parm.sinb1 + xy_y * sinz * this->m_proj_parm.cosb1 / rh);
                        xy_x *= sinz * this->m_proj_parm.cosb1;
                        xy_y = (cosz - sin(lp_lat) * this->m_proj_parm.sinb1) * rh;
                        break;
                    case n_pole:
                        xy_y = -xy_y;
                        lp_lat = half_pi - lp_lat;
                        break;
                    case s_pole:
                        lp_lat -= half_pi;
                        break;
                    }
                    lp_lon = (xy_y == 0. && (this->m_proj_parm.mode == equit || this->m_proj_parm.mode == obliq)) ?
                        0. : atan2(xy_x, xy_y);
                }

                static inline std::string get_name()
                {
                    return "laea_spheroid";
                }

            };

            // Lambert Azimuthal Equal Area
            template <typename Parameters, typename T>
            inline void setup_laea(Parameters& par, par_laea<T>& proj_parm)
            {
                static const T half_pi = detail::half_pi<T>();

                T t;

                t = fabs(par.phi0);
                if (fabs(t - half_pi) < epsilon10)
                    proj_parm.mode = par.phi0 < 0. ? s_pole : n_pole;
                else if (fabs(t) < epsilon10)
                    proj_parm.mode = equit;
                else
                    proj_parm.mode = obliq;
                if (par.es != 0.0) {
                    double sinphi;

                    par.e = sqrt(par.es); // TODO : Isn't it already set?
                    proj_parm.qp = pj_qsfn(1., par.e, par.one_es);
                    proj_parm.mmf = .5 / (1. - par.es);
                    proj_parm.apa = pj_authset<T>(par.es);
                    switch (proj_parm.mode) {
                    case n_pole:
                    case s_pole:
                        proj_parm.dd = 1.;
                        break;
                    case equit:
                        proj_parm.dd = 1. / (proj_parm.rq = sqrt(.5 * proj_parm.qp));
                        proj_parm.xmf = 1.;
                        proj_parm.ymf = .5 * proj_parm.qp;
                        break;
                    case obliq:
                        proj_parm.rq = sqrt(.5 * proj_parm.qp);
                        sinphi = sin(par.phi0);
                        proj_parm.sinb1 = pj_qsfn(sinphi, par.e, par.one_es) / proj_parm.qp;
                        proj_parm.cosb1 = sqrt(1. - proj_parm.sinb1 * proj_parm.sinb1);
                        proj_parm.dd = cos(par.phi0) / (sqrt(1. - par.es * sinphi * sinphi) *
                           proj_parm.rq * proj_parm.cosb1);
                        proj_parm.ymf = (proj_parm.xmf = proj_parm.rq) / proj_parm.dd;
                        proj_parm.xmf *= proj_parm.dd;
                        break;
                    }
                } else {
                    if (proj_parm.mode == obliq) {
                        proj_parm.sinb1 = sin(par.phi0);
                        proj_parm.cosb1 = cos(par.phi0);
                    }
                }
            }

    }} // namespace laea
    #endif // doxygen

    /*!
        \brief Lambert Azimuthal Equal Area projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Example
        \image html ex_laea.gif
    */
    template <typename T, typename Parameters>
    struct laea_ellipsoid : public detail::laea::base_laea_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline laea_ellipsoid(Params const& , Parameters & par)
        {
            detail::laea::setup_laea(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Lambert Azimuthal Equal Area projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Example
        \image html ex_laea.gif
    */
    template <typename T, typename Parameters>
    struct laea_spheroid : public detail::laea::base_laea_spheroid<T, Parameters>
    {
        template <typename Params>
        inline laea_spheroid(Params const& , Parameters & par)
        {
            detail::laea::setup_laea(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI2(srs::spar::proj_laea, laea_spheroid, laea_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI2(laea_entry, laea_spheroid, laea_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(laea_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(laea, laea_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_LAEA_HPP

