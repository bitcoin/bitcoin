///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt
//
// Comparison operators for cpp_int_backend:
//
#ifndef BOOST_MP_CPP_INT_BIT_HPP
#define BOOST_MP_CPP_INT_BIT_HPP

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4319)
#endif

namespace boost { namespace multiprecision { namespace backends {

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_CXX14_CONSTEXPR void is_valid_bitwise_op(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o, const std::integral_constant<int, checked>&)
{
   if (result.sign() || o.sign())
      BOOST_THROW_EXCEPTION(std::range_error("Bitwise operations on negative values results in undefined behavior."));
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_CXX14_CONSTEXPR void is_valid_bitwise_op(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>&, const std::integral_constant<int, unchecked>&) {}

template <unsigned MinBits1, unsigned MaxBits1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_CXX14_CONSTEXPR void is_valid_bitwise_op(
    const cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>& result, const std::integral_constant<int, checked>&)
{
   if (result.sign())
      BOOST_THROW_EXCEPTION(std::range_error("Bitwise operations on negative values results in undefined behavior."));
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_CXX14_CONSTEXPR void is_valid_bitwise_op(
    const cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1>&, const std::integral_constant<int, checked>&) {}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_CXX14_CONSTEXPR void is_valid_bitwise_op(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&, const std::integral_constant<int, unchecked>&) {}

template <class CppInt1, class CppInt2, class Op>
BOOST_MP_CXX14_CONSTEXPR void bitwise_op(
    CppInt1&       result,
    const CppInt2& o,
    Op             op, const std::integral_constant<bool, true>&) noexcept((is_non_throwing_cpp_int<CppInt1>::value))
{
   //
   // There are 4 cases:
   // * Both positive.
   // * result negative, o positive.
   // * o negative, result positive.
   // * Both negative.
   //
   // When one arg is negative we convert to 2's complement form "on the fly",
   // and then convert back to signed-magnitude form at the end.
   //
   // Note however, that if the type is checked, then bitwise ops on negative values
   // are not permitted and an exception will result.
   //
   is_valid_bitwise_op(result, o, typename CppInt1::checked_type());
   //
   // First figure out how big the result needs to be and set up some data:
   //
   unsigned rs = result.size();
   unsigned os = o.size();
   unsigned m(0), x(0);
   minmax(rs, os, m, x);
   result.resize(x, x);
   typename CppInt1::limb_pointer       pr = result.limbs();
   typename CppInt2::const_limb_pointer po = o.limbs();
   for (unsigned i = rs; i < x; ++i)
      pr[i] = 0;

   limb_type next_limb = 0;

   if (!result.sign())
   {
      if (!o.sign())
      {
         for (unsigned i = 0; i < os; ++i)
            pr[i] = op(pr[i], po[i]);
         for (unsigned i = os; i < x; ++i)
            pr[i] = op(pr[i], limb_type(0));
      }
      else
      {
         // "o" is negative:
         double_limb_type carry = 1;
         for (unsigned i = 0; i < os; ++i)
         {
            carry += static_cast<double_limb_type>(~po[i]);
            pr[i] = op(pr[i], static_cast<limb_type>(carry));
            carry >>= CppInt1::limb_bits;
         }
         for (unsigned i = os; i < x; ++i)
         {
            carry += static_cast<double_limb_type>(~limb_type(0));
            pr[i] = op(pr[i], static_cast<limb_type>(carry));
            carry >>= CppInt1::limb_bits;
         }
         // Set the overflow into the "extra" limb:
         carry += static_cast<double_limb_type>(~limb_type(0));
         next_limb = op(limb_type(0), static_cast<limb_type>(carry));
      }
   }
   else
   {
      if (!o.sign())
      {
         // "result" is negative:
         double_limb_type carry = 1;
         for (unsigned i = 0; i < os; ++i)
         {
            carry += static_cast<double_limb_type>(~pr[i]);
            pr[i] = op(static_cast<limb_type>(carry), po[i]);
            carry >>= CppInt1::limb_bits;
         }
         for (unsigned i = os; i < x; ++i)
         {
            carry += static_cast<double_limb_type>(~pr[i]);
            pr[i] = op(static_cast<limb_type>(carry), limb_type(0));
            carry >>= CppInt1::limb_bits;
         }
         // Set the overflow into the "extra" limb:
         carry += static_cast<double_limb_type>(~limb_type(0));
         next_limb = op(static_cast<limb_type>(carry), limb_type(0));
      }
      else
      {
         // both are negative:
         double_limb_type r_carry = 1;
         double_limb_type o_carry = 1;
         for (unsigned i = 0; i < os; ++i)
         {
            r_carry += static_cast<double_limb_type>(~pr[i]);
            o_carry += static_cast<double_limb_type>(~po[i]);
            pr[i] = op(static_cast<limb_type>(r_carry), static_cast<limb_type>(o_carry));
            r_carry >>= CppInt1::limb_bits;
            o_carry >>= CppInt1::limb_bits;
         }
         for (unsigned i = os; i < x; ++i)
         {
            r_carry += static_cast<double_limb_type>(~pr[i]);
            o_carry += static_cast<double_limb_type>(~limb_type(0));
            pr[i] = op(static_cast<limb_type>(r_carry), static_cast<limb_type>(o_carry));
            r_carry >>= CppInt1::limb_bits;
            o_carry >>= CppInt1::limb_bits;
         }
         // Set the overflow into the "extra" limb:
         r_carry += static_cast<double_limb_type>(~limb_type(0));
         o_carry += static_cast<double_limb_type>(~limb_type(0));
         next_limb = op(static_cast<limb_type>(r_carry), static_cast<limb_type>(o_carry));
      }
   }
   //
   // See if the result is negative or not:
   //
   if (static_cast<signed_limb_type>(next_limb) < 0)
   {
      double_limb_type carry = 1;
      for (unsigned i = 0; i < x; ++i)
      {
         carry += static_cast<double_limb_type>(~pr[i]);
         pr[i] = static_cast<limb_type>(carry);
         carry >>= CppInt1::limb_bits;
      }
      if (carry)
      {
         result.resize(x + 1, x);
         if (result.size() > x)
            result.limbs()[x] = static_cast<limb_type>(carry);
      }
      result.sign(true);
   }
   else
      result.sign(false);

   result.normalize();
}

template <class CppInt1, class CppInt2, class Op>
BOOST_MP_CXX14_CONSTEXPR void bitwise_op(
    CppInt1&       result,
    const CppInt2& o,
    Op             op, const std::integral_constant<bool, false>&) noexcept((is_non_throwing_cpp_int<CppInt1>::value))
{
   //
   // Both arguments are unsigned types, very simple case handled as a special case.
   //
   // First figure out how big the result needs to be and set up some data:
   //
   unsigned rs = result.size();
   unsigned os = o.size();
   unsigned m(0), x(0);
   minmax(rs, os, m, x);
   result.resize(x, x);
   typename CppInt1::limb_pointer       pr = result.limbs();
   typename CppInt2::const_limb_pointer po = o.limbs();
   for (unsigned i = rs; i < x; ++i)
      pr[i] = 0;

   for (unsigned i = 0; i < os; ++i)
      pr[i] = op(pr[i], po[i]);
   for (unsigned i = os; i < x; ++i)
      pr[i] = op(pr[i], limb_type(0));

   result.normalize();
}

struct bit_and
{
   BOOST_MP_CXX14_CONSTEXPR limb_type operator()(limb_type a, limb_type b) const noexcept { return a & b; }
};
struct bit_or
{
   BOOST_MP_CXX14_CONSTEXPR limb_type operator()(limb_type a, limb_type b) const noexcept { return a | b; }
};
struct bit_xor
{
   BOOST_MP_CXX14_CONSTEXPR limb_type operator()(limb_type a, limb_type b) const noexcept { return a ^ b; }
};

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_bitwise_and(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   bitwise_op(result, o, bit_and(),
              std::integral_constant<bool, std::numeric_limits<number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> > >::is_signed || std::numeric_limits<number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> > >::is_signed > ());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_bitwise_or(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   bitwise_op(result, o, bit_or(),
              std::integral_constant<bool, std::numeric_limits<number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> > >::is_signed || std::numeric_limits<number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> > >::is_signed > ());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_bitwise_xor(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   bitwise_op(result, o, bit_xor(),
              std::integral_constant<bool, std::numeric_limits<number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> > >::is_signed || std::numeric_limits<number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> > >::is_signed > ());
}
//
// Again for operands which are single limbs:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1> >::value>::type
eval_bitwise_and(
    cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1>& result,
    limb_type                                                                      l) noexcept
{
   result.limbs()[0] &= l;
   result.resize(1, 1);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1> >::value>::type
eval_bitwise_or(
    cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1>& result,
    limb_type                                                                      l) noexcept
{
   result.limbs()[0] |= l;
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1> >::value>::type
eval_bitwise_xor(
    cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1>& result,
    limb_type                                                                      l) noexcept
{
   result.limbs()[0] ^= l;
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_complement(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   static_assert(((Checked1 != checked) || (Checked2 != checked)), "Attempt to take the complement of a signed type results in undefined behavior.");
   // Increment and negate:
   result = o;
   eval_increment(result);
   result.negate();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_complement(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   unsigned os = o.size();
   result.resize(UINT_MAX, os);
   for (unsigned i = 0; i < os; ++i)
      result.limbs()[i] = ~o.limbs()[i];
   for (unsigned i = os; i < result.size(); ++i)
      result.limbs()[i] = ~static_cast<limb_type>(0);
   result.normalize();
}

template <class Int>
inline void left_shift_byte(Int& result, double_limb_type s)
{
   limb_type offset = static_cast<limb_type>(s / Int::limb_bits);
   limb_type shift  = static_cast<limb_type>(s % Int::limb_bits);
   unsigned  ors    = result.size();
   if ((ors == 1) && (!*result.limbs()))
      return; // shifting zero yields zero.
   unsigned rs = ors;
   if (shift && (result.limbs()[ors - 1] >> (Int::limb_bits - shift)))
      ++rs; // Most significant limb will overflow when shifted
   rs += offset;
   result.resize(rs, rs);
   rs = result.size();

   typename Int::limb_pointer pr = result.limbs();

   if (rs != ors)
      pr[rs - 1] = 0u;
   std::size_t bytes = static_cast<std::size_t>(s / CHAR_BIT);
   std::size_t len   = (std::min)(ors * sizeof(limb_type), rs * sizeof(limb_type) - bytes);
   if (bytes >= rs * sizeof(limb_type))
      result = static_cast<limb_type>(0u);
   else
   {
      unsigned char* pc = reinterpret_cast<unsigned char*>(pr);
      std::memmove(pc + bytes, pc, len);
      std::memset(pc, 0, bytes);
   }
}

template <class Int>
inline BOOST_MP_CXX14_CONSTEXPR void left_shift_limb(Int& result, double_limb_type s)
{
   limb_type offset = static_cast<limb_type>(s / Int::limb_bits);
   limb_type shift  = static_cast<limb_type>(s % Int::limb_bits);

   unsigned ors = result.size();
   if ((ors == 1) && (!*result.limbs()))
      return; // shifting zero yields zero.
   unsigned rs = ors;
   if (shift && (result.limbs()[ors - 1] >> (Int::limb_bits - shift)))
      ++rs; // Most significant limb will overflow when shifted
   rs += offset;
   result.resize(rs, rs);

   typename Int::limb_pointer pr = result.limbs();

   if (offset > rs)
   {
      // The result is shifted past the end of the result:
      result = static_cast<limb_type>(0);
      return;
   }

   unsigned i = rs - result.size();
   for (; i < ors; ++i)
      pr[rs - 1 - i] = pr[ors - 1 - i];
   for (; i < rs; ++i)
      pr[rs - 1 - i] = 0;
}

template <class Int>
inline BOOST_MP_CXX14_CONSTEXPR void left_shift_generic(Int& result, double_limb_type s)
{
   limb_type offset = static_cast<limb_type>(s / Int::limb_bits);
   limb_type shift  = static_cast<limb_type>(s % Int::limb_bits);

   unsigned ors = result.size();
   if ((ors == 1) && (!*result.limbs()))
      return; // shifting zero yields zero.
   unsigned rs = ors;
   if (shift && (result.limbs()[ors - 1] >> (Int::limb_bits - shift)))
      ++rs; // Most significant limb will overflow when shifted
   rs += offset;
   result.resize(rs, rs);
   bool truncated = result.size() != rs;

   typename Int::limb_pointer pr = result.limbs();

   if (offset > rs)
   {
      // The result is shifted past the end of the result:
      result = static_cast<limb_type>(0);
      return;
   }

   unsigned i = rs - result.size();
   // This code only works when shift is non-zero, otherwise we invoke undefined behaviour!
   BOOST_ASSERT(shift);
   if (!truncated)
   {
      if (rs > ors + offset)
      {
         pr[rs - 1 - i] = pr[ors - 1 - i] >> (Int::limb_bits - shift);
         --rs;
      }
      else
      {
         pr[rs - 1 - i] = pr[ors - 1 - i] << shift;
         if (ors > 1)
            pr[rs - 1 - i] |= pr[ors - 2 - i] >> (Int::limb_bits - shift);
         ++i;
      }
   }
   for (; rs - i >= 2 + offset; ++i)
   {
      pr[rs - 1 - i] = pr[rs - 1 - i - offset] << shift;
      pr[rs - 1 - i] |= pr[rs - 2 - i - offset] >> (Int::limb_bits - shift);
   }
   if (rs - i >= 1 + offset)
   {
      pr[rs - 1 - i] = pr[rs - 1 - i - offset] << shift;
      ++i;
   }
   for (; i < rs; ++i)
      pr[rs - 1 - i] = 0;
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_left_shift(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    double_limb_type                                                      s) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   if (!s)
      return;

#if BOOST_ENDIAN_LITTLE_BYTE && defined(BOOST_MP_USE_LIMB_SHIFT)
   constexpr const limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - 1;
   constexpr const limb_type byte_shift_mask = CHAR_BIT - 1;

   if ((s & limb_shift_mask) == 0)
   {
      left_shift_limb(result, s);
   }
#ifdef BOOST_MP_NO_CONSTEXPR_DETECTION
   else if ((s & byte_shift_mask) == 0)
#else
   else if (((s & byte_shift_mask) == 0) && !BOOST_MP_IS_CONST_EVALUATED(s))
#endif
   {
      left_shift_byte(result, s);
   }
#elif BOOST_ENDIAN_LITTLE_BYTE
   constexpr const limb_type byte_shift_mask = CHAR_BIT - 1;

#ifdef BOOST_MP_NO_CONSTEXPR_DETECTION
   if ((s & byte_shift_mask) == 0)
#else
   constexpr limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - 1;
   if (BOOST_MP_IS_CONST_EVALUATED(s) && ((s & limb_shift_mask) == 0))
      left_shift_limb(result, s);
   else if (((s & byte_shift_mask) == 0) && !BOOST_MP_IS_CONST_EVALUATED(s))
#endif
   {
      left_shift_byte(result, s);
   }
#else
   constexpr const limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits - 1;

   if ((s & limb_shift_mask) == 0)
   {
      left_shift_limb(result, s);
   }
#endif
   else
   {
      left_shift_generic(result, s);
   }
   //
   // We may have shifted off the end and have leading zeros:
   //
   result.normalize();
}

template <class Int>
inline void right_shift_byte(Int& result, double_limb_type s)
{
   limb_type offset = static_cast<limb_type>(s / Int::limb_bits);
   BOOST_ASSERT((s % CHAR_BIT) == 0);
   unsigned ors = result.size();
   unsigned rs  = ors;
   if (offset >= rs)
   {
      result = limb_type(0);
      return;
   }
   rs -= offset;
   typename Int::limb_pointer pr = result.limbs();
   unsigned char*             pc = reinterpret_cast<unsigned char*>(pr);
   limb_type                  shift = static_cast<limb_type>(s / CHAR_BIT);
   std::memmove(pc, pc + shift, ors * sizeof(pr[0]) - shift);
   shift = (sizeof(limb_type) - shift % sizeof(limb_type)) * CHAR_BIT;
   if (shift < Int::limb_bits)
   {
      pr[ors - offset - 1] &= (static_cast<limb_type>(1u) << shift) - 1;
      if (!pr[ors - offset - 1] && (rs > 1))
         --rs;
   }
   result.resize(rs, rs);
}

template <class Int>
inline BOOST_MP_CXX14_CONSTEXPR void right_shift_limb(Int& result, double_limb_type s)
{
   limb_type offset = static_cast<limb_type>(s / Int::limb_bits);
   BOOST_ASSERT((s % Int::limb_bits) == 0);
   unsigned ors = result.size();
   unsigned rs  = ors;
   if (offset >= rs)
   {
      result = limb_type(0);
      return;
   }
   rs -= offset;
   typename Int::limb_pointer pr = result.limbs();
   unsigned                   i  = 0;
   for (; i < rs; ++i)
      pr[i] = pr[i + offset];
   result.resize(rs, rs);
}

template <class Int>
inline BOOST_MP_CXX14_CONSTEXPR void right_shift_generic(Int& result, double_limb_type s)
{
   limb_type offset = static_cast<limb_type>(s / Int::limb_bits);
   limb_type shift  = static_cast<limb_type>(s % Int::limb_bits);
   unsigned  ors    = result.size();
   unsigned  rs     = ors;
   if (offset >= rs)
   {
      result = limb_type(0);
      return;
   }
   rs -= offset;
   typename Int::limb_pointer pr = result.limbs();
   if ((pr[ors - 1] >> shift) == 0)
   {
      if (--rs == 0)
      {
         result = limb_type(0);
         return;
      }
   }
   unsigned i = 0;

   // This code only works for non-zero shift, otherwise we invoke undefined behaviour!
   BOOST_ASSERT(shift);
   for (; i + offset + 1 < ors; ++i)
   {
      pr[i] = pr[i + offset] >> shift;
      pr[i] |= pr[i + offset + 1] << (Int::limb_bits - shift);
   }
   pr[i] = pr[i + offset] >> shift;
   result.resize(rs, rs);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1> >::value>::type
eval_right_shift(
    cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1>& result,
    double_limb_type                                                               s) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, typename cpp_int_backend<MinBits1, MaxBits1, unsigned_magnitude, Checked1, Allocator1>::checked_type());
   if (!s)
      return;

#if BOOST_ENDIAN_LITTLE_BYTE && defined(BOOST_MP_USE_LIMB_SHIFT)
   constexpr const limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>::limb_bits - 1;
   constexpr const limb_type byte_shift_mask = CHAR_BIT - 1;

   if ((s & limb_shift_mask) == 0)
      right_shift_limb(result, s);
#ifdef BOOST_MP_NO_CONSTEXPR_DETECTION
   else if ((s & byte_shift_mask) == 0)
#else
   else if (((s & byte_shift_mask) == 0) && !BOOST_MP_IS_CONST_EVALUATED(s))
#endif
      right_shift_byte(result, s);
#elif BOOST_ENDIAN_LITTLE_BYTE
   constexpr const limb_type byte_shift_mask = CHAR_BIT - 1;

#ifdef BOOST_MP_NO_CONSTEXPR_DETECTION
   if ((s & byte_shift_mask) == 0)
#else
   constexpr limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>::limb_bits - 1;
   if (BOOST_MP_IS_CONST_EVALUATED(s) && ((s & limb_shift_mask) == 0))
      right_shift_limb(result, s);
   else if (((s & byte_shift_mask) == 0) && !BOOST_MP_IS_CONST_EVALUATED(s))
#endif
      right_shift_byte(result, s);
#else
   constexpr const limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>::limb_bits - 1;

   if ((s & limb_shift_mask) == 0)
      right_shift_limb(result, s);
#endif
   else
      right_shift_generic(result, s);
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1> >::value>::type
eval_right_shift(
    cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>& result,
    double_limb_type                                                             s) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, typename cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>::checked_type());
   if (!s)
      return;

   bool is_neg = result.sign();
   if (is_neg)
      eval_increment(result);

#if BOOST_ENDIAN_LITTLE_BYTE && defined(BOOST_MP_USE_LIMB_SHIFT)
   constexpr const limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>::limb_bits - 1;
   constexpr const limb_type byte_shift_mask = CHAR_BIT - 1;

   if ((s & limb_shift_mask) == 0)
      right_shift_limb(result, s);
#ifdef BOOST_MP_NO_CONSTEXPR_DETECTION
   else if ((s & byte_shift_mask) == 0)
#else
   else if (((s & byte_shift_mask) == 0) && !BOOST_MP_IS_CONST_EVALUATED(s))
#endif
      right_shift_byte(result, s);
#elif BOOST_ENDIAN_LITTLE_BYTE
   constexpr const limb_type byte_shift_mask = CHAR_BIT - 1;

#ifdef BOOST_MP_NO_CONSTEXPR_DETECTION
   if ((s & byte_shift_mask) == 0)
#else
   constexpr limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>::limb_bits - 1;
   if (BOOST_MP_IS_CONST_EVALUATED(s) && ((s & limb_shift_mask) == 0))
      right_shift_limb(result, s);
   else if (((s & byte_shift_mask) == 0) && !BOOST_MP_IS_CONST_EVALUATED(s))
#endif
      right_shift_byte(result, s);
#else
   constexpr const limb_type limb_shift_mask = cpp_int_backend<MinBits1, MaxBits1, signed_magnitude, Checked1, Allocator1>::limb_bits - 1;

   if ((s & limb_shift_mask) == 0)
      right_shift_limb(result, s);
#endif
   else
      right_shift_generic(result, s);
   if (is_neg)
      eval_decrement(result);
}

//
// Over again for trivial cpp_int's:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class T>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value >::type
eval_left_shift(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, T s) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   *result.limbs() = detail::checked_left_shift(*result.limbs(), s, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class T>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value >::type
eval_right_shift(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, T s) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   // Nothing to check here... just make sure we don't invoke undefined behavior:
   is_valid_bitwise_op(result, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   *result.limbs() = (static_cast<unsigned>(s) >= sizeof(*result.limbs()) * CHAR_BIT) ? 0 : (result.sign() ? ((--*result.limbs()) >> s) + 1 : *result.limbs() >> s);
   if (result.sign() && (*result.limbs() == 0))
      result = static_cast<signed_limb_type>(-1);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)>::type
eval_complement(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   static_assert(((Checked1 != checked) || (Checked2 != checked)), "Attempt to take the complement of a signed type results in undefined behavior.");
   //
   // If we're not checked then emulate 2's complement behavior:
   //
   if (o.sign())
   {
      *result.limbs() = *o.limbs() - 1;
      result.sign(false);
   }
   else
   {
      *result.limbs() = 1 + *o.limbs();
      result.sign(true);
   }
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_complement(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = ~*o.limbs();
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_bitwise_and(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() &= *o.limbs();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)>::type
eval_bitwise_and(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, o, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());

   using default_ops::eval_bit_test;
   using default_ops::eval_increment;

   if (result.sign() || o.sign())
   {
      constexpr const unsigned m = static_unsigned_max<static_unsigned_max<MinBits1, MinBits2>::value, static_unsigned_max<MaxBits1, MaxBits2>::value>::value;
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t1(result);
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t2(o);
      eval_bitwise_and(t1, t2);
      bool s = eval_bit_test(t1, m + 1);
      if (s)
      {
         eval_complement(t1, t1);
         eval_increment(t1);
      }
      result = t1;
      result.sign(s);
   }
   else
   {
      *result.limbs() &= *o.limbs();
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_bitwise_or(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() |= *o.limbs();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)>::type
eval_bitwise_or(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, o, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());

   using default_ops::eval_bit_test;
   using default_ops::eval_increment;

   if (result.sign() || o.sign())
   {
      constexpr const unsigned m = static_unsigned_max<static_unsigned_max<MinBits1, MinBits2>::value, static_unsigned_max<MaxBits1, MaxBits2>::value>::value;
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t1(result);
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t2(o);
      eval_bitwise_or(t1, t2);
      bool s = eval_bit_test(t1, m + 1);
      if (s)
      {
         eval_complement(t1, t1);
         eval_increment(t1);
      }
      result = t1;
      result.sign(s);
   }
   else
   {
      *result.limbs() |= *o.limbs();
      result.normalize();
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_bitwise_xor(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() ^= *o.limbs();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value)>::type
eval_bitwise_xor(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   is_valid_bitwise_op(result, o, typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());

   using default_ops::eval_bit_test;
   using default_ops::eval_increment;

   if (result.sign() || o.sign())
   {
      constexpr const unsigned m = static_unsigned_max<static_unsigned_max<MinBits1, MinBits2>::value, static_unsigned_max<MaxBits1, MaxBits2>::value>::value;
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t1(result);
      cpp_int_backend<m + 1, m + 1, unsigned_magnitude, unchecked, void> t2(o);
      eval_bitwise_xor(t1, t2);
      bool s = eval_bit_test(t1, m + 1);
      if (s)
      {
         eval_complement(t1, t1);
         eval_increment(t1);
      }
      result = t1;
      result.sign(s);
   }
   else
   {
      *result.limbs() ^= *o.limbs();
   }
}

}}} // namespace boost::multiprecision::backends

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
