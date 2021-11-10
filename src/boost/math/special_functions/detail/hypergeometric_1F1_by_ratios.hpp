
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HYPERGEOMETRIC_1F1_BY_RATIOS_HPP_
#define BOOST_HYPERGEOMETRIC_1F1_BY_RATIOS_HPP_

#include <boost/math/tools/recurrence.hpp>
#include <boost/math/policies/error_handling.hpp>

  namespace boost { namespace math { namespace detail {

     template <class T, class Policy>
     T hypergeometric_1F1_imp(const T& a, const T& b, const T& z, const Policy& pol, long long& log_scaling);

     /*
      Evaluation by method of ratios for domain b < 0 < a,z

      We first convert the recurrence relation into a ratio
      of M(a+1, b+1, z) / M(a, b, z) using Shintan's equivalence
      between solving a recurrence relation using Miller's method
      and continued fractions.  The continued fraction is VERY rapid
      to converge (typically < 10 terms), but may converge to entirely
      the wrong value if we're in a bad part of the domain.  Strangely
      it seems to matter not whether we use recurrence on a, b or a and b
      they all work or not work about the same, so we might as well make
      life easy for ourselves and use the a and b recurrence to avoid
      having to apply one extra recurrence to convert from an a or b
      recurrence to an a+b one.

      See: H. Shintan, Note on Miller's recurrence algorithm, J. Sci. Hiroshima Univ. Ser. A-I Math., 29 (1965), pp. 121-133.
      Also: Computational Aspects of Three Term Recurrence Relations, SIAM Review, January 1967.

      The following table lists by experiment, how large z can be in order to
      ensure the continued fraction converges to the correct value:

          a         b    max  z
         13,      -130,      22
         13,     -1300,     335
         13,    -13000,    3585
        130,      -130,       8
        130,     -1300,     263
        130,   - 13000,    3420
       1300,      -130,       1
       1300,     -1300,      90
       1300,    -13000,    2650
      13000,       -13,       -
      13000,      -130,       -
      13000,     -1300,      13
      13000,    -13000,     750

      So try z_limit = -b / (4 - 5 * sqrt(log(a)) * a / b);
      or     z_limit = -b / (4 - 5 * (log(a)) * a / b)  for a < 100
      
      This still isn't quite right for both a and b small, but we'll be using a Bessel approximation
      in that region anyway.

      Normalization using wronksian {1,2} from A&S 13.1.20 (also 13.1.12, 13.1.13):

      W{ M(a,b,z), z^(1-b)M(1+a-b, 2-b, z) } = (1-b)z^-b e^z

       = M(a,b,z) M2'(a,b,z) - M'(a,b,z) M2(a,b,z)
       = M(a,b,z) [(a-b+1)z^(1-b)/(2-b) M2(a+1,b+1,z) + (1-b)z^-b M2(a,b,z)] - a/b M(a+1,b+1,z) z^(1-b)M2(a,b,z)
       = M(a,b,z) [(a-b+1)z^(1-b)/(2-b) M2(a+1,b+1,z) + (1-b)z^-b M2(a,b,z)] - a/b R(a,b,z) M(a,b,z) z^(1-b)M2(a,b,z)
       = M(a,b,z) [(a-b+1)z^(1-b)/(2-b) M2(a+1,b+1,z) + (1-b)z^-b M2(a,b,z) - a/b R(a,b,z) z^(1-b)M2(a,b,z) ]
       so:
       (1-b)e^z = M(a,b,z) [(a-b+1)z/(2-b) M2(a+1,b+1,z) + (1-b) M2(a,b,z) - a/b z R(a,b,z) M2(a,b,z) ]

      */

     template <class T>
     inline bool is_in_hypergeometric_1F1_from_function_ratio_negative_b_region(const T& a, const T& b, const T& z)
     {
        BOOST_MATH_STD_USING
        if (a < 100)
           return z < -b / (4 - 5 * (log(a)) * a / b);
        else
           return z < -b / (4 - 5 * sqrt(log(a)) * a / b);
     }

     template <class T, class Policy>
     T hypergeometric_1F1_from_function_ratio_negative_b(const T& a, const T& b, const T& z, const Policy& pol, long long& log_scaling, const T& ratio)
     {
        BOOST_MATH_STD_USING
        //
        // Let M2 = M(1+a-b, 2-b, z)
        // This is going to be a mighty big number:
        //
        long long local_scaling = 0;
        T M2 = boost::math::detail::hypergeometric_1F1_imp(T(1 + a - b), T(2 - b), z, pol, local_scaling);
        log_scaling -= local_scaling; // all the M2 terms are in the denominator
        //
        // Since a, b and z are all likely to be large we need the Wronksian
        // calculation below to not overflow, so scale everything right down:
        //
        if (fabs(M2) > 1)
        {
           long long s = lltrunc(log(fabs(M2)));
           log_scaling -= s;  // M2 will be in the denominator, so subtract the scaling!
           local_scaling += s;
           M2 *= exp(T(-s));
        }
        //
        // Let M3 = M(1+a-b + 1, 2-b + 1, z)
        // we can get to this from the ratio which is cheaper to calculate:
        //
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        boost::math::detail::hypergeometric_1F1_recurrence_a_and_b_coefficients<T> coef2(1 + a - b + 1, 2 - b + 1, z);
        T M3 = boost::math::tools::function_ratio_from_backwards_recurrence(coef2, boost::math::policies::get_epsilon<T, Policy>(), max_iter) * M2;
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1F1_from_function_ratio_negative_b_positive_a<%1%>(%1%,%1%,%1%)", max_iter, pol);
        //
        // Get the RHS of the Wronksian:
        //
        long long fz = lltrunc(z);
        log_scaling += fz;
        T rhs = (1 - b) * exp(z - fz);
        //
        // We need to divide that by:
        // [(a-b+1)z/(2-b) M2(a+1,b+1,z) + (1-b) M2(a,b,z) - a/b z^b R(a,b,z) M2(a,b,z) ]
        // Note that at this stage, both M3 and M2 are scaled by exp(local_scaling).
        //
        T lhs = (a - b + 1) * z * M3 / (2 - b) + (1 - b) * M2 - a * z * ratio * M2 / b;

        return rhs / lhs;
     }

     template <class T, class Policy>
     T hypergeometric_1F1_from_function_ratio_negative_b(const T& a, const T& b, const T& z, const Policy& pol, long long& log_scaling)
     {
        BOOST_MATH_STD_USING
        //
        // Get the function ratio, M(a+1, b+1, z)/M(a,b,z):
        //
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        boost::math::detail::hypergeometric_1F1_recurrence_a_and_b_coefficients<T> coef(a + 1, b + 1, z);
        T ratio = boost::math::tools::function_ratio_from_backwards_recurrence(coef, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1F1_from_function_ratio_negative_b_positive_a<%1%>(%1%,%1%,%1%)", max_iter, pol);
        return hypergeometric_1F1_from_function_ratio_negative_b(a, b, z, pol, log_scaling, ratio);
     }
     //
     // And over again, this time via forwards recurrence when z is large enough:
     //
     template <class T>
     bool hypergeometric_1F1_is_in_forwards_recurence_for_negative_b_region(const T& a, const T& b, const T& z)
     {
        //
        // There's no easy relation between a, b and z that tells us whether we're in the region
        // where forwards recursion is stable, so use a lookup table, note that the minimum
        // permissible z-value is decreasing with a, and increasing with |b|:
        //
        static const float data[][3] = {
           {7.500e+00f, -7.500e+00f, 8.409e+00f },
           {7.500e+00f, -1.125e+01f, 8.409e+00f },
           {7.500e+00f, -1.688e+01f, 9.250e+00f },
           {7.500e+00f, -2.531e+01f, 1.119e+01f },
           {7.500e+00f, -3.797e+01f, 1.354e+01f },
           {7.500e+00f, -5.695e+01f, 1.983e+01f },
           {7.500e+00f, -8.543e+01f, 2.639e+01f },
           {7.500e+00f, -1.281e+02f, 3.864e+01f },
           {7.500e+00f, -1.922e+02f, 5.657e+01f },
           {7.500e+00f, -2.883e+02f, 8.283e+01f },
           {7.500e+00f, -4.325e+02f, 1.213e+02f },
           {7.500e+00f, -6.487e+02f, 1.953e+02f },
           {7.500e+00f, -9.731e+02f, 2.860e+02f },
           {7.500e+00f, -1.460e+03f, 4.187e+02f },
           {7.500e+00f, -2.189e+03f, 6.130e+02f },
           {7.500e+00f, -3.284e+03f, 9.872e+02f },
           {7.500e+00f, -4.926e+03f, 1.445e+03f },
           {7.500e+00f, -7.389e+03f, 2.116e+03f },
           {7.500e+00f, -1.108e+04f, 3.098e+03f },
           {7.500e+00f, -1.663e+04f, 4.990e+03f },
           {1.125e+01f, -7.500e+00f, 6.318e+00f },
           {1.125e+01f, -1.125e+01f, 6.950e+00f },
           {1.125e+01f, -1.688e+01f, 7.645e+00f },
           {1.125e+01f, -2.531e+01f, 9.250e+00f },
           {1.125e+01f, -3.797e+01f, 1.231e+01f },
           {1.125e+01f, -5.695e+01f, 1.639e+01f },
           {1.125e+01f, -8.543e+01f, 2.399e+01f },
           {1.125e+01f, -1.281e+02f, 3.513e+01f },
           {1.125e+01f, -1.922e+02f, 5.657e+01f },
           {1.125e+01f, -2.883e+02f, 8.283e+01f },
           {1.125e+01f, -4.325e+02f, 1.213e+02f },
           {1.125e+01f, -6.487e+02f, 1.776e+02f },
           {1.125e+01f, -9.731e+02f, 2.860e+02f },
           {1.125e+01f, -1.460e+03f, 4.187e+02f },
           {1.125e+01f, -2.189e+03f, 6.130e+02f },
           {1.125e+01f, -3.284e+03f, 9.872e+02f },
           {1.125e+01f, -4.926e+03f, 1.445e+03f },
           {1.125e+01f, -7.389e+03f, 2.116e+03f },
           {1.125e+01f, -1.108e+04f, 3.098e+03f },
           {1.125e+01f, -1.663e+04f, 4.990e+03f },
           {1.688e+01f, -7.500e+00f, 4.747e+00f },
           {1.688e+01f, -1.125e+01f, 5.222e+00f },
           {1.688e+01f, -1.688e+01f, 5.744e+00f },
           {1.688e+01f, -2.531e+01f, 7.645e+00f },
           {1.688e+01f, -3.797e+01f, 1.018e+01f },
           {1.688e+01f, -5.695e+01f, 1.490e+01f },
           {1.688e+01f, -8.543e+01f, 2.181e+01f },
           {1.688e+01f, -1.281e+02f, 3.193e+01f },
           {1.688e+01f, -1.922e+02f, 5.143e+01f },
           {1.688e+01f, -2.883e+02f, 7.530e+01f },
           {1.688e+01f, -4.325e+02f, 1.213e+02f },
           {1.688e+01f, -6.487e+02f, 1.776e+02f },
           {1.688e+01f, -9.731e+02f, 2.600e+02f },
           {1.688e+01f, -1.460e+03f, 4.187e+02f },
           {1.688e+01f, -2.189e+03f, 6.130e+02f },
           {1.688e+01f, -3.284e+03f, 9.872e+02f },
           {1.688e+01f, -4.926e+03f, 1.445e+03f },
           {1.688e+01f, -7.389e+03f, 2.116e+03f },
           {1.688e+01f, -1.108e+04f, 3.098e+03f },
           {1.688e+01f, -1.663e+04f, 4.990e+03f },
           {2.531e+01f, -7.500e+00f, 3.242e+00f },
           {2.531e+01f, -1.125e+01f, 3.566e+00f },
           {2.531e+01f, -1.688e+01f, 4.315e+00f },
           {2.531e+01f, -2.531e+01f, 5.744e+00f },
           {2.531e+01f, -3.797e+01f, 7.645e+00f },
           {2.531e+01f, -5.695e+01f, 1.231e+01f },
           {2.531e+01f, -8.543e+01f, 1.803e+01f },
           {2.531e+01f, -1.281e+02f, 2.903e+01f },
           {2.531e+01f, -1.922e+02f, 4.676e+01f },
           {2.531e+01f, -2.883e+02f, 6.845e+01f },
           {2.531e+01f, -4.325e+02f, 1.102e+02f },
           {2.531e+01f, -6.487e+02f, 1.776e+02f },
           {2.531e+01f, -9.731e+02f, 2.600e+02f },
           {2.531e+01f, -1.460e+03f, 4.187e+02f },
           {2.531e+01f, -2.189e+03f, 6.130e+02f },
           {2.531e+01f, -3.284e+03f, 8.974e+02f },
           {2.531e+01f, -4.926e+03f, 1.445e+03f },
           {2.531e+01f, -7.389e+03f, 2.116e+03f },
           {2.531e+01f, -1.108e+04f, 3.098e+03f },
           {2.531e+01f, -1.663e+04f, 4.990e+03f },
           {3.797e+01f, -7.500e+00f, 2.214e+00f },
           {3.797e+01f, -1.125e+01f, 2.679e+00f },
           {3.797e+01f, -1.688e+01f, 3.242e+00f },
           {3.797e+01f, -2.531e+01f, 4.315e+00f },
           {3.797e+01f, -3.797e+01f, 6.318e+00f },
           {3.797e+01f, -5.695e+01f, 9.250e+00f },
           {3.797e+01f, -8.543e+01f, 1.490e+01f },
           {3.797e+01f, -1.281e+02f, 2.399e+01f },
           {3.797e+01f, -1.922e+02f, 3.864e+01f },
           {3.797e+01f, -2.883e+02f, 6.223e+01f },
           {3.797e+01f, -4.325e+02f, 1.002e+02f },
           {3.797e+01f, -6.487e+02f, 1.614e+02f },
           {3.797e+01f, -9.731e+02f, 2.600e+02f },
           {3.797e+01f, -1.460e+03f, 3.806e+02f },
           {3.797e+01f, -2.189e+03f, 6.130e+02f },
           {3.797e+01f, -3.284e+03f, 8.974e+02f },
           {3.797e+01f, -4.926e+03f, 1.445e+03f },
           {3.797e+01f, -7.389e+03f, 2.116e+03f },
           {3.797e+01f, -1.108e+04f, 3.098e+03f },
           {3.797e+01f, -1.663e+04f, 4.990e+03f },
           {5.695e+01f, -7.500e+00f, 1.513e+00f },
           {5.695e+01f, -1.125e+01f, 1.830e+00f },
           {5.695e+01f, -1.688e+01f, 2.214e+00f },
           {5.695e+01f, -2.531e+01f, 3.242e+00f },
           {5.695e+01f, -3.797e+01f, 4.315e+00f },
           {5.695e+01f, -5.695e+01f, 7.645e+00f },
           {5.695e+01f, -8.543e+01f, 1.231e+01f },
           {5.695e+01f, -1.281e+02f, 1.983e+01f },
           {5.695e+01f, -1.922e+02f, 3.513e+01f },
           {5.695e+01f, -2.883e+02f, 5.657e+01f },
           {5.695e+01f, -4.325e+02f, 9.111e+01f },
           {5.695e+01f, -6.487e+02f, 1.467e+02f },
           {5.695e+01f, -9.731e+02f, 2.363e+02f },
           {5.695e+01f, -1.460e+03f, 3.806e+02f },
           {5.695e+01f, -2.189e+03f, 5.572e+02f },
           {5.695e+01f, -3.284e+03f, 8.974e+02f },
           {5.695e+01f, -4.926e+03f, 1.314e+03f },
           {5.695e+01f, -7.389e+03f, 2.116e+03f },
           {5.695e+01f, -1.108e+04f, 3.098e+03f },
           {5.695e+01f, -1.663e+04f, 4.990e+03f },
           {8.543e+01f, -7.500e+00f, 1.250e+00f },
           {8.543e+01f, -1.125e+01f, 1.250e+00f },
           {8.543e+01f, -1.688e+01f, 1.513e+00f },
           {8.543e+01f, -2.531e+01f, 2.214e+00f },
           {8.543e+01f, -3.797e+01f, 3.242e+00f },
           {8.543e+01f, -5.695e+01f, 5.222e+00f },
           {8.543e+01f, -8.543e+01f, 9.250e+00f },
           {8.543e+01f, -1.281e+02f, 1.639e+01f },
           {8.543e+01f, -1.922e+02f, 2.903e+01f },
           {8.543e+01f, -2.883e+02f, 5.143e+01f },
           {8.543e+01f, -4.325e+02f, 8.283e+01f },
           {8.543e+01f, -6.487e+02f, 1.334e+02f },
           {8.543e+01f, -9.731e+02f, 2.148e+02f },
           {8.543e+01f, -1.460e+03f, 3.460e+02f },
           {8.543e+01f, -2.189e+03f, 5.572e+02f },
           {8.543e+01f, -3.284e+03f, 8.974e+02f },
           {8.543e+01f, -4.926e+03f, 1.314e+03f },
           {8.543e+01f, -7.389e+03f, 2.116e+03f },
           {8.543e+01f, -1.108e+04f, 3.098e+03f },
           {8.543e+01f, -1.663e+04f, 4.536e+03f },
           {1.281e+02f, -7.500e+00f, 1.250e+00f },
           {1.281e+02f, -1.125e+01f, 1.250e+00f },
           {1.281e+02f, -1.688e+01f, 1.250e+00f },
           {1.281e+02f, -2.531e+01f, 1.513e+00f },
           {1.281e+02f, -3.797e+01f, 2.214e+00f },
           {1.281e+02f, -5.695e+01f, 3.923e+00f },
           {1.281e+02f, -8.543e+01f, 6.950e+00f },
           {1.281e+02f, -1.281e+02f, 1.231e+01f },
           {1.281e+02f, -1.922e+02f, 2.181e+01f },
           {1.281e+02f, -2.883e+02f, 4.250e+01f },
           {1.281e+02f, -4.325e+02f, 6.845e+01f },
           {1.281e+02f, -6.487e+02f, 1.213e+02f },
           {1.281e+02f, -9.731e+02f, 1.953e+02f },
           {1.281e+02f, -1.460e+03f, 3.460e+02f },
           {1.281e+02f, -2.189e+03f, 5.572e+02f },
           {1.281e+02f, -3.284e+03f, 8.159e+02f },
           {1.281e+02f, -4.926e+03f, 1.314e+03f },
           {1.281e+02f, -7.389e+03f, 1.924e+03f },
           {1.281e+02f, -1.108e+04f, 3.098e+03f },
           {1.281e+02f, -1.663e+04f, 4.536e+03f },
           {1.922e+02f, -7.500e+00f, 1.250e+00f },
           {1.922e+02f, -1.125e+01f, 1.250e+00f },
           {1.922e+02f, -1.688e+01f, 1.250e+00f },
           {1.922e+02f, -2.531e+01f, 1.250e+00f },
           {1.922e+02f, -3.797e+01f, 1.664e+00f },
           {1.922e+02f, -5.695e+01f, 2.679e+00f },
           {1.922e+02f, -8.543e+01f, 5.222e+00f },
           {1.922e+02f, -1.281e+02f, 9.250e+00f },
           {1.922e+02f, -1.922e+02f, 1.803e+01f },
           {1.922e+02f, -2.883e+02f, 3.193e+01f },
           {1.922e+02f, -4.325e+02f, 5.657e+01f },
           {1.922e+02f, -6.487e+02f, 1.002e+02f },
           {1.922e+02f, -9.731e+02f, 1.776e+02f },
           {1.922e+02f, -1.460e+03f, 3.145e+02f },
           {1.922e+02f, -2.189e+03f, 5.066e+02f },
           {1.922e+02f, -3.284e+03f, 8.159e+02f },
           {1.922e+02f, -4.926e+03f, 1.194e+03f },
           {1.922e+02f, -7.389e+03f, 1.924e+03f },
           {1.922e+02f, -1.108e+04f, 3.098e+03f },
           {1.922e+02f, -1.663e+04f, 4.536e+03f },
           {2.883e+02f, -7.500e+00f, 1.250e+00f },
           {2.883e+02f, -1.125e+01f, 1.250e+00f },
           {2.883e+02f, -1.688e+01f, 1.250e+00f },
           {2.883e+02f, -2.531e+01f, 1.250e+00f },
           {2.883e+02f, -3.797e+01f, 1.250e+00f },
           {2.883e+02f, -5.695e+01f, 2.013e+00f },
           {2.883e+02f, -8.543e+01f, 3.566e+00f },
           {2.883e+02f, -1.281e+02f, 6.950e+00f },
           {2.883e+02f, -1.922e+02f, 1.354e+01f },
           {2.883e+02f, -2.883e+02f, 2.399e+01f },
           {2.883e+02f, -4.325e+02f, 4.676e+01f },
           {2.883e+02f, -6.487e+02f, 8.283e+01f },
           {2.883e+02f, -9.731e+02f, 1.614e+02f },
           {2.883e+02f, -1.460e+03f, 2.600e+02f },
           {2.883e+02f, -2.189e+03f, 4.605e+02f },
           {2.883e+02f, -3.284e+03f, 7.417e+02f },
           {2.883e+02f, -4.926e+03f, 1.194e+03f },
           {2.883e+02f, -7.389e+03f, 1.924e+03f },
           {2.883e+02f, -1.108e+04f, 2.817e+03f },
           {2.883e+02f, -1.663e+04f, 4.536e+03f },
           {4.325e+02f, -7.500e+00f, 1.250e+00f },
           {4.325e+02f, -1.125e+01f, 1.250e+00f },
           {4.325e+02f, -1.688e+01f, 1.250e+00f },
           {4.325e+02f, -2.531e+01f, 1.250e+00f },
           {4.325e+02f, -3.797e+01f, 1.250e+00f },
           {4.325e+02f, -5.695e+01f, 1.375e+00f },
           {4.325e+02f, -8.543e+01f, 2.436e+00f },
           {4.325e+02f, -1.281e+02f, 4.747e+00f },
           {4.325e+02f, -1.922e+02f, 9.250e+00f },
           {4.325e+02f, -2.883e+02f, 1.803e+01f },
           {4.325e+02f, -4.325e+02f, 3.513e+01f },
           {4.325e+02f, -6.487e+02f, 6.845e+01f },
           {4.325e+02f, -9.731e+02f, 1.334e+02f },
           {4.325e+02f, -1.460e+03f, 2.363e+02f },
           {4.325e+02f, -2.189e+03f, 3.806e+02f },
           {4.325e+02f, -3.284e+03f, 6.743e+02f },
           {4.325e+02f, -4.926e+03f, 1.086e+03f },
           {4.325e+02f, -7.389e+03f, 1.749e+03f },
           {4.325e+02f, -1.108e+04f, 2.817e+03f },
           {4.325e+02f, -1.663e+04f, 4.536e+03f },
           {6.487e+02f, -7.500e+00f, 1.250e+00f },
           {6.487e+02f, -1.125e+01f, 1.250e+00f },
           {6.487e+02f, -1.688e+01f, 1.250e+00f },
           {6.487e+02f, -2.531e+01f, 1.250e+00f },
           {6.487e+02f, -3.797e+01f, 1.250e+00f },
           {6.487e+02f, -5.695e+01f, 1.250e+00f },
           {6.487e+02f, -8.543e+01f, 1.664e+00f },
           {6.487e+02f, -1.281e+02f, 3.242e+00f },
           {6.487e+02f, -1.922e+02f, 6.950e+00f },
           {6.487e+02f, -2.883e+02f, 1.354e+01f },
           {6.487e+02f, -4.325e+02f, 2.639e+01f },
           {6.487e+02f, -6.487e+02f, 5.143e+01f },
           {6.487e+02f, -9.731e+02f, 1.002e+02f },
           {6.487e+02f, -1.460e+03f, 1.953e+02f },
           {6.487e+02f, -2.189e+03f, 3.460e+02f },
           {6.487e+02f, -3.284e+03f, 6.130e+02f },
           {6.487e+02f, -4.926e+03f, 9.872e+02f },
           {6.487e+02f, -7.389e+03f, 1.590e+03f },
           {6.487e+02f, -1.108e+04f, 2.561e+03f },
           {6.487e+02f, -1.663e+04f, 4.124e+03f },
           {9.731e+02f, -7.500e+00f, 1.250e+00f },
           {9.731e+02f, -1.125e+01f, 1.250e+00f },
           {9.731e+02f, -1.688e+01f, 1.250e+00f },
           {9.731e+02f, -2.531e+01f, 1.250e+00f },
           {9.731e+02f, -3.797e+01f, 1.250e+00f },
           {9.731e+02f, -5.695e+01f, 1.250e+00f },
           {9.731e+02f, -8.543e+01f, 1.250e+00f },
           {9.731e+02f, -1.281e+02f, 2.214e+00f },
           {9.731e+02f, -1.922e+02f, 4.747e+00f },
           {9.731e+02f, -2.883e+02f, 9.250e+00f },
           {9.731e+02f, -4.325e+02f, 1.983e+01f },
           {9.731e+02f, -6.487e+02f, 3.864e+01f },
           {9.731e+02f, -9.731e+02f, 7.530e+01f },
           {9.731e+02f, -1.460e+03f, 1.467e+02f },
           {9.731e+02f, -2.189e+03f, 2.860e+02f },
           {9.731e+02f, -3.284e+03f, 5.066e+02f },
           {9.731e+02f, -4.926e+03f, 8.974e+02f },
           {9.731e+02f, -7.389e+03f, 1.445e+03f },
           {9.731e+02f, -1.108e+04f, 2.561e+03f },
           {9.731e+02f, -1.663e+04f, 4.124e+03f },
           {1.460e+03f, -7.500e+00f, 1.250e+00f },
           {1.460e+03f, -1.125e+01f, 1.250e+00f },
           {1.460e+03f, -1.688e+01f, 1.250e+00f },
           {1.460e+03f, -2.531e+01f, 1.250e+00f },
           {1.460e+03f, -3.797e+01f, 1.250e+00f },
           {1.460e+03f, -5.695e+01f, 1.250e+00f },
           {1.460e+03f, -8.543e+01f, 1.250e+00f },
           {1.460e+03f, -1.281e+02f, 1.513e+00f },
           {1.460e+03f, -1.922e+02f, 3.242e+00f },
           {1.460e+03f, -2.883e+02f, 6.950e+00f },
           {1.460e+03f, -4.325e+02f, 1.354e+01f },
           {1.460e+03f, -6.487e+02f, 2.903e+01f },
           {1.460e+03f, -9.731e+02f, 5.657e+01f },
           {1.460e+03f, -1.460e+03f, 1.213e+02f },
           {1.460e+03f, -2.189e+03f, 2.148e+02f },
           {1.460e+03f, -3.284e+03f, 4.187e+02f },
           {1.460e+03f, -4.926e+03f, 7.417e+02f },
           {1.460e+03f, -7.389e+03f, 1.314e+03f },
           {1.460e+03f, -1.108e+04f, 2.328e+03f },
           {1.460e+03f, -1.663e+04f, 3.749e+03f },
           {2.189e+03f, -7.500e+00f, 1.250e+00f },
           {2.189e+03f, -1.125e+01f, 1.250e+00f },
           {2.189e+03f, -1.688e+01f, 1.250e+00f },
           {2.189e+03f, -2.531e+01f, 1.250e+00f },
           {2.189e+03f, -3.797e+01f, 1.250e+00f },
           {2.189e+03f, -5.695e+01f, 1.250e+00f },
           {2.189e+03f, -8.543e+01f, 1.250e+00f },
           {2.189e+03f, -1.281e+02f, 1.250e+00f },
           {2.189e+03f, -1.922e+02f, 2.214e+00f },
           {2.189e+03f, -2.883e+02f, 4.747e+00f },
           {2.189e+03f, -4.325e+02f, 9.250e+00f },
           {2.189e+03f, -6.487e+02f, 1.983e+01f },
           {2.189e+03f, -9.731e+02f, 4.250e+01f },
           {2.189e+03f, -1.460e+03f, 8.283e+01f },
           {2.189e+03f, -2.189e+03f, 1.776e+02f },
           {2.189e+03f, -3.284e+03f, 3.460e+02f },
           {2.189e+03f, -4.926e+03f, 6.130e+02f },
           {2.189e+03f, -7.389e+03f, 1.086e+03f },
           {2.189e+03f, -1.108e+04f, 1.924e+03f },
           {2.189e+03f, -1.663e+04f, 3.408e+03f },
           {3.284e+03f, -7.500e+00f, 1.250e+00f },
           {3.284e+03f, -1.125e+01f, 1.250e+00f },
           {3.284e+03f, -1.688e+01f, 1.250e+00f },
           {3.284e+03f, -2.531e+01f, 1.250e+00f },
           {3.284e+03f, -3.797e+01f, 1.250e+00f },
           {3.284e+03f, -5.695e+01f, 1.250e+00f },
           {3.284e+03f, -8.543e+01f, 1.250e+00f },
           {3.284e+03f, -1.281e+02f, 1.250e+00f },
           {3.284e+03f, -1.922e+02f, 1.513e+00f },
           {3.284e+03f, -2.883e+02f, 3.242e+00f },
           {3.284e+03f, -4.325e+02f, 6.318e+00f },
           {3.284e+03f, -6.487e+02f, 1.354e+01f },
           {3.284e+03f, -9.731e+02f, 2.903e+01f },
           {3.284e+03f, -1.460e+03f, 6.223e+01f },
           {3.284e+03f, -2.189e+03f, 1.334e+02f },
           {3.284e+03f, -3.284e+03f, 2.600e+02f },
           {3.284e+03f, -4.926e+03f, 5.066e+02f },
           {3.284e+03f, -7.389e+03f, 9.872e+02f },
           {3.284e+03f, -1.108e+04f, 1.749e+03f },
           {3.284e+03f, -1.663e+04f, 3.098e+03f },
           {4.926e+03f, -7.500e+00f, 1.250e+00f },
           {4.926e+03f, -1.125e+01f, 1.250e+00f },
           {4.926e+03f, -1.688e+01f, 1.250e+00f },
           {4.926e+03f, -2.531e+01f, 1.250e+00f },
           {4.926e+03f, -3.797e+01f, 1.250e+00f },
           {4.926e+03f, -5.695e+01f, 1.250e+00f },
           {4.926e+03f, -8.543e+01f, 1.250e+00f },
           {4.926e+03f, -1.281e+02f, 1.250e+00f },
           {4.926e+03f, -1.922e+02f, 1.250e+00f },
           {4.926e+03f, -2.883e+02f, 2.013e+00f },
           {4.926e+03f, -4.325e+02f, 4.315e+00f },
           {4.926e+03f, -6.487e+02f, 9.250e+00f },
           {4.926e+03f, -9.731e+02f, 2.181e+01f },
           {4.926e+03f, -1.460e+03f, 4.250e+01f },
           {4.926e+03f, -2.189e+03f, 9.111e+01f },
           {4.926e+03f, -3.284e+03f, 1.953e+02f },
           {4.926e+03f, -4.926e+03f, 3.806e+02f },
           {4.926e+03f, -7.389e+03f, 7.417e+02f },
           {4.926e+03f, -1.108e+04f, 1.445e+03f },
           {4.926e+03f, -1.663e+04f, 2.561e+03f },
           {7.389e+03f, -7.500e+00f, 1.250e+00f },
           {7.389e+03f, -1.125e+01f, 1.250e+00f },
           {7.389e+03f, -1.688e+01f, 1.250e+00f },
           {7.389e+03f, -2.531e+01f, 1.250e+00f },
           {7.389e+03f, -3.797e+01f, 1.250e+00f },
           {7.389e+03f, -5.695e+01f, 1.250e+00f },
           {7.389e+03f, -8.543e+01f, 1.250e+00f },
           {7.389e+03f, -1.281e+02f, 1.250e+00f },
           {7.389e+03f, -1.922e+02f, 1.250e+00f },
           {7.389e+03f, -2.883e+02f, 1.375e+00f },
           {7.389e+03f, -4.325e+02f, 2.947e+00f },
           {7.389e+03f, -6.487e+02f, 6.318e+00f },
           {7.389e+03f, -9.731e+02f, 1.490e+01f },
           {7.389e+03f, -1.460e+03f, 3.193e+01f },
           {7.389e+03f, -2.189e+03f, 6.845e+01f },
           {7.389e+03f, -3.284e+03f, 1.334e+02f },
           {7.389e+03f, -4.926e+03f, 2.860e+02f },
           {7.389e+03f, -7.389e+03f, 5.572e+02f },
           {7.389e+03f, -1.108e+04f, 1.086e+03f },
           {7.389e+03f, -1.663e+04f, 2.116e+03f },
           {1.108e+04f, -7.500e+00f, 1.250e+00f },
           {1.108e+04f, -1.125e+01f, 1.250e+00f },
           {1.108e+04f, -1.688e+01f, 1.250e+00f },
           {1.108e+04f, -2.531e+01f, 1.250e+00f },
           {1.108e+04f, -3.797e+01f, 1.250e+00f },
           {1.108e+04f, -5.695e+01f, 1.250e+00f },
           {1.108e+04f, -8.543e+01f, 1.250e+00f },
           {1.108e+04f, -1.281e+02f, 1.250e+00f },
           {1.108e+04f, -1.922e+02f, 1.250e+00f },
           {1.108e+04f, -2.883e+02f, 1.250e+00f },
           {1.108e+04f, -4.325e+02f, 2.013e+00f },
           {1.108e+04f, -6.487e+02f, 4.315e+00f },
           {1.108e+04f, -9.731e+02f, 1.018e+01f },
           {1.108e+04f, -1.460e+03f, 2.181e+01f },
           {1.108e+04f, -2.189e+03f, 4.676e+01f },
           {1.108e+04f, -3.284e+03f, 1.002e+02f },
           {1.108e+04f, -4.926e+03f, 2.148e+02f },
           {1.108e+04f, -7.389e+03f, 4.187e+02f },
           {1.108e+04f, -1.108e+04f, 8.974e+02f },
           {1.108e+04f, -1.663e+04f, 1.749e+03f },
           {1.663e+04f, -7.500e+00f, 1.250e+00f },
           {1.663e+04f, -1.125e+01f, 1.250e+00f },
           {1.663e+04f, -1.688e+01f, 1.250e+00f },
           {1.663e+04f, -2.531e+01f, 1.250e+00f },
           {1.663e+04f, -3.797e+01f, 1.250e+00f },
           {1.663e+04f, -5.695e+01f, 1.250e+00f },
           {1.663e+04f, -8.543e+01f, 1.250e+00f },
           {1.663e+04f, -1.281e+02f, 1.250e+00f },
           {1.663e+04f, -1.922e+02f, 1.250e+00f },
           {1.663e+04f, -2.883e+02f, 1.250e+00f },
           {1.663e+04f, -4.325e+02f, 1.375e+00f },
           {1.663e+04f, -6.487e+02f, 2.947e+00f },
           {1.663e+04f, -9.731e+02f, 6.318e+00f },
           {1.663e+04f, -1.460e+03f, 1.490e+01f },
           {1.663e+04f, -2.189e+03f, 3.193e+01f },
           {1.663e+04f, -3.284e+03f, 6.845e+01f },
           {1.663e+04f, -4.926e+03f, 1.467e+02f },
           {1.663e+04f, -7.389e+03f, 3.145e+02f },
           {1.663e+04f, -1.108e+04f, 6.743e+02f },
           {1.663e+04f, -1.663e+04f, 1.314e+03f },
        };
        if ((a > 1.663e+04) || (-b > 1.663e+04))
           return z > -b;  // Way overly conservative?
        if (a < data[0][0])
           return false;
        int index = 0;
        while (data[index][0] < a)
           ++index;
        if(a != data[index][0])
           --index;
        while ((data[index][1] < b) && (data[index][2] > 1.25))
           --index;
        ++index;
        BOOST_MATH_ASSERT(a > data[index][0]);
        BOOST_MATH_ASSERT(-b < -data[index][1]);
        return z > data[index][2];
     }
     template <class T, class Policy>
     T hypergeometric_1F1_from_function_ratio_negative_b_forwards(const T& a, const T& b, const T& z, const Policy& pol, long long& log_scaling)
     {
        BOOST_MATH_STD_USING
        //
        // Get the function ratio, M(a+1, b+1, z)/M(a,b,z):
        //
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        boost::math::detail::hypergeometric_1F1_recurrence_a_and_b_coefficients<T> coef(a, b, z);
        T ratio = 1 / boost::math::tools::function_ratio_from_forwards_recurrence(coef, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1F1_from_function_ratio_negative_b_positive_a<%1%>(%1%,%1%,%1%)", max_iter, pol);
        //
        // We can't normalise via the Wronksian as the subtraction in the Wronksian will suffer an exquisite amount of cancellation - 
        // potentially many hundreds of digits in this region.  However, if forwards iteration is stable at this point
        // it will also be stable for M(a, b+1, z) etc all the way up to the origin, and hopefully one step beyond.  So
        // use a reference value just over the origin to normalise:
        //
        long long scale = 0;
        int steps = itrunc(ceil(-b));
        T reference_value = hypergeometric_1F1_imp(T(a + steps), T(b + steps), z, pol, log_scaling);
        T found = boost::math::tools::apply_recurrence_relation_forward(boost::math::detail::hypergeometric_1F1_recurrence_a_and_b_coefficients<T>(a + 1, b + 1, z), steps - 1, T(1), ratio, &scale);
        log_scaling -= scale;
        if ((fabs(reference_value) < 1) && (fabs(reference_value) < tools::min_value<T>() * fabs(found)))
        {
           // Possible underflow, rescale
           long long s = lltrunc(tools::log_max_value<T>());
           log_scaling -= s;
           reference_value *= exp(T(s));
        }
        else if ((fabs(found) < 1) && (fabs(reference_value) > tools::max_value<T>() * fabs(found)))
        {
           // Overflow, rescale:
           long long s = lltrunc(tools::log_max_value<T>());
           log_scaling += s;
           reference_value /= exp(T(s));
        }
        return reference_value / found;
     }



     //
     // This next version is largely the same as above, but calculates the ratio for the b recurrence relation
     // which has a larger area of stability than the ab recurrence when a,b < 0.  We can then use a single
     // recurrence step to convert this to the ratio for the ab recursion and proceed largely as before.
     // The routine is quite insensitive to the size of z, but requires |a| < |5b| for accuracy.
     // Fortunately the accuracy outside this domain falls off steadily rather than suddenly switching
     // to a different behaviour.
     //
     template <class T, class Policy>
     T hypergeometric_1F1_from_function_ratio_negative_ab(const T& a, const T& b, const T& z, const Policy& pol, long long& log_scaling)
     {
        BOOST_MATH_STD_USING
        //
        // Get the function ratio, M(a+1, b+1, z)/M(a,b,z):
        //
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        boost::math::detail::hypergeometric_1F1_recurrence_b_coefficients<T> coef(a, b + 1, z);
        T ratio = boost::math::tools::function_ratio_from_backwards_recurrence(coef, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1F1_from_function_ratio_negative_b_positive_a<%1%>(%1%,%1%,%1%)", max_iter, pol);
        //
        // We need to use A&S 13.4.3 to convert a ratio for M(a, b + 1, z) / M(a, b, z)
        // to M(a+1, b+1, z) / M(a, b, z)
        //
        // We have:        (a-b)M(a, b+1, z) - aM(a+1, b+1, z) + bM(a, b, z) = 0
        // and therefore:  M(a + 1, b + 1, z) / M(a, b, z) = ((a - b)M(a, b + 1, z) / M(a, b, z) + b) / a
        //
        ratio = ((a - b) * ratio + b) / a;
        //
        // Let M2 = M(1+a-b, 2-b, z)
        // This is going to be a mighty big number:
        //
        long long local_scaling = 0;
        T M2 = boost::math::detail::hypergeometric_1F1_imp(T(1 + a - b), T(2 - b), z, pol, local_scaling);
        log_scaling -= local_scaling; // all the M2 terms are in the denominator
        //
        // Let M3 = M(1+a-b + 1, 2-b + 1, z)
        // We don't use the ratio to get this as it's not clear that it's reliable:
        //
        long long local_scaling2 = 0;
        T M3 = boost::math::detail::hypergeometric_1F1_imp(T(2 + a - b), T(3 - b), z, pol, local_scaling2);
        //
        // M2 and M3 must be identically scaled:
        //
        if (local_scaling != local_scaling2)
        {
           M3 *= exp(T(local_scaling2 - local_scaling));
        }
        //
        // Get the RHS of the Wronksian:
        //
        long long fz = lltrunc(z);
        log_scaling += fz;
        T rhs = (1 - b) * exp(z - fz);
        //
        // We need to divide that by:
        // [(a-b+1)z/(2-b) M2(a+1,b+1,z) + (1-b) M2(a,b,z) - a/b z^b R(a,b,z) M2(a,b,z) ]
        // Note that at this stage, both M3 and M2 are scaled by exp(local_scaling).
        //
        T lhs = (a - b + 1) * z * M3 / (2 - b) + (1 - b) * M2 - a * z * ratio * M2 / b;

        return rhs / lhs;
     }

  } } } // namespaces

#endif // BOOST_HYPERGEOMETRIC_1F1_BY_RATIOS_HPP_
