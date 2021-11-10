
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2013 Nikhar Agrawal
//  Copyright 2013 Christopher Kormanyos
//  Copyright 2014 John Maddock
//  Copyright 2013 Paul Bristow
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_POLYGAMMA_2013_07_30_HPP_
  #define _BOOST_POLYGAMMA_2013_07_30_HPP_

#include <boost/math/special_functions/factorials.hpp>
#include <boost/math/special_functions/detail/polygamma.hpp>
#include <boost/math/special_functions/trigamma.hpp>

namespace boost { namespace math {

  
  template<class T, class Policy>
  inline typename tools::promote_args<T>::type polygamma(const int n, T x, const Policy& pol)
  {
     //
     // Filter off special cases right at the start:
     //
     if(n == 0)
        return boost::math::digamma(x, pol);
     if(n == 1)
        return boost::math::trigamma(x, pol);
     //
     // We've found some standard library functions to misbehave if any FPU exception flags
     // are set prior to their call, this code will clear those flags, then reset them
     // on exit:
     //
     BOOST_FPU_EXCEPTION_GUARD
     //
     // The type of the result - the common type of T and U after
     // any integer types have been promoted to double:
     //
     typedef typename tools::promote_args<T>::type result_type;
     //
     // The type used for the calculation.  This may be a wider type than
     // the result in order to ensure full precision:
     //
     typedef typename policies::evaluation<result_type, Policy>::type value_type;
     //
     // The type of the policy to forward to the actual implementation.
     // We disable promotion of float and double as that's [possibly]
     // happened already in the line above.  Also reset to the default
     // any policies we don't use (reduces code bloat if we're called
     // multiple times with differing policies we don't actually use).
     // Also normalise the type, again to reduce code bloat in case we're
     // called multiple times with functionally identical policies that happen
     // to be different types.
     //
     typedef typename policies::normalise<
        Policy,
        policies::promote_float<false>,
        policies::promote_double<false>,
        policies::discrete_quantile<>,
        policies::assert_undefined<> >::type forwarding_policy;
     //
     // Whew.  Now we can make the actual call to the implementation.
     // Arguments are explicitly cast to the evaluation type, and the result
     // passed through checked_narrowing_cast which handles things like overflow
     // according to the policy passed:
     //
     return policies::checked_narrowing_cast<result_type, forwarding_policy>(
        detail::polygamma_imp(n, static_cast<value_type>(x), forwarding_policy()),
        "boost::math::polygamma<%1%>(int, %1%)");
  }

  template<class T>
  inline typename tools::promote_args<T>::type polygamma(const int n, T x)
  {
      return boost::math::polygamma(n, x, policies::policy<>());
  }

} } // namespace boost::math

#endif // _BOOST_BERNOULLI_2013_05_30_HPP_

