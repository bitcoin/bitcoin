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

// Purpose: Implementation of the HEALPix and rHEALPix projections.
//          For background see <http://code.scenzgrid.org/index.php/p/scenzgrid-py/source/tree/master/docs/rhealpix_dggs.pdf>.
// Authors: Alex Raichev (raichev@cs.auckland.ac.nz)
//          Michael Speth (spethm@landcareresearch.co.nz)
// Notes:   Raichev implemented these projections in Python and
//          Speth translated them into C here.

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_HEALPIX_HPP
#define BOOST_GEOMETRY_PROJECTIONS_HEALPIX_HPP

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_auth.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/pj_qsfn.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace healpix
    {

            /* Fuzz to handle rounding errors: */
            static const double epsilon = 1e-15;

            template <typename T>
            struct par_healpix
            {
                T qp;
                detail::apa<T> apa;
                int north_square;
                int south_square;
            };

            template <typename T>
            struct cap_map
            {
                T x, y; /* Coordinates of the pole point (point of most extreme latitude on the polar caps). */
                int cn; /* An integer 0--3 indicating the position of the polar cap. */
                enum region_type {north, south, equatorial} region;
            };
            template <typename T>
            struct point_xy
            {
                T x, y;
            };

            /* IDENT, R1, R2, R3, R1 inverse, R2 inverse, R3 inverse:*/
            static double rot[7][2][2] = {
                /* Identity matrix */
                {{1, 0},{0, 1}},
                /* Matrix for counterclockwise rotation by pi/2: */
                {{ 0,-1},{ 1, 0}},
                /* Matrix for counterclockwise rotation by pi: */
                {{-1, 0},{ 0,-1}},
                /* Matrix for counterclockwise rotation by 3*pi/2:  */
                {{ 0, 1},{-1, 0}},
                {{ 0, 1},{-1, 0}}, // 3*pi/2
                {{-1, 0},{ 0,-1}}, // pi
                {{ 0,-1},{ 1, 0}}  // pi/2
            };

            /**
             * Returns the sign of the double.
             * @param v the parameter whose sign is returned.
             * @return 1 for positive number, -1 for negative, and 0 for zero.
             **/
            template <typename T>
            inline T pj_sign (T const& v)
            {
                return v > 0 ? 1 : (v < 0 ? -1 : 0);
            }
            /**
             * Return the index of the matrix in {{{1, 0},{0, 1}}, {{ 0,-1},{ 1, 0}}, {{-1, 0},{ 0,-1}}, {{ 0, 1},{-1, 0}}, {{ 0, 1},{-1, 0}}, {{-1, 0},{ 0,-1}}, {{ 0,-1},{ 1, 0}}}.
             * @param index ranges from -3 to 3.
             */
            inline int get_rotate_index(int index)
            {
                switch(index) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    return 2;
                case 3:
                    return 3;
                case -1:
                    return 4;
                case -2:
                    return 5;
                case -3:
                    return 6;
                }
                return 0;
            }
            /**
             * Return 1 if point (testx, testy) lies in the interior of the polygon
             * determined by the vertices in vert, and return 0 otherwise.
             * See http://paulbourke.net/geometry/polygonmesh/ for more details.
             * @param nvert the number of vertices in the polygon.
             * @param vert the (x, y)-coordinates of the polygon's vertices
             **/
            template <typename T>
            inline int pnpoly(int nvert, T vert[][2], T const& testx, T const& testy)
            {
                int i;
                int counter = 0;
                T xinters;
                point_xy<T> p1, p2;

                /* Check for boundrary cases */
                for (i = 0; i < nvert; i++) {
                    if (testx == vert[i][0] && testy == vert[i][1]) {
                        return 1;
                    }
                }

                p1.x = vert[0][0];
                p1.y = vert[0][1];

                for (i = 1; i < nvert; i++) {
                    p2.x = vert[i % nvert][0];
                    p2.y = vert[i % nvert][1];
                    if (testy > (std::min)(p1.y, p2.y)  &&
                        testy <= (std::max)(p1.y, p2.y) &&
                        testx <= (std::max)(p1.x, p2.x) &&
                        p1.y != p2.y)
                    {
                        xinters = (testy-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
                        if (p1.x == p2.x || testx <= xinters)
                            counter++;
                    }
                    p1 = p2;
                }

                if (counter % 2 == 0) {
                    return 0;
                } else {
                    return 1;
                }
            }
            /**
             * Return 1 if (x, y) lies in (the interior or boundary of) the image of the
             * HEALPix projection (in case proj=0) or in the image the rHEALPix projection
             * (in case proj=1), and return 0 otherwise.
             * @param north_square the position of the north polar square (rHEALPix only)
             * @param south_square the position of the south polar square (rHEALPix only)
             **/
            template <typename T>
            inline int in_image(T const& x, T const& y, int proj, int north_square, int south_square)
            {
                static const T pi = detail::pi<T>();
                static const T half_pi = detail::half_pi<T>();
                static const T fourth_pi = detail::fourth_pi<T>();

                if (proj == 0) {
                    T healpixVertsJit[][2] = {
                        {-pi - epsilon,   fourth_pi},
                        {-3.0*fourth_pi,  half_pi + epsilon},
                        {-half_pi,        fourth_pi + epsilon},
                        {-fourth_pi,      half_pi + epsilon},
                        {0.0,             fourth_pi + epsilon},
                        {fourth_pi,       half_pi + epsilon},
                        {half_pi,         fourth_pi + epsilon},
                        {3.0*fourth_pi,   half_pi + epsilon},
                        {pi + epsilon,    fourth_pi},
                        {pi + epsilon,   -fourth_pi},
                        {3.0*fourth_pi,  -half_pi - epsilon},
                        {half_pi,        -fourth_pi - epsilon},
                        {fourth_pi,      -half_pi - epsilon},
                        {0.0,            -fourth_pi - epsilon},
                        {-fourth_pi,     -half_pi - epsilon},
                        {-half_pi,       -fourth_pi - epsilon},
                        {-3.0*fourth_pi, -half_pi - epsilon},
                        {-pi - epsilon,  -fourth_pi}
                    };
                    return pnpoly((int)sizeof(healpixVertsJit)/
                                  sizeof(healpixVertsJit[0]), healpixVertsJit, x, y);
                } else {
                    T rhealpixVertsJit[][2] = {
                        {-pi - epsilon,                                 fourth_pi + epsilon},
                        {-pi + north_square*half_pi - epsilon,          fourth_pi + epsilon},
                        {-pi + north_square*half_pi - epsilon,          3.0*fourth_pi + epsilon},
                        {-pi + (north_square + 1.0)*half_pi + epsilon,  3.0*fourth_pi + epsilon},
                        {-pi + (north_square + 1.0)*half_pi + epsilon,  fourth_pi + epsilon},
                        {pi + epsilon,                                  fourth_pi + epsilon},
                        {pi + epsilon,                                 -fourth_pi - epsilon},
                        {-pi + (south_square + 1.0)*half_pi + epsilon, -fourth_pi - epsilon},
                        {-pi + (south_square + 1.0)*half_pi + epsilon, -3.0*fourth_pi - epsilon},
                        {-pi + south_square*half_pi - epsilon,         -3.0*fourth_pi - epsilon},
                        {-pi + south_square*half_pi - epsilon,         -fourth_pi - epsilon},
                        {-pi - epsilon,                                -fourth_pi - epsilon}
                    };

                    return pnpoly((int)sizeof(rhealpixVertsJit)/
                                  sizeof(rhealpixVertsJit[0]), rhealpixVertsJit, x, y);
                }
            }
            /**
             * Return the authalic latitude of latitude alpha (if inverse=0) or
             * return the approximate latitude of authalic latitude alpha (if inverse=1).
             * P contains the relavent ellipsoid parameters.
             **/
            template <typename Parameters, typename T>
            inline T auth_lat(const Parameters& par, const par_healpix<T>& proj_parm, T const& alpha, int inverse)
            {
                if (inverse == 0) {
                    /* Authalic latitude. */
                    T q = pj_qsfn(sin(alpha), par.e, 1.0 - par.es);
                    T qp = proj_parm.qp;
                    T ratio = q/qp;

                    if (math::abs(ratio) > 1) {
                        /* Rounding error. */
                        ratio = pj_sign(ratio);
                    }

                    return asin(ratio);
                } else {
                    /* Approximation to inverse authalic latitude. */
                    return pj_authlat(alpha, proj_parm.apa);
                }
            }
            /**
             * Return the HEALPix projection of the longitude-latitude point lp on
             * the unit sphere.
            **/
            template <typename T>
            inline void healpix_sphere(T const& lp_lam, T const& lp_phi, T& xy_x, T& xy_y)
            {               
                static const T pi = detail::pi<T>();
                static const T half_pi = detail::half_pi<T>();
                static const T fourth_pi = detail::fourth_pi<T>();

                T lam = lp_lam;
                T phi = lp_phi;
                T phi0 = asin(T(2.0)/T(3.0));

                /* equatorial region */
                if ( fabsl(phi) <= phi0) {
                    xy_x = lam;
                    xy_y = 3.0*pi/8.0*sin(phi);
                } else {
                    T lamc;
                    T sigma = sqrt(3.0*(1 - math::abs(sin(phi))));
                    T cn = floor(2*lam / pi + 2);
                    if (cn >= 4) {
                        cn = 3;
                    }
                    lamc = -3*fourth_pi + half_pi*cn;
                    xy_x = lamc + (lam - lamc)*sigma;
                    xy_y = pj_sign(phi)*fourth_pi*(2 - sigma);
                }
                return;
            }
            /**
             * Return the inverse of healpix_sphere().
            **/
            template <typename T>
            inline void healpix_sphere_inverse(T const& xy_x, T const& xy_y, T& lp_lam, T& lp_phi)
            {                
                static const T pi = detail::pi<T>();
                static const T half_pi = detail::half_pi<T>();
                static const T fourth_pi = detail::fourth_pi<T>();

                T x = xy_x;
                T y = xy_y;
                T y0 = fourth_pi;

                /* Equatorial region. */
                if (math::abs(y) <= y0) {
                    lp_lam = x;
                    lp_phi = asin(8.0*y/(3.0*pi));
                } else if (fabsl(y) < half_pi) {
                    T cn = floor(2.0*x/pi + 2.0);
                    T xc, tau;
                    if (cn >= 4) {
                        cn = 3;
                    }
                    xc = -3.0*fourth_pi + (half_pi)*cn;
                    tau = 2.0 - 4.0*fabsl(y)/pi;
                    lp_lam = xc + (x - xc)/tau;
                    lp_phi = pj_sign(y)*asin(1.0 - math::pow(tau, 2)/3.0);
                } else {
                    lp_lam = -1.0*pi;
                    lp_phi = pj_sign(y)*half_pi;
                }
                return;
            }
            /**
             * Return the vector sum a + b, where a and b are 2-dimensional vectors.
             * @param ret holds a + b.
             **/
            template <typename T>
            inline void vector_add(const T a[2], const T b[2], T ret[2])
            {
                int i;
                for(i = 0; i < 2; i++) {
                    ret[i] = a[i] + b[i];
                }
            }
            /**
             * Return the vector difference a - b, where a and b are 2-dimensional vectors.
             * @param ret holds a - b.
             **/
            template <typename T>
            inline void vector_sub(const T a[2], const T b[2], T ret[2])
            {
                int i;
                for(i = 0; i < 2; i++) {
                    ret[i] = a[i] - b[i];
                }
            }
            /**
             * Return the 2 x 1 matrix product a*b, where a is a 2 x 2 matrix and
             * b is a 2 x 1 matrix.
             * @param ret holds a*b.
             **/
            template <typename T1, typename T2>
            inline void dot_product(const T1 a[2][2], const T2 b[2], T2 ret[2])
            {
                int i, j;
                int length = 2;
                for(i = 0; i < length; i++) {
                    ret[i] = 0;
                    for(j = 0; j < length; j++) {
                        ret[i] += a[i][j]*b[j];
                    }
                }
            }
            /**
             * Return the number of the polar cap, the pole point coordinates, and
             * the region that (x, y) lies in.
             * If inverse=0, then assume (x,y) lies in the image of the HEALPix
             * projection of the unit sphere.
             * If inverse=1, then assume (x,y) lies in the image of the
             * (north_square, south_square)-rHEALPix projection of the unit sphere.
             **/
            template <typename T>
            inline cap_map<T> get_cap(T x, T const& y, int north_square, int south_square,
                                     int inverse)
            {
                static const T pi = detail::pi<T>();
                static const T half_pi = detail::half_pi<T>();
                static const T fourth_pi = detail::fourth_pi<T>();

                cap_map<T> capmap;
                T c;
                capmap.x = x;
                capmap.y = y;
                if (inverse == 0) {
                    if (y > fourth_pi) {
                        capmap.region = cap_map<T>::north;
                        c = half_pi;
                    } else if (y < -fourth_pi) {
                        capmap.region = cap_map<T>::south;
                        c = -half_pi;
                    } else {
                        capmap.region = cap_map<T>::equatorial;
                        capmap.cn = 0;
                        return capmap;
                    }
                    /* polar region */
                    if (x < -half_pi) {
                        capmap.cn = 0;
                        capmap.x = (-3.0*fourth_pi);
                        capmap.y = c;
                    } else if (x >= -half_pi && x < 0) {
                        capmap.cn = 1;
                        capmap.x = -fourth_pi;
                        capmap.y = c;
                    } else if (x >= 0 && x < half_pi) {
                        capmap.cn = 2;
                        capmap.x = fourth_pi;
                        capmap.y = c;
                    } else {
                        capmap.cn = 3;
                        capmap.x = 3.0*fourth_pi;
                        capmap.y = c;
                    }
                } else {
                    if (y > fourth_pi) {
                        capmap.region = cap_map<T>::north;
                        capmap.x = (-3.0*fourth_pi + north_square*half_pi);
                        capmap.y = half_pi;
                        x = x - north_square*half_pi;
                    } else if (y < -fourth_pi) {
                        capmap.region = cap_map<T>::south;
                        capmap.x = (-3.0*fourth_pi + south_square*pi/2);
                        capmap.y = -half_pi;
                        x = x - south_square*half_pi;
                    } else {
                        capmap.region = cap_map<T>::equatorial;
                        capmap.cn = 0;
                        return capmap;
                    }
                    /* Polar Region, find the HEALPix polar cap number that
                       x, y moves to when rHEALPix polar square is disassembled. */
                    if (capmap.region == cap_map<T>::north) {
                        if (y >= -x - fourth_pi - epsilon && y < x + 5.0*fourth_pi - epsilon) {
                            capmap.cn = (north_square + 1) % 4;
                        } else if (y > -x -fourth_pi + epsilon && y >= x + 5.0*fourth_pi - epsilon) {
                            capmap.cn = (north_square + 2) % 4;
                        } else if (y <= -x -fourth_pi + epsilon && y > x + 5.0*fourth_pi + epsilon) {
                            capmap.cn = (north_square + 3) % 4;
                        } else {
                            capmap.cn = north_square;
                        }
                    } else if (capmap.region == cap_map<T>::south) {
                        if (y <= x + fourth_pi + epsilon && y > -x - 5.0*fourth_pi + epsilon) {
                            capmap.cn = (south_square + 1) % 4;
                        } else if (y < x + fourth_pi - epsilon && y <= -x - 5.0*fourth_pi + epsilon) {
                            capmap.cn = (south_square + 2) % 4;
                        } else if (y >= x + fourth_pi - epsilon && y < -x - 5.0*fourth_pi - epsilon) {
                            capmap.cn = (south_square + 3) % 4;
                        } else {
                            capmap.cn = south_square;
                        }
                    }
                }
                return capmap;
            }
            /**
             * Rearrange point (x, y) in the HEALPix projection by
             * combining the polar caps into two polar squares.
             * Put the north polar square in position north_square and
             * the south polar square in position south_square.
             * If inverse=1, then uncombine the polar caps.
             * @param north_square integer between 0 and 3.
             * @param south_square integer between 0 and 3.
             **/
            template <typename T>
            inline void combine_caps(T& xy_x, T& xy_y, int north_square, int south_square,
                                     int inverse)
            {
                static const T half_pi = detail::half_pi<T>();
                static const T fourth_pi = detail::fourth_pi<T>();

                T v[2];
                T c[2];
                T vector[2];
                T v_min_c[2];
                T ret_dot[2];
                const double (*tmpRot)[2];
                int pole = 0;

                cap_map<T> capmap = get_cap(xy_x, xy_y, north_square, south_square, inverse);
                if (capmap.region == cap_map<T>::equatorial) {
                    xy_x = capmap.x;
                    xy_y = capmap.y;
                    return;
                }

                v[0] = xy_x; v[1] = xy_y;
                c[0] = capmap.x; c[1] = capmap.y;

                if (inverse == 0) {
                    /* Rotate (xy_x, xy_y) about its polar cap tip and then translate it to
                       north_square or south_square. */

                    if (capmap.region == cap_map<T>::north) {
                        pole = north_square;
                        tmpRot = rot[get_rotate_index(capmap.cn - pole)];
                    } else {
                        pole = south_square;
                        tmpRot = rot[get_rotate_index(-1*(capmap.cn - pole))];
                    }
                } else {
                    /* Inverse function.
                     Unrotate (xy_x, xy_y) and then translate it back. */

                    /* disassemble */
                    if (capmap.region == cap_map<T>::north) {
                        pole = north_square;
                        tmpRot = rot[get_rotate_index(-1*(capmap.cn - pole))];
                    } else {
                        pole = south_square;
                        tmpRot = rot[get_rotate_index(capmap.cn - pole)];
                    }
                }

                vector_sub(v, c, v_min_c);
                dot_product(tmpRot, v_min_c, ret_dot);

                {
                    T a[2];
                    /* Workaround cppcheck git issue */
                    T* pa = a;
                    // TODO: in proj4 5.0.0 this line is used instead
                    //pa[0] = -3.0*fourth_pi + ((inverse == 0) ? 0 : capmap.cn) *half_pi;
                    pa[0] = -3.0*fourth_pi + ((inverse == 0) ? pole : capmap.cn) *half_pi;
                    pa[1] = half_pi;
                    vector_add(ret_dot, a, vector);
                }

                xy_x = vector[0];
                xy_y = vector[1];
            }

            template <typename T, typename Parameters>
            struct base_healpix_ellipsoid
            {
                par_healpix<T> m_proj_parm;

                // FORWARD(e_healpix_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    lp_lat = auth_lat(par, m_proj_parm, lp_lat, 0);
                    return healpix_sphere(lp_lon, lp_lat, xy_x, xy_y);
                }

                // INVERSE(e_healpix_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    /* Check whether (x, y) lies in the HEALPix image. */
                    if (in_image(xy_x, xy_y, 0, 0, 0) == 0) {
                        lp_lon = HUGE_VAL;
                        lp_lat = HUGE_VAL;
                        BOOST_THROW_EXCEPTION( projection_exception(error_invalid_x_or_y) );
                    }
                    healpix_sphere_inverse(xy_x, xy_y, lp_lon, lp_lat);
                    lp_lat = auth_lat(par, m_proj_parm, lp_lat, 1);
                }

                static inline std::string get_name()
                {
                    return "healpix_ellipsoid";
                }

            };

            template <typename T, typename Parameters>
            struct base_healpix_spheroid
            {
                par_healpix<T> m_proj_parm;

                // FORWARD(s_healpix_forward)  sphere
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    return healpix_sphere(lp_lon, lp_lat, xy_x, xy_y);
                }

                // INVERSE(s_healpix_inverse)  sphere
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    /* Check whether (x, y) lies in the HEALPix image */
                    if (in_image(xy_x, xy_y, 0, 0, 0) == 0) {
                        lp_lon = HUGE_VAL;
                        lp_lat = HUGE_VAL;
                        BOOST_THROW_EXCEPTION( projection_exception(error_invalid_x_or_y) );
                    }
                    return healpix_sphere_inverse(xy_x, xy_y, lp_lon, lp_lat);
                }

                static inline std::string get_name()
                {
                    return "healpix_spheroid";
                }

            };

            template <typename T, typename Parameters>
            struct base_rhealpix_ellipsoid
            {
                par_healpix<T> m_proj_parm;

                // FORWARD(e_rhealpix_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    lp_lat = auth_lat(par, m_proj_parm, lp_lat, 0);
                    healpix_sphere(lp_lon, lp_lat, xy_x, xy_y);
                    combine_caps(xy_x, xy_y, this->m_proj_parm.north_square, this->m_proj_parm.south_square, 0);
                }

                // INVERSE(e_rhealpix_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    /* Check whether (x, y) lies in the rHEALPix image. */
                    if (in_image(xy_x, xy_y, 1, this->m_proj_parm.north_square, this->m_proj_parm.south_square) == 0) {
                        lp_lon = HUGE_VAL;
                        lp_lat = HUGE_VAL;
                        BOOST_THROW_EXCEPTION( projection_exception(error_invalid_x_or_y) );
                    }
                    combine_caps(xy_x, xy_y, this->m_proj_parm.north_square, this->m_proj_parm.south_square, 1);
                    healpix_sphere_inverse(xy_x, xy_y, lp_lon, lp_lat);
                    lp_lat = auth_lat(par, m_proj_parm, lp_lat, 1);
                }

                static inline std::string get_name()
                {
                    return "rhealpix_ellipsoid";
                }

            };

            template <typename T, typename Parameters>
            struct base_rhealpix_spheroid
            {
                par_healpix<T> m_proj_parm;

                // FORWARD(s_rhealpix_forward)  sphere
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    healpix_sphere(lp_lon, lp_lat, xy_x, xy_y);
                    combine_caps(xy_x, xy_y, this->m_proj_parm.north_square, this->m_proj_parm.south_square, 0);
                }

                // INVERSE(s_rhealpix_inverse)  sphere
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    /* Check whether (x, y) lies in the rHEALPix image. */
                    if (in_image(xy_x, xy_y, 1, this->m_proj_parm.north_square, this->m_proj_parm.south_square) == 0) {
                        lp_lon = HUGE_VAL;
                        lp_lat = HUGE_VAL;
                        BOOST_THROW_EXCEPTION( projection_exception(error_invalid_x_or_y) );
                    }
                    combine_caps(xy_x, xy_y, this->m_proj_parm.north_square, this->m_proj_parm.south_square, 1);
                    return healpix_sphere_inverse(xy_x, xy_y, lp_lon, lp_lat);
                }

                static inline std::string get_name()
                {
                    return "rhealpix_spheroid";
                }

            };

            // HEALPix
            template <typename Parameters, typename T>
            inline void setup_healpix(Parameters& par, par_healpix<T>& proj_parm)
            {
                if (par.es != 0.0) {
                    proj_parm.apa = pj_authset<T>(par.es); /* For auth_lat(). */
                    proj_parm.qp = pj_qsfn(1.0, par.e, par.one_es); /* For auth_lat(). */
                    par.a = par.a*sqrt(0.5*proj_parm.qp); /* Set par.a to authalic radius. */
                    pj_calc_ellipsoid_params(par, par.a, par.es); /* Ensure we have a consistent parameter set */
                } else {
                }
            }

            // rHEALPix
            template <typename Params, typename Parameters, typename T>
            inline void setup_rhealpix(Params const& params, Parameters& par, par_healpix<T>& proj_parm)
            {
                proj_parm.north_square = pj_get_param_i<srs::spar::north_square>(params, "north_square", srs::dpar::north_square);
                proj_parm.south_square = pj_get_param_i<srs::spar::south_square>(params, "south_square", srs::dpar::south_square);
                /* Check for valid north_square and south_square inputs. */
                if ((proj_parm.north_square < 0) || (proj_parm.north_square > 3)) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_axis) );
                }
                if ((proj_parm.south_square < 0) || (proj_parm.south_square > 3)) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_axis) );
                }
                if (par.es != 0.0) {
                    proj_parm.apa = pj_authset<T>(par.es); /* For auth_lat(). */
                    proj_parm.qp = pj_qsfn(1.0, par.e, par.one_es); /* For auth_lat(). */
                    par.a = par.a*sqrt(0.5*proj_parm.qp); /* Set par.a to authalic radius. */
                    // TODO: why not the same as in healpix?
                    //pj_calc_ellipsoid_params(par, par.a, par.es);
                    par.ra = 1.0/par.a;
                } else {
                }
            }

    }} // namespace detail::healpix
    #endif // doxygen

    /*!
        \brief HEALPix projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Spheroid
         - Ellipsoid
        \par Example
        \image html ex_healpix.gif
    */
    template <typename T, typename Parameters>
    struct healpix_ellipsoid : public detail::healpix::base_healpix_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline healpix_ellipsoid(Params const& , Parameters & par)
        {
            detail::healpix::setup_healpix(par, this->m_proj_parm);
        }
    };

    /*!
        \brief HEALPix projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Spheroid
         - Ellipsoid
        \par Example
        \image html ex_healpix.gif
    */
    template <typename T, typename Parameters>
    struct healpix_spheroid : public detail::healpix::base_healpix_spheroid<T, Parameters>
    {
        template <typename Params>
        inline healpix_spheroid(Params const& , Parameters & par)
        {
            detail::healpix::setup_healpix(par, this->m_proj_parm);
        }
    };

    /*!
        \brief rHEALPix projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - north_square (integer)
         - south_square (integer)
        \par Example
        \image html ex_rhealpix.gif
    */
    template <typename T, typename Parameters>
    struct rhealpix_ellipsoid : public detail::healpix::base_rhealpix_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline rhealpix_ellipsoid(Params const& params, Parameters & par)
        {
            detail::healpix::setup_rhealpix(params, par, this->m_proj_parm);
        }
    };

    /*!
        \brief rHEALPix projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - north_square (integer)
         - south_square (integer)
        \par Example
        \image html ex_rhealpix.gif
    */
    template <typename T, typename Parameters>
    struct rhealpix_spheroid : public detail::healpix::base_rhealpix_spheroid<T, Parameters>
    {
        template <typename Params>
        inline rhealpix_spheroid(Params const& params, Parameters & par)
        {
            detail::healpix::setup_rhealpix(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI2(srs::spar::proj_healpix, healpix_spheroid, healpix_ellipsoid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI2(srs::spar::proj_rhealpix, rhealpix_spheroid, rhealpix_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI2(healpix_entry, healpix_spheroid, healpix_ellipsoid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI2(rhealpix_entry, rhealpix_spheroid, rhealpix_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(healpix_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(healpix, healpix_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(rhealpix, rhealpix_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_HEALPIX_HPP

