//  (C) Copyright John Maddock 2005-2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_FRACTION_INCLUDED
#define BOOST_MATH_TOOLS_FRACTION_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/precision.hpp>
#include <boost/math/tools/complex.hpp>
#include <type_traits>
#include <cstdint>
#include <cmath>

namespace boost{ namespace math{ namespace tools{

namespace detail
{

   template <typename T>
   struct is_pair : public std::false_type{};

   template <typename T, typename U>
   struct is_pair<std::pair<T,U>> : public std::true_type{};

   template <typename Gen>
   struct fraction_traits_simple
   {
      using result_type = typename Gen::result_type;
      using  value_type = typename Gen::result_type;

      static result_type a(const value_type&) BOOST_MATH_NOEXCEPT(value_type)
      {
         return 1;
      }
      static result_type b(const value_type& v) BOOST_MATH_NOEXCEPT(value_type)
      {
         return v;
      }
   };

   template <typename Gen>
   struct fraction_traits_pair
   {
      using  value_type = typename Gen::result_type;
      using result_type = typename value_type::first_type;

      static result_type a(const value_type& v) BOOST_MATH_NOEXCEPT(value_type)
      {
         return v.first;
      }
      static result_type b(const value_type& v) BOOST_MATH_NOEXCEPT(value_type)
      {
         return v.second;
      }
   };

   template <typename Gen>
   struct fraction_traits
       : public std::conditional<
         is_pair<typename Gen::result_type>::value,
         fraction_traits_pair<Gen>,
         fraction_traits_simple<Gen>>::type
   {
   };

   template <typename T, bool = is_complex_type<T>::value>
   struct tiny_value
   {
      // For float, double, and long double, 1/min_value<T>() is finite.
      // But for mpfr_float and cpp_bin_float, 1/min_value<T>() is inf.
      // Multiply the min by 16 so that the reciprocal doesn't overflow.
      static T get() {
         return 16*tools::min_value<T>();
      }
   };
   template <typename T>
   struct tiny_value<T, true>
   {
      using value_type = typename T::value_type;
      static T get() {
         return 16*tools::min_value<value_type>();
      }
   };

} // namespace detail

//
// continued_fraction_b
// Evaluates:
//
// b0 +       a1
//      ---------------
//      b1 +     a2
//           ----------
//           b2 +   a3
//                -----
//                b3 + ...
//
// Note that the first a0 returned by generator Gen is discarded.
//
template <typename Gen, typename U>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_b(Gen& g, const U& factor, std::uintmax_t& max_terms)
      noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   BOOST_MATH_STD_USING // ADL of std names

   using traits = detail::fraction_traits<Gen>;
   using result_type = typename traits::result_type;
   using value_type = typename traits::value_type;
   using integer_type = typename integer_scalar_type<result_type>::type;
   using scalar_type = typename scalar_type<result_type>::type;

   integer_type const zero(0), one(1);

   result_type tiny = detail::tiny_value<result_type>::get();
   scalar_type terminator = abs(factor);

   value_type v = g();

   result_type f, C, D, delta;
   f = traits::b(v);
   if(f == zero)
      f = tiny;
   C = f;
   D = 0;

   std::uintmax_t counter(max_terms);
   do{
      v = g();
      D = traits::b(v) + traits::a(v) * D;
      if(D == result_type(0))
         D = tiny;
      C = traits::b(v) + traits::a(v) / C;
      if(C == zero)
         C = tiny;
      D = one/D;
      delta = C*D;
      f = f * delta;
   }while((abs(delta - one) > terminator) && --counter);

   max_terms = max_terms - counter;

   return f;
}

template <typename Gen, typename U>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_b(Gen& g, const U& factor)
   noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   std::uintmax_t max_terms = (std::numeric_limits<std::uintmax_t>::max)();
   return continued_fraction_b(g, factor, max_terms);
}

template <typename Gen>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_b(Gen& g, int bits)
   noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   BOOST_MATH_STD_USING // ADL of std names

   using traits = detail::fraction_traits<Gen>;
   using result_type = typename traits::result_type;

   result_type factor = ldexp(1.0f, 1 - bits); // 1 / pow(result_type(2), bits);
   std::uintmax_t max_terms = (std::numeric_limits<std::uintmax_t>::max)();
   return continued_fraction_b(g, factor, max_terms);
}

template <typename Gen>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_b(Gen& g, int bits, std::uintmax_t& max_terms)
   noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   BOOST_MATH_STD_USING // ADL of std names

   using traits = detail::fraction_traits<Gen>;
   using result_type = typename traits::result_type;

   result_type factor = ldexp(1.0f, 1 - bits); // 1 / pow(result_type(2), bits);
   return continued_fraction_b(g, factor, max_terms);
}

//
// continued_fraction_a
// Evaluates:
//
//            a1
//      ---------------
//      b1 +     a2
//           ----------
//           b2 +   a3
//                -----
//                b3 + ...
//
// Note that the first a1 and b1 returned by generator Gen are both used.
//
template <typename Gen, typename U>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_a(Gen& g, const U& factor, std::uintmax_t& max_terms)
   noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   BOOST_MATH_STD_USING // ADL of std names

   using traits = detail::fraction_traits<Gen>;
   using result_type = typename traits::result_type;
   using value_type = typename traits::value_type;
   using integer_type = typename integer_scalar_type<result_type>::type;
   using scalar_type = typename scalar_type<result_type>::type;

   integer_type const zero(0), one(1);

   result_type tiny = detail::tiny_value<result_type>::get();
   scalar_type terminator = abs(factor);

   value_type v = g();

   result_type f, C, D, delta, a0;
   f = traits::b(v);
   a0 = traits::a(v);
   if(f == zero)
      f = tiny;
   C = f;
   D = 0;

   std::uintmax_t counter(max_terms);

   do{
      v = g();
      D = traits::b(v) + traits::a(v) * D;
      if(D == zero)
         D = tiny;
      C = traits::b(v) + traits::a(v) / C;
      if(C == zero)
         C = tiny;
      D = one/D;
      delta = C*D;
      f = f * delta;
   }while((abs(delta - one) > terminator) && --counter);

   max_terms = max_terms - counter;

   return a0/f;
}

template <typename Gen, typename U>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_a(Gen& g, const U& factor)
   noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   std::uintmax_t max_iter = (std::numeric_limits<std::uintmax_t>::max)();
   return continued_fraction_a(g, factor, max_iter);
}

template <typename Gen>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_a(Gen& g, int bits)
   noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   BOOST_MATH_STD_USING // ADL of std names

   typedef detail::fraction_traits<Gen> traits;
   typedef typename traits::result_type result_type;

   result_type factor = ldexp(1.0f, 1-bits); // 1 / pow(result_type(2), bits);
   std::uintmax_t max_iter = (std::numeric_limits<std::uintmax_t>::max)();

   return continued_fraction_a(g, factor, max_iter);
}

template <typename Gen>
inline typename detail::fraction_traits<Gen>::result_type continued_fraction_a(Gen& g, int bits, std::uintmax_t& max_terms)
   noexcept(BOOST_MATH_IS_FLOAT(typename detail::fraction_traits<Gen>::result_type) && noexcept(std::declval<Gen>()()))
{
   BOOST_MATH_STD_USING // ADL of std names

   using traits = detail::fraction_traits<Gen>;
   using result_type = typename traits::result_type;

   result_type factor = ldexp(1.0f, 1-bits); // 1 / pow(result_type(2), bits);
   return continued_fraction_a(g, factor, max_terms);
}

} // namespace tools
} // namespace math
} // namespace boost

#endif // BOOST_MATH_TOOLS_FRACTION_INCLUDED
