//    boost atanh.hpp header file

//  (C) Copyright Hubert Holin 2001.
//  (C) Copyright John Maddock 2008.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_ATANH_HPP
#define BOOST_ATANH_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <cmath>
#include <boost/math/tools/precision.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

// This is the inverse of the hyperbolic tangent function.

namespace boost
{
    namespace math
    {
       namespace detail
       {
        // This is the main fare
        
        template<typename T, typename Policy>
        inline T    atanh_imp(const T x, const Policy& pol)
        {
            BOOST_MATH_STD_USING
            static const char* function = "boost::math::atanh<%1%>(%1%)";
            
            if(x < -1)
            {
               return policies::raise_domain_error<T>(
                  function,
                  "atanh requires x >= -1, but got x = %1%.", x, pol);
            }
            else if(x > 1)
            {
               return policies::raise_domain_error<T>(
                  function,
                  "atanh requires x <= 1, but got x = %1%.", x, pol);
            }
            else if((boost::math::isnan)(x))
            {
               return policies::raise_domain_error<T>(
                  function,
                  "atanh requires -1 <= x <= 1, but got x = %1%.", x, pol);
            }
            else if(x < -1 + tools::epsilon<T>())
            {
               // -Infinity:
               return -policies::raise_overflow_error<T>(function, 0, pol);
            }
            else if(x > 1 - tools::epsilon<T>())
            {
               // Infinity:
               return policies::raise_overflow_error<T>(function, 0, pol);
            }
            else if(abs(x) >= tools::forth_root_epsilon<T>())
            {
                // http://functions.wolfram.com/ElementaryFunctions/ArcTanh/02/
                if(abs(x) < 0.5f)
                   return (boost::math::log1p(x, pol) - boost::math::log1p(-x, pol)) / 2;
                return(log( (1 + x) / (1 - x) ) / 2);
            }
            else
            {
                // http://functions.wolfram.com/ElementaryFunctions/ArcTanh/06/01/03/01/
                // approximation by taylor series in x at 0 up to order 2
                T    result = x;
                
                if    (abs(x) >= tools::root_epsilon<T>())
                {
                    T    x3 = x*x*x;
                    
                    // approximation by taylor series in x at 0 up to order 4
                    result += x3/static_cast<T>(3);
                }
                
                return(result);
            }
        }
       }

        template<typename T, typename Policy>
        inline typename tools::promote_args<T>::type atanh(T x, const Policy&)
        {
            typedef typename tools::promote_args<T>::type result_type;
            typedef typename policies::evaluation<result_type, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy, 
               policies::promote_float<false>, 
               policies::promote_double<false>, 
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
           return policies::checked_narrowing_cast<result_type, forwarding_policy>(
              detail::atanh_imp(static_cast<value_type>(x), forwarding_policy()),
              "boost::math::atanh<%1%>(%1%)");
        }
        template<typename T>
        inline typename tools::promote_args<T>::type atanh(T x)
        {
           return boost::math::atanh(x, policies::policy<>());
        }

    }
}

#endif /* BOOST_ATANH_HPP */



