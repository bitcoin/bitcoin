///////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MATH_CPP_BIN_FLOAT_HPP
#define BOOST_MATH_CPP_BIN_FLOAT_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/integer.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/multiprecision/detail/float_string_cvt.hpp>
#include <boost/multiprecision/traits/max_digits10.hpp>
#include <boost/multiprecision/detail/hash.hpp>

//
// Some includes we need from Boost.Math, since we rely on that library to provide these functions:
//
#include <boost/math/special_functions/asinh.hpp>
#include <boost/math/special_functions/acosh.hpp>
#include <boost/math/special_functions/atanh.hpp>
#include <boost/math/special_functions/cbrt.hpp>
#include <boost/math/special_functions/expm1.hpp>
#include <boost/math/special_functions/gamma.hpp>

#ifdef BOOST_HAS_FLOAT128
#include <quadmath.h>
#endif

namespace boost {
namespace multiprecision {
namespace backends {

enum digit_base_type
{
   digit_base_2  = 2,
   digit_base_10 = 10
};

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4522 6326) // multiple assignment operators specified, comparison of two constants
#endif

namespace detail {

template <class U>
inline typename std::enable_if<boost::multiprecision::detail::is_unsigned<U>::value, bool>::type is_negative(U) { return false; }
template <class S>
inline typename std::enable_if< !boost::multiprecision::detail::is_unsigned<S>::value, bool>::type is_negative(S s) { return s < 0; }

template <class Float, int, bool = number_category<Float>::value == number_kind_floating_point>
struct is_cpp_bin_float_implicitly_constructible_from_type
{
   static constexpr const bool value = false;
};

template <class Float, int bit_count>
struct is_cpp_bin_float_implicitly_constructible_from_type<Float, bit_count, true>
{
   static constexpr const bool value = (std::numeric_limits<Float>::digits <= (int)bit_count) && (std::numeric_limits<Float>::radix == 2) && std::numeric_limits<Float>::is_specialized
#ifdef BOOST_HAS_FLOAT128
                             && !std::is_same<Float, __float128>::value
#endif
                             && (std::is_floating_point<Float>::value || is_number<Float>::value);
};

template <class Float, int, bool = number_category<Float>::value == number_kind_floating_point>
struct is_cpp_bin_float_explicitly_constructible_from_type
{
   static constexpr const bool value = false;
};

template <class Float, int bit_count>
struct is_cpp_bin_float_explicitly_constructible_from_type<Float, bit_count, true>
{
   static constexpr const bool value = (std::numeric_limits<Float>::digits > (int)bit_count) && (std::numeric_limits<Float>::radix == 2) && std::numeric_limits<Float>::is_specialized
#ifdef BOOST_HAS_FLOAT128
                             && !std::is_same<Float, __float128>::value
#endif
       ;
};

} // namespace detail

template <unsigned Digits, digit_base_type DigitBase = digit_base_10, class Allocator = void, class Exponent = int, Exponent MinExponent = 0, Exponent MaxExponent = 0>
class cpp_bin_float
{
 public:
   static constexpr const unsigned                                                                                                                                                     bit_count = DigitBase == digit_base_2 ? Digits : (Digits * 1000uL) / 301uL + (((Digits * 1000uL) % 301) ? 2u : 1u);
   using rep_type = cpp_int_backend<std::is_void<Allocator>::value ? bit_count : 0, bit_count, is_void<Allocator>::value ? unsigned_magnitude : signed_magnitude, unchecked, Allocator>        ;
   using double_rep_type = cpp_int_backend<std::is_void<Allocator>::value ? 2 * bit_count : 0, 2 * bit_count, is_void<Allocator>::value ? unsigned_magnitude : signed_magnitude, unchecked, Allocator>;

   using signed_types = typename rep_type::signed_types             ;
   using unsigned_types = typename rep_type::unsigned_types           ;
   using float_types = std::tuple<float, double, long double>;
   using exponent_type = Exponent                                    ;

   static constexpr const exponent_type max_exponent_limit = boost::integer_traits<exponent_type>::const_max - 2 * static_cast<exponent_type>(bit_count);
   static constexpr const exponent_type min_exponent_limit = boost::integer_traits<exponent_type>::const_min + 2 * static_cast<exponent_type>(bit_count);

   static_assert(MinExponent >= min_exponent_limit, "Template parameter MinExponent is too negative for our internal logic to function correctly, sorry!");
   static_assert(MaxExponent <= max_exponent_limit, "Template parameter MaxExponent is too large for our internal logic to function correctly, sorry!");
   static_assert(MinExponent <= 0, "Template parameter MinExponent can not be positive!");
   static_assert(MaxExponent >= 0, "Template parameter MaxExponent can not be negative!");

   static constexpr const exponent_type max_exponent = MaxExponent == 0 ? max_exponent_limit : MaxExponent;
   static constexpr const exponent_type min_exponent = MinExponent == 0 ? min_exponent_limit : MinExponent;

   static constexpr const exponent_type exponent_zero     = max_exponent + 1;
   static constexpr const exponent_type exponent_infinity = max_exponent + 2;
   static constexpr const exponent_type exponent_nan      = max_exponent + 3;

 private:
   rep_type      m_data;
   exponent_type m_exponent;
   bool          m_sign;

 public:
   cpp_bin_float() noexcept(noexcept(rep_type())) : m_data(), m_exponent(exponent_zero), m_sign(false) {}

   cpp_bin_float(const cpp_bin_float& o) noexcept(noexcept(rep_type(std::declval<const rep_type&>())))
       : m_data(o.m_data), m_exponent(o.m_exponent), m_sign(o.m_sign) {}

   template <unsigned D, digit_base_type B, class A, class E, E MinE, E MaxE>
   cpp_bin_float(const cpp_bin_float<D, B, A, E, MinE, MaxE>& o, typename std::enable_if<(bit_count >= cpp_bin_float<D, B, A, E, MinE, MaxE>::bit_count)>::type const* = 0)
   {
      *this = o;
   }
   template <unsigned D, digit_base_type B, class A, class E, E MinE, E MaxE>
   explicit cpp_bin_float(const cpp_bin_float<D, B, A, E, MinE, MaxE>& o, typename std::enable_if< !(bit_count >= cpp_bin_float<D, B, A, E, MinE, MaxE>::bit_count)>::type const* = 0)
       : m_exponent(o.exponent()), m_sign(o.sign())
   {
      *this = o;
   }
   // rvalue copy:
   template <unsigned D, digit_base_type B, class A, class E, E MinE, E MaxE>
   cpp_bin_float(cpp_bin_float<D, B, A, E, MinE, MaxE>&& o, typename std::enable_if<(bit_count >= cpp_bin_float<D, B, A, E, MinE, MaxE>::bit_count)>::type const* = 0)noexcept(noexcept(rep_type(std::declval<rep_type&&>())))
   {
      *this = std::move(o);
   }
   template <unsigned D, digit_base_type B, class A, class E, E MinE, E MaxE>
   explicit cpp_bin_float(cpp_bin_float<D, B, A, E, MinE, MaxE>&& o, typename std::enable_if< !(bit_count >= cpp_bin_float<D, B, A, E, MinE, MaxE>::bit_count)>::type const* = 0) noexcept(noexcept(rep_type(std::declval<rep_type&&>())))
       : m_exponent(o.exponent()), m_sign(o.sign())
   {
      *this = std::move(o);
   }
   template <class Float>
   cpp_bin_float(const Float& f,
                 typename std::enable_if<detail::is_cpp_bin_float_implicitly_constructible_from_type<Float, bit_count>::value>::type const* = 0)
       : m_data(), m_exponent(0), m_sign(false)
   {
      this->assign_float(f);
   }

   template <class Float>
   explicit cpp_bin_float(const Float& f,
                          typename std::enable_if<detail::is_cpp_bin_float_explicitly_constructible_from_type<Float, bit_count>::value>::type const* = 0)
       : m_data(), m_exponent(0), m_sign(false)
   {
      this->assign_float(f);
   }
#ifdef BOOST_HAS_FLOAT128
   template <class Float>
   cpp_bin_float(const Float& f,
                 typename std::enable_if<
                     std::is_same<Float, __float128>::value && ((int)bit_count >= 113)>::type const* = 0)
       : m_data(), m_exponent(0), m_sign(false)
   {
      this->assign_float(f);
   }
   template <class Float>
   explicit cpp_bin_float(const Float& f,
                          typename std::enable_if<
                              std::is_same<Float, __float128>::value && ((int)bit_count < 113)>::type const* = 0)
       : m_data(), m_exponent(0), m_sign(false)
   {
      this->assign_float(f);
   }
#endif
   cpp_bin_float& operator=(const cpp_bin_float& o) noexcept(noexcept(std::declval<rep_type&>() = std::declval<const rep_type&>()))
   {
      m_data     = o.m_data;
      m_exponent = o.m_exponent;
      m_sign     = o.m_sign;
      return *this;
   }

   template <class A, class E, E MinE, E MaxE>
   cpp_bin_float& operator=(const cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>& o) noexcept(noexcept(std::declval<rep_type&>() = std::declval<const rep_type&>()))
   {
      m_data     = o.bits();
      m_sign     = o.sign();
      if (o.exponent() == cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>::exponent_zero)
         m_exponent = exponent_zero;
      else if (o.exponent() == cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>::exponent_nan)
         m_exponent = exponent_nan;
      else if (o.exponent() == cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>::exponent_infinity)
         m_exponent = exponent_infinity;
      else if (o.exponent() > cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent)
      {
         // Overflow:
         exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity;
         bits() = static_cast<limb_type>(0u);
      }
      else if (o.exponent() < cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent)
      {
         // Underflow:
         exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
         bits() = static_cast<limb_type>(0u);
      }
      else
         m_exponent = o.exponent();
      return *this;
   }
   // rvalue copy:
   template <class A, class E, E MinE, E MaxE>
   cpp_bin_float& operator=(cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>&& o) noexcept(noexcept(std::declval<rep_type&>() = std::declval<rep_type&&>()))
   {
      m_data     = std::move(o.bits());
      m_sign     = o.sign();
      if (o.exponent() == cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>::exponent_zero)
         m_exponent = exponent_zero;
      else if (o.exponent() == cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>::exponent_nan)
         m_exponent = exponent_nan;
      else if (o.exponent() == cpp_bin_float<Digits, DigitBase, A, E, MinE, MaxE>::exponent_infinity)
         m_exponent = exponent_infinity;
      else if (o.exponent() > cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent)
      {
         // Overflow:
         exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity;
         bits() = static_cast<limb_type>(0u);
      }
      else if (o.exponent() < cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent)
      {
         // Underflow:
         exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
         bits() = static_cast<limb_type>(0u);
      }
      else
         m_exponent = o.exponent();
      return *this;
   }
   template <unsigned D, digit_base_type B, class A, class E, E MinE, E MaxE>
   cpp_bin_float& operator=(const cpp_bin_float<D, B, A, E, MinE, MaxE>& f)
   {
      switch (eval_fpclassify(f))
      {
      case FP_ZERO:
         m_data     = limb_type(0);
         m_sign     = f.sign();
         m_exponent = exponent_zero;
         break;
      case FP_NAN:
         m_data     = limb_type(0);
         m_sign     = false;
         m_exponent = exponent_nan;
         break;
         ;
      case FP_INFINITE:
         m_data     = limb_type(0);
         m_sign     = f.sign();
         m_exponent = exponent_infinity;
         break;
      default:
         typename cpp_bin_float<D, B, A, E, MinE, MaxE>::rep_type b(f.bits());
         this->exponent() = f.exponent() + (E)bit_count - (E)cpp_bin_float<D, B, A, E, MinE, MaxE>::bit_count;
         this->sign()     = f.sign();
         copy_and_round(*this, b);
      }
      return *this;
   }
#ifdef BOOST_HAS_FLOAT128
   template <class Float>
   typename std::enable_if<
       (number_category<Float>::value == number_kind_floating_point)
           //&& (std::numeric_limits<Float>::digits <= (int)bit_count)
           && ((std::numeric_limits<Float>::radix == 2) || (std::is_same<Float, __float128>::value)),
       cpp_bin_float&>::type
   operator=(const Float& f)
#else
   template <class Float>
   typename std::enable_if<
       (number_category<Float>::value == number_kind_floating_point)
           //&& (std::numeric_limits<Float>::digits <= (int)bit_count)
           && (std::numeric_limits<Float>::radix == 2),
       cpp_bin_float&>::type
   operator=(const Float& f)
#endif
   {
      return assign_float(f);
   }

#ifdef BOOST_HAS_FLOAT128
   template <class Float>
   typename std::enable_if<std::is_same<Float, __float128>::value, cpp_bin_float&>::type assign_float(Float f)
   {
      using default_ops::eval_add;
      using bf_int_type = typename boost::multiprecision::detail::canonical<int, cpp_bin_float>::type;
      if (f == 0)
      {
         m_data     = limb_type(0);
         m_sign     = (signbitq(f) > 0);
         m_exponent = exponent_zero;
         return *this;
      }
      else if (isnanq(f))
      {
         m_data     = limb_type(0);
         m_sign     = false;
         m_exponent = exponent_nan;
         return *this;
      }
      else if (isinfq(f))
      {
         m_data     = limb_type(0);
         m_sign     = (f < 0);
         m_exponent = exponent_infinity;
         return *this;
      }
      if (f < 0)
      {
         *this = -f;
         this->negate();
         return *this;
      }

      using ui_type = typename std::tuple_element<0, unsigned_types>::type;
      m_data     = static_cast<ui_type>(0u);
      m_sign     = false;
      m_exponent = 0;

      constexpr const int bits = sizeof(int) * CHAR_BIT - 1;
      int              e;
      f = frexpq(f, &e);
      while (f)
      {
         f = ldexpq(f, bits);
         e -= bits;
         int ipart = (int)truncq(f);
         f -= ipart;
         m_exponent += bits;
         cpp_bin_float t;
         t = static_cast<bf_int_type>(ipart);
         eval_add(*this, t);
      }
      m_exponent += static_cast<Exponent>(e);
      return *this;
   }
#endif
#ifdef BOOST_HAS_FLOAT128
   template <class Float>
   typename std::enable_if<std::is_floating_point<Float>::value && !std::is_same<Float, __float128>::value, cpp_bin_float&>::type assign_float(Float f)
#else
   template <class Float>
   typename std::enable_if<std::is_floating_point<Float>::value, cpp_bin_float&>::type assign_float(Float f)
#endif
   {
      BOOST_MATH_STD_USING
      using default_ops::eval_add;
      using bf_int_type = typename boost::multiprecision::detail::canonical<int, cpp_bin_float>::type;

      switch ((boost::math::fpclassify)(f))
      {
      case FP_ZERO:
         m_data     = limb_type(0);
         m_sign     = ((boost::math::signbit)(f) > 0);
         m_exponent = exponent_zero;
         return *this;
      case FP_NAN:
         m_data     = limb_type(0);
         m_sign     = false;
         m_exponent = exponent_nan;
         return *this;
      case FP_INFINITE:
         m_data     = limb_type(0);
         m_sign     = (f < 0);
         m_exponent = exponent_infinity;
         return *this;
      }
      if (f < 0)
      {
         *this = -f;
         this->negate();
         return *this;
      }

      using ui_type = typename std::tuple_element<0, unsigned_types>::type;
      m_data     = static_cast<ui_type>(0u);
      m_sign     = false;
      m_exponent = 0;

      constexpr const int bits = sizeof(int) * CHAR_BIT - 1;
      int              e;
      f = frexp(f, &e);
      while (f)
      {
         f = ldexp(f, bits);
         e -= bits;
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
         int ipart = itrunc(f);
#else
         int ipart = static_cast<int>(f);
#endif
         f -= ipart;
         m_exponent += bits;
         cpp_bin_float t;
         t = static_cast<bf_int_type>(ipart);
         eval_add(*this, t);
      }
      m_exponent += static_cast<Exponent>(e);
      return *this;
   }

   template <class Float>
   typename std::enable_if<
       (number_category<Float>::value == number_kind_floating_point) && !std::is_floating_point<Float>::value && (number_category<Float>::value == number_kind_floating_point),
       cpp_bin_float&>::type
   assign_float(Float f)
   {
      BOOST_MATH_STD_USING
      using default_ops::eval_add;
      using default_ops::eval_convert_to;
      using default_ops::eval_get_sign;
      using default_ops::eval_subtract;

      using f_int_type = typename boost::multiprecision::detail::canonical<int, Float>::type        ;
      using bf_int_type = typename boost::multiprecision::detail::canonical<int, cpp_bin_float>::type;

      switch (eval_fpclassify(f))
      {
      case FP_ZERO:
         m_data     = limb_type(0);
         m_sign     = (eval_get_sign(f) > 0);
         m_exponent = exponent_zero;
         return *this;
      case FP_NAN:
         m_data     = limb_type(0);
         m_sign     = false;
         m_exponent = exponent_nan;
         return *this;
      case FP_INFINITE:
         m_data     = limb_type(0);
         m_sign     = eval_get_sign(f) < 0;
         m_exponent = exponent_infinity;
         return *this;
      }
      if (eval_get_sign(f) < 0)
      {
         f.negate();
         assign_float(f);
         this->negate();
         return *this;
      }

      using ui_type = typename std::tuple_element<0, unsigned_types>::type;
      m_data     = static_cast<ui_type>(0u);
      m_sign     = false;
      m_exponent = 0;

      constexpr const int bits = sizeof(int) * CHAR_BIT - 1;
      int              e;
      eval_frexp(f, f, &e);
      while (eval_get_sign(f) != 0)
      {
         eval_ldexp(f, f, bits);
         e -= bits;
         int ipart;
         eval_convert_to(&ipart, f);
         eval_subtract(f, static_cast<f_int_type>(ipart));
         m_exponent += bits;
         eval_add(*this, static_cast<bf_int_type>(ipart));
      }
      m_exponent += e;
      if (m_exponent > max_exponent)
         m_exponent = exponent_infinity;
      if (m_exponent < min_exponent)
      {
         m_data     = limb_type(0u);
         m_exponent = exponent_zero;
         m_sign     = (eval_get_sign(f) > 0);
      }
      else if (eval_get_sign(m_data) == 0)
      {
         m_exponent = exponent_zero;
         m_sign     = (eval_get_sign(f) > 0);
      }
      return *this;
   }
   template <class B, expression_template_option et>
   cpp_bin_float& assign_float(const number<B, et>& f)
   {
      return assign_float(f.backend());
   }
   
   template <class I>
   typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value, cpp_bin_float&>::type operator=(const I& i)
   {
      using default_ops::eval_bit_test;
      if (!i)
      {
         m_data     = static_cast<limb_type>(0);
         m_exponent = exponent_zero;
         m_sign     = false;
      }
      else
      {
         using ui_type = typename boost::multiprecision::detail::make_unsigned<I>::type                                      ;
         ui_type                                                                            fi = static_cast<ui_type>(boost::multiprecision::detail::unsigned_abs(i));
         using ar_type = typename boost::multiprecision::detail::canonical<ui_type, rep_type>::type;
         m_data         = static_cast<ar_type>(fi);
         unsigned shift = msb(fi);
         if (shift >= bit_count)
         {
            m_exponent = static_cast<Exponent>(shift);
            m_data     = static_cast<ar_type>(fi >> (shift + 1 - bit_count));
         }
         else
         {
            m_exponent = static_cast<Exponent>(shift);
            eval_left_shift(m_data, bit_count - shift - 1);
         }
         BOOST_ASSERT(eval_bit_test(m_data, bit_count - 1));
         m_sign = detail::is_negative(i);
      }
      return *this;
   }

   cpp_bin_float& operator=(const char* s);

   void swap(cpp_bin_float& o) noexcept
   {
      m_data.swap(o.m_data);
      std::swap(m_exponent, o.m_exponent);
      std::swap(m_sign, o.m_sign);
   }

   std::string str(std::streamsize dig, std::ios_base::fmtflags f) const;

   void negate()
   {
      if (m_exponent != exponent_nan)
         m_sign = !m_sign;
   }

   int compare(const cpp_bin_float& o) const noexcept
   {
      if (m_sign != o.m_sign)
         return (m_exponent == exponent_zero) && (m_exponent == o.m_exponent) ? 0 : m_sign ? -1 : 1;
      int result;
      if (m_exponent == exponent_nan)
         return -1;
      else if (m_exponent != o.m_exponent)
      {
         if (m_exponent == exponent_zero)
            result = -1;
         else if (o.m_exponent == exponent_zero)
            result = 1;
         else
            result = m_exponent > o.m_exponent ? 1 : -1;
      }
      else
         result = m_data.compare(o.m_data);
      if (m_sign)
         result = -result;
      return result;
   }
   template <class A>
   int compare(const A& o) const noexcept
   {
      cpp_bin_float b;
      b = o;
      return compare(b);
   }

   rep_type&            bits() { return m_data; }
   const rep_type&      bits() const { return m_data; }
   exponent_type&       exponent() { return m_exponent; }
   const exponent_type& exponent() const { return m_exponent; }
   bool&                sign() { return m_sign; }
   const bool&          sign() const { return m_sign; }
   void                 check_invariants()
   {
      using default_ops::eval_bit_test;
      using default_ops::eval_is_zero;
      if ((m_exponent <= max_exponent) && (m_exponent >= min_exponent))
      {
         BOOST_ASSERT(eval_bit_test(m_data, bit_count - 1));
      }
      else
      {
         BOOST_ASSERT(m_exponent > max_exponent);
         BOOST_ASSERT(m_exponent <= exponent_nan);
         BOOST_ASSERT(eval_is_zero(m_data));
      }
   }
   template <class Archive>
   void serialize(Archive& ar, const unsigned int /*version*/)
   {
      ar& boost::make_nvp("data", m_data);
      ar& boost::make_nvp("exponent", m_exponent);
      ar& boost::make_nvp("sign", m_sign);
   }
};

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class Int>
inline void copy_and_round(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, Int& arg, int bits_to_keep = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count)
{
   // Precondition: exponent of res must have been set before this function is called
   // as we may need to adjust it based on how many bits_to_keep in arg are set.
   using default_ops::eval_bit_test;
   using default_ops::eval_get_sign;
   using default_ops::eval_increment;
   using default_ops::eval_left_shift;
   using default_ops::eval_lsb;
   using default_ops::eval_msb;
   using default_ops::eval_right_shift;

   // cancellation may have resulted in arg being all zeros:
   if (eval_get_sign(arg) == 0)
   {
      res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
      res.sign()     = false;
      res.bits()     = static_cast<limb_type>(0u);
      return;
   }
   int msb = eval_msb(arg);
   if (static_cast<int>(bits_to_keep) > msb + 1)
   {
      // Must have had cancellation in subtraction,
      // or be converting from a narrower type, so shift left:
      res.bits() = arg;
      eval_left_shift(res.bits(), bits_to_keep - msb - 1);
      res.exponent() -= static_cast<Exponent>(bits_to_keep - msb - 1);
   }
   else if (static_cast<int>(bits_to_keep) < msb + 1)
   {
      // We have more bits_to_keep than we need, so round as required,
      // first get the rounding bit:
      bool roundup = eval_bit_test(arg, msb - bits_to_keep);
      // Then check for a tie:
      if (roundup && (msb - bits_to_keep == (int)eval_lsb(arg)))
      {
         // Ties round towards even:
         if (!eval_bit_test(arg, msb - bits_to_keep + 1))
            roundup = false;
      }
      // Shift off the bits_to_keep we don't need:
      eval_right_shift(arg, msb - bits_to_keep + 1);
      res.exponent() += static_cast<Exponent>(msb - bits_to_keep + 1);
      if (roundup)
      {
         eval_increment(arg);
         if (bits_to_keep)
         {
            if (eval_bit_test(arg, bits_to_keep))
            {
               // This happens very very rairly, all the bits left after
               // truncation must be 1's and we're rounding up an order of magnitude:
               eval_right_shift(arg, 1u);
               ++res.exponent();
            }
         }
         else
         {
            // We get here when bits_to_keep is zero but we're rounding up,
            // as a result we end up with a single digit that is a 1:
            ++bits_to_keep;
         }
      }
      if (bits_to_keep != cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count)
      {
         // Normalize result when we're rounding to fewer bits than we can hold, only happens in conversions
         // to narrower types:
         eval_left_shift(arg, cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - bits_to_keep);
         res.exponent() -= static_cast<Exponent>(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - bits_to_keep);
      }
      res.bits() = arg;
   }
   else
   {
      res.bits() = arg;
   }
   if (!bits_to_keep && !res.bits().limbs()[0])
   {
      // We're keeping zero bits and did not round up, so result is zero:
      res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
      return;
   }
   // Result must be normalized:
   BOOST_ASSERT(((int)eval_msb(res.bits()) == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1));

   if (res.exponent() > cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent)
   {
      // Overflow:
      res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity;
      res.bits()     = static_cast<limb_type>(0u);
   }
   else if (res.exponent() < cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent)
   {
      // Underflow:
      res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
      res.bits()     = static_cast<limb_type>(0u);
   }
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class BinFloat2, class BinFloat3>
inline void do_eval_add(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const BinFloat2& a, const BinFloat3& b)
{
   if (a.exponent() < b.exponent())
   {
      bool s = a.sign();
      do_eval_add(res, b, a);
      if (res.sign() != s)
         res.negate();
      return;
   }

   using default_ops::eval_add;
   using default_ops::eval_bit_test;

   using exponent_type = typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type;

   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type dt;

   // Special cases first:
   switch (a.exponent())
   {
   case BinFloat2::exponent_zero:
   {
      bool s     = a.sign();
      res        = b;
      res.sign() = s;
      return;
   }
   case BinFloat2::exponent_infinity:
      if (b.exponent() == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan)
         res = b;
      else
         res = a;
      return; // result is still infinite.
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      res = a;
      return; // result is still a NaN.
   }
   switch (b.exponent())
   {
   case BinFloat3::exponent_zero:
      res = a;
      return;
   case BinFloat3::exponent_infinity:
      res = b;
      if (res.sign())
         res.negate();
      return; // result is infinite.
   case BinFloat3::exponent_nan:
      res = b;
      return; // result is a NaN.
   }

   static_assert(boost::integer_traits<exponent_type>::const_max - cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count > cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent, "Exponent range check failed");

   bool s = a.sign();
   dt     = a.bits();
   if (a.exponent() > (int)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count + b.exponent())
   {
      res.exponent() = a.exponent();
   }
   else
   {
      exponent_type e_diff = a.exponent() - b.exponent();
      BOOST_ASSERT(e_diff >= 0);
      eval_left_shift(dt, e_diff);
      res.exponent() = a.exponent() - e_diff;
      eval_add(dt, b.bits());
   }

   copy_and_round(res, dt);
   res.check_invariants();
   if (res.sign() != s)
      res.negate();
}

template <class BinFloat1, class BinFloat2, class BinFloat3>
inline void do_eval_subtract(BinFloat1& res, const BinFloat2& a, const BinFloat3& b)
{
   using default_ops::eval_bit_test;
   using default_ops::eval_decrement;
   using default_ops::eval_subtract;

   typename BinFloat1::double_rep_type dt;

   // Special cases first:
   switch (a.exponent())
   {
   case BinFloat2::exponent_zero:
      if (b.exponent() == BinFloat3::exponent_nan)
         res = std::numeric_limits<number<BinFloat1> >::quiet_NaN().backend();
      else
      {
         bool s = a.sign();
         res    = b;
         if (res.exponent() == BinFloat1::exponent_zero)
            res.sign() = false;
         else if (res.sign() == s)
            res.negate();
      }
      return;
   case BinFloat2::exponent_infinity:
      if ((b.exponent() == BinFloat3::exponent_nan) || (b.exponent() == BinFloat3::exponent_infinity))
         res = std::numeric_limits<number<BinFloat1> >::quiet_NaN().backend();
      else
         res = a;
      return;
   case BinFloat2::exponent_nan:
      res = a;
      return; // result is still a NaN.
   }
   switch (b.exponent())
   {
   case BinFloat3::exponent_zero:
      res = a;
      return;
   case BinFloat3::exponent_infinity:
      res.exponent() = BinFloat1::exponent_infinity;
      res.sign()     = !a.sign();
      res.bits()     = static_cast<limb_type>(0u);
      return; // result is a NaN.
   case BinFloat3::exponent_nan:
      res = b;
      return; // result is still a NaN.
   }

   bool s = a.sign();
   if ((a.exponent() > b.exponent()) || ((a.exponent() == b.exponent()) && a.bits().compare(b.bits()) >= 0))
   {
      dt = a.bits();
      if (a.exponent() <= (int)BinFloat1::bit_count + b.exponent())
      {
         typename BinFloat1::exponent_type e_diff = a.exponent() - b.exponent();
         eval_left_shift(dt, e_diff);
         res.exponent() = a.exponent() - e_diff;
         eval_subtract(dt, b.bits());
      }
      else if (a.exponent() == (int)BinFloat1::bit_count + b.exponent() + 1)
      {
         if ((eval_lsb(a.bits()) == BinFloat1::bit_count - 1)
            && (eval_lsb(b.bits()) != BinFloat1::bit_count - 1))
         {
            eval_left_shift(dt, 1);
            eval_decrement(dt);
            res.exponent() = a.exponent() - 1;
         }
         else
            res.exponent() = a.exponent();
      }
      else
         res.exponent() = a.exponent();
   }
   else
   {
      dt = b.bits();
      if (b.exponent() <= (int)BinFloat1::bit_count + a.exponent())
      {
         typename BinFloat1::exponent_type e_diff = a.exponent() - b.exponent();
         eval_left_shift(dt, -e_diff);
         res.exponent() = b.exponent() + e_diff;
         eval_subtract(dt, a.bits());
      }
      else if (b.exponent() == (int)BinFloat1::bit_count + a.exponent() + 1)
      {
         if ((eval_lsb(a.bits()) != BinFloat1::bit_count - 1)
            && eval_lsb(b.bits()))
         {
            eval_left_shift(dt, 1);
            eval_decrement(dt);
            res.exponent() = b.exponent() - 1;
         }
         else
            res.exponent() = b.exponent();
      }
      else
         res.exponent() = b.exponent();
      s = !s;
   }

   copy_and_round(res, dt);
   if (res.exponent() == BinFloat1::exponent_zero)
      res.sign() = false;
   else if (res.sign() != s)
      res.negate();
   res.check_invariants();
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, 
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2,
   class Allocator3, class Exponent3, Exponent MinE3, Exponent MaxE3>
inline void eval_add(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a, 
   const cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>& b)
{
   if (a.sign() == b.sign())
      do_eval_add(res, a, b);
   else
      do_eval_subtract(res, a, b);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2>
inline void eval_add(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a)
{
   return eval_add(res, res, a);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2,
   class Allocator3, class Exponent3, Exponent MinE3, Exponent MaxE3>
inline void eval_subtract(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a, 
   const cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>& b)
{
   if (a.sign() != b.sign())
      do_eval_add(res, a, b);
   else
      do_eval_subtract(res, a, b);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2>
inline void eval_subtract(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a)
{
   return eval_subtract(res, res, a);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, 
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2, 
   class Allocator3, class Exponent3, Exponent MinE3, Exponent MaxE3>
inline void eval_multiply(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a, 
   const cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>& b)
{
   using default_ops::eval_bit_test;
   using default_ops::eval_multiply;

   // Special cases first:
   switch (a.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_zero:
   {
      if (b.exponent() == cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_nan)
         res = b;
      else if (b.exponent() == cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_infinity)
         res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
      else
      {
         bool s     = a.sign() != b.sign();
         res        = a;
         res.sign() = s;
      }
      return;
   }
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_infinity:
      switch (b.exponent())
      {
      case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_zero:
         res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
         break;
      case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_nan:
         res = b;
         break;
      default:
         bool s     = a.sign() != b.sign();
         res        = a;
         res.sign() = s;
         break;
      }
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_nan:
      res = a;
      return;
   }
   if (b.exponent() > cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::max_exponent)
   {
      bool s     = a.sign() != b.sign();
      res        = b;
      res.sign() = s;
      return;
   }
   if ((a.exponent() > 0) && (b.exponent() > 0))
   {
      if (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent + 2 - a.exponent() < b.exponent())
      {
         // We will certainly overflow:
         bool s         = a.sign() != b.sign();
         res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity;
         res.sign()     = s;
         res.bits()     = static_cast<limb_type>(0u);
         return;
      }
   }
   if ((a.exponent() < 0) && (b.exponent() < 0))
   {
      if (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent - 2 - a.exponent() > b.exponent())
      {
         // We will certainly underflow:
         res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
         res.sign()     = a.sign() != b.sign();
         res.bits()     = static_cast<limb_type>(0u);
         return;
      }
   }

   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type dt;
   eval_multiply(dt, a.bits(), b.bits());
   res.exponent() = a.exponent() + b.exponent() - (Exponent)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count + 1;
   copy_and_round(res, dt);
   res.check_invariants();
   res.sign() = a.sign() != b.sign();
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2>
inline void eval_multiply(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a)
{
   eval_multiply(res, res, a);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2, class U>
inline typename std::enable_if<boost::multiprecision::detail::is_unsigned<U>::value>::type eval_multiply(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a, const U& b)
{
   using default_ops::eval_bit_test;
   using default_ops::eval_multiply;

   // Special cases first:
   switch (a.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_zero:
   {
      bool s     = a.sign();
      res        = a;
      res.sign() = s;
      return;
   }
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_infinity:
      if (b == 0)
         res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
      else
         res = a;
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_nan:
      res = a;
      return;
   }

   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type                                                                     dt;
   using canon_ui_type = typename boost::multiprecision::detail::canonical<U, typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type>::type;
   eval_multiply(dt, a.bits(), static_cast<canon_ui_type>(b));
   res.exponent() = a.exponent();
   copy_and_round(res, dt);
   res.check_invariants();
   res.sign() = a.sign();
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class U>
inline typename std::enable_if<boost::multiprecision::detail::is_unsigned<U>::value>::type eval_multiply(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const U& b)
{
   eval_multiply(res, res, b);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2, class S>
inline typename std::enable_if<boost::multiprecision::detail::is_signed<S>::value && boost::multiprecision::detail::is_integral<S>::value>::type eval_multiply(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& a, const S& b)
{
   using ui_type = typename boost::multiprecision::detail::make_unsigned<S>::type;
   eval_multiply(res, a, static_cast<ui_type>(boost::multiprecision::detail::unsigned_abs(b)));
   if (b < 0)
      res.negate();
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class S>
inline typename std::enable_if<boost::multiprecision::detail::is_signed<S>::value && boost::multiprecision::detail::is_integral<S>::value>::type eval_multiply(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const S& b)
{
   eval_multiply(res, res, b);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2,
   class Allocator3, class Exponent3, Exponent MinE3, Exponent MaxE3>
inline void eval_divide(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& u, 
   const cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>& v)
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 6326) // comparison of two constants
#endif
   using default_ops::eval_bit_test;
   using default_ops::eval_get_sign;
   using default_ops::eval_increment;
   using default_ops::eval_qr;
   using default_ops::eval_subtract;

   //
   // Special cases first:
   //
   switch (u.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_zero:
   {
      switch (v.exponent())
      {
      case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_zero:
      case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_nan:
         res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
         return;
      }
      bool s     = u.sign() != v.sign();
      res        = u;
      res.sign() = s;
      return;
   }
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_infinity:
   {
      switch (v.exponent())
      {
      case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_infinity:
      case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_nan:
         res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
         return;
      }
      bool s     = u.sign() != v.sign();
      res        = u;
      res.sign() = s;
      return;
   }
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_nan:
      res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
      return;
   }
   switch (v.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_zero:
   {
      bool s     = u.sign() != v.sign();
      res        = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::infinity().backend();
      res.sign() = s;
      return;
   }
   case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_infinity:
      res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
      res.bits()     = limb_type(0);
      res.sign()     = u.sign() != v.sign();
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator3, Exponent3, MinE3, MaxE3>::exponent_nan:
      res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
      return;
   }

   // We can scale u and v so that both are integers, then perform integer
   // division to obtain quotient q and remainder r, such that:
   //
   // q * v + r = u
   //
   // and hense:
   //
   // q + r/v = u/v
   //
   // From this, assuming q has cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count
   // bits we only need to determine whether
   // r/v is less than, equal to, or greater than 0.5 to determine rounding -
   // this we can do with a shift and comparison.
   //
   // We can set the exponent and sign of the result up front:
   //
   if ((v.exponent() < 0) && (u.exponent() > 0))
   {
      // Check for overflow:
      if (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent + v.exponent() < u.exponent() - 1)
      {
         res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity;
         res.sign()     = u.sign() != v.sign();
         res.bits()     = static_cast<limb_type>(0u);
         return;
      }
   }
   else if ((v.exponent() > 0) && (u.exponent() < 0))
   {
      // Check for underflow:
      if (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent + v.exponent() > u.exponent())
      {
         // We will certainly underflow:
         res.exponent() = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
         res.sign()     = u.sign() != v.sign();
         res.bits()     = static_cast<limb_type>(0u);
         return;
      }
   }
   res.exponent() = u.exponent() - v.exponent() - 1;
   res.sign()     = u.sign() != v.sign();
   //
   // Now get the quotient and remainder:
   //
   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type t(u.bits()), t2(v.bits()), q, r;
   eval_left_shift(t, cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count);
   eval_qr(t, t2, q, r);
   //
   // We now have either "cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count"
   // or "cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count+1" significant
   // bits in q.
   //
   constexpr const unsigned limb_bits = sizeof(limb_type) * CHAR_BIT;
   if (eval_bit_test(q, cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count))
   {
      //
      // OK we have cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count+1 bits,
      // so we already have rounding info,
      // we just need to changes things if the last bit is 1 and either the
      // remainder is non-zero (ie we do not have a tie) or the quotient would
      // be odd if it were shifted to the correct number of bits (ie a tiebreak).
      //
      BOOST_ASSERT((eval_msb(q) == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count));
      if ((q.limbs()[0] & 1u) && (eval_get_sign(r) || (q.limbs()[0] & 2u)))
      {
         eval_increment(q);
      }
   }
   else
   {
      //
      // We have exactly "cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count" bits in q.
      // Get rounding info, which we can get by comparing 2r with v.
      // We want to call copy_and_round to handle rounding and general cleanup,
      // so we'll left shift q and add some fake digits on the end to represent
      // how we'll be rounding.
      //
      BOOST_ASSERT((eval_msb(q) == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1));
      constexpr const unsigned lshift = (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count < limb_bits) ? 2 : limb_bits;
      eval_left_shift(q, lshift);
      res.exponent() -= lshift;
      eval_left_shift(r, 1u);
      int c = r.compare(v.bits());
      if (c == 0)
         q.limbs()[0] |= static_cast<limb_type>(1u) << (lshift - 1);
      else if (c > 0)
         q.limbs()[0] |= (static_cast<limb_type>(1u) << (lshift - 1)) + static_cast<limb_type>(1u);
   }
   copy_and_round(res, q);
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2>
inline void eval_divide(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& arg)
{
   eval_divide(res, res, arg);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2, class U>
inline typename std::enable_if<boost::multiprecision::detail::is_unsigned<U>::value>::type eval_divide(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& u, const U& v)
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 6326) // comparison of two constants
#endif
   using default_ops::eval_bit_test;
   using default_ops::eval_get_sign;
   using default_ops::eval_increment;
   using default_ops::eval_qr;
   using default_ops::eval_subtract;

   //
   // Special cases first:
   //
   switch (u.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_zero:
   {
      if (v == 0)
      {
         res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
         return;
      }
      bool s     = u.sign() != (v < 0);
      res        = u;
      res.sign() = s;
      return;
   }
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_infinity:
      res = u;
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>::exponent_nan:
      res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
      return;
   }
   if (v == 0)
   {
      bool s     = u.sign();
      res        = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::infinity().backend();
      res.sign() = s;
      return;
   }

   // We can scale u and v so that both are integers, then perform integer
   // division to obtain quotient q and remainder r, such that:
   //
   // q * v + r = u
   //
   // and hense:
   //
   // q + r/v = u/v
   //
   // From this, assuming q has "cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count" cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count, we only need to determine whether
   // r/v is less than, equal to, or greater than 0.5 to determine rounding -
   // this we can do with a shift and comparison.
   //
   // We can set the exponent and sign of the result up front:
   //
   int gb         = msb(v);
   res.exponent() = u.exponent() - static_cast<Exponent>(gb) - static_cast<Exponent>(1);
   res.sign()     = u.sign();
   //
   // Now get the quotient and remainder:
   //
   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type t(u.bits()), q, r;
   eval_left_shift(t, gb + 1);
   eval_qr(t, number<typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type>::canonical_value(v), q, r);
   //
   // We now have either "cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count" or "cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count+1" significant cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count in q.
   //
   constexpr const unsigned limb_bits = sizeof(limb_type) * CHAR_BIT;
   if (eval_bit_test(q, cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count))
   {
      //
      // OK we have cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count+1 cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count, so we already have rounding info,
      // we just need to changes things if the last bit is 1 and the
      // remainder is non-zero (ie we do not have a tie).
      //
      BOOST_ASSERT((eval_msb(q) == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count));
      if ((q.limbs()[0] & 1u) && eval_get_sign(r))
      {
         eval_increment(q);
      }
   }
   else
   {
      //
      // We have exactly "cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count" cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count in q.
      // Get rounding info, which we can get by comparing 2r with v.
      // We want to call copy_and_round to handle rounding and general cleanup,
      // so we'll left shift q and add some fake cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count on the end to represent
      // how we'll be rounding.
      //
      BOOST_ASSERT((eval_msb(q) == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1));
      constexpr const unsigned lshift = cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count < limb_bits ? 2 : limb_bits;
      eval_left_shift(q, lshift);
      res.exponent() -= lshift;
      eval_left_shift(r, 1u);
      int c = r.compare(number<typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type>::canonical_value(v));
      if (c == 0)
         q.limbs()[0] |= static_cast<limb_type>(1u) << (lshift - 1);
      else if (c > 0)
         q.limbs()[0] |= (static_cast<limb_type>(1u) << (lshift - 1)) + static_cast<limb_type>(1u);
   }
   copy_and_round(res, q);
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class U>
inline typename std::enable_if<boost::multiprecision::detail::is_unsigned<U>::value>::type eval_divide(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const U& v)
{
   eval_divide(res, res, v);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2, class S>
inline typename std::enable_if<boost::multiprecision::detail::is_signed<S>::value && boost::multiprecision::detail::is_integral<S>::value>::type eval_divide(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, 
   const cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2>& u, const S& v)
{
   using ui_type = typename boost::multiprecision::detail::make_unsigned<S>::type;
   eval_divide(res, u, static_cast<ui_type>(boost::multiprecision::detail::unsigned_abs(v)));
   if (v < 0)
      res.negate();
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class S>
inline typename std::enable_if<boost::multiprecision::detail::is_signed<S>::value && boost::multiprecision::detail::is_integral<S>::value>::type eval_divide(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const S& v)
{
   eval_divide(res, res, v);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline int eval_get_sign(const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   return arg.exponent() == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero ? 0 : arg.sign() ? -1 : 1;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline bool eval_is_zero(const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   return arg.exponent() == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline bool eval_eq(const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& a, cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& b)
{
   if (a.exponent() == b.exponent())
   {
      if (a.exponent() == cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero)
         return true;
      return (a.sign() == b.sign()) && (a.bits().compare(b.bits()) == 0) && (a.exponent() != cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan);
   }
   return false;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_convert_to(boost::long_long_type* res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
      *res = 0;
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert NaN to integer."));
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      *res = (std::numeric_limits<boost::long_long_type>::max)();
      if (arg.sign())
         *res = -*res;
      return;
   }
   using shift_type = typename std::conditional<sizeof(typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type) < sizeof(int), int, typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type>::type;
   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::rep_type                                                                                                                                                              man(arg.bits());
   shift_type                                                                                                                                                                                                                                        shift = (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1 - arg.exponent();
   if (shift > (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1)
   {
      *res = 0;
      return;
   }
   if (arg.sign() && (arg.compare((std::numeric_limits<boost::long_long_type>::min)()) <= 0))
   {
      *res = (std::numeric_limits<boost::long_long_type>::min)();
      return;
   }
   else if (!arg.sign() && (arg.compare((std::numeric_limits<boost::long_long_type>::max)()) >= 0))
   {
      *res = (std::numeric_limits<boost::long_long_type>::max)();
      return;
   }

   if (shift < 0)
   {
      if (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - shift <= std::numeric_limits<boost::long_long_type>::digits)
      {
         // We have more bits in long_long_type than the float, so it's OK to left shift:
         eval_convert_to(res, man);
         *res <<= -shift;
      }
      else
      {
         *res = (std::numeric_limits<boost::long_long_type>::max)();
         return;
      }
   }
   else
   {
      eval_right_shift(man, shift);
      eval_convert_to(res, man);
   }
   if (arg.sign())
   {
      *res = -*res;
   }
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_convert_to(boost::ulong_long_type* res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
      *res = 0;
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert NaN to integer."));
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      *res = (std::numeric_limits<boost::ulong_long_type>::max)();
      return;
   }
   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::rep_type                                                                                                                                                              man(arg.bits());
   using shift_type = typename std::conditional<sizeof(typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type) < sizeof(int), int, typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type>::type;
   shift_type                                                                                                                                                                                                                                        shift = (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1 - arg.exponent();
   if (shift > (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1)
   {
      *res = 0;
      return;
   }
   else if (shift < 0)
   {
      if (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - shift <= std::numeric_limits<boost::ulong_long_type>::digits)
      {
         // We have more bits in ulong_long_type than the float, so it's OK to left shift:
         eval_convert_to(res, man);
         *res <<= -shift;
         return;
      }
      *res = (std::numeric_limits<boost::ulong_long_type>::max)();
      return;
   }
   eval_right_shift(man, shift);
   eval_convert_to(res, man);
}

template <class Float, unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline typename std::enable_if<std::is_floating_point<Float>::value>::type eval_convert_to(Float* res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& original_arg)
{
   using conv_type = cpp_bin_float<std::numeric_limits<Float>::digits, digit_base_2, void, Exponent, MinE, MaxE>;
   using common_exp_type = typename std::common_type<typename conv_type::exponent_type, int>::type                    ;
   //
   // Special cases first:
   //
   switch (original_arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
      *res = 0;
      if (original_arg.sign())
         *res = -*res;
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      *res = std::numeric_limits<Float>::quiet_NaN();
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      *res = (std::numeric_limits<Float>::infinity)();
      if (original_arg.sign())
         *res = -*res;
      return;
   }
   //
   // Check for super large exponent that must be converted to infinity:
   //
   if (original_arg.exponent() > std::numeric_limits<Float>::max_exponent)
   {
      *res = std::numeric_limits<Float>::has_infinity ? std::numeric_limits<Float>::infinity() : (std::numeric_limits<Float>::max)();
      if (original_arg.sign())
         *res = -*res;
      return;
   }
   //
   // Figure out how many digits we will have in our result,
   // allowing for a possibly denormalized result:
   //
   common_exp_type digits_to_round_to = std::numeric_limits<Float>::digits;
   if (original_arg.exponent() < std::numeric_limits<Float>::min_exponent - 1)
   {
      common_exp_type diff = original_arg.exponent();
      diff -= std::numeric_limits<Float>::min_exponent - 1;
      digits_to_round_to += diff;
   }
   if (digits_to_round_to < 0)
   {
      // Result must be zero:
      *res = 0;
      if (original_arg.sign())
         *res = -*res;
      return;
   }
   //
   // Perform rounding first, then afterwards extract the digits:
   //
   cpp_bin_float<std::numeric_limits<Float>::digits, digit_base_2, Allocator, Exponent, MinE, MaxE> arg;
   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::rep_type             bits(original_arg.bits());
   arg.exponent() = original_arg.exponent();
   copy_and_round(arg, bits, (int)digits_to_round_to);
   common_exp_type e = arg.exponent();
   e -= cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1;
   constexpr const unsigned limbs_needed      = std::numeric_limits<Float>::digits / (sizeof(*arg.bits().limbs()) * CHAR_BIT) + (std::numeric_limits<Float>::digits % (sizeof(*arg.bits().limbs()) * CHAR_BIT) ? 1 : 0);
   unsigned              first_limb_needed = arg.bits().size() - limbs_needed;
   *res                                    = 0;
   e += first_limb_needed * sizeof(*arg.bits().limbs()) * CHAR_BIT;
   while (first_limb_needed < arg.bits().size())
   {
      *res += std::ldexp(static_cast<Float>(arg.bits().limbs()[first_limb_needed]), static_cast<int>(e));
      ++first_limb_needed;
      e += sizeof(*arg.bits().limbs()) * CHAR_BIT;
   }
   if (original_arg.sign())
      *res = -*res;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_frexp(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg, Exponent* e)
{
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      *e  = 0;
      res = arg;
      return;
   }
   res            = arg;
   *e             = arg.exponent() + 1;
   res.exponent() = -1;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class I>
inline void eval_frexp(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg, I* pe)
{
   Exponent e;
   eval_frexp(res, arg, &e);
   if ((e > (std::numeric_limits<I>::max)()) || (e < (std::numeric_limits<I>::min)()))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Exponent was outside of the range of the argument type to frexp."));
   }
   *pe = static_cast<I>(e);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_ldexp(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg, Exponent e)
{
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      res = arg;
      return;
   }
   if ((e > 0) && (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent - e < arg.exponent()))
   {
      // Overflow:
      res        = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::infinity().backend();
      res.sign() = arg.sign();
   }
   else if ((e < 0) && (cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent - e > arg.exponent()))
   {
      // Underflow:
      res = limb_type(0);
   }
   else
   {
      res = arg;
      res.exponent() += e;
   }
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class I>
inline typename std::enable_if<boost::multiprecision::detail::is_unsigned<I>::value>::type eval_ldexp(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg, I e)
{
   using si_type = typename boost::multiprecision::detail::make_signed<I>::type;
   if (e > static_cast<I>((std::numeric_limits<si_type>::max)()))
      res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::infinity().backend();
   else
      eval_ldexp(res, arg, static_cast<si_type>(e));
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class I>
inline typename std::enable_if<boost::multiprecision::detail::is_signed<I>::value && boost::multiprecision::detail::is_integral<I>::value>::type eval_ldexp(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg, I e)
{
   if ((e > (std::numeric_limits<Exponent>::max)()) || (e < (std::numeric_limits<Exponent>::min)()))
   {
      res = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::infinity().backend();
      if (e < 0)
         res.negate();
   }
   else
      eval_ldexp(res, arg, static_cast<Exponent>(e));
}

/*
* Sign manipulation
*/

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   unsigned Digits2, digit_base_type DigitBase2, class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2>
inline void eval_abs(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits2, DigitBase2, Allocator2, Exponent2, MinE2, MaxE2>& arg)
{
   res        = arg;
   res.sign() = false;
}
template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_abs(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   res        = arg;
   res.sign() = false;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE,
   unsigned Digits2, digit_base_type DigitBase2, class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2>
inline void eval_fabs(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits2, DigitBase2, Allocator2, Exponent2, MinE2, MaxE2>& arg)
{
   res        = arg;
   res.sign() = false;
}
template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_fabs(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   res        = arg;
   res.sign() = false;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline int eval_fpclassify(const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
      return FP_ZERO;
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      return FP_INFINITE;
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      return FP_NAN;
   }
   return FP_NORMAL;
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_sqrt(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   using default_ops::eval_bit_test;
   using default_ops::eval_increment;
   using default_ops::eval_integer_sqrt;
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      errno = EDOM;
      // fallthrough...
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
      res = arg;
      return;
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      if (arg.sign())
      {
         res   = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
         errno = EDOM;
      }
      else
         res = arg;
      return;
   }
   if (arg.sign())
   {
      res   = std::numeric_limits<number<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > >::quiet_NaN().backend();
      errno = EDOM;
      return;
   }

   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::double_rep_type t(arg.bits()), r, s;
   eval_left_shift(t, arg.exponent() & 1 ? cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count : cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1);
   eval_integer_sqrt(s, r, t);

   if (!eval_bit_test(s, cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count))
   {
      // We have exactly the right number of cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count in the result, round as required:
      if (s.compare(r) < 0)
      {
         eval_increment(s);
      }
   }
   typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type ae = arg.exponent();
   res.exponent()                                                                               = ae / 2;
   res.sign() = false;
   if ((ae & 1) && (ae < 0))
      --res.exponent();
   copy_and_round(res, s);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_floor(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   using default_ops::eval_increment;
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      errno = EDOM;
      // fallthrough...
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      res = arg;
      return;
   }
   using shift_type = typename std::conditional<sizeof(typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type) < sizeof(int), int, typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type>::type;
   shift_type                                                                                                                                                                                                                                        shift =
       (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - arg.exponent() - 1;
   if ((arg.exponent() > (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent) || (shift <= 0))
   {
      // Either arg is already an integer, or a special value:
      res = arg;
      return;
   }
   if (shift >= (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count)
   {
      res = static_cast<signed_limb_type>(arg.sign() ? -1 : 0);
      return;
   }
   bool fractional = (shift_type)eval_lsb(arg.bits()) < shift;
   res             = arg;
   eval_right_shift(res.bits(), shift);
   if (fractional && res.sign())
   {
      eval_increment(res.bits());
      if (eval_msb(res.bits()) != cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1 - shift)
      {
         // Must have extended result by one bit in the increment:
         --shift;
         ++res.exponent();
      }
   }
   eval_left_shift(res.bits(), shift);
}

template <unsigned Digits, digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
inline void eval_ceil(cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& res, const cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>& arg)
{
   using default_ops::eval_increment;
   switch (arg.exponent())
   {
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity:
      errno = EDOM;
      // fallthrough...
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_zero:
   case cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan:
      res = arg;
      return;
   }
   using shift_type = typename std::conditional<sizeof(typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type) < sizeof(int), int, typename cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type>::type;
   shift_type                                                                                                                                                                                                                                        shift = (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - arg.exponent() - 1;
   if ((arg.exponent() > (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent) || (shift <= 0))
   {
      // Either arg is already an integer, or a special value:
      res = arg;
      return;
   }
   if (shift >= (shift_type)cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count)
   {
      bool s     = arg.sign(); // takes care of signed zeros
      res        = static_cast<signed_limb_type>(arg.sign() ? 0 : 1);
      res.sign() = s;
      return;
   }
   bool fractional = (shift_type)eval_lsb(arg.bits()) < shift;
   res             = arg;
   eval_right_shift(res.bits(), shift);
   if (fractional && !res.sign())
   {
      eval_increment(res.bits());
      if (eval_msb(res.bits()) != cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1 - shift)
      {
         // Must have extended result by one bit in the increment:
         --shift;
         ++res.exponent();
      }
   }
   eval_left_shift(res.bits(), shift);
}

template <unsigned D1, backends::digit_base_type B1, class A1, class E1, E1 M1, E1 M2>
int eval_signbit(const cpp_bin_float<D1, B1, A1, E1, M1, M2>& val)
{
   return val.sign();
}

template <unsigned D1, backends::digit_base_type B1, class A1, class E1, E1 M1, E1 M2>
inline std::size_t hash_value(const cpp_bin_float<D1, B1, A1, E1, M1, M2>& val)
{
   std::size_t result = hash_value(val.bits());
   boost::multiprecision::detail::hash_combine(result, val.exponent(), val.sign());
   return result;
}

} // namespace backends

namespace detail {

template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinExponent, Exponent MaxExponent>
struct transcendental_reduction_type<boost::multiprecision::backends::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinExponent, MaxExponent> >
{
   //
   // The type used for trigonometric reduction needs 3 times the precision of the base type.
   // This is double the precision of the original type, plus the largest exponent supported.
   // As a practical measure the largest argument supported is 1/eps, as supporting larger
   // arguments requires the division of argument by PI/2 to also be done at higher precision,
   // otherwise the result (an integer) can not be represented exactly.
   // 
   // See ARGUMENT REDUCTION FOR HUGE ARGUMENTS. K C Ng.
   //
   using type = boost::multiprecision::backends::cpp_bin_float<
       boost::multiprecision::backends::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinExponent, MaxExponent>::bit_count * 3, 
       boost::multiprecision::backends::digit_base_2, 
       Allocator, Exponent, MinExponent, MaxExponent>;
};

} // namespace detail

template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Exponent, Exponent MinE, Exponent MaxE, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates>
    copysign BOOST_PREVENT_MACRO_SUBSTITUTION(
        const boost::multiprecision::number<boost::multiprecision::backends::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates>& a,
        const boost::multiprecision::number<boost::multiprecision::backends::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates>& b)
{
   boost::multiprecision::number<boost::multiprecision::backends::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> res(a);
   res.backend().sign() = b.backend().sign();
   return res;
}

using backends::cpp_bin_float;
using backends::digit_base_10;
using backends::digit_base_2;

template <unsigned Digits, backends::digit_base_type DigitBase, class Exponent, Exponent MinE, Exponent MaxE, class Allocator>
struct number_category<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> > : public std::integral_constant<int, boost::multiprecision::number_kind_floating_point>
{};

template <unsigned Digits, backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE>
struct expression_template_default<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE> >
{
   static constexpr const expression_template_option value = std::is_void<Allocator>::value ? et_off : et_on;
};

template <unsigned Digits, backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, class Allocator2, class Exponent2, Exponent MinE2, Exponent MaxE2>
struct is_equivalent_number_type<cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, cpp_bin_float<Digits, DigitBase, Allocator2, Exponent2, MinE2, MaxE2> >
   : public std::integral_constant<bool, true> {};

using cpp_bin_float_50 = number<backends::cpp_bin_float<50> > ;
using cpp_bin_float_100 = number<backends::cpp_bin_float<100> >;

using cpp_bin_float_single = number<backends::cpp_bin_float<24, backends::digit_base_2, void, std::int16_t, -126, 127>, et_off>       ;
using cpp_bin_float_double = number<backends::cpp_bin_float<53, backends::digit_base_2, void, std::int16_t, -1022, 1023>, et_off>     ;
using cpp_bin_float_double_extended = number<backends::cpp_bin_float<64, backends::digit_base_2, void, std::int16_t, -16382, 16383>, et_off>   ;
using cpp_bin_float_quad = number<backends::cpp_bin_float<113, backends::digit_base_2, void, std::int16_t, -16382, 16383>, et_off>  ;
using cpp_bin_float_oct = number<backends::cpp_bin_float<237, backends::digit_base_2, void, std::int32_t, -262142, 262143>, et_off>;

} // namespace multiprecision

namespace math {

using boost::multiprecision::copysign;
using boost::multiprecision::signbit;

} // namespace math

} // namespace boost

#include <boost/multiprecision/cpp_bin_float/io.hpp>
#include <boost/multiprecision/cpp_bin_float/transcendental.hpp>

namespace std {

//
// numeric_limits [partial] specializations for the types declared in this header:
//
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
class numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >
{
   using number_type = boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates>;

 public:
   static constexpr bool is_specialized = true;
   static number_type(min)()
   {
      static std::pair<bool, number_type> value;
      if (!value.first)
      {
         value.first = true;
         using ui_type = typename std::tuple_element<0, typename number_type::backend_type::unsigned_types>::type;
         value.second.backend()            = ui_type(1u);
         value.second.backend().exponent() = boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent;
      }
      return value.second;
   }
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif
   static number_type(max)()
   {
      static std::pair<bool, number_type> value;
      if (!value.first)
      {
         value.first = true;
         BOOST_IF_CONSTEXPR(std::is_void<Allocator>::value)
            eval_complement(value.second.backend().bits(), value.second.backend().bits());
         else
         {
            // We jump through hoops here using the backend type directly just to keep VC12 happy
            // (ie compiler workaround, for very strange compiler bug):
            using boost::multiprecision::default_ops::eval_add;
            using boost::multiprecision::default_ops::eval_decrement;
            using boost::multiprecision::default_ops::eval_left_shift;
            using int_backend_type = typename number_type::backend_type::rep_type                               ;
            using ui_type = typename std::tuple_element<0, typename int_backend_type::unsigned_types>::type;
            int_backend_type                                                                    i;
            i = ui_type(1u);
            eval_left_shift(i, boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count - 1);
            int_backend_type j(i);
            eval_decrement(i);
            eval_add(j, i);
            value.second.backend().bits() = j;
         }
         value.second.backend().exponent() = boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent;
      }
      return value.second;
   }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
   static constexpr number_type lowest()
   {
      return -(max)();
   }
   static constexpr int digits   = boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::bit_count;
   static constexpr int digits10 = boost::multiprecision::detail::calc_digits10<digits>::value;
   // Is this really correct???
   static constexpr int  max_digits10 = boost::multiprecision::detail::calc_max_digits10<digits>::value;
   static constexpr bool is_signed    = true;
   static constexpr bool is_integer   = false;
   static constexpr bool is_exact     = false;
   static constexpr int  radix        = 2;
   static number_type          epsilon()
   {
      static std::pair<bool, number_type> value;
      if (!value.first)
      {
         // We jump through hoops here just to keep VC12 happy (ie compiler workaround, for very strange compiler bug):
         using ui_type = typename std::tuple_element<0, typename number_type::backend_type::unsigned_types>::type;
         value.first            = true;
         value.second.backend() = ui_type(1u);
         value.second           = ldexp(value.second, 1 - (int)digits);
      }
      return value.second;
   }
   // What value should this be????
   static number_type round_error()
   {
      // returns 0.5
      static std::pair<bool, number_type> value;
      if (!value.first)
      {
         value.first = true;
         // We jump through hoops here just to keep VC12 happy (ie compiler workaround, for very strange compiler bug):
         using ui_type = typename std::tuple_element<0, typename number_type::backend_type::unsigned_types>::type;
         value.second.backend() = ui_type(1u);
         value.second           = ldexp(value.second, -1);
      }
      return value.second;
   }
   static constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type min_exponent      = boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::min_exponent;
   static constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type min_exponent10    = (min_exponent / 1000) * 301L;
   static constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type max_exponent      = boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::max_exponent;
   static constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type max_exponent10    = (max_exponent / 1000) * 301L;
   static constexpr bool                                                                                                             has_infinity      = true;
   static constexpr bool                                                                                                             has_quiet_NaN     = true;
   static constexpr bool                                                                                                             has_signaling_NaN = false;
   static constexpr float_denorm_style has_denorm                                                                                                      = denorm_absent;
   static constexpr bool               has_denorm_loss                                                                                                 = false;
   static number_type                        infinity()
   {
      static std::pair<bool, number_type> value;
      if (!value.first)
      {
         value.first                       = true;
         value.second.backend().exponent() = boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_infinity;
      }
      return value.second;
   }
   static number_type quiet_NaN()
   {
      static std::pair<bool, number_type> value;
      if (!value.first)
      {
         value.first                       = true;
         value.second.backend().exponent() = boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_nan;
      }
      return value.second;
   }
   static constexpr number_type signaling_NaN()
   {
      return number_type(0);
   }
   static constexpr number_type denorm_min() { return number_type(0); }
   static constexpr bool        is_iec559         = false;
   static constexpr bool        is_bounded        = true;
   static constexpr bool        is_modulo         = false;
   static constexpr bool        traps             = true;
   static constexpr bool        tinyness_before   = false;
   static constexpr float_round_style round_style = round_to_nearest;
};

template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::digits;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::digits10;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::max_digits10;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::is_signed;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::is_integer;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::is_exact;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::radix;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::min_exponent;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::min_exponent10;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::max_exponent;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr typename boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>::exponent_type numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::max_exponent10;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::has_infinity;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::has_quiet_NaN;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::has_signaling_NaN;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_denorm_style numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::has_denorm;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::has_denorm_loss;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::is_iec559;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::is_bounded;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::is_modulo;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::traps;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::tinyness_before;
template <unsigned Digits, boost::multiprecision::backends::digit_base_type DigitBase, class Allocator, class Exponent, Exponent MinE, Exponent MaxE, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_round_style numeric_limits<boost::multiprecision::number<boost::multiprecision::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinE, MaxE>, ExpressionTemplates> >::round_style;


} // namespace std

#endif
