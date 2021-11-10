//  Copyright (c) 2006 Xiaogang Zhang
//  Copyright (c) 2006 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  History:
//  XZ wrote the original of this file as part of the Google
//  Summer of Code 2006.  JM modified it to fit into the
//  Boost.Math conceptual framework better, and to ensure
//  that the code continues to work no matter how many digits
//  type T has.

#ifndef BOOST_MATH_ELLINT_2_HPP
#define BOOST_MATH_ELLINT_2_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/ellint_rf.hpp>
#include <boost/math/special_functions/ellint_rd.hpp>
#include <boost/math/special_functions/ellint_rg.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/tools/workaround.hpp>
#include <boost/math/special_functions/round.hpp>

// Elliptic integrals (complete and incomplete) of the second kind
// Carlson, Numerische Mathematik, vol 33, 1 (1979)

namespace boost { namespace math { 
   
template <class T1, class T2, class Policy>
typename tools::promote_args<T1, T2>::type ellint_2(T1 k, T2 phi, const Policy& pol);
   
namespace detail{

template <typename T, typename Policy>
T ellint_e_imp(T k, const Policy& pol);

// Elliptic integral (Legendre form) of the second kind
template <typename T, typename Policy>
T ellint_e_imp(T phi, T k, const Policy& pol)
{
    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;

    bool invert = false;
    if (phi == 0)
       return 0;

    if(phi < 0)
    {
       phi = fabs(phi);
       invert = true;
    }

    T result;

    if(phi >= tools::max_value<T>())
    {
       // Need to handle infinity as a special case:
       result = policies::raise_overflow_error<T>("boost::math::ellint_e<%1%>(%1%,%1%)", 0, pol);
    }
    else if(phi > 1 / tools::epsilon<T>())
    {
       // Phi is so large that phi%pi is necessarily zero (or garbage),
       // just return the second part of the duplication formula:
       result = 2 * phi * ellint_e_imp(k, pol) / constants::pi<T>();
    }
    else if(k == 0)
    {
       return invert ? T(-phi) : phi;
    }
    else if(fabs(k) == 1)
    {
       //
       // For k = 1 ellipse actually turns to a line and every pi/2 in phi is exactly 1 in arc length
       // Periodicity though is in pi, curve follows sin(pi) for 0 <= phi <= pi/2 and then
       // 2 - sin(pi- phi) = 2 + sin(phi - pi) for pi/2 <= phi <= pi, so general form is:
       //
       // 2n + sin(phi - n * pi) ; |phi - n * pi| <= pi / 2
       //
       T m = boost::math::round(phi / boost::math::constants::pi<T>());
       T remains = phi - m * boost::math::constants::pi<T>();
       T value = 2 * m + sin(remains);

       // negative arc length for negative phi
       return invert ? -value : value;
    }
    else
    {
       // Carlson's algorithm works only for |phi| <= pi/2,
       // use the integrand's periodicity to normalize phi
       //
       // Xiaogang's original code used a cast to long long here
       // but that fails if T has more digits than a long long,
       // so rewritten to use fmod instead:
       //
       T rphi = boost::math::tools::fmod_workaround(phi, T(constants::half_pi<T>()));
       T m = boost::math::round((phi - rphi) / constants::half_pi<T>());
       int s = 1;
       if(boost::math::tools::fmod_workaround(m, T(2)) > 0.5)
       {
          m += 1;
          s = -1;
          rphi = constants::half_pi<T>() - rphi;
       }
       T k2 = k * k;
       if(boost::math::pow<3>(rphi) * k2 / 6 < tools::epsilon<T>() * fabs(rphi))
       {
          // See http://functions.wolfram.com/EllipticIntegrals/EllipticE2/06/01/03/0001/
          result = s * rphi;
       }
       else
       {
          // http://dlmf.nist.gov/19.25#E10
          T sinp = sin(rphi);
          if (k2 * sinp * sinp >= 1)
          {
             return policies::raise_domain_error<T>("boost::math::ellint_2<%1%>(%1%, %1%)", "The parameter k is out of range, got k = %1%", k, pol);
          }
          T cosp = cos(rphi);
          T c = 1 / (sinp * sinp);
          T cm1 = cosp * cosp / (sinp * sinp);  // c - 1
          result = s * ((1 - k2) * ellint_rf_imp(cm1, T(c - k2), c, pol) + k2 * (1 - k2) * ellint_rd(cm1, c, T(c - k2), pol) / 3 + k2 * sqrt(cm1 / (c * (c - k2))));
       }
       if(m != 0)
          result += m * ellint_e_imp(k, pol);
    }
    return invert ? T(-result) : result;
}

// Complete elliptic integral (Legendre form) of the second kind
template <typename T, typename Policy>
T ellint_e_imp(T k, const Policy& pol)
{
    BOOST_MATH_STD_USING
    using namespace boost::math::tools;

    if (abs(k) > 1)
    {
       return policies::raise_domain_error<T>("boost::math::ellint_e<%1%>(%1%)",
            "Got k = %1%, function requires |k| <= 1", k, pol);
    }
    if (abs(k) == 1)
    {
        return static_cast<T>(1);
    }

    T x = 0;
    T t = k * k;
    T y = 1 - t;
    T z = 1;
    T value = 2 * ellint_rg_imp(x, y, z, pol);

    return value;
}

template <typename T, typename Policy>
inline typename tools::promote_args<T>::type ellint_2(T k, const Policy& pol, const std::true_type&)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::ellint_e_imp(static_cast<value_type>(k), pol), "boost::math::ellint_2<%1%>(%1%)");
}

// Elliptic integral (Legendre form) of the second kind
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type ellint_2(T1 k, T2 phi, const std::false_type&)
{
   return boost::math::ellint_2(k, phi, policies::policy<>());
}

} // detail

// Complete elliptic integral (Legendre form) of the second kind
template <typename T>
inline typename tools::promote_args<T>::type ellint_2(T k)
{
   return ellint_2(k, policies::policy<>());
}

// Elliptic integral (Legendre form) of the second kind
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type ellint_2(T1 k, T2 phi)
{
   typedef typename policies::is_policy<T2>::type tag_type;
   return detail::ellint_2(k, phi, tag_type());
}

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type ellint_2(T1 k, T2 phi, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::ellint_e_imp(static_cast<value_type>(phi), static_cast<value_type>(k), pol), "boost::math::ellint_2<%1%>(%1%,%1%)");
}

}} // namespaces

#endif // BOOST_MATH_ELLINT_2_HPP

