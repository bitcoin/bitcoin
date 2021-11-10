// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017, 2018.
// Modifications copyright (c) 2017-2018, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// PROJ4 is converted to Geometry Library by Barend Gehrels (Geodan, Amsterdam)

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

/* meridional distance for ellipsoid and inverse
**	8th degree - accurate to < 1e-5 meters when used in conjunction
**		with typical major axis values.
**	Inverse determines phi to EPS (1e-11) radians, about 1e-6 seconds.
*/

#ifndef BOOST_GEOMETRY_PROJECTIONS_PJ_MLFN_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PJ_MLFN_HPP


#include <cstdlib>

#include <boost/geometry/srs/projections/exception.hpp>
#include <boost/geometry/srs/projections/impl/pj_strerrno.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry { namespace projections {

namespace detail {

template <typename T>
struct en
{
    static const std::size_t size = 5;

    T const& operator[](size_t i) const { return data[i]; }
    T & operator[](size_t i) { return data[i]; }

private:
    T data[5];
};

template <typename T>
inline en<T> pj_enfn(T const& es)
{
    static const T C00 = 1.;
    static const T C02 = .25;
    static const T C04 = .046875;
    static const T C06 = .01953125;
    static const T C08 = .01068115234375;
    static const T C22 = .75;
    static const T C44 = .46875;
    static const T C46 = .01302083333333333333;
    static const T C48 = .00712076822916666666;
    static const T C66 = .36458333333333333333;
    static const T C68 = .00569661458333333333;
    static const T C88 = .3076171875;

    T t;
    detail::en<T> en;

    {
        en[0] = C00 - es * (C02 + es * (C04 + es * (C06 + es * C08)));
        en[1] = es * (C22 - es * (C04 + es * (C06 + es * C08)));
        en[2] = (t = es * es) * (C44 - es * (C46 + es * C48));
        en[3] = (t *= es) * (C66 - es * C68);
        en[4] = t * es * C88;
    }
    
    return en;
}

template <typename T>
inline T pj_mlfn(T const& phi, T sphi, T cphi, detail::en<T> const& en)
{
    cphi *= sphi;
    sphi *= sphi;
    return(en[0] * phi - cphi * (en[1] + sphi*(en[2]
        + sphi*(en[3] + sphi*en[4]))));
}

template <typename T>
inline T pj_inv_mlfn(T const& arg, T const& es, detail::en<T> const& en)
{
    static const T EPS = 1e-11;
    static const int MAX_ITER = 10;

    T s, t, phi, k = 1./(1.-es);
    int i;

    phi = arg;
    for (i = MAX_ITER; i ; --i) { /* rarely goes over 2 iterations */
        s = sin(phi);
        t = 1. - es * s * s;
        phi -= t = (pj_mlfn(phi, s, cos(phi), en) - arg) * (t * sqrt(t)) * k;
        if (geometry::math::abs(t) < EPS)
            return phi;
    }
    BOOST_THROW_EXCEPTION( projection_exception(error_non_conv_inv_meri_dist) );
    return phi;
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif
