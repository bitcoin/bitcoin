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

// Purpose:  Implementation of the krovak (Krovak) projection.
//           Definition: http://www.ihsenergy.com/epsg/guid7.html#1.4.3
// Author:   Thomas Flemming, tf@ttqv.com
// Copyright (c) 2001, Thomas Flemming, tf@ttqv.com

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_KROVAK_HPP
#define BOOST_GEOMETRY_PROJECTIONS_KROVAK_HPP

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace krovak
    {
            static double epsilon = 1e-15;
            static double S45 = 0.785398163397448;  /* 45 deg */
            static double S90 = 1.570796326794896;  /* 90 deg */
            static double UQ  = 1.04216856380474;   /* DU(2, 59, 42, 42.69689) */
            static double S0  = 1.37008346281555;   /* Latitude of pseudo standard parallel 78deg 30'00" N */
            /* Not sure at all of the appropriate number for max_iter... */
            static int max_iter = 100;

            template <typename T>
            struct par_krovak
            {
                T alpha;
                T k;
                T n;
                T rho0;
                T ad;
                int czech;
            };

            /**
               NOTES: According to EPSG the full Krovak projection method should have
                      the following parameters.  Within PROJ.4 the azimuth, and pseudo
                      standard parallel are hardcoded in the algorithm and can't be
                      altered from outside.  The others all have defaults to match the
                      common usage with Krovak projection.

              lat_0 = latitude of centre of the projection

              lon_0 = longitude of centre of the projection

              ** = azimuth (true) of the centre line passing through the centre of the projection

              ** = latitude of pseudo standard parallel

              k  = scale factor on the pseudo standard parallel

              x_0 = False Easting of the centre of the projection at the apex of the cone

              y_0 = False Northing of the centre of the projection at the apex of the cone

             **/

            template <typename T, typename Parameters>
            struct base_krovak_ellipsoid
            {
                par_krovak<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T gfi, u, deltav, s, d, eps, rho;

                    gfi = math::pow( (T(1) + par.e * sin(lp_lat)) / (T(1) - par.e * sin(lp_lat)), this->m_proj_parm.alpha * par.e / T(2));

                    u = 2. * (atan(this->m_proj_parm.k * math::pow( tan(lp_lat / T(2) + S45), this->m_proj_parm.alpha) / gfi)-S45);
                    deltav = -lp_lon * this->m_proj_parm.alpha;

                    s = asin(cos(this->m_proj_parm.ad) * sin(u) + sin(this->m_proj_parm.ad) * cos(u) * cos(deltav));
                    d = asin(cos(u) * sin(deltav) / cos(s));

                    eps = this->m_proj_parm.n * d;
                    rho = this->m_proj_parm.rho0 * math::pow(tan(S0 / T(2) + S45) , this->m_proj_parm.n) / math::pow(tan(s / T(2) + S45) , this->m_proj_parm.n);

                    xy_y = rho * cos(eps);
                    xy_x = rho * sin(eps);

                    xy_y *= this->m_proj_parm.czech;
                    xy_x *= this->m_proj_parm.czech;
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    T u, deltav, s, d, eps, rho, fi1, xy0;
                    int i;

                    // TODO: replace with std::swap()
                    xy0 = xy_x;
                    xy_x = xy_y;
                    xy_y = xy0;

                    xy_x *= this->m_proj_parm.czech;
                    xy_y *= this->m_proj_parm.czech;

                    rho = sqrt(xy_x * xy_x + xy_y * xy_y);
                    eps = atan2(xy_y, xy_x);

                    d = eps / sin(S0);
                    s = T(2) * (atan(math::pow(this->m_proj_parm.rho0 / rho, T(1) / this->m_proj_parm.n) * tan(S0 / T(2) + S45)) - S45);

                    u = asin(cos(this->m_proj_parm.ad) * sin(s) - sin(this->m_proj_parm.ad) * cos(s) * cos(d));
                    deltav = asin(cos(s) * sin(d) / cos(u));

                    lp_lon = par.lam0 - deltav / this->m_proj_parm.alpha;

                    /* ITERATION FOR lp_lat */
                    fi1 = u;

                    for (i = max_iter; i ; --i) {
                        lp_lat = T(2) * ( atan( math::pow( this->m_proj_parm.k, T(-1) / this->m_proj_parm.alpha)  *
                                              math::pow( tan(u / T(2) + S45) , T(1) / this->m_proj_parm.alpha)  *
                                              math::pow( (T(1) + par.e * sin(fi1)) / (T(1) - par.e * sin(fi1)) , par.e / T(2))
                                            )  - S45);

                        if (fabs(fi1 - lp_lat) < epsilon)
                            break;
                        fi1 = lp_lat;
                    }
                    if( i == 0 )
                        BOOST_THROW_EXCEPTION( projection_exception(error_non_convergent) );

                   lp_lon -= par.lam0;
                }

                static inline std::string get_name()
                {
                    return "krovak_ellipsoid";
                }

            };

            // Krovak
            template <typename Params, typename Parameters, typename T>
            inline void setup_krovak(Params const& params, Parameters& par, par_krovak<T>& proj_parm)
            {
                T u0, n0, g;

                /* we want Bessel as fixed ellipsoid */
                par.a = 6377397.155;
                par.es = 0.006674372230614;
                par.e = sqrt(par.es);

                /* if latitude of projection center is not set, use 49d30'N */
                if (!pj_param_exists<srs::spar::lat_0>(params, "lat_0", srs::dpar::lat_0))
                    par.phi0 = 0.863937979737193;

                /* if center long is not set use 42d30'E of Ferro - 17d40' for Ferro */
                /* that will correspond to using longitudes relative to greenwich    */
                /* as input and output, instead of lat/long relative to Ferro */
                if (!pj_param_exists<srs::spar::lon_0>(params, "lon_0", srs::dpar::lon_0))
                    par.lam0 = 0.7417649320975901 - 0.308341501185665;

                /* if scale not set default to 0.9999 */
                if (!pj_param_exists<srs::spar::k>(params, "k", srs::dpar::k))
                    par.k0 = 0.9999;

                proj_parm.czech = 1;
                if( !pj_param_exists<srs::spar::czech>(params, "czech", srs::dpar::czech) )
                    proj_parm.czech = -1;

                /* Set up shared parameters between forward and inverse */
                proj_parm.alpha = sqrt(T(1) + (par.es * math::pow(cos(par.phi0), 4)) / (T(1) - par.es));
                u0 = asin(sin(par.phi0) / proj_parm.alpha);
                g = math::pow( (T(1) + par.e * sin(par.phi0)) / (T(1) - par.e * sin(par.phi0)) , proj_parm.alpha * par.e / T(2) );
                proj_parm.k = tan( u0 / 2. + S45) / math::pow(tan(par.phi0 / T(2) + S45) , proj_parm.alpha) * g;
                n0 = sqrt(T(1) - par.es) / (T(1) - par.es * math::pow(sin(par.phi0), 2));
                proj_parm.n = sin(S0);
                proj_parm.rho0 = par.k0 * n0 / tan(S0);
                proj_parm.ad = S90 - UQ;
            }

    }} // namespace detail::krovak
    #endif // doxygen

    /*!
        \brief Krovak projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Ellipsoid
        \par Projection parameters
         - lat_ts: Latitude of true scale (degrees)
         - lat_0: Latitude of origin
         - lon_0: Central meridian
         - k: Scale factor on the pseudo standard parallel
        \par Example
        \image html ex_krovak.gif
    */
    template <typename T, typename Parameters>
    struct krovak_ellipsoid : public detail::krovak::base_krovak_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline krovak_ellipsoid(Params const& params, Parameters & par)
        {
            detail::krovak::setup_krovak(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_krovak, krovak_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(krovak_entry, krovak_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(krovak_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(krovak, krovak_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_KROVAK_HPP

