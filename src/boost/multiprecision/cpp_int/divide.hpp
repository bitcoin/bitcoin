///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt
//
// Comparison operators for cpp_int_backend:
//
#ifndef BOOST_MP_CPP_INT_DIV_HPP
#define BOOST_MP_CPP_INT_DIV_HPP

namespace boost { namespace multiprecision { namespace backends {

template <class CppInt1, class CppInt2, class CppInt3>
BOOST_MP_CXX14_CONSTEXPR void divide_unsigned_helper(
    CppInt1*       result,
    const CppInt2& x,
    const CppInt3& y,
    CppInt1&       r)
{
   if (((void*)result == (void*)&x) || ((void*)&r == (void*)&x))
   {
      CppInt2 t(x);
      divide_unsigned_helper(result, t, y, r);
      return;
   }
   if (((void*)result == (void*)&y) || ((void*)&r == (void*)&y))
   {
      CppInt3 t(y);
      divide_unsigned_helper(result, x, t, r);
      return;
   }

   /*
    Very simple, fairly braindead long division.
    Start by setting the remainder equal to x, and the
    result equal to 0.  Then in each loop we calculate our
    "best guess" for how many times y divides into r,
    add our guess to the result, and subtract guess*y
    from the remainder r.  One wrinkle is that the remainder
    may go negative, in which case we subtract the current guess
    from the result rather than adding.  The value of the guess
    is determined by dividing the most-significant-limb of the
    current remainder by the most-significant-limb of y.

    Note that there are more efficient algorithms than this
    available, in particular see Knuth Vol 2.  However for small
    numbers of limbs this generally outperforms the alternatives
    and avoids the normalisation step which would require extra storage.
    */

   using default_ops::eval_subtract;

   if (result == &r)
   {
      CppInt1 rem;
      divide_unsigned_helper(result, x, y, rem);
      r = rem;
      return;
   }

   //
   // Find the most significant words of numerator and denominator.
   //
   limb_type y_order = y.size() - 1;

   if (y_order == 0)
   {
      //
      // Only a single non-zero limb in the denominator, in this case
      // we can use a specialized divide-by-single-limb routine which is
      // much faster.  This also handles division by zero:
      //
      divide_unsigned_helper(result, x, y.limbs()[y_order], r);
      return;
   }

   typename CppInt2::const_limb_pointer px = x.limbs();
   typename CppInt3::const_limb_pointer py = y.limbs();

   limb_type r_order = x.size() - 1;
   if ((r_order == 0) && (*px == 0))
   {
      // x is zero, so is the result:
      r = x;
      if (result)
         *result = x;
      return;
   }

   r = x;
   r.sign(false);
   if (result)
      *result = static_cast<limb_type>(0u);
   //
   // Check if the remainder is already less than the divisor, if so
   // we already have the result.  Note we try and avoid a full compare
   // if we can:
   //
   if (r_order <= y_order)
   {
      if ((r_order < y_order) || (r.compare_unsigned(y) < 0))
      {
         return;
      }
   }

   CppInt1 t;
   bool    r_neg = false;

   //
   // See if we can short-circuit long division, and use basic arithmetic instead:
   //
   if (r_order == 0)
   {
      if (result)
      {
         *result = px[0] / py[0];
      }
      r = px[0] % py[0];
      return;
   }
   else if (r_order == 1)
   {
      double_limb_type a = (static_cast<double_limb_type>(px[1]) << CppInt1::limb_bits) | px[0];
      double_limb_type b = y_order ? (static_cast<double_limb_type>(py[1]) << CppInt1::limb_bits) | py[0]
                                   : py[0];
      if (result)
      {
         *result = a / b;
      }
      r = a % b;
      return;
   }
   //
   // prepare result:
   //
   if (result)
      result->resize(1 + r_order - y_order, 1 + r_order - y_order);
   typename CppInt1::const_limb_pointer prem = r.limbs();
   // This is initialised just to keep the compiler from emitting useless warnings later on:
   typename CppInt1::limb_pointer pr = typename CppInt1::limb_pointer();
   if (result)
   {
      pr = result->limbs();
      for (unsigned i = 1; i < 1 + r_order - y_order; ++i)
         pr[i] = 0;
   }
   bool first_pass = true;

   do
   {
      //
      // Calculate our best guess for how many times y divides into r:
      //
      limb_type guess = 1;
      if ((prem[r_order] <= py[y_order]) && (r_order > 0))
      {
         double_limb_type a = (static_cast<double_limb_type>(prem[r_order]) << CppInt1::limb_bits) | prem[r_order - 1];
         double_limb_type b = py[y_order];
         double_limb_type v = a / b;
         if (v <= CppInt1::max_limb_value)
         {
            guess = static_cast<limb_type>(v);
            --r_order;
         }
      }
      else if (r_order == 0)
      {
         guess = prem[0] / py[y_order];
      }
      else
      {
         double_limb_type a = (static_cast<double_limb_type>(prem[r_order]) << CppInt1::limb_bits) | prem[r_order - 1];
         double_limb_type b = (y_order > 0) ? (static_cast<double_limb_type>(py[y_order]) << CppInt1::limb_bits) | py[y_order - 1] : (static_cast<double_limb_type>(py[y_order]) << CppInt1::limb_bits);
         BOOST_ASSERT(b);
         double_limb_type v = a / b;
         guess              = static_cast<limb_type>(v);
      }
      BOOST_ASSERT(guess); // If the guess ever gets to zero we go on forever....
      //
      // Update result:
      //
      limb_type shift = r_order - y_order;
      if (result)
      {
         if (r_neg)
         {
            if (pr[shift] > guess)
               pr[shift] -= guess;
            else
            {
               t.resize(shift + 1, shift + 1);
               t.limbs()[shift] = guess;
               for (unsigned i = 0; i < shift; ++i)
                  t.limbs()[i] = 0;
               eval_subtract(*result, t);
            }
         }
         else if (CppInt1::max_limb_value - pr[shift] > guess)
            pr[shift] += guess;
         else
         {
            t.resize(shift + 1, shift + 1);
            t.limbs()[shift] = guess;
            for (unsigned i = 0; i < shift; ++i)
               t.limbs()[i] = 0;
            eval_add(*result, t);
         }
      }
      //
      // Calculate guess * y, we use a fused mutiply-shift O(N) for this
      // rather than a full O(N^2) multiply:
      //
      double_limb_type carry = 0;
      t.resize(y.size() + shift + 1, y.size() + shift);
      bool                           truncated_t = (t.size() != y.size() + shift + 1);
      typename CppInt1::limb_pointer pt          = t.limbs();
      for (unsigned i = 0; i < shift; ++i)
         pt[i] = 0;
      for (unsigned i = 0; i < y.size(); ++i)
      {
         carry += static_cast<double_limb_type>(py[i]) * static_cast<double_limb_type>(guess);
#ifdef __MSVC_RUNTIME_CHECKS
         pt[i + shift] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
#else
         pt[i + shift]    = static_cast<limb_type>(carry);
#endif
         carry >>= CppInt1::limb_bits;
      }
      if (carry && !truncated_t)
      {
#ifdef __MSVC_RUNTIME_CHECKS
         pt[t.size() - 1] = static_cast<limb_type>(carry & ~static_cast<limb_type>(0));
#else
         pt[t.size() - 1] = static_cast<limb_type>(carry);
#endif
      }
      else if (!truncated_t)
      {
         t.resize(t.size() - 1, t.size() - 1);
      }
      //
      // Update r in a way that won't actually produce a negative result
      // in case the argument types are unsigned:
      //
      if (truncated_t && carry)
      {
         // We need to calculate 2^n + t - r
         // where n is the number of bits in this type.
         // Simplest way is to get 2^n - r by complementing
         // r, then add t to it.  Note that we can't call eval_complement
         // in case this is a signed checked type:
         for (unsigned i = 0; i <= r_order; ++i)
            r.limbs()[i] = ~prem[i];
         r.normalize();
         eval_increment(r);
         eval_add(r, t);
         r_neg = !r_neg;
      }
      else if (r.compare(t) > 0)
      {
         eval_subtract(r, t);
      }
      else
      {
         r.swap(t);
         eval_subtract(r, t);
         prem  = r.limbs();
         r_neg = !r_neg;
      }
      //
      // First time through we need to strip any leading zero, otherwise
      // the termination condition goes belly-up:
      //
      if (result && first_pass)
      {
         first_pass = false;
         while (pr[result->size() - 1] == 0)
            result->resize(result->size() - 1, result->size() - 1);
      }
      //
      // Update r_order:
      //
      r_order = r.size() - 1;
      if (r_order < y_order)
         break;
   }
   // Termination condition is really just a check that r > y, but with a common
   // short-circuit case handled first:
   while ((r_order > y_order) || (r.compare_unsigned(y) >= 0));

   //
   // We now just have to normalise the result:
   //
   if (r_neg && eval_get_sign(r))
   {
      // We have one too many in the result:
      if (result)
         eval_decrement(*result);
      if (y.sign())
      {
         r.negate();
         eval_subtract(r, y);
      }
      else
         eval_subtract(r, y, r);
   }

   BOOST_ASSERT(r.compare_unsigned(y) < 0); // remainder must be less than the divisor or our code has failed
}

template <class CppInt1, class CppInt2>
BOOST_MP_CXX14_CONSTEXPR void divide_unsigned_helper(
    CppInt1*       result,
    const CppInt2& x,
    limb_type      y,
    CppInt1&       r)
{
   if (((void*)result == (void*)&x) || ((void*)&r == (void*)&x))
   {
      CppInt2 t(x);
      divide_unsigned_helper(result, t, y, r);
      return;
   }

   if (result == &r)
   {
      CppInt1 rem;
      divide_unsigned_helper(result, x, y, rem);
      r = rem;
      return;
   }

   // As above, but simplified for integer divisor:

   using default_ops::eval_subtract;

   if (y == 0)
   {
      BOOST_THROW_EXCEPTION(std::overflow_error("Integer Division by zero."));
   }
   //
   // Find the most significant word of numerator.
   //
   limb_type r_order = x.size() - 1;

   //
   // Set remainder and result to their initial values:
   //
   r = x;
   r.sign(false);
   typename CppInt1::limb_pointer pr = r.limbs();

   //
   // check for x < y, try to do this without actually having to
   // do a full comparison:
   //
   if ((r_order == 0) && (*pr < y))
   {
      if (result)
         *result = static_cast<limb_type>(0u);
      return;
   }

   //
   // See if we can short-circuit long division, and use basic arithmetic instead:
   //
   if (r_order == 0)
   {
      if (result)
      {
         *result = *pr / y;
         result->sign(x.sign());
      }
      *pr %= y;
      r.sign(x.sign());
      return;
   }
   else if (r_order == 1)
   {
      double_limb_type a = (static_cast<double_limb_type>(pr[r_order]) << CppInt1::limb_bits) | pr[0];
      if (result)
      {
         *result = a / y;
         result->sign(x.sign());
      }
      r = a % y;
      r.sign(x.sign());
      return;
   }

   // This is initialised just to keep the compiler from emitting useless warnings later on:
   typename CppInt1::limb_pointer pres = typename CppInt1::limb_pointer();
   if (result)
   {
      result->resize(r_order + 1, r_order + 1);
      pres = result->limbs();
      if (result->size() > r_order)
         pres[r_order] = 0; // just in case we don't set the most significant limb below.
   }

   do
   {
      //
      // Calculate our best guess for how many times y divides into r:
      //
      if ((pr[r_order] < y) && r_order)
      {
         double_limb_type a = (static_cast<double_limb_type>(pr[r_order]) << CppInt1::limb_bits) | pr[r_order - 1];
         double_limb_type b = a % y;
         r.resize(r.size() - 1, r.size() - 1);
         --r_order;
         pr[r_order] = static_cast<limb_type>(b);
         if (result)
            pres[r_order] = static_cast<limb_type>(a / y);
         if (r_order && pr[r_order] == 0)
         {
            --r_order; // No remainder, division was exact.
            r.resize(r.size() - 1, r.size() - 1);
            if (result)
               pres[r_order] = static_cast<limb_type>(0u);
         }
      }
      else
      {
         if (result)
            pres[r_order] = pr[r_order] / y;
         pr[r_order] %= y;
         if (r_order && pr[r_order] == 0)
         {
            --r_order; // No remainder, division was exact.
            r.resize(r.size() - 1, r.size() - 1);
            if (result)
               pres[r_order] = static_cast<limb_type>(0u);
         }
      }
   }
   // Termination condition is really just a check that r >= y, but with two common
   // short-circuit cases handled first:
   while (r_order || (pr[r_order] >= y));

   if (result)
   {
      result->normalize();
      result->sign(x.sign());
   }
   r.normalize();
   r.sign(x.sign());

   BOOST_ASSERT(r.compare(y) < 0); // remainder must be less than the divisor or our code has failed
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2, unsigned MinBits3, unsigned MaxBits3, cpp_integer_type SignType3, cpp_int_check_type Checked3, class Allocator3>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3> >::value>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>& b)
{
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> r;
   bool                                                                 s = a.sign() != b.sign();
   divide_unsigned_helper(&result, a, b, r);
   result.sign(s);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    limb_type&                                                                  b)
{
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> r;
   bool                                                                 s = a.sign();
   divide_unsigned_helper(&result, a, b, r);
   result.sign(s);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    signed_limb_type&                                                           b)
{
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> r;
   bool                                                                 s = a.sign() != (b < 0);
   divide_unsigned_helper(&result, a, static_cast<limb_type>(boost::multiprecision::detail::unsigned_abs(b)), r);
   result.sign(s);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& b)
{
   // There is no in place divide:
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> a(result);
   eval_divide(result, a, b);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    limb_type                                                             b)
{
   // There is no in place divide:
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> a(result);
   eval_divide(result, a, b);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    signed_limb_type                                                      b)
{
   // There is no in place divide:
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> a(result);
   eval_divide(result, a, b);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2, unsigned MinBits3, unsigned MaxBits3, cpp_integer_type SignType3, cpp_int_check_type Checked3, class Allocator3>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3> >::value>::type
eval_modulus(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const cpp_int_backend<MinBits3, MaxBits3, SignType3, Checked3, Allocator3>& b)
{
   bool s = a.sign();
   if (b.size() == 1)
      eval_modulus(result, a, *b.limbs());
   else
      divide_unsigned_helper(static_cast<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>*>(0), a, b, result);
   result.sign(s);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_modulus(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
    const limb_type                                                             mod)
{
   const int              n         = a.size();
   const double_limb_type two_n_mod = static_cast<limb_type>(1u) + (~static_cast<limb_type>(0u) - mod) % mod;
   limb_type              res       = a.limbs()[n - 1] % mod;

   for (int i = n - 2; i >= 0; --i)
      res = (res * two_n_mod + a.limbs()[i]) % mod;
   //
   // We must not modify result until here in case
   // result and a are the same object:
   //
   result.resize(1, 1);
   *result.limbs() = res;
   result.sign(a.sign());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_modulus(
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
   const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& a,
   signed_limb_type                                                            b)
{
   const limb_type t = b < 0 ? -b : b;
   eval_modulus(result, a, t);
   result.sign(a.sign());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1, unsigned MinBits2, unsigned MaxBits2, cpp_integer_type SignType2, cpp_int_check_type Checked2, class Allocator2>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && !is_trivial_cpp_int<cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2> >::value>::type
eval_modulus(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits2, MaxBits2, SignType2, Checked2, Allocator2>& b)
{
   // There is no in place divide:
   cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> a(result);
   eval_modulus(result, a, b);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_modulus(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    limb_type                                                             b)
{
   // Single limb modulus is in place:
   eval_modulus(result, result, b);
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_modulus(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& result,
    signed_limb_type                                                      b)
{
   // Single limb modulus is in place:
   eval_modulus(result, result, b);
}

//
// Over again for trivial cpp_int's:
//
template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && (is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value || is_signed_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value)>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& o)
{
   if (!*o.limbs())
      BOOST_THROW_EXCEPTION(std::overflow_error("Division by zero."));
   *result.limbs() /= *o.limbs();
   result.sign(result.sign() != o.sign());
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_unsigned_number<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_divide(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& o)
{
   if (!*o.limbs())
      BOOST_THROW_EXCEPTION(std::overflow_error("Division by zero."));
   *result.limbs() /= *o.limbs();
}

template <unsigned MinBits1, unsigned MaxBits1, cpp_integer_type SignType1, cpp_int_check_type Checked1, class Allocator1>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value && is_trivial_cpp_int<cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1> >::value>::type
eval_modulus(
    cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>&       result,
    const cpp_int_backend<MinBits1, MaxBits1, SignType1, Checked1, Allocator1>& o)
{
   if (!*o.limbs())
      BOOST_THROW_EXCEPTION(std::overflow_error("Division by zero."));
   *result.limbs() %= *o.limbs();
   result.sign(result.sign());
}

}}} // namespace boost::multiprecision::backends

#endif
