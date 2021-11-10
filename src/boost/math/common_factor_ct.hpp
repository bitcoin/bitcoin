//  Boost common_factor_ct.hpp header file  ----------------------------------//

//  (C) Copyright John Maddock 2017.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_MATH_COMMON_FACTOR_CT_HPP
#define BOOST_MATH_COMMON_FACTOR_CT_HPP

#ifndef BOOST_MATH_STANDALONE
#include <boost/integer/common_factor_ct.hpp>
#include <boost/math/tools/header_deprecated.hpp>

BOOST_MATH_HEADER_DEPRECATED("<boost/integer/common_factor_ct.hpp>");

namespace boost
{
namespace math
{

   using boost::integer::static_gcd;
   using boost::integer::static_lcm;
   using boost::integer::static_gcd_type;

}  // namespace math
}  // namespace boost
#else
#error Common factor is not available in standalone mode because it requires boost.integer.
#endif // BOOST_MATH_STANDALONE

#endif  // BOOST_MATH_COMMON_FACTOR_CT_HPP
