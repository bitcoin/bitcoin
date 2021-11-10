//    boost asinh.hpp header file

//  (C) Copyright Eric Ford 2001 & Hubert Holin.
//  (C) Copyright John Maddock 2008.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_ACOSH_HPP
#define BOOST_ACOSH_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <cmath>
#include <boost/math/tools/precision.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

// This is the inverse of the hyperbolic cosine function.

namespace boost
{
    namespace math
    {
       namespace detail
       {
        template<typename T, typename Policy>
        inline T    acosh_imp(const T x, const Policy& pol)
        {
            BOOST_MATH_STD_USING
            
            if((x < 1) || (boost::math::isnan)(x))
            {
               return policies::raise_domain_error<T>(
                  "boost::math::acosh<%1%>(%1%)",
                  "acosh requires x >= 1, but got x = %1%.", x, pol);
            }
            else if    ((x - 1) >= tools::root_epsilon<T>())
            {
                if    (x > 1 / tools::root_epsilon<T>())
                {
                    // http://functions.wolfram.com/ElementaryFunctions/ArcCosh/06/01/06/01/0001/
                    // approximation by laurent series in 1/x at 0+ order from -1 to 0
                    return log(x) + constants::ln_two<T>();
                }
                else if(x < 1.5f)
                {
                   // This is just a rearrangement of the standard form below
                   // devised to minimise loss of precision when x ~ 1:
                   T y = x - 1;
                   return boost::math::log1p(y + sqrt(y * y + 2 * y), pol);
                }
                else
                {
                    // http://functions.wolfram.com/ElementaryFunctions/ArcCosh/02/
                    return( log( x + sqrt(x * x - 1) ) );
                }
            }
            else
            {
                // see http://functions.wolfram.com/ElementaryFunctions/ArcCosh/06/01/04/01/0001/
                T y = x - 1;
                
                // approximation by taylor series in y at 0 up to order 2
                T result = sqrt(2 * y) * (1 - y /12 + 3 * y * y / 160);
                return result;
            }
        }
       }

        template<typename T, typename Policy>
        inline typename tools::promote_args<T>::type acosh(T x, const Policy&)
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
              detail::acosh_imp(static_cast<value_type>(x), forwarding_policy()),
              "boost::math::acosh<%1%>(%1%)");
        }
        template<typename T>
        inline typename tools::promote_args<T>::type acosh(T x)
        {
           return boost::math::acosh(x, policies::policy<>());
        }

    }
}

#endif /* BOOST_ACOSH_HPP */


