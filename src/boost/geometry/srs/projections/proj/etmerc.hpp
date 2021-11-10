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

// Copyright (c) 2008   Gerald I. Evenden

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

/* The code in this file is largly based upon procedures:
 *
 * Written by: Knud Poder and Karsten Engsager
 *
 * Based on math from: R.Koenig and K.H. Weise, "Mathematische
 * Grundlagen der hoeheren Geodaesie und Kartographie,
 * Springer-Verlag, Berlin/Goettingen" Heidelberg, 1951.
 *
 * Modified and used here by permission of Reference Networks
 * Division, Kort og Matrikelstyrelsen (KMS), Copenhagen, Denmark
*/

#ifndef BOOST_GEOMETRY_PROJECTIONS_ETMERC_HPP
#define BOOST_GEOMETRY_PROJECTIONS_ETMERC_HPP

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/function_overloads.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

#include <boost/math/special_functions/hypot.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace etmerc
    {

            static const int PROJ_ETMERC_ORDER = 6;

            template <typename T>
            struct par_etmerc
            {
                T    Qn;    /* Merid. quad., scaled to the projection */
                T    Zb;    /* Radius vector in polar coord. systems  */
                T    cgb[6]; /* Constants for Gauss -> Geo lat */
                T    cbg[6]; /* Constants for Geo lat -> Gauss */
                T    utg[6]; /* Constants for transv. merc. -> geo */
                T    gtu[6]; /* Constants for geo -> transv. merc. */
            };

            template <typename T>
            inline T log1py(T const& x) {              /* Compute log(1+x) accurately */
                volatile T
                  y = 1 + x,
                  z = y - 1;
                /* Here's the explanation for this magic: y = 1 + z, exactly, and z
                 * approx x, thus log(y)/z (which is nearly constant near z = 0) returns
                 * a good approximation to the true log(1 + x)/x.  The multiplication x *
                 * (log(y)/z) introduces little additional error. */
                return z == 0 ? x : x * log(y) / z;
            }

            template <typename T>
            inline T asinhy(T const& x) {              /* Compute asinh(x) accurately */
                T y = fabs(x);         /* Enforce odd parity */
                y = log1py(y * (1 + y/(boost::math::hypot(1.0, y) + 1)));
                return x < 0 ? -y : y;
            }

            template <typename T>
            inline T gatg(const T *p1, int len_p1, T const& B) {
                const T *p;
                T h = 0, h1, h2 = 0, cos_2B;

                cos_2B = 2*cos(2*B);
                for (p = p1 + len_p1, h1 = *--p; p - p1; h2 = h1, h1 = h)
                    h = -h2 + cos_2B*h1 + *--p;
                return (B + h*sin(2*B));
            }

            /* Complex Clenshaw summation */
            template <typename T>
            inline T clenS(const T *a, int size, T const& arg_r, T const& arg_i, T *R, T *I) {
                T      r, i, hr, hr1, hr2, hi, hi1, hi2;
                T      sin_arg_r, cos_arg_r, sinh_arg_i, cosh_arg_i;

                /* arguments */
                const T* p = a + size;
                sin_arg_r  = sin(arg_r);
                cos_arg_r  = cos(arg_r);
                sinh_arg_i = sinh(arg_i);
                cosh_arg_i = cosh(arg_i);
                r          =  2*cos_arg_r*cosh_arg_i;
                i          = -2*sin_arg_r*sinh_arg_i;
                /* summation loop */
                for (hi1 = hr1 = hi = 0, hr = *--p; a - p;) {
                    hr2 = hr1;
                    hi2 = hi1;
                    hr1 = hr;
                    hi1 = hi;
                    hr  = -hr2 + r*hr1 - i*hi1 + *--p;
                    hi  = -hi2 + i*hr1 + r*hi1;
                }
                r   = sin_arg_r*cosh_arg_i;
                i   = cos_arg_r*sinh_arg_i;
                *R  = r*hr - i*hi;
                *I  = r*hi + i*hr;
                return(*R);
            }

            /* Real Clenshaw summation */
            template <typename T>
            inline T clens(const T *a, int size, T const& arg_r) {
                T      r, hr, hr1, hr2, cos_arg_r;

                const T* p = a + size;
                cos_arg_r  = cos(arg_r);
                r          =  2*cos_arg_r;

                /* summation loop */
                for (hr1 = 0, hr = *--p; a - p;) {
                    hr2 = hr1;
                    hr1 = hr;
                    hr  = -hr2 + r*hr1 + *--p;
                }
                return(sin(arg_r)*hr);
            }

            template <typename T, typename Parameters>
            struct base_etmerc_ellipsoid
            {
                par_etmerc<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T sin_Cn, cos_Cn, cos_Ce, sin_Ce, dCn, dCe;
                    T Cn = lp_lat, Ce = lp_lon;

                    /* ell. LAT, LNG -> Gaussian LAT, LNG */
                    Cn  = gatg(this->m_proj_parm.cbg, PROJ_ETMERC_ORDER, Cn);
                    /* Gaussian LAT, LNG -> compl. sph. LAT */
                    sin_Cn = sin(Cn);
                    cos_Cn = cos(Cn);
                    sin_Ce = sin(Ce);
                    cos_Ce = cos(Ce);

                    Cn     = atan2(sin_Cn, cos_Ce*cos_Cn);
                    Ce     = atan2(sin_Ce*cos_Cn, boost::math::hypot(sin_Cn, cos_Cn*cos_Ce));

                    /* compl. sph. N, E -> ell. norm. N, E */
                    Ce  = asinhy(tan(Ce));     /* Replaces: Ce  = log(tan(fourth_pi + Ce*0.5)); */
                    Cn += clenS(this->m_proj_parm.gtu, PROJ_ETMERC_ORDER, 2*Cn, 2*Ce, &dCn, &dCe);
                    Ce += dCe;
                    if (fabs(Ce) <= 2.623395162778) {
                        xy_y  = this->m_proj_parm.Qn * Cn + this->m_proj_parm.Zb;  /* Northing */
                        xy_x  = this->m_proj_parm.Qn * Ce;  /* Easting  */
                    } else
                        xy_x = xy_y = HUGE_VAL;
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    T sin_Cn, cos_Cn, cos_Ce, sin_Ce, dCn, dCe;
                    T Cn = xy_y, Ce = xy_x;

                    /* normalize N, E */
                    Cn = (Cn - this->m_proj_parm.Zb)/this->m_proj_parm.Qn;
                    Ce = Ce/this->m_proj_parm.Qn;

                    if (fabs(Ce) <= 2.623395162778) { /* 150 degrees */
                        /* norm. N, E -> compl. sph. LAT, LNG */
                        Cn += clenS(this->m_proj_parm.utg, PROJ_ETMERC_ORDER, 2*Cn, 2*Ce, &dCn, &dCe);
                        Ce += dCe;
                        Ce = atan(sinh(Ce)); /* Replaces: Ce = 2*(atan(exp(Ce)) - fourth_pi); */
                        /* compl. sph. LAT -> Gaussian LAT, LNG */
                        sin_Cn = sin(Cn);
                        cos_Cn = cos(Cn);
                        sin_Ce = sin(Ce);
                        cos_Ce = cos(Ce);
                        Ce     = atan2(sin_Ce, cos_Ce*cos_Cn);
                        Cn     = atan2(sin_Cn*cos_Ce, boost::math::hypot(sin_Ce, cos_Ce*cos_Cn));
                        /* Gaussian LAT, LNG -> ell. LAT, LNG */
                        lp_lat = gatg(this->m_proj_parm.cgb,  PROJ_ETMERC_ORDER, Cn);
                        lp_lon = Ce;
                    }
                    else
                        lp_lat = lp_lon = HUGE_VAL;
                }

                static inline std::string get_name()
                {
                    return "etmerc_ellipsoid";
                }

            };

            template <typename Parameters, typename T>
            inline void setup(Parameters& par, par_etmerc<T>& proj_parm)
            {
                T f, n, np, Z;

                if (par.es <= 0) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_ellipsoid_use_required) );
                }

                f = par.es / (1 + sqrt(1 -  par.es)); /* Replaces: f = 1 - sqrt(1-par.es); */

                /* third flattening */
                np = n = f/(2 - f);

                /* COEF. OF TRIG SERIES GEO <-> GAUSS */
                /* cgb := Gaussian -> Geodetic, KW p190 - 191 (61) - (62) */
                /* cbg := Geodetic -> Gaussian, KW p186 - 187 (51) - (52) */
                /* PROJ_ETMERC_ORDER = 6th degree : Engsager and Poder: ICC2007 */

                proj_parm.cgb[0] = n*( 2 + n*(-2/3.0  + n*(-2      + n*(116/45.0 + n*(26/45.0 +
                            n*(-2854/675.0 ))))));
                proj_parm.cbg[0] = n*(-2 + n*( 2/3.0  + n*( 4/3.0  + n*(-82/45.0 + n*(32/45.0 +
                            n*( 4642/4725.0))))));
                np     *= n;
                proj_parm.cgb[1] = np*(7/3.0 + n*( -8/5.0  + n*(-227/45.0 + n*(2704/315.0 +
                            n*( 2323/945.0)))));
                proj_parm.cbg[1] = np*(5/3.0 + n*(-16/15.0 + n*( -13/9.0  + n*( 904/315.0 +
                            n*(-1522/945.0)))));
                np     *= n;
                /* n^5 coeff corrected from 1262/105 -> -1262/105 */
                proj_parm.cgb[2] = np*( 56/15.0  + n*(-136/35.0 + n*(-1262/105.0 +
                            n*( 73814/2835.0))));
                proj_parm.cbg[2] = np*(-26/15.0  + n*(  34/21.0 + n*(    8/5.0   +
                            n*(-12686/2835.0))));
                np     *= n;
                /* n^5 coeff corrected from 322/35 -> 332/35 */
                proj_parm.cgb[3] = np*(4279/630.0 + n*(-332/35.0 + n*(-399572/14175.0)));
                proj_parm.cbg[3] = np*(1237/630.0 + n*( -12/5.0  + n*( -24832/14175.0)));
                np     *= n;
                proj_parm.cgb[4] = np*(4174/315.0 + n*(-144838/6237.0 ));
                proj_parm.cbg[4] = np*(-734/315.0 + n*( 109598/31185.0));
                np     *= n;
                proj_parm.cgb[5] = np*(601676/22275.0 );
                proj_parm.cbg[5] = np*(444337/155925.0);

                /* Constants of the projections */
                /* Transverse Mercator (UTM, ITM, etc) */
                np = n*n;
                /* Norm. mer. quad, K&W p.50 (96), p.19 (38b), p.5 (2) */
                proj_parm.Qn = par.k0/(1 + n) * (1 + np*(1/4.0 + np*(1/64.0 + np/256.0)));
                /* coef of trig series */
                /* utg := ell. N, E -> sph. N, E,  KW p194 (65) */
                /* gtu := sph. N, E -> ell. N, E,  KW p196 (69) */
                proj_parm.utg[0] = n*(-0.5  + n*( 2/3.0 + n*(-37/96.0 + n*( 1/360.0 +
                            n*(  81/512.0 + n*(-96199/604800.0))))));
                proj_parm.gtu[0] = n*( 0.5  + n*(-2/3.0 + n*(  5/16.0 + n*(41/180.0 +
                            n*(-127/288.0 + n*(  7891/37800.0 ))))));
                proj_parm.utg[1] = np*(-1/48.0 + n*(-1/15.0 + n*(437/1440.0 + n*(-46/105.0 +
                            n*( 1118711/3870720.0)))));
                proj_parm.gtu[1] = np*(13/48.0 + n*(-3/5.0  + n*(557/1440.0 + n*(281/630.0 +
                            n*(-1983433/1935360.0)))));
                np      *= n;
                proj_parm.utg[2] = np*(-17/480.0 + n*(  37/840.0 + n*(  209/4480.0  +
                            n*( -5569/90720.0 ))));
                proj_parm.gtu[2] = np*( 61/240.0 + n*(-103/140.0 + n*(15061/26880.0 +
                            n*(167603/181440.0))));
                np      *= n;
                proj_parm.utg[3] = np*(-4397/161280.0 + n*(  11/504.0 + n*( 830251/7257600.0)));
                proj_parm.gtu[3] = np*(49561/161280.0 + n*(-179/168.0 + n*(6601661/7257600.0)));
                np     *= n;
                proj_parm.utg[4] = np*(-4583/161280.0 + n*(  108847/3991680.0));
                proj_parm.gtu[4] = np*(34729/80640.0  + n*(-3418889/1995840.0));
                np     *= n;
                proj_parm.utg[5] = np*(-20648693/638668800.0);
                proj_parm.gtu[5] = np*(212378941/319334400.0);

                /* Gaussian latitude value of the origin latitude */
                Z = gatg(proj_parm.cbg, PROJ_ETMERC_ORDER, par.phi0);

                /* Origin northing minus true northing at the origin latitude */
                /* i.e. true northing = N - proj_parm.Zb                         */
                proj_parm.Zb  = - proj_parm.Qn*(Z + clens(proj_parm.gtu, PROJ_ETMERC_ORDER, 2*Z));
            }

            // Extended Transverse Mercator
            template <typename Parameters, typename T>
            inline void setup_etmerc(Parameters& par, par_etmerc<T>& proj_parm)
            {
                setup(par, proj_parm);
            }

            // Universal Transverse Mercator (UTM)
            template <typename Params, typename Parameters, typename T>
            inline void setup_utm(Params const& params, Parameters& par, par_etmerc<T>& proj_parm)
            {
                static const T pi = detail::pi<T>();

                int zone;

                if (par.es == 0.0) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_ellipsoid_use_required) );
                }

                par.y0 = pj_get_param_b<srs::spar::south>(params, "south", srs::dpar::south) ? 10000000. : 0.;
                par.x0 = 500000.;
                if (pj_param_i<srs::spar::zone>(params, "zone", srs::dpar::zone, zone)) /* zone input ? */
                {
                    if (zone > 0 && zone <= 60)
                        --zone;
                    else {
                        BOOST_THROW_EXCEPTION( projection_exception(error_invalid_utm_zone) );
                    }
                }
                else /* nearest central meridian input */
                {
                    zone = int_floor((adjlon(par.lam0) + pi) * 30. / pi);
                    if (zone < 0)
                        zone = 0;
                    else if (zone >= 60)
                        zone = 59;
                }
                par.lam0 = (zone + .5) * pi / 30. - pi;
                par.k0 = 0.9996;
                par.phi0 = 0.;

                setup(par, proj_parm);
            }

    }} // namespace detail::etmerc
    #endif // doxygen

    /*!
        \brief Extended Transverse Mercator projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Cylindrical
         - Spheroid
        \par Projection parameters
         - lat_ts: Latitude of true scale
         - lat_0: Latitude of origin
        \par Example
        \image html ex_etmerc.gif
    */
    template <typename T, typename Parameters>
    struct etmerc_ellipsoid : public detail::etmerc::base_etmerc_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline etmerc_ellipsoid(Params const& , Parameters & par)
        {
            detail::etmerc::setup_etmerc(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Universal Transverse Mercator (UTM) projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Cylindrical
         - Spheroid
        \par Projection parameters
         - zone: UTM Zone (integer)
         - south: Denotes southern hemisphere UTM zone (boolean)
        \par Example
        \image html ex_utm.gif
    */
    template <typename T, typename Parameters>
    struct utm_ellipsoid : public detail::etmerc::base_etmerc_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline utm_ellipsoid(Params const& params, Parameters & par)
        {
            detail::etmerc::setup_utm(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_etmerc, etmerc_ellipsoid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_utm, utm_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(etmerc_entry, etmerc_ellipsoid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(utm_entry, utm_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(etmerc_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(etmerc, etmerc_entry);
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(utm, utm_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_ETMERC_HPP

