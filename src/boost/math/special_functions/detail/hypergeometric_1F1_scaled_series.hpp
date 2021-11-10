///////////////////////////////////////////////////////////////////////////////
//  Copyright 2017 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_MATH_HYPERGEOMETRIC_1F1_SCALED_SERIES_HPP
#define BOOST_MATH_HYPERGEOMETRIC_1F1_SCALED_SERIES_HPP

#include <array>
#include <cstdint>

  namespace boost{ namespace math{ namespace detail{

     template <class T, class Policy>
     T hypergeometric_1F1_scaled_series(const T& a, const T& b, T z, const Policy& pol, const char* function)
     {
        BOOST_MATH_STD_USING
        //
        // Result is returned scaled by e^-z.
        // Whenever the terms start becoming too large, we scale by some factor e^-n
        // and keep track of the integer scaling factor n.  At the end we can perform
        // an exact subtraction of n from z and scale the result:
        //
        T sum(0), term(1), upper_limit(sqrt(boost::math::tools::max_value<T>())), diff;
        unsigned n = 0;
        long long log_scaling_factor = 1 - lltrunc(boost::math::tools::log_max_value<T>());
        T scaling_factor = exp(T(log_scaling_factor));
        std::intmax_t current_scaling = 0;

        do
        {
           sum += term;
           if (sum >= upper_limit)
           {
              sum *= scaling_factor;
              term *= scaling_factor;
              current_scaling += log_scaling_factor;
           }
           term *= (((a + n) / ((b + n) * (n + 1))) * z);
           if (n > boost::math::policies::get_max_series_iterations<Policy>())
              return boost::math::policies::raise_evaluation_error(function, "Series did not converge, best value is %1%", sum, pol);
           ++n;
           diff = fabs(term / sum);
        } while (diff > boost::math::policies::get_epsilon<T, Policy>());

        z = -z - current_scaling;
        while (z < log_scaling_factor)
        {
           z -= log_scaling_factor;
           sum *= scaling_factor;
        }
        return sum * exp(z);
     }



  } } } // namespaces

#endif // BOOST_MATH_HYPERGEOMETRIC_1F1_SCALED_SERIES_HPP
