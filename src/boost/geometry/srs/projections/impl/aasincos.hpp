// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2018.
// Modifications copyright (c) 2018, Oracle and/or its affiliates.
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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_AASINCOS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_AASINCOS_HPP


#include <cmath>

#include <boost/geometry/srs/projections/exception.hpp>
#include <boost/geometry/srs/projections/impl/pj_strerrno.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry { namespace projections
{

namespace detail
{

namespace aasincos
{
    template <typename T>
    inline T ONE_TOL() { return 1.00000000000001; }
    //template <typename T>
    //inline T TOL() { return 0.000000001; }
    template <typename T>
    inline T ATOL() { return 1e-50; }
}

template <typename T>
inline T aasin(T const& v)
{
    T av = 0;

    if ((av = geometry::math::abs(v)) >= 1.0)
    {
        if (av > aasincos::ONE_TOL<T>())
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_acos_asin_arg_too_large) );
        }
        return (v < 0.0 ? -geometry::math::half_pi<T>() : geometry::math::half_pi<T>());
    }

    return asin(v);
}

template <typename T>
inline T aacos(T const& v)
{
    T av = 0;

    if ((av = geometry::math::abs(v)) >= 1.0)
    {
        if (av > aasincos::ONE_TOL<T>())
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_acos_asin_arg_too_large) );
        }
        return (v < 0.0 ? geometry::math::pi<T>() : 0.0);
    }

    return acos(v);
}

template <typename T>
inline T asqrt(T const& v)
{
    return ((v <= 0) ? 0 : sqrt(v));
}

template <typename T>
inline T aatan2(T const& n, T const& d)
{
    return ((geometry::math::abs(n) < aasincos::ATOL<T>()
        && geometry::math::abs(d) < aasincos::ATOL<T>()) ? 0.0 : atan2(n, d));
}


} // namespace detail


}}} // namespace boost::geometry::projections


#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_AASINCOS_HPP
