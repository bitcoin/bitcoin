///////////////////////////////////////////////////////////////
//  Copyright 2012-20 John Maddock. 
//  Copyright 2019-20 Christopher Kormanyos. 
//  Copyright 2019-20 Madhur Chauhan. 
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt
//
// Comparison operators for cpp_int_backend:
//
#ifndef BOOST_MP_CPP_INT_MUL_HPP
#define BOOST_MP_CPP_INT_MUL_HPP

#include <boost/multiprecision/integer.hpp>

namespace boost { namespace multiprecision { namespace backends {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif
//
// Multiplication by a single limb:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const limb_type&                                                            val) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   if (!val)
   {
      result = static_cast<limb_type>(0);
      return;
   }
   if ((void*)&a != (void*)&result)
      result.resize(a.size(), a.size());
   double_limb_type                                                                                  carry = 0;
   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_pointer       p     = result.limbs();
   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_pointer       pe    = result.limbs() + result.size();
   typename cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>::const_limb_pointer pa    = a.limbs();
   while (p != pe)
   {
      carry += static_cast<double_limb_type>(*pa) * static_cast<double_limb_type>(val);
#ifdef __MSVC_RUNTIME_CHECKS
      *p = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
#else
      *p = static_cast<limb_type>(carry);
#endif
      carry >>= cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
      ++p, ++pa;
   }
   if (carry)
   {
      unsigned i = result.size();
      result.resize(i + 1, i + 1);
      if (result.size() > i)
         result.limbs()[i] = static_cast<limb_type>(carry);
   }
   result.sign(a.sign());
   if (is_fixed_precision<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value)
      result.normalize();
}

//
// resize_for_carry forces a resize of the underlying buffer only if a previous request
// for "required" elements could possibly have failed, *and* we have checking enabled.
// This will cause an overflow error inside resize():
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR void resize_for_carry(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& /*result*/, unsigned /*required*/) {}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, class Allocator1>
inline BOOST_MP_CXX14_CONSTEXPR void resize_for_carry(cpp_int_backend<MinBits1, MaxBits1, SignType1, checked, Allocator1>& result, unsigned required)
{
   if (result.size() < required)
      result.resize(required, required);
}
//
// Minimum number of limbs required for Karatsuba to be worthwhile:
//
#ifdef BOOST_MP_KARATSUBA_CUTOFF
const size_t karatsuba_cutoff = BOOST_MP_KARATSUBA_CUTOFF;
#else
const size_t karatsuba_cutoff = 40;
#endif
//
// Core (recursive) Karatsuba multiplication, all the storage required is allocated upfront and 
// passed down the stack in this routine.  Note that all the cpp_int_backend's must be the same type
// and full variable precision.  Karatsuba really doesn't play nice with fixed-size integers.  If necessary
// fixed precision integers will get aliased as variable-precision types before this is called.
//
template <unsigned MinBits, unsigned MaxBits, cpp_int_check_type Checked, class Allocator>
inline void multiply_karatsuba(
    cpp_int_backend<MinBits, MaxBits, signed_magnitude, Checked, Allocator>&       result,
    const cpp_int_backend<MinBits, MaxBits, signed_magnitude, Checked, Allocator>& a,
    const cpp_int_backend<MinBits, MaxBits, signed_magnitude, Checked, Allocator>& b,
    typename cpp_int_backend<MinBits, MaxBits, signed_magnitude, Checked, Allocator>::scoped_shared_storage& storage)
{
   using cpp_int_type = cpp_int_backend<MinBits, MaxBits, signed_magnitude, Checked, Allocator>;

   unsigned as = a.size();
   unsigned bs = b.size();
   //
   // Termination condition: if either argument is smaller than karatsuba_cutoff
   // then schoolboy multiplication will be faster:
   //
   if ((as < karatsuba_cutoff) || (bs < karatsuba_cutoff))
   {
      eval_multiply(result, a, b);
      return;
   }
   //
   // Partitioning size: split the larger of a and b into 2 halves
   //
   unsigned n  = (as > bs ? as : bs) / 2 + 1;
   //
   // Partition a and b into high and low parts.
   // ie write a, b as a = a_h * 2^n + a_l, b = b_h * 2^n + b_l
   //
   // We could copy the high and low parts into new variables, but we'll
   // use aliasing to reference the internal limbs of a and b.  There is one wart here:
   // if a and b are mismatched in size, then n may be larger than the smaller
   // of a and b.  In that situation the high part is zero, and we have no limbs
   // to alias, so instead alias a local variable.
   // This raises 2 questions:
   // * Is this the best way to partition a and b?
   // * Since we have one high part zero, the arithmetic simplifies considerably, 
   //   so should we have a special routine for this?
   // 
   unsigned          sz = (std::min)(as, n);
   const cpp_int_type a_l(a.limbs(), 0, sz);

   sz = (std::min)(bs, n);
   const cpp_int_type b_l(b.limbs(), 0, sz);

   limb_type          zero = 0;
   const cpp_int_type a_h(as > n ? a.limbs() + n : &zero, 0, as > n ? as - n : 1);
   const cpp_int_type b_h(bs > n ? b.limbs() + n : &zero, 0, bs > n ? bs - n : 1);
   //
   // The basis for the Karatsuba algorithm is as follows:
   //
   // let                x = a_h * b_ h
   //                    y = a_l * b_l
   //                    z = (a_h + a_l)*(b_h + b_l) - x - y
   // and therefore  a * b = x * (2 ^ (2 * n))+ z * (2 ^ n) + y
   //
   // Begin by allocating our temporaries, these alias the memory already allocated in the shared storage:
   //
   cpp_int_type t1(storage, 2 * n + 2);
   cpp_int_type t2(storage, n + 1);
   cpp_int_type t3(storage, n + 1);
   //
   // Now we want:
   //
   // result = | a_h*b_h  | a_l*b_l |
   // (bits)              <-- 2*n -->
   //
   // We create aliases for the low and high parts of result, and multiply directly into them:
   //
   cpp_int_type result_low(result.limbs(), 0, 2 * n);
   cpp_int_type result_high(result.limbs(), 2 * n, result.size() - 2 * n);
   //
   // low part of result is a_l * b_l:
   //
   multiply_karatsuba(result_low, a_l, b_l, storage);
   //
   // We haven't zeroed out memory in result, so set to zero any unused limbs,
   // if a_l and b_l have mostly random bits then nothing happens here, but if
   // one is zero or nearly so, then a memset might be faster... it's not clear
   // that it's worth the extra logic though (and is darn hard to measure
   // what the "average" case is).
   //
   for (unsigned i = result_low.size(); i < 2 * n; ++i)
      result.limbs()[i] = 0;
   //
   // Set the high part of result to a_h * b_h:
   //
   multiply_karatsuba(result_high, a_h, b_h, storage);
   for (unsigned i = result_high.size() + 2 * n; i < result.size(); ++i)
      result.limbs()[i] = 0;
   //
   // Now calculate (a_h+a_l)*(b_h+b_l):
   //
   add_unsigned(t2, a_l, a_h);
   add_unsigned(t3, b_l, b_h);
   multiply_karatsuba(t1, t2, t3, storage); // t1 = (a_h+a_l)*(b_h+b_l)
   //
   // There is now a slight deviation from Karatsuba, we want to subtract
   // a_l*b_l + a_h*b_h from t1, but rather than use an addition and a subtraction
   // plus one temporary, we'll use 2 subtractions.  On the minus side, a subtraction
   // is on average slightly slower than an addition, but we save a temporary (ie memory)
   // and also hammer the same piece of memory over and over rather than 2 disparate
   // memory regions.  Overall it seems to be a slight win.
   //
   subtract_unsigned(t1, t1, result_high);
   subtract_unsigned(t1, t1, result_low);
   //
   // The final step is to left shift t1 by n bits and add to the result.
   // Rather than do an actual left shift, we can simply alias the result
   // and add to the alias:
   //
   cpp_int_type result_alias(result.limbs(), n, result.size() - n);
   add_unsigned(result_alias, result_alias, t1);
   //
   // Free up storage for use by sister branches to this one:
   //
   storage.deallocate(t1.capacity() + t2.capacity() + t3.capacity());

   result.normalize();
}

inline unsigned karatsuba_storage_size(unsigned s)
{
   // 
   // This estimates how much memory we will need based on
   // s-limb multiplication.  In an ideal world the number of limbs
   // would halve with each recursion, and our storage requirements
   // would be 4s in the limit, and rather less in practice since
   // we bail out long before we reach one limb.  In the real world
   // we don't quite halve s in each recursion, so this is an heuristic
   // which over-estimates how much we need.  We could compute an exact
   // value, but it would be rather time consuming.
   //
   return 5 * s;
}
//
// There are 2 entry point routines for Karatsuba multiplication:
// one for variable precision types, and one for fixed precision types.
// These are responsible for allocating all the storage required for the recursive
// routines above, and are always at the outermost level.
//
// Normal variable precision case comes first:
//
template <unsigned MinBits, unsigned MaxBits, cpp_integer_type SignType, cpp_int_check_type Checked, class Allocator>
inline typename std::enable_if<!is_fixed_precision<cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator> >::value>::type
setup_karatsuba(
   cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>& result,
   const cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>& a,
   const cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>& b)
{
   unsigned as = a.size();
   unsigned bs = b.size();
   unsigned s = as > bs ? as : bs;
   unsigned storage_size = karatsuba_storage_size(s);
   if (storage_size < 300)
   {
      //
      // Special case: if we don't need too much memory, we can use stack based storage
      // and save a call to the allocator, this allows us to use Karatsuba multiply
      // at lower limb counts than would otherwise be possible:
      //
      limb_type limbs[300];
      typename cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>::scoped_shared_storage storage(limbs, storage_size);
      multiply_karatsuba(result, a, b, storage);
   }
   else
   {
      typename cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>::scoped_shared_storage storage(result.allocator(), storage_size);
      multiply_karatsuba(result, a, b, storage);
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2, unsigned MinBits3, unsigned MaxBits3, cpp_integer_type SignType3, cpp_int_check_type Checked3, class Allocator3>
inline typename std::enable_if<is_fixed_precision<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_fixed_precision<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value || is_fixed_precision<cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3> >::value>::type
setup_karatsuba(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>& b)
{
   //
   // Now comes the fixed precision case.
   // In fact Karatsuba doesn't really work with fixed precision since the logic
   // requires that we calculate all the bits of the result (especially in the
   // temporaries used internally).  So... we'll convert all the arguments
   // to variable precision types by aliasing them, this also
   // reduce the number of template instantations:
   //
   using variable_precision_type = cpp_int_backend<0, 0, signed_magnitude, unchecked, std::allocator<limb_type> >;
   variable_precision_type a_t(a.limbs(), 0, a.size()), b_t(b.limbs(), 0, b.size());
   unsigned as = a.size();
   unsigned bs = b.size();
   unsigned s = as > bs ? as : bs;
   unsigned sz = as + bs;
   unsigned storage_size = karatsuba_storage_size(s);

   if (!is_fixed_precision<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || (sz * sizeof(limb_type) * CHAR_BIT <= MaxBits1))
   {
      // Result is large enough for all the bits of the result, so we can use aliasing:
      result.resize(sz, sz);
      variable_precision_type t(result.limbs(), 0, result.size());
      typename variable_precision_type::scoped_shared_storage storage(t.allocator(), storage_size);
      multiply_karatsuba(t, a_t, b_t, storage);
      result.resize(t.size(), t.size());
   }
   else
   {
      //
      // Not enough bit in result for the answer, so we must use a temporary
      // and then truncate (ie modular arithmetic):
      //
      typename variable_precision_type::scoped_shared_storage storage(variable_precision_type::allocator_type(), sz + storage_size);
      variable_precision_type t(storage, sz);
      multiply_karatsuba(t, a_t, b_t, storage);
      //
      // If there is truncation, and result is a checked type then this will throw:
      //
      result = t;
   }
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2, unsigned MinBits3, unsigned MaxBits3, cpp_integer_type SignType3, cpp_int_check_type Checked3, class Allocator3>
inline typename std::enable_if<!is_fixed_precision<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_fixed_precision<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && !is_fixed_precision<cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3> >::value>::type
setup_karatsuba(
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
   const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
   const cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>& b)
{
   //
   // Variable precision, mixed arguments, just alias and forward:
   //
   using variable_precision_type = cpp_int_backend<0, 0, signed_magnitude, unchecked, std::allocator<limb_type> >;
   variable_precision_type a_t(a.limbs(), 0, a.size()), b_t(b.limbs(), 0, b.size());
   unsigned as = a.size();
   unsigned bs = b.size();
   unsigned s = as > bs ? as : bs;
   unsigned sz = as + bs;
   unsigned storage_size = karatsuba_storage_size(s);

   result.resize(sz, sz);
   variable_precision_type t(result.limbs(), 0, result.size());
   typename variable_precision_type::scoped_shared_storage storage(t.allocator(), storage_size);
   multiply_karatsuba(t, a_t, b_t, storage);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2, unsigned MinBits3, unsigned MaxBits3, cpp_integer_type SignType3, cpp_int_check_type Checked3, class Allocator3>
inline BOOST_MP_CXX14_CONSTEXPR void
eval_multiply_comba(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>& b) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   // 
   // see PR #182
   // Comba Multiplier - based on Paul Comba's
   // Exponentiation cryptosystems on the IBM PC, 1990
   //
   int as                                                                                         = a.size(),
       bs                                                                                         = b.size(),
       rs                                                                                         = result.size();
   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_pointer pr = result.limbs();

   double_limb_type carry    = 0,
                    temp     = 0;
   limb_type      overflow   = 0;
   const unsigned limb_bits  = sizeof(limb_type) * CHAR_BIT;
   const bool     must_throw = rs < as + bs - 1;
   for (int r = 0, lim = (std::min)(rs, as + bs - 1); r < lim; ++r, overflow = 0)
   {
      int i = r >= as ? as - 1 : r,
          j = r - i,
          k = i < bs - j ? i + 1 : bs - j; // min(i+1, bs-j);

      typename cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>::const_limb_pointer pa = a.limbs() + i;
      typename cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>::const_limb_pointer pb = b.limbs() + j;

      temp = carry;
      carry += static_cast<double_limb_type>(*(pa)) * (*(pb));
      overflow += carry < temp;
      for (--k; k; k--)
      {
         temp = carry;
         carry += static_cast<double_limb_type>(*(--pa)) * (*(++pb));
         overflow += carry < temp;
      }
      *(pr++) = static_cast<limb_type>(carry);
      carry   = (static_cast<double_limb_type>(overflow) << limb_bits) | (carry >> limb_bits);
   }
   if (carry || must_throw)
   {
      resize_for_carry(result, as + bs);
      if ((int)result.size() >= as + bs)
         *pr = static_cast<limb_type>(carry);
   }
}
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2, unsigned MinBits3, unsigned MaxBits3, cpp_integer_type SignType3, cpp_int_check_type Checked3, class Allocator3>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>& b) 
   noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value 
      && (karatsuba_cutoff * sizeof(limb_type) * CHAR_BIT > MaxBits1) 
      && (karatsuba_cutoff * sizeof(limb_type)* CHAR_BIT > MaxBits2) 
      && (karatsuba_cutoff * sizeof(limb_type)* CHAR_BIT > MaxBits3)))
{
   // Uses simple (O(n^2)) multiplication when the limbs are less
   // otherwise switches to karatsuba algorithm based on experimental value (~40 limbs)
   //
   // Trivial cases first:
   //
   unsigned                                                                                          as = a.size();
   unsigned                                                                                          bs = b.size();
   typename cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>::const_limb_pointer pa = a.limbs();
   typename cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>::const_limb_pointer pb = b.limbs();
   if (as == 1)
   {
      bool s = b.sign() != a.sign();
      if (bs == 1)
      {
         result = static_cast<double_limb_type>(*pa) * static_cast<double_limb_type>(*pb);
      }
      else
      {
         limb_type l = *pa;
         eval_multiply(result, b, l);
      }
      result.sign(s);
      return;
   }
   if (bs == 1)
   {
      bool      s = b.sign() != a.sign();
      limb_type l = *pb;
      eval_multiply(result, a, l);
      result.sign(s);
      return;
   }

   if ((void*)&result == (void*)&a)
   {
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t(a);
      eval_multiply(result, t, b);
      return;
   }
   if ((void*)&result == (void*)&b)
   {
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t(b);
      eval_multiply(result, a, t);
      return;
   }

   constexpr const double_limb_type limb_max = ~static_cast<limb_type>(0u);
   constexpr const double_limb_type double_limb_max = ~static_cast<double_limb_type>(0u);

   result.resize(as + bs, as + bs - 1);
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   if (!BOOST_MP_IS_CONST_EVALUATED(as) && (as >= karatsuba_cutoff && bs >= karatsuba_cutoff))
#else
   if (as >= karatsuba_cutoff && bs >= karatsuba_cutoff)
#endif
   {
      setup_karatsuba(result, a, b);
      //
      // Set the sign of the result:
      //
      result.sign(a.sign() != b.sign());
      return;
   }
   typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_pointer pr = result.limbs();
   static_assert(double_limb_max - 2 * limb_max >= limb_max * limb_max, "failed limb size sanity check");

#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   if (BOOST_MP_IS_CONST_EVALUATED(as))
   {
      for (unsigned i = 0; i < result.size(); ++i)
         pr[i] = 0;
   }
   else
#endif
   std::memset(pr, 0, result.size() * sizeof(limb_type));   

#if defined(BOOST_MP_COMBA)
       // 
       // Comba Multiplier might not be efficient because of less efficient assembly
       // by the compiler as of 09/01/2020 (DD/MM/YY). See PR #182
       // Till then this will lay dormant :(
       //
       eval_multiply_comba(result, a, b);
#else

   double_limb_type carry = 0;
   for (unsigned i = 0; i < as; ++i)
   {
      BOOST_ASSERT(result.size() > i);
      unsigned inner_limit = !is_fixed_precision<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value ? bs : (std::min)(result.size() - i, bs);
      unsigned j           = 0;
      for (; j < inner_limit; ++j)
      {
         BOOST_ASSERT(i + j < result.size());
#if (!defined(__GLIBCXX__) && !defined(__GLIBCPP__)) || !BOOST_WORKAROUND(BOOST_GCC_VERSION, <= 50100)
         BOOST_ASSERT(!std::numeric_limits<double_limb_type>::is_specialized || ((std::numeric_limits<double_limb_type>::max)() - carry >
                                                                                 static_cast<double_limb_type>(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::max_limb_value) * static_cast<double_limb_type>(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::max_limb_value)));
#endif
         carry += static_cast<double_limb_type>(pa[i]) * static_cast<double_limb_type>(pb[j]);
         BOOST_ASSERT(!std::numeric_limits<double_limb_type>::is_specialized || ((std::numeric_limits<double_limb_type>::max)() - carry >= pr[i + j]));
         carry += pr[i + j];
#ifdef __MSVC_RUNTIME_CHECKS
         pr[i + j] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
#else
         pr[i + j] = static_cast<limb_type>(carry);
#endif
         carry >>= cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::limb_bits;
         BOOST_ASSERT(carry <= (cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::max_limb_value));
      }
      if (carry)
      {
         resize_for_carry(result, i + j + 1); // May throw if checking is enabled
         if (i + j < result.size())
#ifdef __MSVC_RUNTIME_CHECKS
            pr[i + j] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
#else
            pr[i + j] = static_cast<limb_type>(carry);
#endif
      }
      carry = 0;
   }
#endif // ifdef(BOOST_MP_COMBA) ends

   result.normalize();
   //
   // Set the sign of the result:
   //
   result.sign(a.sign() != b.sign());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a) 
   noexcept((noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>&>()))))
{
   eval_multiply(result, result, a);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, const limb_type& val) 
   noexcept((noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const limb_type&>()))))
{
   eval_multiply(result, result, val);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const double_limb_type&                                                     val) 
   noexcept(
      (noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>&>(), std::declval<const limb_type&>())))
      && (noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>())))
   )
{
   if (val <= (std::numeric_limits<limb_type>::max)())
   {
      eval_multiply(result, a, static_cast<limb_type>(val));
   }
   else
   {
#if BOOST_ENDIAN_LITTLE_BYTE && !defined(BOOST_MP_TEST_NO_LE)
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t(val);
#else
      cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t;
      t = val;
#endif
      eval_multiply(result, a, t);
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, const double_limb_type& val)
   noexcept((noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const double_limb_type&>()))))
{
   eval_multiply(result, result, val);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const signed_limb_type&                                                     val) 
   noexcept((noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>&>(), std::declval<const limb_type&>()))))
{
   if (val > 0)
      eval_multiply(result, a, static_cast<limb_type>(val));
   else
   {
      eval_multiply(result, a, static_cast<limb_type>(boost::multiprecision::detail::unsigned_abs(val)));
      result.negate();
   }
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, const signed_limb_type& val)
   noexcept((noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const limb_type&>()))))
{
   eval_multiply(result, result, val);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const signed_double_limb_type&                                              val)
   noexcept(
   (noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>&>(), std::declval<const limb_type&>())))
      && (noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>())))
   )
{
   if (val > 0)
   {
      if (val <= (std::numeric_limits<limb_type>::max)())
      {
         eval_multiply(result, a, static_cast<limb_type>(val));
         return;
      }
   }
   else if (val >= -static_cast<signed_double_limb_type>((std::numeric_limits<limb_type>::max)()))
   {
      eval_multiply(result, a, static_cast<limb_type>(boost::multiprecision::detail::unsigned_abs(val)));
      result.negate();
      return;
   }
#if BOOST_ENDIAN_LITTLE_BYTE && !defined(BOOST_MP_TEST_NO_LE)
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t(val);
#else
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> t;
   t = val;
#endif
   eval_multiply(result, a, t);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result, const signed_double_limb_type& val)
   noexcept(
   (noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const limb_type&>())))
   && (noexcept(eval_multiply(std::declval<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>(), std::declval<const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&>())))
   )
{
   eval_multiply(result, result, val);
}

//
// Now over again for trivial cpp_int's:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value)>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = detail::checked_multiply(*result.limbs(), *o.limbs(), typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   result.sign(result.sign() != o.sign());
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& o) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = detail::checked_multiply(*result.limbs(), *o.limbs(), typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value)>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& b) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = detail::checked_multiply(*a.limbs(), *b.limbs(), typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   result.sign(a.sign() != b.sign());
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& a,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& b) noexcept((is_non_throwing_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value))
{
   *result.limbs() = detail::checked_multiply(*a.limbs(), *b.limbs(), typename cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>::checked_type());
   result.normalize();
}

//
// Special routines for multiplying two integers to obtain a multiprecision result:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    signed_double_limb_type a, signed_double_limb_type b)
{
   constexpr const signed_double_limb_type mask = ~static_cast<limb_type>(0);
   constexpr const unsigned limb_bits = sizeof(limb_type) * CHAR_BIT;

   bool s = false;
   if (a < 0)
   {
      a = -a;
      s = true;
   }
   if (b < 0)
   {
      b = -b;
      s = !s;
   }
   double_limb_type w = a & mask;
   double_limb_type x = a >> limb_bits;
   double_limb_type y = b & mask;
   double_limb_type z = b >> limb_bits;

   result.resize(4, 4);
   limb_type* pr = result.limbs();

   double_limb_type carry = w * y;
#ifdef __MSVC_RUNTIME_CHECKS
   pr[0] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   carry >>= limb_bits;
   carry += w * z + x * y;
   pr[1] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   carry >>= limb_bits;
   carry += x * z;
   pr[2] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   pr[3] = static_cast<limb_type>(carry >> limb_bits);
#else
   pr[0] = static_cast<limb_type>(carry);
   carry >>= limb_bits;
   carry += w * z + x * y;
   pr[1] = static_cast<limb_type>(carry);
   carry >>= limb_bits;
   carry += x * z;
   pr[2] = static_cast<limb_type>(carry);
   pr[3] = static_cast<limb_type>(carry >> limb_bits);
#endif
   result.sign(s);
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    double_limb_type a, double_limb_type b)
{
   constexpr const signed_double_limb_type mask = ~static_cast<limb_type>(0);
   constexpr const unsigned limb_bits = sizeof(limb_type) * CHAR_BIT;

   double_limb_type w = a & mask;
   double_limb_type x = a >> limb_bits;
   double_limb_type y = b & mask;
   double_limb_type z = b >> limb_bits;

   result.resize(4, 4);
   limb_type* pr = result.limbs();

   double_limb_type carry = w * y;
#ifdef __MSVC_RUNTIME_CHECKS
   pr[0] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   carry >>= limb_bits;
   carry += w * z;
   pr[1] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   carry >>= limb_bits;
   pr[2] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   carry = x * y + pr[1];
   pr[1] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   carry >>= limb_bits;
   carry += pr[2] + x * z;
   pr[2] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
   pr[3] = static_cast<limb_type>(carry >> limb_bits);
#else
   pr[0] = static_cast<limb_type>(carry);
   carry >>= limb_bits;
   carry += w * z;
   pr[1] = static_cast<limb_type>(carry);
   carry >>= limb_bits;
   pr[2] = static_cast<limb_type>(carry);
   carry = x * y + pr[1];
   pr[1] = static_cast<limb_type>(carry);
   carry >>= limb_bits;
   carry += pr[2] + x * z;
   pr[2] = static_cast<limb_type>(carry);
   pr[3] = static_cast<limb_type>(carry >> limb_bits);
#endif
   result.sign(false);
   result.normalize();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1,
          unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    !is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> const& a,
    cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> const& b)
{
   using canonical_type = typename boost::multiprecision::detail::canonical<typename cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>::local_limb_type, cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::type;
   eval_multiply(result, static_cast<canonical_type>(*a.limbs()), static_cast<canonical_type>(*b.limbs()));
   result.sign(a.sign() != b.sign());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class SI>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_signed<SI>::value && boost::multiprecision::detail::is_integral<SI>::value && (sizeof(SI) <= sizeof(signed_double_limb_type) / 2)>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    SI a, SI b)
{
   result = static_cast<signed_double_limb_type>(a) * static_cast<signed_double_limb_type>(b);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, class UI>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_unsigned<UI>::value && (sizeof(UI) <= sizeof(signed_double_limb_type) / 2)>::type
eval_multiply(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    UI a, UI b)
{
   result = static_cast<double_limb_type>(a) * static_cast<double_limb_type>(b);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}}} // namespace boost::multiprecision::backends

#endif
