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

#ifndef BOOST_MATH_ELLINT_D_HPP
#define BOOST_MATH_ELLINT_D_HPP

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
typename tools::promote_args<T1, T2>::type ellint_d(T1 k, T2 phi, const Policy& pol);
   
namespace detail{

template <typename T, typename Policy>
T ellint_d_imp(T k, const Policy& pol);

// Elliptic integral (Legendre form) of the second kind
template <typename T, typename Policy>
T ellint_d_imp(T phi, T k, const Policy& pol)
{
    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;

    bool invert = false;
    if(phi < 0)
    {
       phi = fabs(phi);
       invert = true;
    }

    T result;

    if(phi >= tools::max_value<T>())
    {
       // Need to handle infinity as a special case:
       result = policies::raise_overflow_error<T>("boost::math::ellint_d<%1%>(%1%,%1%)", 0, pol);
    }
    else if(phi > 1 / tools::epsilon<T>())
    {
       // Phi is so large that phi%pi is necessarily zero (or garbage),
       // just return the second part of the duplication formula:
       result = 2 * phi * ellint_d_imp(k, pol) / constants::pi<T>();
    }
    else
    {
       // Carlson's algorithm works only for |phi| <= pi/2,
       // use the integrand's periodicity to normalize phi
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
       BOOST_MATH_INSTRUMENT_VARIABLE(rphi);
       BOOST_MATH_INSTRUMENT_VARIABLE(m);
       T sinp = sin(rphi);
       T cosp = cos(rphi);
       BOOST_MATH_INSTRUMENT_VARIABLE(sinp);
       BOOST_MATH_INSTRUMENT_VARIABLE(cosp);
       T c = 1 / (sinp * sinp);
       T cm1 = cosp * cosp / (sinp * sinp);  // c - 1
       T k2 = k * k;
       if(k2 * sinp * sinp > 1)
       {
          return policies::raise_domain_error<T>("boost::math::ellint_d<%1%>(%1%, %1%)", "The parameter k is out of range, got k = %1%", k, pol);
       }
       else if(rphi == 0)
       {
          result = 0;
       }
       else
       {
          // http://dlmf.nist.gov/19.25#E10
          result = s * ellint_rd_imp(cm1, T(c - k2), c, pol) / 3;
          BOOST_MATH_INSTRUMENT_VARIABLE(result);
       }
       if(m != 0)
          result += m * ellint_d_imp(k, pol);
    }
    return invert ? T(-result) : result;
}

// Complete elliptic integral (Legendre form) of the second kind
template <typename T, typename Policy>
T ellint_d_imp(T k, const Policy& pol)
{
    BOOST_MATH_STD_USING
    using namespace boost::math::tools;

    if (abs(k) >= 1)
    {
       return policies::raise_domain_error<T>("boost::math::ellint_d<%1%>(%1%)",
            "Got k = %1%, function requires |k| <= 1", k, pol);
    }
    if(fabs(k) <= tools::root_epsilon<T>())
       return constants::pi<T>() / 4;

    T x = 0;
    T t = k * k;
    T y = 1 - t;
    T z = 1;
    T value = ellint_rd_imp(x, y, z, pol) / 3;

    return value;
}

template <typename T, typename Policy>
inline typename tools::promote_args<T>::type ellint_d(T k, const Policy& pol, const std::true_type&)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::ellint_d_imp(static_cast<value_type>(k), pol), "boost::math::ellint_d<%1%>(%1%)");
}

// Elliptic integral (Legendre form) of the second kind
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type ellint_d(T1 k, T2 phi, const std::false_type&)
{
   return boost::math::ellint_d(k, phi, policies::policy<>());
}

} // detail

// Complete elliptic integral (Legendre form) of the second kind
template <typename T>
inline typename tools::promote_args<T>::type ellint_d(T k)
{
   return ellint_d(k, policies::policy<>());
}

// Elliptic integral (Legendre form) of the second kind
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type ellint_d(T1 k, T2 phi)
{
   typedef typename policies::is_policy<T2>::type tag_type;
   return detail::ellint_d(k, phi, tag_type());
}

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type ellint_d(T1 k, T2 phi, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::ellint_d_imp(static_cast<value_type>(phi), static_cast<value_type>(k), pol), "boost::math::ellint_2<%1%>(%1%,%1%)");
}

}} // namespaces

#endif // BOOST_MATH_ELLINT_D_HPP

