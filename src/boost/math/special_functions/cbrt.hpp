//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SF_CBRT_HPP
#define BOOST_MATH_SF_CBRT_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/rational.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <type_traits>
#include <cstdint>

namespace boost{ namespace math{

namespace detail
{

struct big_int_type
{
   operator std::uintmax_t() const;
};

template <typename T>
struct largest_cbrt_int_type
{
   using type = typename std::conditional<
      std::is_convertible<big_int_type, T>::value,
      std::uintmax_t,
      unsigned int
   >::type;
};

template <typename T, typename Policy>
T cbrt_imp(T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   //
   // cbrt approximation for z in the range [0.5,1]
   // It's hard to say what number of terms gives the optimum
   // trade off between precision and performance, this seems
   // to be about the best for double precision.
   //
   // Maximum Deviation Found:                     1.231e-006
   // Expected Error Term:                         -1.231e-006
   // Maximum Relative Change in Control Points:   5.982e-004
   //
   static const T P[] = { 
      static_cast<T>(0.37568269008611818),
      static_cast<T>(1.3304968705558024),
      static_cast<T>(-1.4897101632445036),
      static_cast<T>(1.2875573098219835),
      static_cast<T>(-0.6398703759826468),
      static_cast<T>(0.13584489959258635),
   };
   static const T correction[] = {
      static_cast<T>(0.62996052494743658238360530363911),  // 2^-2/3
      static_cast<T>(0.79370052598409973737585281963615),  // 2^-1/3
      static_cast<T>(1),
      static_cast<T>(1.2599210498948731647672106072782),   // 2^1/3
      static_cast<T>(1.5874010519681994747517056392723),   // 2^2/3
   };
   if((boost::math::isinf)(z) || (z == 0))
      return z;
   if(!(boost::math::isfinite)(z))
   {
      return policies::raise_domain_error("boost::math::cbrt<%1%>(%1%)", "Argument to function must be finite but got %1%.", z, pol);
   }

   int i_exp, sign(1);
   if(z < 0)
   {
      z = -z;
      sign = -sign;
   }

   T guess = frexp(z, &i_exp);
   int original_i_exp = i_exp; // save for later
   guess = tools::evaluate_polynomial(P, guess);
   int i_exp3 = i_exp / 3;

   using shift_type = typename largest_cbrt_int_type<T>::type;

   static_assert( ::std::numeric_limits<shift_type>::radix == 2, "The radix of the type to shift to must be 2.");

   if(abs(i_exp3) < std::numeric_limits<shift_type>::digits)
   {
      if(i_exp3 > 0)
         guess *= shift_type(1u) << i_exp3;
      else
         guess /= shift_type(1u) << -i_exp3;
   }
   else
   {
      guess = ldexp(guess, i_exp3);
   }
   i_exp %= 3;
   guess *= correction[i_exp + 2];
   //
   // Now inline Halley iteration.
   // We do this here rather than calling tools::halley_iterate since we can
   // simplify the expressions algebraically, and don't need most of the error
   // checking of the boilerplate version as we know in advance that the function
   // is well behaved...
   //
   using prec = typename policies::precision<T, Policy>::type;
   constexpr auto prec3 = prec::value / 3;
   constexpr auto new_prec = prec3 + 3;
   using new_policy = typename policies::normalise<Policy, policies::digits2<new_prec>>::type;
   //
   // Epsilon calculation uses compile time arithmetic when it's available for type T,
   // otherwise uses ldexp to calculate at runtime:
   //
   T eps = (new_prec > 3) ? policies::get_epsilon<T, new_policy>() : ldexp(T(1), -2 - tools::digits<T>() / 3);
   T diff;

   if(original_i_exp < std::numeric_limits<T>::max_exponent - 3)
   {
      //
      // Safe from overflow, use the fast method:
      //
      do
      {
         T g3 = guess * guess * guess;
         diff = (g3 + z + z) / (g3 + g3 + z);
         guess *= diff;
      }
      while(fabs(1 - diff) > eps);
   }
   else
   {
      //
      // Either we're ready to overflow, or we can't tell because numeric_limits isn't
      // available for type T:
      //
      do
      {
         T g2 = guess * guess;
         diff = (g2 - z / guess) / (2 * guess + z / g2);
         guess -= diff;
      }
      while((guess * eps) < fabs(diff));
   }

   return sign * guess;
}

} // namespace detail

template <typename T, typename Policy>
inline typename tools::promote_args<T>::type cbrt(T z, const Policy& pol)
{
   using result_type = typename tools::promote_args<T>::type;
   using value_type = typename policies::evaluation<result_type, Policy>::type;
   return static_cast<result_type>(detail::cbrt_imp(value_type(z), pol));
}

template <typename T>
inline typename tools::promote_args<T>::type cbrt(T z)
{
   return cbrt(z, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_SF_CBRT_HPP




