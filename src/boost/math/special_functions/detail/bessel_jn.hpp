//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_JN_HPP
#define BOOST_MATH_BESSEL_JN_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/detail/bessel_j0.hpp>
#include <boost/math/special_functions/detail/bessel_j1.hpp>
#include <boost/math/special_functions/detail/bessel_jy.hpp>
#include <boost/math/special_functions/detail/bessel_jy_asym.hpp>
#include <boost/math/special_functions/detail/bessel_jy_series.hpp>

// Bessel function of the first kind of integer order
// J_n(z) is the minimal solution
// n < abs(z), forward recurrence stable and usable
// n >= abs(z), forward recurrence unstable, use Miller's algorithm

namespace boost { namespace math { namespace detail{

template <typename T, typename Policy>
T bessel_jn(int n, T x, const Policy& pol)
{
    T value(0), factor, current, prev, next;

    BOOST_MATH_STD_USING

    //
    // Reflection has to come first:
    //
    if (n < 0)
    {
        factor = static_cast<T>((n & 0x1) ? -1 : 1);  // J_{-n}(z) = (-1)^n J_n(z)
        n = -n;
    }
    else
    {
        factor = 1;
    }
    if(x < 0)
    {
        factor *= (n & 0x1) ? -1 : 1;  // J_{n}(-z) = (-1)^n J_n(z)
        x = -x;
    }
    //
    // Special cases:
    //
    if(asymptotic_bessel_large_x_limit(T(n), x))
       return factor * asymptotic_bessel_j_large_x_2<T>(T(n), x, pol);
    if (n == 0)
    {
        return factor * bessel_j0(x);
    }
    if (n == 1)
    {
        return factor * bessel_j1(x);
    }

    if (x == 0)                             // n >= 2
    {
        return static_cast<T>(0);
    }

    BOOST_MATH_ASSERT(n > 1);
    T scale = 1;
    if (n < abs(x))                         // forward recurrence
    {
        prev = bessel_j0(x);
        current = bessel_j1(x);
        policies::check_series_iterations<T>("boost::math::bessel_j_n<%1%>(%1%,%1%)", n, pol);
        for (int k = 1; k < n; k++)
        {
            T fact = 2 * k / x;
            //
            // rescale if we would overflow or underflow:
            //
            if((fabs(fact) > 1) && ((tools::max_value<T>() - fabs(prev)) / fabs(fact) < fabs(current)))
            {
               scale /= current;
               prev /= current;
               current = 1;
            }
            value = fact * current - prev;
            prev = current;
            current = value;
        }
    }
    else if((x < 1) || (n > x * x / 4) || (x < 5))
    {
       return factor * bessel_j_small_z_series(T(n), x, pol);
    }
    else                                    // backward recurrence
    {
        T fn; int s;                        // fn = J_(n+1) / J_n
        // |x| <= n, fast convergence for continued fraction CF1
        boost::math::detail::CF1_jy(static_cast<T>(n), x, &fn, &s, pol);
        prev = fn;
        current = 1;
        // Check recursion won't go on too far:
        policies::check_series_iterations<T>("boost::math::bessel_j_n<%1%>(%1%,%1%)", n, pol);
        for (int k = n; k > 0; k--)
        {
            T fact = 2 * k / x;
            if((fabs(fact) > 1) && ((tools::max_value<T>() - fabs(prev)) / fabs(fact) < fabs(current)))
            {
               prev /= current;
               scale /= current;
               current = 1;
            }
            next = fact * current - prev;
            prev = current;
            current = next;
        }
        value = bessel_j0(x) / current;       // normalization
        scale = 1 / scale;
    }
    value *= factor;

    if(tools::max_value<T>() * scale < fabs(value))
       return policies::raise_overflow_error<T>("boost::math::bessel_jn<%1%>(%1%,%1%)", 0, pol);

    return value / scale;
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_JN_HPP

