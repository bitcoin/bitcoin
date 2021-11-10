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

#ifndef BOOST_GEOMETRY_PROJECTIONS_GNOM_HPP
#define BOOST_GEOMETRY_PROJECTIONS_GNOM_HPP

#include <boost/config.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/math/special_functions/hypot.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace gnom
    {

            static const double epsilon10 = 1.e-10;
            enum mode_type {
                n_pole = 0,
                s_pole = 1,
                equit  = 2,
                obliq  = 3
            };

            template <typename T>
            struct par_gnom
            {
                T    sinph0;
                T    cosph0;
                mode_type mode;
            };

            template <typename T, typename Parameters>
            struct base_gnom_spheroid
            {
                par_gnom<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T  coslam, cosphi, sinphi;

                    sinphi = sin(lp_lat);
                    cosphi = cos(lp_lat);
                    coslam = cos(lp_lon);

                    switch (this->m_proj_parm.mode) {
                    case equit:
                        xy_y = cosphi * coslam;
                        break;
                    case obliq:
                        xy_y = this->m_proj_parm.sinph0 * sinphi + this->m_proj_parm.cosph0 * cosphi * coslam;
                        break;
                    case s_pole:
                        xy_y = - sinphi;
                        break;
                    case n_pole:
                        xy_y = sinphi;
                        break;
                    }

                    if (xy_y <= epsilon10) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }

                    xy_x = (xy_y = 1. / xy_y) * cosphi * sin(lp_lon);
                    switch (this->m_proj_parm.mode) {
                    case equit:
                        xy_y *= sinphi;
                        break;
                    case obliq:
                        xy_y *= this->m_proj_parm.cosph0 * sinphi - this->m_proj_parm.sinph0 * cosphi * coslam;
                        break;
                    case n_pole:
                        coslam = - coslam;
                        BOOST_FALLTHROUGH;
                    case s_pole:
                        xy_y *= cosphi * coslam;
                        break;
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T  rh, cosz, sinz;

                    rh = boost::math::hypot(xy_x, xy_y);
                    sinz = sin(lp_lat = atan(rh));
                    cosz = sqrt(1. - sinz * sinz);

                    if (fabs(rh) <= epsilon10) {
                        lp_lat = par.phi0;
                        lp_lon = 0.;
                    } else {
                        switch (this->m_proj_parm.mode) {
                        case obliq:
                            lp_lat = cosz * this->m_proj_parm.sinph0 + xy_y * sinz * this->m_proj_parm.cosph0 / rh;
                            if (fabs(lp_lat) >= 1.)
                                lp_lat = lp_lat > 0. ? half_pi : -half_pi;
                            else
                                lp_lat = asin(lp_lat);
                            xy_y = (cosz - this->m_proj_parm.sinph0 * sin(lp_lat)) * rh;
                            xy_x *= sinz * this->m_proj_parm.cosph0;
                            break;
                        case equit:
                            lp_lat = xy_y * sinz / rh;
                            if (fabs(lp_lat) >= 1.)
                                lp_lat = lp_lat > 0. ? half_pi : -half_pi;
                            else
                                lp_lat = asin(lp_lat);
                            xy_y = cosz * rh;
                            xy_x *= sinz;
                            break;
                        case s_pole:
                            lp_lat -= half_pi;
                            break;
                        case n_pole:
                            lp_lat = half_pi - lp_lat;
                            xy_y = -xy_y;
                            break;
                        }
                        lp_lon = atan2(xy_x, xy_y);
                    }
                }

                static inline std::string get_name()
                {
                    return "gnom_spheroid";
                }

            };

            // Gnomonic
            template <typename Parameters, typename T>
            inline void setup_gnom(Parameters& par, par_gnom<T>& proj_parm)
            {
                static const T half_pi = detail::half_pi<T>();

                if (fabs(fabs(par.phi0) - half_pi) < epsilon10) {
                    proj_parm.mode = par.phi0 < 0. ? s_pole : n_pole;
                } else if (fabs(par.phi0) < epsilon10) {
                    proj_parm.mode = equit;
                } else {
                    proj_parm.mode = obliq;
                    proj_parm.sinph0 = sin(par.phi0);
                    proj_parm.cosph0 = cos(par.phi0);
                }

                par.es = 0.;
            }

    }} // namespace detail::gnom
    #endif // doxygen

    /*!
        \brief Gnomonic projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
        \par Example
        \image html ex_gnom.gif
    */
    template <typename T, typename Parameters>
    struct gnom_spheroid : public detail::gnom::base_gnom_spheroid<T, Parameters>
    {
        template <typename Params>
        inline gnom_spheroid(Params const& , Parameters & par)
        {
            detail::gnom::setup_gnom(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_gnom, gnom_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(gnom_entry, gnom_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(gnom_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(gnom, gnom_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_GNOM_HPP

