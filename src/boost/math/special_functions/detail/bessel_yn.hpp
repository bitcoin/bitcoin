//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_YN_HPP
#define BOOST_MATH_BESSEL_YN_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/detail/bessel_y0.hpp>
#include <boost/math/special_functions/detail/bessel_y1.hpp>
#include <boost/math/special_functions/detail/bessel_jy_series.hpp>
#include <boost/math/policies/error_handling.hpp>

// Bessel function of the second kind of integer order
// Y_n(z) is the dominant solution, forward recurrence always OK (though unstable)

namespace boost { namespace math { namespace detail{

template <typename T, typename Policy>
T bessel_yn(int n, T x, const Policy& pol)
{
    BOOST_MATH_STD_USING
    T value, factor, current, prev;

    using namespace boost::math::tools;

    static const char* function = "boost::math::bessel_yn<%1%>(%1%,%1%)";

    if ((x == 0) && (n == 0))
    {
       return -policies::raise_overflow_error<T>(function, 0, pol);
    }
    if (x <= 0)
    {
       return policies::raise_domain_error<T>(function,
            "Got x = %1%, but x must be > 0, complex result not supported.", x, pol);
    }

    //
    // Reflection comes first:
    //
    if (n < 0)
    {
        factor = static_cast<T>((n & 0x1) ? -1 : 1);  // Y_{-n}(z) = (-1)^n Y_n(z)
        n = -n;
    }
    else
    {
        factor = 1;
    }
    if(x < policies::get_epsilon<T, Policy>())
    {
       T scale = 1;
       value = bessel_yn_small_z(n, x, &scale, pol);
       if(tools::max_value<T>() * fabs(scale) < fabs(value))
          return boost::math::sign(scale) * boost::math::sign(value) * policies::raise_overflow_error<T>(function, 0, pol);
       value /= scale;
    }
    else if(asymptotic_bessel_large_x_limit(n, x))
    {
       value = factor * asymptotic_bessel_y_large_x_2(static_cast<T>(abs(n)), x, pol);
    }
    else if (n == 0)
    {
        value = bessel_y0(x, pol);
    }
    else if (n == 1)
    {
        value = factor * bessel_y1(x, pol);
    }
    else
    {
       prev = bessel_y0(x, pol);
       current = bessel_y1(x, pol);
       int k = 1;
       BOOST_MATH_ASSERT(k < n);
       policies::check_series_iterations<T>("boost::math::bessel_y_n<%1%>(%1%,%1%)", n, pol);
       T mult = 2 * k / x;
       value = mult * current - prev;
       prev = current;
       current = value;
       ++k;
       if((mult > 1) && (fabs(current) > 1))
       {
          prev /= current;
          factor /= current;
          value /= current;
          current = 1;
       }
       while(k < n)
       {
           mult = 2 * k / x;
           value = mult * current - prev;
           prev = current;
           current = value;
           ++k;
       }
       if(fabs(tools::max_value<T>() * factor) < fabs(value))
          return sign(value) * sign(factor) * policies::raise_overflow_error<T>(function, 0, pol);
       value /= factor;
    }
    return value;
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_YN_HPP

