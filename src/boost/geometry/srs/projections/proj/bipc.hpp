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

#ifndef BOOST_GEOMETRY_PROJECTIONS_BIPC_HPP
#define BOOST_GEOMETRY_PROJECTIONS_BIPC_HPP

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
    namespace detail { namespace bipc
    {

            static const double epsilon = 1e-10;
            static const double epsilon10 = 1e-10;
            static const double one_plus_eps = 1.000000001;
            static const int n_iter = 10;
            static const double lamB = -.34894976726250681539;
            static const double n = .63055844881274687180;
            static const double F = 1.89724742567461030582;
            static const double Azab = .81650043674686363166;
            static const double Azba = 1.82261843856185925133;
            static const double const_T = 1.27246578267089012270;
            static const double rhoc = 1.20709121521568721927;
            static const double cAzc = .69691523038678375519;
            static const double sAzc = .71715351331143607555;
            static const double C45 = .70710678118654752469;
            static const double S45 = .70710678118654752410;
            static const double C20 = .93969262078590838411;
            static const double S20 = -.34202014332566873287;
            static const double R110 = 1.91986217719376253360;
            static const double R104 = 1.81514242207410275904;

            struct par_bipc
            {
                bool   noskew;
            };

            template <typename T, typename Parameters>
            struct base_bipc_spheroid
            {
                par_bipc m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T pi = detail::pi<T>();

                    T cphi, sphi, tphi, t, al, Az, z, Av, cdlam, sdlam, r;
                    int tag;

                    cphi = cos(lp_lat);
                    sphi = sin(lp_lat);
                    cdlam = cos(sdlam = lamB - lp_lon);
                    sdlam = sin(sdlam);
                    if (fabs(fabs(lp_lat) - half_pi) < epsilon10) {
                        Az = lp_lat < 0. ? pi : 0.;
                        tphi = HUGE_VAL;
                    } else {
                        tphi = sphi / cphi;
                        Az = atan2(sdlam , C45 * (tphi - cdlam));
                    }
                    if( (tag = (Az > Azba)) ) {
                        cdlam = cos(sdlam = lp_lon + R110);
                        sdlam = sin(sdlam);
                        z = S20 * sphi + C20 * cphi * cdlam;
                        if (fabs(z) > 1.) {
                            if (fabs(z) > one_plus_eps)
                                BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                            else
                                z = z < 0. ? -1. : 1.;
                        } else
                            z = acos(z);
                        if (tphi != HUGE_VAL)
                            Az = atan2(sdlam, (C20 * tphi - S20 * cdlam));
                        Av = Azab;
                        xy_y = rhoc;
                    } else {
                        z = S45 * (sphi + cphi * cdlam);
                        if (fabs(z) > 1.) {
                            if (fabs(z) > one_plus_eps)
                                BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                            else
                                z = z < 0. ? -1. : 1.;
                        } else
                            z = acos(z);
                        Av = Azba;
                        xy_y = -rhoc;
                    }
                    if (z < 0.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    r = F * (t = math::pow(tan(T(0.5) * z), n));
                    if ((al = .5 * (R104 - z)) < 0.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    al = (t + math::pow(al, n)) / const_T;
                    if (fabs(al) > 1.) {
                        if (fabs(al) > one_plus_eps)
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        else
                            al = al < 0. ? -1. : 1.;
                    } else
                        al = acos(al);
                    if (fabs(t = n * (Av - Az)) < al)
                        r /= cos(al + (tag ? t : -t));
                    xy_x = r * sin(t);
                    xy_y += (tag ? -r : r) * cos(t);
                    if (this->m_proj_parm.noskew) {
                        t = xy_x;
                        xy_x = -xy_x * cAzc - xy_y * sAzc;
                        xy_y = -xy_y * cAzc + t * sAzc;
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    T t, r, rp, rl, al, z, fAz, Az, s, c, Av;
                    int neg, i;

                    if (this->m_proj_parm.noskew) {
                        t = xy_x;
                        xy_x = -xy_x * cAzc + xy_y * sAzc;
                        xy_y = -xy_y * cAzc - t * sAzc;
                    }
                    if( (neg = (xy_x < 0.)) ) {
                        xy_y = rhoc - xy_y;
                        s = S20;
                        c = C20;
                        Av = Azab;
                    } else {
                        xy_y += rhoc;
                        s = S45;
                        c = C45;
                        Av = Azba;
                    }
                    rl = rp = r = boost::math::hypot(xy_x, xy_y);
                    fAz = fabs(Az = atan2(xy_x, xy_y));
                    for (i = n_iter; i ; --i) {
                        z = 2. * atan(math::pow(r / F,T(1) / n));
                        al = acos((math::pow(tan(T(0.5) * z), n) +
                           math::pow(tan(T(0.5) * (R104 - z)), n)) / const_T);
                        if (fAz < al)
                            r = rp * cos(al + (neg ? Az : -Az));
                        if (fabs(rl - r) < epsilon)
                            break;
                        rl = r;
                    }
                    if (! i)
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    Az = Av - Az / n;
                    lp_lat = asin(s * cos(z) + c * sin(z) * cos(Az));
                    lp_lon = atan2(sin(Az), c / tan(z) - s * cos(Az));
                    if (neg)
                        lp_lon -= R110;
                    else
                        lp_lon = lamB - lp_lon;
                }

                static inline std::string get_name()
                {
                    return "bipc_spheroid";
                }

            };

            // Bipolar conic of western hemisphere
            template <typename Params, typename Parameters>
            inline void setup_bipc(Params const& params, Parameters& par, par_bipc& proj_parm)
            {
                proj_parm.noskew = pj_get_param_b<srs::spar::ns>(params, "ns", srs::dpar::ns);
                par.es = 0.;
            }

    }} // namespace detail::bipc
    #endif // doxygen

    /*!
        \brief Bipolar conic of western hemisphere projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Conic
         - Spheroid
        \par Projection parameters
         - ns (boolean)
        \par Example
        \image html ex_bipc.gif
    */
    template <typename T, typename Parameters>
    struct bipc_spheroid : public detail::bipc::base_bipc_spheroid<T, Parameters>
    {
        template <typename Params>
        inline bipc_spheroid(Params const& params, Parameters & par)
        {
            detail::bipc::setup_bipc(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_bipc, bipc_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(bipc_entry, bipc_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(bipc_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(bipc, bipc_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_BIPC_HPP

