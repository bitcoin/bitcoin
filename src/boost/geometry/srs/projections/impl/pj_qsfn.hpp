// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017.
// Modifications copyright (c) 2017, Oracle and/or its affiliates.
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

#ifndef BOOST_GEOMETRY_PROJECTIONS_PJ_QSFN_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PJ_QSFN_HPP


namespace boost { namespace geometry { namespace projections
{
    
namespace detail {

/* determine small q */
template <typename T>
inline T pj_qsfn(T const& sinphi, T const& e, T const& one_es)
{
    static const T EPSILON = 1.0e-7;

    if (e >= EPSILON)
    {
        T con = e * sinphi;
        return (one_es * (sinphi / (1. - con * con) -
           (.5 / e) * log ((1. - con) / (1. + con))));
    } else
        return (sinphi + sinphi);
}


static const int MAX_C = 9;

template <typename T>
struct AUTHALIC
{
    T C[MAX_C], CP[MAX_C], CQ[MAX_C];
};

/**
 * @brief determine authalic latitude
 * @param[in] phi geographic latitude
 * @param[in] a initialized structure pointer
 * @return authalic latitude
 */
template <typename T>
inline T proj_qsfn(T const& phi, AUTHALIC<T> const& a)
{
    T s, s2, sum;
    int i = MAX_C;

    s = sin(phi);
    s2 = s * s;
    sum = a.CQ[MAX_C - 1];
    while (--i) sum = a.CQ[i] + s2 * sum;
    return(s * sum);
}

} // namespace detail

}}} // namespace boost::geometry::projections

#endif
