///////////////////////////////////////////////////////////////
//  Copyright 2012-2020 John Maddock.
//  Copyright 2020 Madhur Chauhan.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//   https://www.boost.org/LICENSE_1_0.txt)
//
// Comparison operators for cpp_int_backend:
//
#ifndef BOOST_MP_CPP_INT_MISC_HPP
#define BOOST_MP_CPP_INT_MISC_HPP

#include <boost/multiprecision/detail/constexpr.hpp>
#include <boost/multiprecision/detail/bitscan.hpp> // lsb etc
#include <boost/multiprecision/detail/hash.hpp>
#include <boost/integer/common_factor_rt.hpp>      // gcd/lcm
#include <numeric> // std::gcd

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4702)
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#endif

namespace boost { namespace multiprecision { namespace backends {

template <class T, bool has_limits = std::numeric_limits<T>::is_specialized>
struct numeric_limits_workaround : public std::numeric_limits<T>
{
};
template <class R>
struct numeric_limits_workaround<R, false>
{
   static constexpr unsigned digits = ~static_cast<R>(0) < 0 ? sizeof(R) * CHAR_BIT - 1 : sizeof(R) * CHAR_BIT;
   static constexpr R (min)(){ return (static_cast<R>(-1) < 0) ? static_cast<R>(1) << digits : 0; }
   static constexpr R (max)() { return (static_cast<R>(-1) < 0) ? ~(static_cast<R>(1) << digits) : ~static_cast<R>(0); }
};

template <class R, class CppInt>
BOOST_MP_CXX14_CONSTEXPR void check_in_range(const CppInt& val, const std::integral_constant<int, checked>&)
{
   using cast_type = typename boost::multiprecision::detail::canonical<R, CppInt>::type;

   if (val.sign())
   {
      BOOST_IF_CONSTEXPR (boost::multiprecision::detail::is_signed<R>::value == false)
         BOOST_THROW_EXCEPTION(std::range_error("Attempt to assign a negative value to an unsigned type."));
      if (val.compare(static_cast<cast_type>((numeric_limits_workaround<R>::min)())) < 0)
         BOOST_THROW_EXCEPTION(std::overflow_error("Could not convert to the target type - -value is out of range."));
   }
   else
   {
      if (val.compare(static_cast<cast_type>((numeric_limits_workaround<R>::max)())) > 0)
         BOOST_THROW_EXCEPTION(std::overflow_error("Could not convert to the target type - -value is out of range."));
   }
}
template <class R, class CppInt>
inline BOOST_MP_CXX14_CONSTEXPR void check_in_range(const CppInt& /*val*/, const std::integral_constant<int, unchecked>&) noexcept {}

inline BOOST_MP_CXX14_CONSTEXPR void check_is_negative(const std::integral_constant<bool, true>&) noexcept {}
inline void                          check_is_negative(const std::integral_constant<bool, false>&)
{
   BOOST_THROW_EXCEPTION(std::range_error("Attempt to assign a negative value to an unsigned type."));
}

template <class Integer>
inline BOOST_MP_CXX14_CONSTEXPR Integer negate_integer(Integer i, const std::integral_constant<bool, true>&) noexcept
{
   return -i;
}
template <class Integer>
inline BOOST_MP_CXX14_CONSTEXPR Integer negate_integer(Integer i, const std::integral_constant<bool, false>&) noexcept
{
   return ~(i - 1);
}

template <class R, unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<R>::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, void>::type
eval_convert_to(R* result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& backend)
{
   using checked_type = std::integral_constant<int, Checked1>;
   check_in_range<R>(backend, checked_type());

   BOOST_IF_CONSTEXPR(numeric_limits_workaround<R>::digits < cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits)
   {
      if ((backend.sign() && boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value) && (1 + static_cast<boost::multiprecision::limb_type>((std::numeric_limits<R>::max)()) <= backend.limbs()[0]))
      {
         *result = (numeric_limits_workaround<R>::min)();
         return;
      }
      else if (boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value && !backend.sign() && static_cast<boost::multiprecision::limb_type>((std::numeric_limits<R>::max)()) <= backend.limbs()[0])
      {
         *result = (numeric_limits_workaround<R>::max)();
         return;
      }
      else
         *result = static_cast<R>(backend.limbs()[0]);
   }
   else
      *result = static_cast<R>(backend.limbs()[0]);
   unsigned shift = cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   unsigned i     = 1;
   BOOST_IF_CONSTEXPR(numeric_limits_workaround<R>::digits > cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits)
   {
      while ((i < backend.size()) && (shift < static_cast<unsigned>(numeric_limits_workaround<R>::digits - cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits)))
      {
         *result += static_cast<R>(backend.limbs()[i]) << shift;
         shift += cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
         ++i;
      }
      //
      // We have one more limb to extract, but may not need all the bits, so treat this as a special case:
      //
      if (i < backend.size())
      {
         const limb_type mask = numeric_limits_workaround<R>::digits - shift == cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits ? ~static_cast<limb_type>(0) : (static_cast<limb_type>(1u) << (numeric_limits_workaround<R>::digits - shift)) - 1;
         *result += (static_cast<R>(backend.limbs()[i]) & mask) << shift;
         if ((static_cast<R>(backend.limbs()[i]) & static_cast<limb_type>(~mask)) || (i + 1 < backend.size()))
         {
            // Overflow:
            if (backend.sign())
            {
               check_is_negative(boost::multiprecision::detail::is_signed<R>());
               *result = (numeric_limits_workaround<R>::min)();
            }
            else if (boost::multiprecision::detail::is_signed<R>::value)
               *result = (numeric_limits_workaround<R>::max)();
            return;
         }
      }
   }
   else if (backend.size() > 1)
   {
      // Overflow:
      if (backend.sign())
      {
         check_is_negative(boost::multiprecision::detail::is_signed<R>());
         *result = (numeric_limits_workaround<R>::min)();
      }
      else if (boost::multiprecision::detail::is_signed<R>::value)
         *result = (numeric_limits_workaround<R>::max)();
      return;
   }
   if (backend.sign())
   {
      check_is_negative(std::integral_constant<bool, boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value>());
      *result = negate_integer(*result, std::integral_constant<bool, boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value>());
   }
}

template <class R, unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_floating_point<R>::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, void>::type
eval_convert_to(R* result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& backend) noexcept(boost::multiprecision::detail::is_arithmetic<R>::value)
{
   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::const_limb_pointer p     = backend.limbs();
   unsigned                                                                                          shift = cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   *result                                                                                                 = static_cast<R>(*p);
   for (unsigned i = 1; i < backend.size(); ++i)
   {
      *result += static_cast<R>(std::ldexp(static_cast<long double>(p[i]), shift));
      shift += cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   }
   if (backend.sign())
      *result = -*result;
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, bool>::type
eval_is_zero(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val) noexcept
{
   return (val.size() == 1) && (val.limbs()[0] == 0);
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, int>::type
eval_get_sign(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val) noexcept
{
   return eval_is_zero(val) ? 0 : val.sign() ? -1 : 1;
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_abs(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   result = val;
   result.sign(false);
}

//
// Get the location of the least-significant-bit:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, unsigned>::type
eval_lsb(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a)
{
   using default_ops::eval_get_sign;
   if (eval_get_sign(a) == 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("No bits were set in the operand."));
   }
   if (a.sign())
   {
      BOOST_THROW_EXCEPTION(std::domain_error("Testing individual bits in negative values is not supported - results are undefined."));
   }

   //
   // Find the index of the least significant limb that is non-zero:
   //
   unsigned index = 0;
   while (!a.limbs()[index] && (index < a.size()))
      ++index;
   //
   // Find the index of the least significant bit within that limb:
   //
   unsigned result = boost::multiprecision::detail::find_lsb(a.limbs()[index]);

   return result + index * cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
}

//
// Get the location of the most-significant-bit:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, unsigned>::type
eval_msb_imp(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a)
{
   //
   // Find the index of the most significant bit that is non-zero:
   //
   return (a.size() - 1) * cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits + boost::multiprecision::detail::find_msb(a.limbs()[a.size() - 1]);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, unsigned>::type
eval_msb(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a)
{
   using default_ops::eval_get_sign;
   if (eval_get_sign(a) == 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("No bits were set in the operand."));
   }
   if (a.sign())
   {
      BOOST_THROW_EXCEPTION(std::domain_error("Testing individual bits in negative values is not supported - results are undefined."));
   }
   return eval_msb_imp(a);
}

#ifdef BOOST_GCC
//
// We really shouldn't need to be disabling this warning, but it really does appear to be
// spurious.  The warning appears only when in release mode, and asserts are on.
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, bool>::type
eval_bit_test(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val, unsigned index) noexcept
{
   unsigned  offset = index / cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   unsigned  shift  = index % cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   limb_type mask   = shift ? limb_type(1u) << shift : limb_type(1u);
   if (offset >= val.size())
      return false;
   return val.limbs()[offset] & mask ? true : false;
}

#ifdef BOOST_GCC
#pragma GCC diagnostic pop
#endif

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_bit_set(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val, unsigned index)
{
   unsigned  offset = index / cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   unsigned  shift  = index % cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   limb_type mask   = shift ? limb_type(1u) << shift : limb_type(1u);
   if (offset >= val.size())
   {
      unsigned os = val.size();
      val.resize(offset + 1, offset + 1);
      if (offset >= val.size())
         return; // fixed precision overflow
      for (unsigned i = os; i <= offset; ++i)
         val.limbs()[i] = 0;
   }
   val.limbs()[offset] |= mask;
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_bit_unset(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val, unsigned index) noexcept
{
   unsigned  offset = index / cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   unsigned  shift  = index % cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   limb_type mask   = shift ? limb_type(1u) << shift : limb_type(1u);
   if (offset >= val.size())
      return;
   val.limbs()[offset] &= ~mask;
   val.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_bit_flip(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val, unsigned index)
{
   unsigned  offset = index / cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   unsigned  shift  = index % cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
   limb_type mask   = shift ? limb_type(1u) << shift : limb_type(1u);
   if (offset >= val.size())
   {
      unsigned os = val.size();
      val.resize(offset + 1, offset + 1);
      if (offset >= val.size())
         return; // fixed precision overflow
      for (unsigned i = os; i <= offset; ++i)
         val.limbs()[i] = 0;
   }
   val.limbs()[offset] ^= mask;
   val.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_qr(
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& x,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& y,
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       q,
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       r) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   divide_unsigned_helper(&q, x, y, r);
   q.sign(x.sign() != y.sign());
   r.sign(x.sign());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_qr(
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& x,
    limb_type                                                                   y,
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       q,
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       r) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   divide_unsigned_helper(&q, x, y, r);
   q.sign(x.sign());
   r.sign(x.sign());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<U>::value>::type eval_qr(
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& x,
    U                                                                           y,
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       q,
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       r) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   using default_ops::eval_qr;
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t;
   t = y;
   eval_qr(x, t, q, r);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class Integer>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_unsigned<Integer>::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, Integer>::type
eval_integer_modulus(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a, Integer mod)
{
   BOOST_IF_CONSTEXPR (sizeof(Integer) <= sizeof(limb_type))
   {
      if (mod <= (std::numeric_limits<limb_type>::max)())
      {
         const int              n = a.size();
         const double_limb_type two_n_mod = static_cast<limb_type>(1u) + (~static_cast<limb_type>(0u) - mod) % mod;
         limb_type              res = a.limbs()[n - 1] % mod;

         for (int i = n - 2; i >= 0; --i)
            res = static_cast<limb_type>((res * two_n_mod + a.limbs()[i]) % mod);
         return res;
      }
      else
         return default_ops::eval_integer_modulus(a, mod);
   }
   else
   {
      return default_ops::eval_integer_modulus(a, mod);
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class Integer>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_signed<Integer>::value && boost::multiprecision::detail::is_integral<Integer>::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, Integer>::type
eval_integer_modulus(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& x, Integer val)
{
   return eval_integer_modulus(x, boost::multiprecision::detail::unsigned_abs(val));
}

BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR limb_type eval_gcd(limb_type u, limb_type v)
{
   // boundary cases
   if (!u || !v)
      return u | v;
#if __cpp_lib_gcd_lcm >= 201606L
   return std::gcd(u, v);
#else
   unsigned shift = boost::multiprecision::detail::find_lsb(u | v);
   u >>= boost::multiprecision::detail::find_lsb(u);
   do
   {
      v >>= boost::multiprecision::detail::find_lsb(v);
      if (u > v)
         std_constexpr::swap(u, v);
      v -= u;
   } while (v);
   return u << shift;
#endif
}

inline BOOST_MP_CXX14_CONSTEXPR double_limb_type eval_gcd(double_limb_type u, double_limb_type v)
{
#if (__cpp_lib_gcd_lcm >= 201606L) && (!defined(BOOST_HAS_INT128) || !defined(__STRICT_ANSI__))
   return std::gcd(u, v);
#else
   unsigned shift = boost::multiprecision::detail::find_lsb(u | v);
   u >>= boost::multiprecision::detail::find_lsb(u);
   do
   {
      v >>= boost::multiprecision::detail::find_lsb(v);
      if (u > v)
         std_constexpr::swap(u, v);
      v -= u;
   } while (v);
   return u << shift;
#endif
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_gcd(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
    limb_type                                                                   b)
{
   int s = eval_get_sign(a);
   if (!b || !s)
   {
      result = a;
      *result.limbs() |= b;
   }
   else
   {
      eval_modulus(result, a, b);
      limb_type& res = *result.limbs();
      res = eval_gcd(res, b);
   }
   result.sign(false);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_gcd(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
    double_limb_type                                                            b)
{
   int s = eval_get_sign(a);
   if (!b || !s)
   {
      if (!s)
         result = b;
      else
         result = a;
      return;
   }
   double_limb_type res = 0;
   if(a.sign() == 0)
      res = eval_integer_modulus(a, b);
   else
   {
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t(a);
      t.negate();
      res = eval_integer_modulus(t, b);
   }
   res            = eval_gcd(res, b);
   result = res;
   result.sign(false);
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_gcd(
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
   const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
   signed_double_limb_type                                                     v)
{
   eval_gcd(result, a, static_cast<double_limb_type>(v < 0 ? -v : v));
}
//
// These 2 overloads take care of gcd against an (unsigned) short etc:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class Integer>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_unsigned<Integer>::value && (sizeof(Integer) <= sizeof(limb_type)) && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_gcd(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
    const Integer&                                                              v)
{
   eval_gcd(result, a, static_cast<limb_type>(v));
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class Integer>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_signed<Integer>::value && boost::multiprecision::detail::is_integral<Integer>::value && (sizeof(Integer) <= sizeof(limb_type)) && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_gcd(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
    const Integer&                                                              v)
{
   eval_gcd(result, a, static_cast<limb_type>(v < 0 ? -v : v));
}
//
// What follows is Lehmer's GCD algorithm:
// Essentially this uses the leading digit(s) of U and V
// only to run a "simulated" Euclid algorithm.  It stops
// when the calculated quotient differs from what would have been
// the true quotient.  At that point the cosequences are used to
// calculate the new U and V.  A nice lucid description appears
// in "An Analysis of Lehmer's Euclidean GCD Algorithm",
// by Jonathan Sorenson.  https://www.researchgate.net/publication/2424634_An_Analysis_of_Lehmer%27s_Euclidean_GCD_Algorithm
// DOI: 10.1145/220346.220378.
//
// There are two versions of this algorithm here, and both are "double digit"
// variations: which is to say if there are k bits per limb, then they extract
// 2k bits into a double_limb_type and then run the algorithm on that.  The first
// version is a straightforward version of the algorithm, and is designed for
// situations where double_limb_type is a native integer (for example where
// limb_type is a 32-bit integer on a 64-bit machine).  For 32-bit limbs it
// reduces the size of U by about 30 bits per call.  The second is a more complex
// version for situations where double_limb_type is a synthetic type: for example
// __int128.  For 64 bit limbs it reduces the size of U by about 62 bits per call.
//
// The complexity of the algorithm given by Sorenson is roughly O(ln^2(N)) for
// two N bit numbers.
//
// The original double-digit version of the algorithm is described in:
// 
// "A Double Digit Lehmer-Euclid Algorithm for Finding the GCD of Long Integers",
// Tudor Jebelean, J Symbolic Computation, 1995 (19), 145.
//
#ifndef BOOST_HAS_INT128
//
// When double_limb_type is a native integer type then we should just use it and not worry about the consequences.
// This can eliminate approximately a full limb with each call.
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class Storage>
void eval_gcd_lehmer(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& U, cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& V, unsigned lu, Storage& storage)
{
   //
   // Extract the leading 2 * bits_per_limb bits from U and V:
   //
   unsigned         h = lu % bits_per_limb;
   double_limb_type u = (static_cast<double_limb_type>((U.limbs()[U.size() - 1])) << bits_per_limb) | U.limbs()[U.size() - 2];
   double_limb_type v = (static_cast<double_limb_type>((V.size() < U.size() ? 0 : V.limbs()[V.size() - 1])) << bits_per_limb) | V.limbs()[U.size() - 2];
   if (h)
   {
      u <<= bits_per_limb - h;
      u |= U.limbs()[U.size() - 3] >> h;
      v <<= bits_per_limb - h;
      v |= V.limbs()[U.size() - 3] >> h;
   }
   //
   // Co-sequences x an y: we need only the last 3 values of these,
   // the first 2 values are known correct, the third gets checked
   // in each loop operation, and we terminate when they go wrong.
   //
   // x[i+0] is positive for even i.
   // y[i+0] is positive for odd i.
   //
   // However we track only absolute values here:
   //
   double_limb_type x[3] = {1, 0};
   double_limb_type y[3] = {0, 1};
   unsigned         i    = 0;

#ifdef BOOST_MP_GCD_DEBUG
   cpp_int UU, VV;
   UU = U;
   VV = V;
#endif

   while (true)
   {
      double_limb_type q  = u / v;
      x[2]                = x[0] + q * x[1];
      y[2]                = y[0] + q * y[1];
      double_limb_type tu = u;
      u                   = v;
      v                   = tu - q * v;
      ++i;
      //
      // We must make sure that y[2] occupies a single limb otherwise
      // the multiprecision multiplications below would be much more expensive.
      // This can sometimes lose us one iteration, but is worth it for improved
      // calculation efficiency.
      //
      if (y[2] >> bits_per_limb)
         break;
      //
      // These are Jebelean's exact termination conditions:
      //
      if ((i & 1u) == 0)
      {
         BOOST_ASSERT(u > v);
         if ((v < x[2]) || ((u - v) < (y[2] + y[1])))
            break;
      }
      else
      {
         BOOST_ASSERT(u > v);
         if ((v < y[2]) || ((u - v) < (x[2] + x[1])))
            break;
      }
#ifdef BOOST_MP_GCD_DEBUG
      BOOST_ASSERT(q == UU / VV);
      UU %= VV;
      UU.swap(VV);
#endif
      x[0] = x[1];
      x[1] = x[2];
      y[0] = y[1];
      y[1] = y[2];
   }
   if (i == 1)
   {
      // No change to U and V we've stalled!
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t;
      eval_modulus(t, U, V);
      U.swap(V);
      V.swap(t);
      return;
   }
   //
   // Update U and V.
   // We have:
   //
   // U = x[0]U + y[0]V and
   // V = x[1]U + y[1]V.
   //
   // But since we track only absolute values of x and y
   // we have to take account of the implied signs and perform
   // the appropriate subtraction depending on the whether i is
   // even or odd:
   //
   unsigned                                                             ts = U.size() + 1;
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t1(storage, ts), t2(storage, ts), t3(storage, ts);
   eval_multiply(t1, U, static_cast<limb_type>(x[0]));
   eval_multiply(t2, V, static_cast<limb_type>(y[0]));
   eval_multiply(t3, U, static_cast<limb_type>(x[1]));
   if ((i & 1u) == 0)
   {
      if (x[0] == 0)
         U = t2;
      else
      {
         BOOST_ASSERT(t2.compare(t1) >= 0);
         eval_subtract(U, t2, t1);
         BOOST_ASSERT(U.sign() == false);
      }
   }
   else
   {
      BOOST_ASSERT(t1.compare(t2) >= 0);
      eval_subtract(U, t1, t2);
      BOOST_ASSERT(U.sign() == false);
   }
   eval_multiply(t2, V, static_cast<limb_type>(y[1]));
   if (i & 1u)
   {
      if (x[1] == 0)
         V = t2;
      else
      {
         BOOST_ASSERT(t2.compare(t3) >= 0);
         eval_subtract(V, t2, t3);
         BOOST_ASSERT(V.sign() == false);
      }
   }
   else
   {
      BOOST_ASSERT(t3.compare(t2) >= 0);
      eval_subtract(V, t3, t2);
      BOOST_ASSERT(V.sign() == false);
   }
   BOOST_ASSERT(U.compare(V) >= 0);
   BOOST_ASSERT(lu > eval_msb(U));
#ifdef BOOST_MP_GCD_DEBUG

   BOOST_ASSERT(UU == U);
   BOOST_ASSERT(VV == V);

   extern unsigned total_lehmer_gcd_calls;
   extern unsigned total_lehmer_gcd_bits_saved;
   extern unsigned total_lehmer_gcd_cycles;

   ++total_lehmer_gcd_calls;
   total_lehmer_gcd_bits_saved += lu - eval_msb(U);
   total_lehmer_gcd_cycles += i;
#endif
   if (lu < 2048)
   {
      //
      // Since we have stripped all common powers of 2 from U and V at the start
      // if either are even at this point, we can remove stray powers of 2 now.
      // Note that it is not possible for *both* U and V to be even at this point.
      //
      // This has an adverse effect on performance for high bit counts, but has
      // a significant positive effect for smaller counts.
      //
      if ((U.limbs()[0] & 1u) == 0)
      {
         eval_right_shift(U, eval_lsb(U));
         if (U.compare(V) < 0)
            U.swap(V);
      }
      else if ((V.limbs()[0] & 1u) == 0)
      {
         eval_right_shift(V, eval_lsb(V));
      }
   }
   storage.deallocate(ts * 3);
}

#else
//
// This branch is taken when double_limb_type is a synthetic type with no native hardware support.
// For example __int128.  The assumption is that add/subtract/multiply of double_limb_type are efficient,
// but that division is very slow.
//
// We begin with a specialized routine for division.
// We know that u > v > ~limb_type(0), and therefore
// that the result will fit into a single limb_type.
// We also know that most of the time this is called the result will be 1.
// For small limb counts, this almost doubles the performance of Lehmer's routine!
//
BOOST_FORCEINLINE void divide_subtract(limb_type& q, double_limb_type& u, const double_limb_type& v)
{
   BOOST_ASSERT(q == 1); // precondition on entry.
   u -= v;
   while (u >= v)
   {
      u -= v;
      if (++q > 30)
      {
         limb_type t = u / v;
         u -= t * v;
         q += t;
      }
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class Storage>
void eval_gcd_lehmer(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& U, cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& V, unsigned lu, Storage& storage)
{
   //
   // Extract the leading 2*bits_per_limb bits from U and V:
   //
   unsigned  h = lu % bits_per_limb;
   double_limb_type u, v;
   if (h)
   {
      u = (static_cast<double_limb_type>((U.limbs()[U.size() - 1])) << bits_per_limb) | U.limbs()[U.size() - 2];
      v = (static_cast<double_limb_type>((V.size() < U.size() ? 0 : V.limbs()[V.size() - 1])) << bits_per_limb) | V.limbs()[U.size() - 2];
      u <<= bits_per_limb - h;
      u |= U.limbs()[U.size() - 3] >> h;
      v <<= bits_per_limb - h;
      v |= V.limbs()[U.size() - 3] >> h;
   }
   else
   {
      u = (static_cast<double_limb_type>(U.limbs()[U.size() - 1]) << bits_per_limb) | U.limbs()[U.size() - 2];
      v = (static_cast<double_limb_type>(V.limbs()[U.size() - 1]) << bits_per_limb) | V.limbs()[U.size() - 2];
   }
   //
   // Cosequences are stored as limb_types, we take care not to overflow these:
   //
   // x[i+0] is positive for even i.
   // y[i+0] is positive for odd i.
   //
   // However we track only absolute values here:
   //
   limb_type x[3] = { 1, 0 };
   limb_type y[3] = { 0, 1 };
   unsigned  i = 0;

#ifdef BOOST_MP_GCD_DEBUG
   cpp_int UU, VV;
   UU = U;
   VV = V;
#endif
   //
   // We begine by running a single digit version of Lehmer's algorithm, we still have
   // to track u and v at double precision, but this adds only a tiny performance penalty.
   // What we gain is fast division, and fast termination testing.
   // When you see static_cast<limb_type>(u >> bits_per_limb) here, this is really just
   // a direct access to the upper bits_per_limb of the double limb type.  For __int128
   // this is simple a load of the upper 64 bits and the "shift" is optimised away.
   //
   double_limb_type old_u, old_v;
   while (true)
   {
      limb_type q = static_cast<limb_type>(u >> bits_per_limb) / static_cast<limb_type>(v >> bits_per_limb);
      x[2] = x[0] + q * x[1];
      y[2] = y[0] + q * y[1];
      double_limb_type tu = u;
      old_u = u;
      old_v = v;
      u = v;
      double_limb_type t = q * v;
      if (tu < t)
      {
         ++i;
         break;
      }
      v = tu - t;
      ++i;
      BOOST_ASSERT((u <= v) || (t / q == old_v));
      if (u <= v)
      {
         // We've gone terribly wrong, probably numeric overflow:
         break;
      }
      if ((i & 1u) == 0)
      {
         if ((static_cast<limb_type>(v >> bits_per_limb) < x[2]) || ((static_cast<limb_type>(u >> bits_per_limb) - static_cast<limb_type>(v >> bits_per_limb)) < (y[2] + y[1])))
            break;
      }
      else
      {
         if ((static_cast<limb_type>(v >> bits_per_limb) < y[2]) || ((static_cast<limb_type>(u >> bits_per_limb) - static_cast<limb_type>(v >> bits_per_limb)) < (x[2] + x[1])))
            break;
      }
#ifdef BOOST_MP_GCD_DEBUG
      BOOST_ASSERT(q == UU / VV);
      UU %= VV;
      UU.swap(VV);
#endif
      x[0] = x[1];
      x[1] = x[2];
      y[0] = y[1];
      y[1] = y[2];
   }
   //
   // We get here when the single digit algorithm has gone wrong, back up i, u and v:
   //
   --i;
   u = old_u;
   v = old_v;
   //
   // Now run the full double-digit algorithm:
   //
   while (true)
   {
      limb_type q = 1;
      double_limb_type tt = u;
      divide_subtract(q, u, v);
      std::swap(u, v);
      tt = y[0] + q * static_cast<double_limb_type>(y[1]);
      //
      // If calculation of y[2] would overflow a single limb, then we *must* terminate.
      // Note that x[2] < y[2] so there is no need to check that as well:
      //
      if (tt >> bits_per_limb)
      {
         ++i;
         break;
      }
      x[2] = x[0] + q * x[1];
      y[2] = tt;
      ++i;
      if ((i & 1u) == 0)
      {
         BOOST_ASSERT(u > v);
         if ((v < x[2]) || ((u - v) < (static_cast<double_limb_type>(y[2]) + y[1])))
            break;
      }
      else
      {
         BOOST_ASSERT(u > v);
         if ((v < y[2]) || ((u - v) < (static_cast<double_limb_type>(x[2]) + x[1])))
            break;
      }
#ifdef BOOST_MP_GCD_DEBUG
      BOOST_ASSERT(q == UU / VV);
      UU %= VV;
      UU.swap(VV);
#endif
      x[0] = x[1];
      x[1] = x[2];
      y[0] = y[1];
      y[1] = y[2];
   }
   if (i == 1)
   {
      // No change to U and V we've stalled!
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t;
      eval_modulus(t, U, V);
      U.swap(V);
      V.swap(t);
      return;
   }
   //
   // Update U and V.
   // We have:
   //
   // U = x[0]U + y[0]V and
   // V = x[1]U + y[1]V.
   //
   // But since we track only absolute values of x and y
   // we have to take account of the implied signs and perform
   // the appropriate subtraction depending on the whether i is
   // even or odd:
   //
   unsigned ts = U.size() + 1;
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t1(storage, ts), t2(storage, ts), t3(storage, ts);
   eval_multiply(t1, U, x[0]);
   eval_multiply(t2, V, y[0]);
   eval_multiply(t3, U, x[1]);
   if ((i & 1u) == 0)
   {
      if (x[0] == 0)
         U = t2;
      else
      {
         BOOST_ASSERT(t2.compare(t1) >= 0);
         eval_subtract(U, t2, t1);
         BOOST_ASSERT(U.sign() == false);
      }
   }
   else
   {
      BOOST_ASSERT(t1.compare(t2) >= 0);
      eval_subtract(U, t1, t2);
      BOOST_ASSERT(U.sign() == false);
   }
   eval_multiply(t2, V, y[1]);
   if (i & 1u)
   {
      if (x[1] == 0)
         V = t2;
      else
      {
         BOOST_ASSERT(t2.compare(t3) >= 0);
         eval_subtract(V, t2, t3);
         BOOST_ASSERT(V.sign() == false);
      }
   }
   else
   {
      BOOST_ASSERT(t3.compare(t2) >= 0);
      eval_subtract(V, t3, t2);
      BOOST_ASSERT(V.sign() == false);
   }
   BOOST_ASSERT(U.compare(V) >= 0);
   BOOST_ASSERT(lu > eval_msb(U));
#ifdef BOOST_MP_GCD_DEBUG

   BOOST_ASSERT(UU == U);
   BOOST_ASSERT(VV == V);

   extern unsigned total_lehmer_gcd_calls;
   extern unsigned total_lehmer_gcd_bits_saved;
   extern unsigned total_lehmer_gcd_cycles;

   ++total_lehmer_gcd_calls;
   total_lehmer_gcd_bits_saved += lu - eval_msb(U);
   total_lehmer_gcd_cycles += i;
#endif
   if (lu < 2048)
   {
      //
      // Since we have stripped all common powers of 2 from U and V at the start
      // if either are even at this point, we can remove stray powers of 2 now.
      // Note that it is not possible for *both* U and V to be even at this point.
      //
      // This has an adverse effect on performance for high bit counts, but has
      // a significant positive effect for smaller counts.
      //
      if ((U.limbs()[0] & 1u) == 0)
      {
         eval_right_shift(U, eval_lsb(U));
         if (U.compare(V) < 0)
            U.swap(V);
      }
      else if ((V.limbs()[0] & 1u) == 0)
      {
         eval_right_shift(V, eval_lsb(V));
      }
   }
   storage.deallocate(ts * 3);
}

#endif

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_gcd(
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
   const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
   const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& b)
{
   using default_ops::eval_get_sign;
   using default_ops::eval_is_zero;
   using default_ops::eval_lsb;

   if (a.size() == 1)
   {
      eval_gcd(result, b, *a.limbs());
      return;
   }
   if (b.size() == 1)
   {
      eval_gcd(result, a, *b.limbs());
      return;
   }
   unsigned temp_size = (std::max)(a.size(), b.size()) + 1;
   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::scoped_shared_storage storage(a, temp_size * 6);

   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> U(storage, temp_size);
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> V(storage, temp_size);
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t(storage, temp_size);
   U = a;
   V = b;

   int s = eval_get_sign(U);

   /* GCD(0,x) := x */
   if (s < 0)
   {
      U.negate();
   }
   else if (s == 0)
   {
      result = V;
      return;
   }
   s = eval_get_sign(V);
   if (s < 0)
   {
      V.negate();
   }
   else if (s == 0)
   {
      result = U;
      return;
   }
   //
   // Remove common factors of 2:
   //
   unsigned us = eval_lsb(U);
   unsigned vs = eval_lsb(V);
   int      shift = (std::min)(us, vs);
   if (us)
      eval_right_shift(U, us);
   if (vs)
      eval_right_shift(V, vs);

   if (U.compare(V) < 0)
      U.swap(V);

   while (!eval_is_zero(V))
   {
      if (U.size() <= 2)
      {
         //
         // Special case: if V has no more than 2 limbs
         // then we can reduce U and V to a pair of integers and perform
         // direct integer gcd:
         //
         if (U.size() == 1)
            U = eval_gcd(*V.limbs(), *U.limbs());
         else
         {
            double_limb_type i = U.limbs()[0] | (static_cast<double_limb_type>(U.limbs()[1]) << sizeof(limb_type) * CHAR_BIT);
            double_limb_type j = (V.size() == 1) ? *V.limbs() : V.limbs()[0] | (static_cast<double_limb_type>(V.limbs()[1]) << sizeof(limb_type) * CHAR_BIT);
            U = eval_gcd(i, j);
         }
         break;
      }
      unsigned lu = eval_msb(U) + 1;
      unsigned lv = eval_msb(V) + 1;
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
      if (!BOOST_MP_IS_CONST_EVALUATED(lu) && (lu - lv <= bits_per_limb / 2))
#else
      if (lu - lv <= bits_per_limb / 2)
#endif
      {
         eval_gcd_lehmer(U, V, lu, storage);
      }
      else
      {
         eval_modulus(t, U, V);
         U.swap(V);
         V.swap(t);
      }
   }
   result = U;
   if (shift)
      eval_left_shift(result, shift);
}
//
// Now again for trivial backends:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_gcd(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& b) noexcept
{
   *result.limbs() = boost::integer::gcd(*a.limbs(), *b.limbs());
}
// This one is only enabled for unchecked cpp_int's, for checked int's we need the checking in the default version:
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && (Checked1 == unchecked)>::type
eval_lcm(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& b) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = boost::integer::lcm(*a.limbs(), *b.limbs());
   result.normalize(); // result may overflow the specified number of bits
}

inline void conversion_overflow(const std::integral_constant<int, checked>&)
{
   BOOST_THROW_EXCEPTION(std::overflow_error("Overflow in conversion to narrower type"));
}
inline BOOST_MP_CXX14_CONSTEXPR void conversion_overflow(const std::integral_constant<int, unchecked>&) {}

#if defined(__clang__) && defined(__MINGW32__)
//
// clang-11 on Mingw segfaults on conversion of __int128 -> float.
// See: https://bugs.llvm.org/show_bug.cgi?id=48941
// These workarounds pass everything through an intermediate uint64_t.
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && std::is_same<typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::local_limb_type, double_limb_type>::value>::type
eval_convert_to(float* result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val)
{
   float f = static_cast<std::uint64_t>((*val.limbs()) >> 64);
   *result = std::ldexp(f, 64);
   *result += static_cast<std::uint64_t>((*val.limbs()));
   if(val.sign())
      *result = -*result;
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && std::is_same<typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::local_limb_type, double_limb_type>::value>::type
eval_convert_to(double* result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val)
{
   float f = static_cast<std::uint64_t>((*val.limbs()) >> 64);
   *result = std::ldexp(f, 64);
   *result += static_cast<std::uint64_t>((*val.limbs()));
   if(val.sign())
      *result = -*result;
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && std::is_same<typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::local_limb_type, double_limb_type>::value>::type
eval_convert_to(long double* result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val)
{
   float f = static_cast<std::uint64_t>((*val.limbs()) >> 64);
   *result = std::ldexp(f, 64);
   *result += static_cast<std::uint64_t>((*val.limbs()));
   if(val.sign())
      *result = -*result;
}
#endif

template <class R, unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && std::is_convertible<typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::local_limb_type, R>::value>::type
eval_convert_to(R* result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val)
{
   using common_type = typename std::common_type<R, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::local_limb_type>::type;
   BOOST_IF_CONSTEXPR(std::numeric_limits<R>::is_specialized)
   {
      if (static_cast<common_type>(*val.limbs()) > static_cast<common_type>((std::numeric_limits<R>::max)()))
      {
         if (val.isneg())
         {
            check_is_negative(std::integral_constant < bool, (boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value) || (number_category<R>::value == number_kind_floating_point) > ());
            if (static_cast<common_type>(*val.limbs()) > -static_cast<common_type>((std::numeric_limits<R>::min)()))
               conversion_overflow(typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
            *result = (std::numeric_limits<R>::min)();
         }
         else
         {
            conversion_overflow(typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
            *result = boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value ? (std::numeric_limits<R>::max)() : static_cast<R>(*val.limbs());
         }
      }
      else
      {
         *result = static_cast<R>(*val.limbs());
         if (val.isneg())
         {
            check_is_negative(std::integral_constant < bool, (boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value) || (number_category<R>::value == number_kind_floating_point) > ());
            *result = negate_integer(*result, std::integral_constant < bool, is_signed_number<R>::value || (number_category<R>::value == number_kind_floating_point) > ());
         }
      }
   }
   else
   {
      *result = static_cast<R>(*val.limbs());
      if (val.isneg())
      {
         check_is_negative(std::integral_constant<bool, (boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value) || (number_category<R>::value == number_kind_floating_point) > ());
         *result = negate_integer(*result, std::integral_constant<bool, is_signed_number<R>::value || (number_category<R>::value == number_kind_floating_point) > ());
      }
   }
}

template <class R, unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && std::is_convertible<typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::local_limb_type, R>::value>::type
eval_convert_to(R* result, const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val)
{
   using common_type = typename std::common_type<R, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::local_limb_type>::type;
   BOOST_IF_CONSTEXPR(std::numeric_limits<R>::is_specialized)
   {
      if(static_cast<common_type>(*val.limbs()) > static_cast<common_type>((std::numeric_limits<R>::max)()))
      {
         conversion_overflow(typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
         *result = boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value ? (std::numeric_limits<R>::max)() : static_cast<R>(*val.limbs());
      }
      else
         *result = static_cast<R>(*val.limbs());
   }
   else
      *result = static_cast<R>(*val.limbs());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, unsigned>::type
eval_lsb(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a)
{
   using default_ops::eval_get_sign;
   if (eval_get_sign(a) == 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("No bits were set in the operand."));
   }
   if (a.sign())
   {
      BOOST_THROW_EXCEPTION(std::domain_error("Testing individual bits in negative values is not supported - results are undefined."));
   }
   //
   // Find the index of the least significant bit within that limb:
   //
   return boost::multiprecision::detail::find_lsb(*a.limbs());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, unsigned>::type
eval_msb_imp(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a)
{
   //
   // Find the index of the least significant bit within that limb:
   //
   return boost::multiprecision::detail::find_msb(*a.limbs());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value, unsigned>::type
eval_msb(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a)
{
   using default_ops::eval_get_sign;
   if (eval_get_sign(a) == 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("No bits were set in the operand."));
   }
   if (a.sign())
   {
      BOOST_THROW_EXCEPTION(std::domain_error("Testing individual bits in negative values is not supported - results are undefined."));
   }
   return eval_msb_imp(a);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR std::size_t hash_value(const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& val) noexcept
{
   std::size_t result = 0;
   for (unsigned i = 0; i < val.size(); ++i)
   {
      boost::multiprecision::detail::hash_combine(result, val.limbs()[i]);
   }
   boost::multiprecision::detail::hash_combine(result, val.sign());
   return result;
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

}}} // namespace boost::multiprecision::backends

#endif
