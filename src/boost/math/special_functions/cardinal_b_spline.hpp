//  (C) Copyright Nick Thompson 2019.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_CARDINAL_B_SPLINE_HPP
#define BOOST_MATH_SPECIAL_CARDINAL_B_SPLINE_HPP

#include <array>
#include <cmath>
#include <limits>
#include <type_traits>

namespace boost { namespace math {

namespace detail {

  template<class Real>
  inline Real B1(Real x)
  {
    if (x < 0)
    {
      return B1(-x);
    }
    if (x < Real(1))
    {
      return 1 - x;
    }
    return Real(0);
  }
}

template<unsigned n, typename Real>
Real cardinal_b_spline(Real x) {
    static_assert(!std::is_integral<Real>::value, "Does not work with integral types.");

    if (x < 0) {
        // All B-splines are even functions:
        return cardinal_b_spline<n, Real>(-x);
    }

    if  (n==0)
    {
        if (x < Real(1)/Real(2)) {
            return Real(1);
        }
        else if (x == Real(1)/Real(2)) {
            return Real(1)/Real(2);
        }
        else {
            return Real(0);
        }
    }

    if (n==1)
    {
        return detail::B1(x);
    }

    Real supp_max = (n+1)/Real(2);
    if (x >= supp_max)
    {
        return Real(0);
    }

    // Fill v with values of B1:
    // At most two of these terms are nonzero, and at least 1.
    // There is only one non-zero term when n is odd and x = 0.
    std::array<Real, n> v;
    Real z = x + 1 - supp_max;
    for (unsigned i = 0; i < n; ++i)
    {
        v[i] = detail::B1(z);
        z += 1;
    }

    Real smx = supp_max - x;
    for (unsigned j = 2; j <= n; ++j)
    {
        Real a = (j + 1 - smx);
        Real b = smx;
        for(unsigned k = 0; k <= n - j; ++k)
        {
            v[k] = (a*v[k+1] + b*v[k])/Real(j);
            a += 1;
            b -= 1;
        }
    }

    return v[0];
}


template<unsigned n, typename Real>
Real cardinal_b_spline_prime(Real x)
{
    static_assert(!std::is_integral<Real>::value, "Cardinal B-splines do not work with integer types.");

    if (x < 0)
    {
        // All B-splines are even functions, so derivatives are odd:
        return -cardinal_b_spline_prime<n, Real>(-x);
    }


    if (n==0)
    {
        // Kinda crazy but you get what you ask for!
        if (x == Real(1)/Real(2))
        {
            return std::numeric_limits<Real>::infinity();
        }
        else
        {
            return Real(0);
        }
    }

    if (n==1)
    {
        if (x==0)
        {
            return Real(0);
        }
        if (x==1)
        {
            return -Real(1)/Real(2);
        }
        return Real(-1);
    }


    Real supp_max = (n+1)/Real(2);
    if (x >= supp_max)
    {
        return Real(0);
    }

    // Now we want to evaluate B_{n}(x), but stop at the second to last step and collect B_{n-1}(x+1/2) and B_{n-1}(x-1/2):
    std::array<Real, n> v;
    Real z = x + 1 - supp_max;
    for (unsigned i = 0; i < n; ++i)
    {
        v[i] = detail::B1(z);
        z += 1;
    }

    Real smx = supp_max - x;
    for (unsigned j = 2; j <= n - 1; ++j)
    {
        Real a = (j + 1 - smx);
        Real b = smx;
        for(unsigned k = 0; k <= n - j; ++k)
        {
            v[k] = (a*v[k+1] + b*v[k])/Real(j);
            a += 1;
            b -= 1;
        }
    }

    return v[1] - v[0];
}


template<unsigned n, typename Real>
Real cardinal_b_spline_double_prime(Real x)
{
    static_assert(!std::is_integral<Real>::value, "Cardinal B-splines do not work with integer types.");
    static_assert(n >= 3, "n>=3 for second derivatives of cardinal B-splines is required.");

    if (x < 0)
    {
        // All B-splines are even functions, so second derivatives are even:
        return cardinal_b_spline_double_prime<n, Real>(-x);
    }


    Real supp_max = (n+1)/Real(2);
    if (x >= supp_max)
    {
        return Real(0);
    }

    // Now we want to evaluate B_{n}(x), but stop at the second to last step and collect B_{n-1}(x+1/2) and B_{n-1}(x-1/2):
    std::array<Real, n> v;
    Real z = x + 1 - supp_max;
    for (unsigned i = 0; i < n; ++i)
    {
        v[i] = detail::B1(z);
        z += 1;
    }

    Real smx = supp_max - x;
    for (unsigned j = 2; j <= n - 2; ++j)
    {
        Real a = (j + 1 - smx);
        Real b = smx;
        for(unsigned k = 0; k <= n - j; ++k)
        {
            v[k] = (a*v[k+1] + b*v[k])/Real(j);
            a += 1;
            b -= 1;
        }
    }

    return v[2] - 2*v[1] + v[0];
}


template<unsigned n, class Real>
Real forward_cardinal_b_spline(Real x)
{
    static_assert(!std::is_integral<Real>::value, "Cardinal B-splines do not work with integral types.");
    return cardinal_b_spline<n>(x - (n+1)/Real(2));
}

}}
#endif
