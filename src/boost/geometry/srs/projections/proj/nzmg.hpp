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

// Purpose:  Implementation of the nzmg (New Zealand Map Grid) projection.
//           Very loosely based upon DMA code by Bradford W. Drew
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

#ifndef BOOST_GEOMETRY_PROJECTIONS_NZMG_HPP
#define BOOST_GEOMETRY_PROJECTIONS_NZMG_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_zpoly1.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace nzmg
    {

            static const double epsilon = 1e-10;
            static const int Nbf = 5;
            static const int Ntpsi = 9;
            static const int Ntphi = 8;

            template <typename T>
            inline T sec5_to_rad() { return 0.4848136811095359935899141023; }
            template <typename T>
            inline T rad_to_sec5() { return 2.062648062470963551564733573; }

            template <typename T>
            inline const pj_complex<T> * bf()
            {
                static const pj_complex<T> result[] = {
                    {.7557853228,    0.0},
                    {.249204646,    .003371507},
                    {-.001541739,    .041058560},
                    {-.10162907,    .01727609},
                    {-.26623489,    -.36249218},
                    {-.6870983,    -1.1651967}
                };
                return result;
            }

            template <typename T>
            inline const T * tphi()
            {
                static const T result[] = { 1.5627014243, .5185406398, -.03333098,
                                            -.1052906,   -.0368594,     .007317,
                                             .01220,      .00394,      -.0013 };
                return result;
            }
            template <typename T>
            inline const T * tpsi()
            {
                static const T result[] = { .6399175073, -.1358797613, .063294409, -.02526853, .0117879,
                                           -.0055161,     .0026906,   -.001333,     .00067,   -.00034 };
                return result;
            }

            template <typename T, typename Parameters>
            struct base_nzmg_ellipsoid
            {
                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T rad_to_sec5 = nzmg::rad_to_sec5<T>();

                    pj_complex<T> p;
                    const T * C;
                    int i;

                    lp_lat = (lp_lat - par.phi0) * rad_to_sec5;
                    for (p.r = *(C = tpsi<T>() + (i = Ntpsi)); i ; --i)
                        p.r = *--C + lp_lat * p.r;
                    p.r *= lp_lat;
                    p.i = lp_lon;
                    p = pj_zpoly1(p, bf<T>(), Nbf);
                    xy_x = p.i;
                    xy_y = p.r;
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T sec5_to_rad = nzmg::sec5_to_rad<T>();

                    int nn, i;
                    pj_complex<T> p, f, fp, dp;
                    T den;
                    const T* C;

                    p.r = xy_y;
                    p.i = xy_x;
                    for (nn = 20; nn ;--nn) {
                        f = pj_zpolyd1(p, bf<T>(), Nbf, &fp);
                        f.r -= xy_y;
                        f.i -= xy_x;
                        den = fp.r * fp.r + fp.i * fp.i;
                        p.r += dp.r = -(f.r * fp.r + f.i * fp.i) / den;
                        p.i += dp.i = -(f.i * fp.r - f.r * fp.i) / den;
                        if ((fabs(dp.r) + fabs(dp.i)) <= epsilon)
                            break;
                    }
                    if (nn) {
                        lp_lon = p.i;
                        for (lp_lat = *(C = tphi<T>() + (i = Ntphi)); i ; --i)
                            lp_lat = *--C + p.r * lp_lat;
                        lp_lat = par.phi0 + p.r * lp_lat * sec5_to_rad;
                    } else
                        lp_lon = lp_lat = HUGE_VAL;
                }

                static inline std::string get_name()
                {
                    return "nzmg_ellipsoid";
                }

            };

            // New Zealand Map Grid
            template <typename Parameters>
            inline void setup_nzmg(Parameters& par)
            {
                typedef typename Parameters::type calc_t;
                static const calc_t d2r = geometry::math::d2r<calc_t>();

                /* force to International major axis */
                par.a = 6378388.0;
                par.ra = 1. / par.a;
                par.lam0 = 173. * d2r;
                par.phi0 = -41. * d2r;
                par.x0 = 2510000.;
                par.y0 = 6023150.;
            }

    }} // namespace detail::nzmg
    #endif // doxygen

    /*!
        \brief New Zealand Map Grid projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Fixed Earth
        \par Example
        \image html ex_nzmg.gif
    */
    template <typename T, typename Parameters>
    struct nzmg_ellipsoid : public detail::nzmg::base_nzmg_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline nzmg_ellipsoid(Params const& , Parameters & par)
        {
            detail::nzmg::setup_nzmg(par);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_nzmg, nzmg_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(nzmg_entry, nzmg_ellipsoid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(nzmg_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(nzmg, nzmg_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_NZMG_HPP

