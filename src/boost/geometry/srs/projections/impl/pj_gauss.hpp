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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_GAUSS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_GAUSS_HPP


#include <boost/geometry/srs/projections/constants.hpp>
#include <boost/geometry/srs/projections/exception.hpp>


namespace boost { namespace geometry { namespace projections {

namespace detail {


template <typename T>
struct gauss
{
    T C;
    T K;
    T e;
    T ratexp;
};

template <typename T>
inline T srat(T const& esinp, T const& exp)
{
    return (math::pow((T(1) - esinp) / (T(1) + esinp), exp));
}

template <typename T>
inline gauss<T> gauss_ini(T const& e, T const& phi0, T& chi, T& rc)
{
    static const T fourth_pi = detail::fourth_pi<T>();

    using std::asin;
    using std::cos;
    using std::sin;
    using std::sqrt;
    using std::tan;

    T sphi = 0;
    T cphi = 0;
    T es = 0;

    gauss<T> en;
    es = e * e;
    en.e = e;
    sphi = sin(phi0);
    cphi = cos(phi0);
    cphi *= cphi;

    rc = sqrt(1.0 - es) / (1.0 - es * sphi * sphi);
    en.C = sqrt(1.0 + es * cphi * cphi / (1.0 - es));
    chi = asin(sphi / en.C);
    en.ratexp = 0.5 * en.C * e;
    en.K = tan(0.5 * chi + fourth_pi)
           / (math::pow(tan(T(0.5) * phi0 + fourth_pi), en.C) * srat(en.e * sphi, en.ratexp));

    return en;
}

template <typename T>
inline void gauss_fwd(gauss<T> const& en, T& lam, T& phi)
{
    static const T fourth_pi = detail::fourth_pi<T>();
    static const T half_pi = detail::half_pi<T>();

    phi = T(2) * atan(en.K * math::pow(tan(T(0.5) * phi + fourth_pi), en.C)
          * srat(en.e * sin(phi), en.ratexp) ) - half_pi;

    lam *= en.C;
}

template <typename T>
inline void gauss_inv(gauss<T> const& en, T& lam, T& phi)
{
    static const int max_iter = 20;
    static const T fourth_pi = detail::fourth_pi<T>();
    static const T half_pi = detail::half_pi<T>();
    static const T del_tol = 1e-14;

    lam /= en.C;
    const T num = math::pow(tan(T(0.5) * phi + fourth_pi) / en.K, T(1) / en.C);

    int i = 0;
    for (i = max_iter; i; --i)
    {
        const T elp_phi = 2.0 * atan(num * srat(en.e * sin(phi), - 0.5 * en.e)) - half_pi;

        if (geometry::math::abs(elp_phi - phi) < del_tol)
        {
            break;
        }
        phi = elp_phi;
    }

    /* convergence failed */
    if (!i)
    {
        BOOST_THROW_EXCEPTION( projection_exception(error_non_conv_inv_meri_dist) );
    }
}

} // namespace detail

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_GAUSS_HPP
