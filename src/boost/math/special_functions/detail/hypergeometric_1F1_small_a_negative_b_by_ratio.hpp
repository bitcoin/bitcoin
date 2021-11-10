
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_MATH_HYPERGEOMETRIC_1F1_SMALL_A_NEG_B_HPP
#define BOOST_MATH_HYPERGEOMETRIC_1F1_SMALL_A_NEG_B_HPP

#include <algorithm>
#include <boost/math/tools/recurrence.hpp>

  namespace boost { namespace math { namespace detail {

     // forward declaration for initial values
     template <class T, class Policy>
     inline T hypergeometric_1F1_imp(const T& a, const T& b, const T& z, const Policy& pol);

     template <class T, class Policy>
     inline T hypergeometric_1F1_imp(const T& a, const T& b, const T& z, const Policy& pol, long long& log_scaling);

      template <class T>
      T max_b_for_1F1_small_a_negative_b_by_ratio(const T& z)
      {
         if (z < -998)
            return (z * 2) / 3;
         float max_b[][2] = 
         {
            { 0.0f, -47.3046f }, {-6.7275f, -52.0351f }, { -8.9543f, -57.2386f }, {-11.9182f, -62.9625f }, {-14.421f, -69.2587f }, {-19.1943f, -76.1846f }, {-23.2252f, -83.803f }, {-28.1024f, -92.1833f }, {-34.0039f, -101.402f }, {-37.4043f, -111.542f }, {-45.2593f, -122.696f }, {-54.7637f, -134.966f }, {-60.2401f, -148.462f }, {-72.8905f, -163.308f }, {-88.1975f, -179.639f }, {-88.1975f, -197.603f }, {-106.719f, -217.363f }, {-129.13f, -239.1f }, {-142.043f, -263.01f }, {-156.247f, -289.311f }, {-189.059f, -318.242f }, {-207.965f, -350.066f }, {-228.762f, -385.073f }, {-276.801f, -423.58f }, {-304.482f, -465.938f }, {-334.93f, -512.532f }, {-368.423f, -563.785f }, {-405.265f, -620.163f }, {-445.792f, -682.18f }, {-539.408f, -750.398f }, {-593.349f, -825.437f }, {-652.683f, -907.981f }, {-717.952f, -998.779f }
         };
         auto p = std::lower_bound(max_b, max_b + sizeof(max_b) / sizeof(max_b[0]), z, [](const float (&a)[2], const T& z) { return a[1] > z; });
         T b = p - max_b ? (*--p)[0] : 0;
         //
         // We need approximately an extra 10 recurrences per 50 binary digits precision above that of double:
         //
         b += (std::max)(0, boost::math::tools::digits<T>() - 53) / 5;
         return b;
      }

      template <class T, class Policy>
      T hypergeometric_1F1_small_a_negative_b_by_ratio(const T& a, const T& b, const T& z, const Policy& pol, long long& log_scaling)
      {
         BOOST_MATH_STD_USING
         //
         // We grab the ratio for M[a, b, z] / M[a, b+1, z] and use it to seed 2 initial values,
         // then recurse until b > 0, compute a reference value and normalize (Millers method).
         //
         int iterations = itrunc(-b, pol);
         std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
         T ratio = boost::math::tools::function_ratio_from_forwards_recurrence(boost::math::detail::hypergeometric_1F1_recurrence_b_coefficients<T>(a, b, z), boost::math::tools::epsilon<T>(), max_iter);
         boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1F1_small_a_negative_b_by_ratio<%1%>(%1%,%1%,%1%)", max_iter, pol);
         T first = 1;
         T second = 1 / ratio;
         long long scaling1 = 0;
         BOOST_MATH_ASSERT(b + iterations != a);
         second = boost::math::tools::apply_recurrence_relation_forward(boost::math::detail::hypergeometric_1F1_recurrence_b_coefficients<T>(a, b + 1, z), iterations, first, second, &scaling1);
         long long scaling2 = 0;
         first = hypergeometric_1F1_imp(a, T(b + iterations + 1), z, pol, scaling2);
         //
         // Result is now first/second * e^(scaling2 - scaling1)
         //
         log_scaling += scaling2 - scaling1;
         return first / second;
      }


  } } } // namespaces

#endif // BOOST_MATH_HYPERGEOMETRIC_1F1_SMALL_A_NEG_B_HPP
