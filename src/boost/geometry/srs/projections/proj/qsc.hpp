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

// This implements the Quadrilateralized Spherical Cube (QSC) projection.
// Copyright (c) 2011, 2012  Martin Lambers <marlam@marlam.de>

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

// The QSC projection was introduced in:
// [OL76]
// E.M. O'Neill and R.E. Laubscher, "Extended Studies of a Quadrilateralized
// Spherical Cube Earth Data Base", Naval Environmental Prediction Research
// Facility Tech. Report NEPRF 3-76 (CSC), May 1976.

// The preceding shift from an ellipsoid to a sphere, which allows to apply
// this projection to ellipsoids as used in the Ellipsoidal Cube Map model,
// is described in
// [LK12]
// M. Lambers and A. Kolb, "Ellipsoidal Cube Maps for Accurate Rendering of
// Planetary-Scale Terrain Data", Proc. Pacfic Graphics (Short Papers), Sep.
// 2012

// You have to choose one of the following projection centers,
// corresponding to the centers of the six cube faces:
// phi0 = 0.0, lam0 = 0.0       ("front" face)
// phi0 = 0.0, lam0 = 90.0      ("right" face)
// phi0 = 0.0, lam0 = 180.0     ("back" face)
// phi0 = 0.0, lam0 = -90.0     ("left" face)
// phi0 = 90.0                  ("top" face)
// phi0 = -90.0                 ("bottom" face)
// Other projection centers will not work!

// In the projection code below, each cube face is handled differently.
// See the computation of the face parameter in the ENTRY0(qsc) function
// and the handling of different face values (FACE_*) in the forward and
// inverse projections.

// Furthermore, the projection is originally only defined for theta angles
// between (-1/4 * PI) and (+1/4 * PI) on the current cube face. This area
// of definition is named AREA_0 in the projection code below. The other
// three areas of a cube face are handled by rotation of AREA_0.

#ifndef BOOST_GEOMETRY_PROJECTIONS_QSC_HPP
#define BOOST_GEOMETRY_PROJECTIONS_QSC_HPP

#include <boost/core/ignore_unused.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace qsc
    {

            /* The six cube faces. */
            enum face_type {
                face_front  = 0,
                face_right  = 1,
                face_back   = 2,
                face_left   = 3,
                face_top    = 4,
                face_bottom = 5
            };

            template <typename T>
            struct par_qsc
            {
                T a_squared;
                T b;
                T one_minus_f;
                T one_minus_f_squared;
                face_type face;
            };

            static const double epsilon10 = 1.e-10;

            /* The four areas on a cube face. AREA_0 is the area of definition,
             * the other three areas are counted counterclockwise. */
            enum area_type {
                area_0 = 0,
                area_1 = 1,
                area_2 = 2,
                area_3 = 3
            };

            /* Helper function for forward projection: compute the theta angle
             * and determine the area number. */
            template <typename T>
            inline T qsc_fwd_equat_face_theta(T const& phi, T const& y, T const& x, area_type *area)
            {
                static const T fourth_pi = detail::fourth_pi<T>();
                static const T half_pi = detail::half_pi<T>();
                static const T pi = detail::pi<T>();

                T theta;
                if (phi < epsilon10) {
                    *area = area_0;
                    theta = 0.0;
                } else {
                    theta = atan2(y, x);
                    if (fabs(theta) <= fourth_pi) {
                        *area = area_0;
                    } else if (theta > fourth_pi && theta <= half_pi + fourth_pi) {
                        *area = area_1;
                        theta -= half_pi;
                    } else if (theta > half_pi + fourth_pi || theta <= -(half_pi + fourth_pi)) {
                        *area = area_2;
                        theta = (theta >= 0.0 ? theta - pi : theta + pi);
                    } else {
                        *area = area_3;
                        theta += half_pi;
                    }
                }
                return theta;
            }

            /* Helper function: shift the longitude. */
            template <typename T>
            inline T qsc_shift_lon_origin(T const& lon, T const& offset)
            {
                static const T pi = detail::pi<T>();
                static const T two_pi = detail::two_pi<T>();

                T slon = lon + offset;
                if (slon < -pi) {
                    slon += two_pi;
                } else if (slon > +pi) {
                    slon -= two_pi;
                }
                return slon;
            }

            /* Forward projection, ellipsoid */

            template <typename T, typename Parameters>
            struct base_qsc_ellipsoid
            {
                par_qsc<T> m_proj_parm;

                // FORWARD(e_forward)
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T fourth_pi = detail::fourth_pi<T>();
                    static const T half_pi = detail::half_pi<T>();
                    static const T pi = detail::pi<T>();

                        T lat, lon;
                        T theta, phi;
                        T t, mu; /* nu; */ 
                        area_type area;

                        /* Convert the geodetic latitude to a geocentric latitude.
                         * This corresponds to the shift from the ellipsoid to the sphere
                         * described in [LK12]. */
                        if (par.es != 0.0) {
                            lat = atan(this->m_proj_parm.one_minus_f_squared * tan(lp_lat));
                        } else {
                            lat = lp_lat;
                        }

                        /* Convert the input lat, lon into theta, phi as used by QSC.
                         * This depends on the cube face and the area on it.
                         * For the top and bottom face, we can compute theta and phi
                         * directly from phi, lam. For the other faces, we must use
                         * unit sphere cartesian coordinates as an intermediate step. */
                        lon = lp_lon;
                        if (this->m_proj_parm.face == face_top) {
                            phi = half_pi - lat;
                            if (lon >= fourth_pi && lon <= half_pi + fourth_pi) {
                                area = area_0;
                                theta = lon - half_pi;
                            } else if (lon > half_pi + fourth_pi || lon <= -(half_pi + fourth_pi)) {
                                area = area_1;
                                theta = (lon > 0.0 ? lon - pi : lon + pi);
                            } else if (lon > -(half_pi + fourth_pi) && lon <= -fourth_pi) {
                                area = area_2;
                                theta = lon + half_pi;
                            } else {
                                area = area_3;
                                theta = lon;
                            }
                        } else if (this->m_proj_parm.face == face_bottom) {
                            phi = half_pi + lat;
                            if (lon >= fourth_pi && lon <= half_pi + fourth_pi) {
                                area = area_0;
                                theta = -lon + half_pi;
                            } else if (lon < fourth_pi && lon >= -fourth_pi) {
                                area = area_1;
                                theta = -lon;
                            } else if (lon < -fourth_pi && lon >= -(half_pi + fourth_pi)) {
                                area = area_2;
                                theta = -lon - half_pi;
                            } else {
                                area = area_3;
                                theta = (lon > 0.0 ? -lon + pi : -lon - pi);
                            }
                        } else {
                            T q, r, s;
                            T sinlat, coslat;
                            T sinlon, coslon;

                            if (this->m_proj_parm.face == face_right) {
                                lon = qsc_shift_lon_origin(lon, +half_pi);
                            } else if (this->m_proj_parm.face == face_back) {
                                lon = qsc_shift_lon_origin(lon, +pi);
                            } else if (this->m_proj_parm.face == face_left) {
                                lon = qsc_shift_lon_origin(lon, -half_pi);
                            }
                            sinlat = sin(lat);
                            coslat = cos(lat);
                            sinlon = sin(lon);
                            coslon = cos(lon);
                            q = coslat * coslon;
                            r = coslat * sinlon;
                            s = sinlat;

                            if (this->m_proj_parm.face == face_front) {
                                phi = acos(q);
                                theta = qsc_fwd_equat_face_theta(phi, s, r, &area);
                            } else if (this->m_proj_parm.face == face_right) {
                                phi = acos(r);
                                theta = qsc_fwd_equat_face_theta(phi, s, -q, &area);
                            } else if (this->m_proj_parm.face == face_back) {
                                phi = acos(-q);
                                theta = qsc_fwd_equat_face_theta(phi, s, -r, &area);
                            } else if (this->m_proj_parm.face == face_left) {
                                phi = acos(-r);
                                theta = qsc_fwd_equat_face_theta(phi, s, q, &area);
                            } else {
                                /* Impossible */
                                phi = theta = 0.0;
                                area = area_0;
                            }
                        }

                        /* Compute mu and nu for the area of definition.
                         * For mu, see Eq. (3-21) in [OL76], but note the typos:
                         * compare with Eq. (3-14). For nu, see Eq. (3-38). */
                        mu = atan((12.0 / pi) * (theta + acos(sin(theta) * cos(fourth_pi)) - half_pi));
                        // TODO: (cos(mu) * cos(mu)) could be replaced with sqr(cos(mu))
                        t = sqrt((1.0 - cos(phi)) / (cos(mu) * cos(mu)) / (1.0 - cos(atan(1.0 / cos(theta)))));
                        /* nu = atan(t);        We don't really need nu, just t, see below. */

                        /* Apply the result to the real area. */
                        if (area == area_1) {
                            mu += half_pi;
                        } else if (area == area_2) {
                            mu += pi;
                        } else if (area == area_3) {
                            mu += half_pi + pi;
                        }

                        /* Now compute x, y from mu and nu */
                        /* t = tan(nu); */
                        xy_x = t * cos(mu);
                        xy_y = t * sin(mu);
                }
                /* Inverse projection, ellipsoid */

                // INVERSE(e_inverse)
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T pi = detail::pi<T>();

                        T mu, nu, cosmu, tannu;
                        T tantheta, theta, cosphi, phi;
                        T t;
                        int area;

                        /* Convert the input x, y to the mu and nu angles as used by QSC.
                         * This depends on the area of the cube face. */
                        nu = atan(sqrt(xy_x * xy_x + xy_y * xy_y));
                        mu = atan2(xy_y, xy_x);
                        if (xy_x >= 0.0 && xy_x >= fabs(xy_y)) {
                            area = area_0;
                        } else if (xy_y >= 0.0 && xy_y >= fabs(xy_x)) {
                            area = area_1;
                            mu -= half_pi;
                        } else if (xy_x < 0.0 && -xy_x >= fabs(xy_y)) {
                            area = area_2;
                            mu = (mu < 0.0 ? mu + pi : mu - pi);
                        } else {
                            area = area_3;
                            mu += half_pi;
                        }

                        /* Compute phi and theta for the area of definition.
                         * The inverse projection is not described in the original paper, but some
                         * good hints can be found here (as of 2011-12-14):
                         * http://fits.gsfc.nasa.gov/fitsbits/saf.93/saf.9302
                         * (search for "Message-Id: <9302181759.AA25477 at fits.cv.nrao.edu>") */
                        t = (pi / 12.0) * tan(mu);
                        tantheta = sin(t) / (cos(t) - (1.0 / sqrt(2.0)));
                        theta = atan(tantheta);
                        cosmu = cos(mu);
                        tannu = tan(nu);
                        cosphi = 1.0 - cosmu * cosmu * tannu * tannu * (1.0 - cos(atan(1.0 / cos(theta))));
                        if (cosphi < -1.0) {
                            cosphi = -1.0;
                        } else if (cosphi > +1.0) {
                            cosphi = +1.0;
                        }

                        /* Apply the result to the real area on the cube face.
                         * For the top and bottom face, we can compute phi and lam directly.
                         * For the other faces, we must use unit sphere cartesian coordinates
                         * as an intermediate step. */
                        if (this->m_proj_parm.face == face_top) {
                            phi = acos(cosphi);
                            lp_lat = half_pi - phi;
                            if (area == area_0) {
                                lp_lon = theta + half_pi;
                            } else if (area == area_1) {
                                lp_lon = (theta < 0.0 ? theta + pi : theta - pi);
                            } else if (area == area_2) {
                                lp_lon = theta - half_pi;
                            } else /* area == AREA_3 */ {
                                lp_lon = theta;
                            }
                        } else if (this->m_proj_parm.face == face_bottom) {
                            phi = acos(cosphi);
                            lp_lat = phi - half_pi;
                            if (area == area_0) {
                                lp_lon = -theta + half_pi;
                            } else if (area == area_1) {
                                lp_lon = -theta;
                            } else if (area == area_2) {
                                lp_lon = -theta - half_pi;
                            } else /* area == area_3 */ {
                                lp_lon = (theta < 0.0 ? -theta - pi : -theta + pi);
                            }
                        } else {
                            /* Compute phi and lam via cartesian unit sphere coordinates. */
                            T q, r, s;
                            q = cosphi;
                            t = q * q;
                            if (t >= 1.0) {
                                s = 0.0;
                            } else {
                                s = sqrt(1.0 - t) * sin(theta);
                            }
                            t += s * s;
                            if (t >= 1.0) {
                                r = 0.0;
                            } else {
                                r = sqrt(1.0 - t);
                            }
                            /* Rotate q,r,s into the correct area. */
                            if (area == area_1) {
                                t = r;
                                r = -s;
                                s = t;
                            } else if (area == area_2) {
                                r = -r;
                                s = -s;
                            } else if (area == area_3) {
                                t = r;
                                r = s;
                                s = -t;
                            }
                            /* Rotate q,r,s into the correct cube face. */
                            if (this->m_proj_parm.face == face_right) {
                                t = q;
                                q = -r;
                                r = t;
                            } else if (this->m_proj_parm.face == face_back) {
                                q = -q;
                                r = -r;
                            } else if (this->m_proj_parm.face == face_left) {
                                t = q;
                                q = r;
                                r = -t;
                            }
                            /* Now compute phi and lam from the unit sphere coordinates. */
                            lp_lat = acos(-s) - half_pi;
                            lp_lon = atan2(r, q);
                            if (this->m_proj_parm.face == face_right) {
                                lp_lon = qsc_shift_lon_origin(lp_lon, -half_pi);
                            } else if (this->m_proj_parm.face == face_back) {
                                lp_lon = qsc_shift_lon_origin(lp_lon, -pi);
                            } else if (this->m_proj_parm.face == face_left) {
                                lp_lon = qsc_shift_lon_origin(lp_lon, +half_pi);
                            }
                        }

                        /* Apply the shift from the sphere to the ellipsoid as described
                         * in [LK12]. */
                        if (par.es != 0.0) {
                            int invert_sign;
                            T tanphi, xa;
                            invert_sign = (lp_lat < 0.0 ? 1 : 0);
                            tanphi = tan(lp_lat);
                            xa = this->m_proj_parm.b / sqrt(tanphi * tanphi + this->m_proj_parm.one_minus_f_squared);
                            lp_lat = atan(sqrt(par.a * par.a - xa * xa) / (this->m_proj_parm.one_minus_f * xa));
                            if (invert_sign) {
                                lp_lat = -lp_lat;
                            }
                        }
                }

                static inline std::string get_name()
                {
                    return "qsc_ellipsoid";
                }

            };

            // Quadrilateralized Spherical Cube
            template <typename Parameters, typename T>
            inline void setup_qsc(Parameters const& par, par_qsc<T>& proj_parm)
            {
                static const T fourth_pi = detail::fourth_pi<T>();
                static const T half_pi = detail::half_pi<T>();

                /* Determine the cube face from the center of projection. */
                if (par.phi0 >= half_pi - fourth_pi / 2.0) {
                    proj_parm.face = face_top;
                } else if (par.phi0 <= -(half_pi - fourth_pi / 2.0)) {
                    proj_parm.face = face_bottom;
                } else if (fabs(par.lam0) <= fourth_pi) {
                    proj_parm.face = face_front;
                } else if (fabs(par.lam0) <= half_pi + fourth_pi) {
                    proj_parm.face = (par.lam0 > 0.0 ? face_right : face_left);
                } else {
                    proj_parm.face = face_back;
                }
                /* Fill in useful values for the ellipsoid <-> sphere shift
                 * described in [LK12]. */
                if (par.es != 0.0) {
                    proj_parm.a_squared = par.a * par.a;
                    proj_parm.b = par.a * sqrt(1.0 - par.es);
                    proj_parm.one_minus_f = 1.0 - (par.a - proj_parm.b) / par.a;
                    proj_parm.one_minus_f_squared = proj_parm.one_minus_f * proj_parm.one_minus_f;
                }
            }

    }} // namespace detail::qsc
    #endif // doxygen

    /*!
        \brief Quadrilateralized Spherical Cube projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
        \par Example
        \image html ex_qsc.gif
    */
    template <typename T, typename Parameters>
    struct qsc_ellipsoid : public detail::qsc::base_qsc_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline qsc_ellipsoid(Params const& , Parameters const& par)
        {
            detail::qsc::setup_qsc(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_qsc, qsc_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(qsc_entry, qsc_ellipsoid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(qsc_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(qsc, qsc_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_QSC_HPP

