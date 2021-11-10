///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MATH_MAX_DIGITS10_HPP
#define BOOST_MATH_MAX_DIGITS10_HPP

namespace boost {
namespace multiprecision {
namespace detail {

template <unsigned digits>
struct calc_max_digits10
{
   static constexpr unsigned max_digits_10(unsigned d)
   {
      //
      // We need ceil(log10(2) * d) + 1 decimal places to
      // guarantee round tripping, see: https://www.exploringbinary.com/number-of-digits-required-for-round-trip-conversions/
      // and references therein.  Since log10(2) is irrational, then d*log10(2) will
      // never be exactly an integer so we can replace by trunc(log10(2) * d) + 2
      // and avoid the call to ceil:
      //
      return static_cast<unsigned>(0.301029995663981195213738894724493026768189881462108541310 * d) + 2;
   }
   static constexpr const unsigned value = max_digits_10(digits);
};

template <unsigned digits>
struct calc_digits10
{
   static constexpr unsigned digits_10(unsigned d)
   {
      //
      // We need floor(log10(2) * (d-1)), see: 
      // https://www.exploringbinary.com/number-of-digits-required-for-round-trip-conversions/
      // and references therein.
      //
      return static_cast<unsigned>(0.301029995663981195213738894724493026768189881462108541310 * (d - 1));
   }
   static constexpr const unsigned value = digits_10(digits);
};

}}} // namespace boost::multiprecision::detail

#endif
