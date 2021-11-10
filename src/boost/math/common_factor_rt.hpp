//  (C) Copyright John Maddock 2017.

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_COMMON_FACTOR_RT_HPP
#define BOOST_MATH_COMMON_FACTOR_RT_HPP

#ifndef BOOST_MATH_STANDALONE
#include <boost/integer/common_factor_rt.hpp>
#include <boost/math/tools/header_deprecated.hpp>

BOOST_MATH_HEADER_DEPRECATED("<boost/integer/common_factor_rt.hpp>");

namespace boost {
   namespace math {
      using boost::integer::gcd;
      using boost::integer::lcm;
      using boost::integer::gcd_range;
      using boost::integer::lcm_range;
      using boost::integer::gcd_evaluator;
      using boost::integer::lcm_evaluator;
   }
}
#else
#error Common factor is not available in standalone mode because it requires boost.integer.
#endif // BOOST_MATH_STANDALONE

#endif  // BOOST_MATH_COMMON_FACTOR_RT_HPP
