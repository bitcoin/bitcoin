
// Copyright Christopher Kormanyos 2002 - 2011.
// Copyright 2011 John Maddock.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// This work is based on an earlier work:
// "Algorithm 910: A Portable C++ Multiple-Precision System for Special-Function Calculations",
// in ACM TOMS, {VOL 37, ISSUE 4, (February 2011)} (C) ACM, 2011. http://doi.acm.org/10.1145/1916461.1916469
//
// This file has no include guards or namespaces - it's expanded inline inside default_ops.hpp
//

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 6326) // comparison of two constants
#pragma warning(disable : 4127) // conditional expression is constant
#endif

template <class T>
void hyp0F1(T& result, const T& b, const T& x)
{
   using si_type = typename boost::multiprecision::detail::canonical<std::int32_t, T>::type ;
   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;

   // Compute the series representation of Hypergeometric0F1 taken from
   // http://functions.wolfram.com/HypergeometricFunctions/Hypergeometric0F1/06/01/01/
   // There are no checks on input range or parameter boundaries.

   T x_pow_n_div_n_fact(x);
   T pochham_b(b);
   T bp(b);

   eval_divide(result, x_pow_n_div_n_fact, pochham_b);
   eval_add(result, ui_type(1));

   si_type n;

   T tol;
   tol = ui_type(1);
   eval_ldexp(tol, tol, 1 - boost::multiprecision::detail::digits2<number<T, et_on> >::value());
   eval_multiply(tol, result);
   if (eval_get_sign(tol) < 0)
      tol.negate();
   T term;

   const int series_limit =
       boost::multiprecision::detail::digits2<number<T, et_on> >::value() < 100
           ? 100
           : boost::multiprecision::detail::digits2<number<T, et_on> >::value();
   // Series expansion of hyperg_0f1(; b; x).
   for (n = 2; n < series_limit; ++n)
   {
      eval_multiply(x_pow_n_div_n_fact, x);
      eval_divide(x_pow_n_div_n_fact, n);
      eval_increment(bp);
      eval_multiply(pochham_b, bp);

      eval_divide(term, x_pow_n_div_n_fact, pochham_b);
      eval_add(result, term);

      bool neg_term = eval_get_sign(term) < 0;
      if (neg_term)
         term.negate();
      if (term.compare(tol) <= 0)
         break;
   }

   if (n >= series_limit)
      BOOST_THROW_EXCEPTION(std::runtime_error("H0F1 Failed to Converge"));
}

template <class T, unsigned N, bool b = boost::multiprecision::detail::is_variable_precision<boost::multiprecision::number<T> >::value>
struct scoped_N_precision
{
   template <class U>
   scoped_N_precision(U const&) {}
   template <class U>
   void reduce(U&) {}
};

template <class T, unsigned N>
struct scoped_N_precision<T, N, true>
{
   unsigned old_precision, old_arg_precision;
   scoped_N_precision(T& arg)
   {
      old_precision     = T::thread_default_precision();
      old_arg_precision = arg.precision();
      T::thread_default_precision(old_arg_precision * N);
      arg.precision(old_arg_precision * N);
   }
   ~scoped_N_precision()
   {
      T::thread_default_precision(old_precision);
   }
   void reduce(T& arg) 
   {
      arg.precision(old_arg_precision);
   }
};

template <class T>
void reduce_n_half_pi(T& arg, const T& n, bool go_down)
{
   //
   // We need to perform argument reduction at 3 times the precision of arg
   // in order to ensure a correct result up to arg = 1/epsilon.  Beyond that
   // the value of n will have been incorrectly calculated anyway since it will
   // have a value greater than 1/epsilon and no longer be an exact integer value.
   //
   // More information in ARGUMENT REDUCTION FOR HUGE ARGUMENTS. K C Ng.
   //
   // There are two mutually exclusive ways to achieve this, both of which are 
   // supported here:
   // 1) To define a fixed precision type with 3 times the precision for the calculation.
   // 2) To dynamically increase the precision of the variables.
   //
   using reduction_type = typename boost::multiprecision::detail::transcendental_reduction_type<T>::type;
   //
   // Make a copy of the arg at higher precision:
   //
   reduction_type big_arg(arg);
   //
   // Dynamically increase precision when supported, this increases the default
   // and ups the precision of big_arg to match:
   //
   scoped_N_precision<T, 3> scoped_precision(big_arg);
   //
   // High precision PI:
   //
   reduction_type reduction = get_constant_pi<reduction_type>();
   eval_ldexp(reduction, reduction, -1); // divide by 2
   eval_multiply(reduction, n);
   BOOST_MATH_INSTRUMENT_CODE(big_arg.str(10, std::ios_base::scientific));
   BOOST_MATH_INSTRUMENT_CODE(reduction.str(10, std::ios_base::scientific));

   if (go_down)
      eval_subtract(big_arg, reduction, big_arg);
   else
      eval_subtract(big_arg, reduction);
   arg = T(big_arg);
   //
   // If arg is a variable precision type, then we have just copied the
   // precision of big_arg s well it's value.  Reduce the precision now:
   //
   scoped_precision.reduce(arg);
   BOOST_MATH_INSTRUMENT_CODE(big_arg.str(10, std::ios_base::scientific));
   BOOST_MATH_INSTRUMENT_CODE(arg.str(10, std::ios_base::scientific));
}

template <class T>
void eval_sin(T& result, const T& x)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The sin function is only valid for floating point types.");
   BOOST_MATH_INSTRUMENT_CODE(x.str(0, std::ios_base::scientific));
   if (&result == &x)
   {
      T temp;
      eval_sin(temp, x);
      result = temp;
      return;
   }

   using si_type = typename boost::multiprecision::detail::canonical<std::int32_t, T>::type ;
   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;
   using fp_type = typename std::tuple_element<0, typename T::float_types>::type                         ;

   switch (eval_fpclassify(x))
   {
   case FP_INFINITE:
   case FP_NAN:
      BOOST_IF_CONSTEXPR(std::numeric_limits<number<T, et_on> >::has_quiet_NaN)
      {
         result = std::numeric_limits<number<T, et_on> >::quiet_NaN().backend();
         errno  = EDOM;
      }
      else
         BOOST_THROW_EXCEPTION(std::domain_error("Result is undefined or complex and there is no NaN for this number type."));
      return;
   case FP_ZERO:
      result = x;
      return;
   default:;
   }

   // Local copy of the argument
   T xx = x;

   // Analyze and prepare the phase of the argument.
   // Make a local, positive copy of the argument, xx.
   // The argument xx will be reduced to 0 <= xx <= pi/2.
   bool b_negate_sin = false;

   if (eval_get_sign(x) < 0)
   {
      xx.negate();
      b_negate_sin = !b_negate_sin;
   }

   T n_pi, t;
   T half_pi = get_constant_pi<T>();
   eval_ldexp(half_pi, half_pi, -1); // divide by 2
   // Remove multiples of pi/2.
   if (xx.compare(half_pi) > 0)
   {
      eval_divide(n_pi, xx, half_pi);
      eval_trunc(n_pi, n_pi);
      t = ui_type(4);
      eval_fmod(t, n_pi, t);
      bool b_go_down = false;
      if (t.compare(ui_type(1)) == 0)
      {
         b_go_down = true;
      }
      else if (t.compare(ui_type(2)) == 0)
      {
         b_negate_sin = !b_negate_sin;
      }
      else if (t.compare(ui_type(3)) == 0)
      {
         b_negate_sin = !b_negate_sin;
         b_go_down    = true;
      }

      if (b_go_down)
         eval_increment(n_pi);
      //
      // If n_pi is > 1/epsilon, then it is no longer an exact integer value
      // but an approximation.  As a result we can no longer reliably reduce
      // xx to 0 <= xx < pi/2, nor can we tell the sign of the result as we need
      // n_pi % 4 for that, but that will always be zero in this situation.
      // We could use a higher precision type for n_pi, along with division at
      // higher precision, but that's rather expensive.  So for now we do not support
      // this, and will see if anyone complains and has a legitimate use case.
      //
      if (n_pi.compare(get_constant_one_over_epsilon<T>()) > 0)
      {
         result = ui_type(0);
         return;
      }

      reduce_n_half_pi(xx, n_pi, b_go_down);
      //
      // Post reduction we may be a few ulp below zero or above pi/2
      // given that n_pi was calculated at working precision and not
      // at the higher precision used for reduction.  Correct that now:
      //
      if (eval_get_sign(xx) < 0)
      {
         xx.negate();
         b_negate_sin = !b_negate_sin;
      }
      if (xx.compare(half_pi) > 0)
      {
         eval_ldexp(half_pi, half_pi, 1);
         eval_subtract(xx, half_pi, xx);
         eval_ldexp(half_pi, half_pi, -1);
         b_go_down = !b_go_down;
      }

      BOOST_MATH_INSTRUMENT_CODE(xx.str(0, std::ios_base::scientific));
      BOOST_MATH_INSTRUMENT_CODE(n_pi.str(0, std::ios_base::scientific));
      BOOST_ASSERT(xx.compare(half_pi) <= 0);
      BOOST_ASSERT(xx.compare(ui_type(0)) >= 0);
   }

   t = half_pi;
   eval_subtract(t, xx);

   const bool b_zero    = eval_get_sign(xx) == 0;
   const bool b_pi_half = eval_get_sign(t) == 0;

   BOOST_MATH_INSTRUMENT_CODE(xx.str(0, std::ios_base::scientific));
   BOOST_MATH_INSTRUMENT_CODE(t.str(0, std::ios_base::scientific));

   // Check if the reduced argument is very close to 0 or pi/2.
   const bool b_near_zero    = xx.compare(fp_type(1e-1)) < 0;
   const bool b_near_pi_half = t.compare(fp_type(1e-1)) < 0;

   if (b_zero)
   {
      result = ui_type(0);
   }
   else if (b_pi_half)
   {
      result = ui_type(1);
   }
   else if (b_near_zero)
   {
      eval_multiply(t, xx, xx);
      eval_divide(t, si_type(-4));
      T t2;
      t2 = fp_type(1.5);
      hyp0F1(result, t2, t);
      BOOST_MATH_INSTRUMENT_CODE(result.str(0, std::ios_base::scientific));
      eval_multiply(result, xx);
   }
   else if (b_near_pi_half)
   {
      eval_multiply(t, t);
      eval_divide(t, si_type(-4));
      T t2;
      t2 = fp_type(0.5);
      hyp0F1(result, t2, t);
      BOOST_MATH_INSTRUMENT_CODE(result.str(0, std::ios_base::scientific));
   }
   else
   {
      // Scale to a small argument for an efficient Taylor series,
      // implemented as a hypergeometric function. Use a standard
      // divide by three identity a certain number of times.
      // Here we use division by 3^9 --> (19683 = 3^9).

      constexpr const si_type n_scale           = 9;
      constexpr const si_type n_three_pow_scale = static_cast<si_type>(19683L);

      eval_divide(xx, n_three_pow_scale);

      // Now with small arguments, we are ready for a series expansion.
      eval_multiply(t, xx, xx);
      eval_divide(t, si_type(-4));
      T t2;
      t2 = fp_type(1.5);
      hyp0F1(result, t2, t);
      BOOST_MATH_INSTRUMENT_CODE(result.str(0, std::ios_base::scientific));
      eval_multiply(result, xx);

      // Convert back using multiple angle identity.
      for (std::int32_t k = static_cast<std::int32_t>(0); k < n_scale; k++)
      {
         // Rescale the cosine value using the multiple angle identity.
         eval_multiply(t2, result, ui_type(3));
         eval_multiply(t, result, result);
         eval_multiply(t, result);
         eval_multiply(t, ui_type(4));
         eval_subtract(result, t2, t);
      }
   }

   if (b_negate_sin)
      result.negate();
   BOOST_MATH_INSTRUMENT_CODE(result.str(0, std::ios_base::scientific));
}

template <class T>
void eval_cos(T& result, const T& x)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The cos function is only valid for floating point types.");
   if (&result == &x)
   {
      T temp;
      eval_cos(temp, x);
      result = temp;
      return;
   }

   using si_type = typename boost::multiprecision::detail::canonical<std::int32_t, T>::type ;
   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;

   switch (eval_fpclassify(x))
   {
   case FP_INFINITE:
   case FP_NAN:
      BOOST_IF_CONSTEXPR(std::numeric_limits<number<T, et_on> >::has_quiet_NaN)
      {
         result = std::numeric_limits<number<T, et_on> >::quiet_NaN().backend();
         errno  = EDOM;
      }
      else
         BOOST_THROW_EXCEPTION(std::domain_error("Result is undefined or complex and there is no NaN for this number type."));
      return;
   case FP_ZERO:
      result = ui_type(1);
      return;
   default:;
   }

   // Local copy of the argument
   T xx = x;

   // Analyze and prepare the phase of the argument.
   // Make a local, positive copy of the argument, xx.
   // The argument xx will be reduced to 0 <= xx <= pi/2.
   bool b_negate_cos = false;

   if (eval_get_sign(x) < 0)
   {
      xx.negate();
   }
   BOOST_MATH_INSTRUMENT_CODE(xx.str(0, std::ios_base::scientific));

   T n_pi, t;
   T half_pi = get_constant_pi<T>();
   eval_ldexp(half_pi, half_pi, -1); // divide by 2
   // Remove even multiples of pi.
   if (xx.compare(half_pi) > 0)
   {
      eval_divide(t, xx, half_pi);
      eval_trunc(n_pi, t);
      BOOST_MATH_INSTRUMENT_CODE(n_pi.str(0, std::ios_base::scientific));
      t = ui_type(4);
      eval_fmod(t, n_pi, t);

      bool b_go_down = false;
      if (t.compare(ui_type(0)) == 0)
      {
         b_go_down = true;
      }
      else if (t.compare(ui_type(1)) == 0)
      {
         b_negate_cos = true;
      }
      else if (t.compare(ui_type(2)) == 0)
      {
         b_go_down    = true;
         b_negate_cos = true;
      }
      else
      {
         BOOST_ASSERT(t.compare(ui_type(3)) == 0);
      }

      if (b_go_down)
         eval_increment(n_pi);
      //
      // If n_pi is > 1/epsilon, then it is no longer an exact integer value
      // but an approximation.  As a result we can no longer reliably reduce
      // xx to 0 <= xx < pi/2, nor can we tell the sign of the result as we need
      // n_pi % 4 for that, but that will always be zero in this situation.
      // We could use a higher precision type for n_pi, along with division at
      // higher precision, but that's rather expensive.  So for now we do not support
      // this, and will see if anyone complains and has a legitimate use case.
      //
      if (n_pi.compare(get_constant_one_over_epsilon<T>()) > 0)
      {
         result = ui_type(1);
         return;
      }

      reduce_n_half_pi(xx, n_pi, b_go_down);
      //
      // Post reduction we may be a few ulp below zero or above pi/2
      // given that n_pi was calculated at working precision and not
      // at the higher precision used for reduction.  Correct that now:
      //
      if (eval_get_sign(xx) < 0)
      {
         xx.negate();
         b_negate_cos = !b_negate_cos;
      }
      if (xx.compare(half_pi) > 0)
      {
         eval_ldexp(half_pi, half_pi, 1);
         eval_subtract(xx, half_pi, xx);
         eval_ldexp(half_pi, half_pi, -1);
      }
      BOOST_ASSERT(xx.compare(half_pi) <= 0);
      BOOST_ASSERT(xx.compare(ui_type(0)) >= 0);
   }
   else
   {
      n_pi = ui_type(1);
      reduce_n_half_pi(xx, n_pi, true);
   }

   const bool b_zero = eval_get_sign(xx) == 0;

   if (b_zero)
   {
      result = si_type(0);
   }
   else
   {
      eval_sin(result, xx);
   }
   if (b_negate_cos)
      result.negate();
   BOOST_MATH_INSTRUMENT_CODE(result.str(0, std::ios_base::scientific));
}

template <class T>
void eval_tan(T& result, const T& x)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The tan function is only valid for floating point types.");
   if (&result == &x)
   {
      T temp;
      eval_tan(temp, x);
      result = temp;
      return;
   }
   T t;
   eval_sin(result, x);
   eval_cos(t, x);
   eval_divide(result, t);
}

template <class T>
void hyp2F1(T& result, const T& a, const T& b, const T& c, const T& x)
{
   // Compute the series representation of hyperg_2f1 taken from
   // Abramowitz and Stegun 15.1.1.
   // There are no checks on input range or parameter boundaries.

   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;

   T x_pow_n_div_n_fact(x);
   T pochham_a(a);
   T pochham_b(b);
   T pochham_c(c);
   T ap(a);
   T bp(b);
   T cp(c);

   eval_multiply(result, pochham_a, pochham_b);
   eval_divide(result, pochham_c);
   eval_multiply(result, x_pow_n_div_n_fact);
   eval_add(result, ui_type(1));

   T lim;
   eval_ldexp(lim, result, 1 - boost::multiprecision::detail::digits2<number<T, et_on> >::value());

   if (eval_get_sign(lim) < 0)
      lim.negate();

   ui_type n;
   T       term;

   const unsigned series_limit =
       boost::multiprecision::detail::digits2<number<T, et_on> >::value() < 100
           ? 100
           : boost::multiprecision::detail::digits2<number<T, et_on> >::value();
   // Series expansion of hyperg_2f1(a, b; c; x).
   for (n = 2; n < series_limit; ++n)
   {
      eval_multiply(x_pow_n_div_n_fact, x);
      eval_divide(x_pow_n_div_n_fact, n);

      eval_increment(ap);
      eval_multiply(pochham_a, ap);
      eval_increment(bp);
      eval_multiply(pochham_b, bp);
      eval_increment(cp);
      eval_multiply(pochham_c, cp);

      eval_multiply(term, pochham_a, pochham_b);
      eval_divide(term, pochham_c);
      eval_multiply(term, x_pow_n_div_n_fact);
      eval_add(result, term);

      if (eval_get_sign(term) < 0)
         term.negate();
      if (lim.compare(term) >= 0)
         break;
   }
   if (n > series_limit)
      BOOST_THROW_EXCEPTION(std::runtime_error("H2F1 failed to converge."));
}

template <class T>
void eval_asin(T& result, const T& x)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The asin function is only valid for floating point types.");
   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;
   using fp_type = typename std::tuple_element<0, typename T::float_types>::type                         ;

   if (&result == &x)
   {
      T t(x);
      eval_asin(result, t);
      return;
   }

   switch (eval_fpclassify(x))
   {
   case FP_NAN:
   case FP_INFINITE:
      BOOST_IF_CONSTEXPR(std::numeric_limits<number<T, et_on> >::has_quiet_NaN)
      {
         result = std::numeric_limits<number<T, et_on> >::quiet_NaN().backend();
         errno  = EDOM;
      }
      else
         BOOST_THROW_EXCEPTION(std::domain_error("Result is undefined or complex and there is no NaN for this number type."));
      return;
   case FP_ZERO:
      result = x;
      return;
   default:;
   }

   const bool b_neg = eval_get_sign(x) < 0;

   T xx(x);
   if (b_neg)
      xx.negate();

   int c = xx.compare(ui_type(1));
   if (c > 0)
   {
      BOOST_IF_CONSTEXPR(std::numeric_limits<number<T, et_on> >::has_quiet_NaN)
      {
         result = std::numeric_limits<number<T, et_on> >::quiet_NaN().backend();
         errno  = EDOM;
      }
      else
         BOOST_THROW_EXCEPTION(std::domain_error("Result is undefined or complex and there is no NaN for this number type."));
      return;
   }
   else if (c == 0)
   {
      result = get_constant_pi<T>();
      eval_ldexp(result, result, -1);
      if (b_neg)
         result.negate();
      return;
   }

   if (xx.compare(fp_type(1e-3)) < 0)
   {
      // http://functions.wolfram.com/ElementaryFunctions/ArcSin/26/01/01/
      eval_multiply(xx, xx);
      T t1, t2;
      t1 = fp_type(0.5f);
      t2 = fp_type(1.5f);
      hyp2F1(result, t1, t1, t2, xx);
      eval_multiply(result, x);
      return;
   }
   else if (xx.compare(fp_type(1 - 5e-2f)) > 0)
   {
      // http://functions.wolfram.com/ElementaryFunctions/ArcSin/26/01/01/
      // This branch is simlilar in complexity to Newton iterations down to
      // the above limit.  It is *much* more accurate.
      T dx1;
      T t1, t2;
      eval_subtract(dx1, ui_type(1), xx);
      t1 = fp_type(0.5f);
      t2 = fp_type(1.5f);
      eval_ldexp(dx1, dx1, -1);
      hyp2F1(result, t1, t1, t2, dx1);
      eval_ldexp(dx1, dx1, 2);
      eval_sqrt(t1, dx1);
      eval_multiply(result, t1);
      eval_ldexp(t1, get_constant_pi<T>(), -1);
      result.negate();
      eval_add(result, t1);
      if (b_neg)
         result.negate();
      return;
   }
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   using guess_type = typename boost::multiprecision::detail::canonical<long double, T>::type;
#else
   using guess_type = fp_type;
#endif
   // Get initial estimate using standard math function asin.
   guess_type dd;
   eval_convert_to(&dd, xx);

   result = (guess_type)(std::asin(dd));

   // Newton-Raphson iteration, we should double our precision with each iteration,
   // in practice this seems to not quite work in all cases... so terminate when we
   // have at least 2/3 of the digits correct on the assumption that the correction
   // we've just added will finish the job...

   std::intmax_t current_precision = eval_ilogb(result);
   std::intmax_t target_precision  = std::numeric_limits<number<T> >::is_specialized ? 
      current_precision - 1 - (std::numeric_limits<number<T> >::digits * 2) / 3
      : current_precision - 1 - (boost::multiprecision::detail::digits2<number<T> >::value() * 2) / 3;

   // Newton-Raphson iteration
   while (current_precision > target_precision)
   {
      T sine, cosine;
      eval_sin(sine, result);
      eval_cos(cosine, result);
      eval_subtract(sine, xx);
      eval_divide(sine, cosine);
      eval_subtract(result, sine);
      current_precision = eval_ilogb(sine);
      if (current_precision <= (std::numeric_limits<typename T::exponent_type>::min)() + 1)
         break;
   }
   if (b_neg)
      result.negate();
}

template <class T>
inline void eval_acos(T& result, const T& x)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The acos function is only valid for floating point types.");
   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;

   switch (eval_fpclassify(x))
   {
   case FP_NAN:
   case FP_INFINITE:
      BOOST_IF_CONSTEXPR(std::numeric_limits<number<T, et_on> >::has_quiet_NaN)
      {
         result = std::numeric_limits<number<T, et_on> >::quiet_NaN().backend();
         errno  = EDOM;
      }
      else
         BOOST_THROW_EXCEPTION(std::domain_error("Result is undefined or complex and there is no NaN for this number type."));
      return;
   case FP_ZERO:
      result = get_constant_pi<T>();
      eval_ldexp(result, result, -1); // divide by two.
      return;
   }

   T xx;
   eval_abs(xx, x);
   int c = xx.compare(ui_type(1));

   if (c > 0)
   {
      BOOST_IF_CONSTEXPR(std::numeric_limits<number<T, et_on> >::has_quiet_NaN)
      {
         result = std::numeric_limits<number<T, et_on> >::quiet_NaN().backend();
         errno  = EDOM;
      }
      else
         BOOST_THROW_EXCEPTION(std::domain_error("Result is undefined or complex and there is no NaN for this number type."));
      return;
   }
   else if (c == 0)
   {
      if (eval_get_sign(x) < 0)
         result = get_constant_pi<T>();
      else
         result = ui_type(0);
      return;
   }

   using fp_type = typename std::tuple_element<0, typename T::float_types>::type;

   if (xx.compare(fp_type(1e-3)) < 0)
   {
      // https://functions.wolfram.com/ElementaryFunctions/ArcCos/26/01/01/
      eval_multiply(xx, xx);
      T t1, t2;
      t1 = fp_type(0.5f);
      t2 = fp_type(1.5f);
      hyp2F1(result, t1, t1, t2, xx);
      eval_multiply(result, x);
      eval_ldexp(t1, get_constant_pi<T>(), -1);
      result.negate();
      eval_add(result, t1);
      return;
   }
   if (eval_get_sign(x) < 0)
   {
      eval_acos(result, xx);
      result.negate();
      eval_add(result, get_constant_pi<T>());
      return;
   }
   else if (xx.compare(fp_type(0.85)) > 0)
   {
      // https://functions.wolfram.com/ElementaryFunctions/ArcCos/26/01/01/
      // This branch is simlilar in complexity to Newton iterations down to
      // the above limit.  It is *much* more accurate.
      T dx1;
      T t1, t2;
      eval_subtract(dx1, ui_type(1), xx);
      t1 = fp_type(0.5f);
      t2 = fp_type(1.5f);
      eval_ldexp(dx1, dx1, -1);
      hyp2F1(result, t1, t1, t2, dx1);
      eval_ldexp(dx1, dx1, 2);
      eval_sqrt(t1, dx1);
      eval_multiply(result, t1);
      return;
   }

#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   using guess_type = typename boost::multiprecision::detail::canonical<long double, T>::type;
#else
   using guess_type = fp_type;
#endif
   // Get initial estimate using standard math function asin.
   guess_type dd;
   eval_convert_to(&dd, xx);

   result = (guess_type)(std::acos(dd));

   // Newton-Raphson iteration, we should double our precision with each iteration,
   // in practice this seems to not quite work in all cases... so terminate when we
   // have at least 2/3 of the digits correct on the assumption that the correction
   // we've just added will finish the job...

   std::intmax_t current_precision = eval_ilogb(result);
   std::intmax_t target_precision = std::numeric_limits<number<T> >::is_specialized ?
      current_precision - 1 - (std::numeric_limits<number<T> >::digits * 2) / 3
      : current_precision - 1 - (boost::multiprecision::detail::digits2<number<T> >::value() * 2) / 3;

   // Newton-Raphson iteration
   while (current_precision > target_precision)
   {
      T sine, cosine;
      eval_sin(sine, result);
      eval_cos(cosine, result);
      eval_subtract(cosine, xx);
      cosine.negate();
      eval_divide(cosine, sine);
      eval_subtract(result, cosine);
      current_precision = eval_ilogb(cosine);
      if (current_precision <= (std::numeric_limits<typename T::exponent_type>::min)() + 1)
         break;
   }
}

template <class T>
void eval_atan(T& result, const T& x)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The atan function is only valid for floating point types.");
   using si_type = typename boost::multiprecision::detail::canonical<std::int32_t, T>::type ;
   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;
   using fp_type = typename std::tuple_element<0, typename T::float_types>::type                         ;

   switch (eval_fpclassify(x))
   {
   case FP_NAN:
      result = x;
      errno  = EDOM;
      return;
   case FP_ZERO:
      result = x;
      return;
   case FP_INFINITE:
      if (eval_get_sign(x) < 0)
      {
         eval_ldexp(result, get_constant_pi<T>(), -1);
         result.negate();
      }
      else
         eval_ldexp(result, get_constant_pi<T>(), -1);
      return;
   default:;
   }

   const bool b_neg = eval_get_sign(x) < 0;

   T xx(x);
   if (b_neg)
      xx.negate();

   if (xx.compare(fp_type(0.1)) < 0)
   {
      T t1, t2, t3;
      t1 = ui_type(1);
      t2 = fp_type(0.5f);
      t3 = fp_type(1.5f);
      eval_multiply(xx, xx);
      xx.negate();
      hyp2F1(result, t1, t2, t3, xx);
      eval_multiply(result, x);
      return;
   }

   if (xx.compare(fp_type(10)) > 0)
   {
      T t1, t2, t3;
      t1 = fp_type(0.5f);
      t2 = ui_type(1u);
      t3 = fp_type(1.5f);
      eval_multiply(xx, xx);
      eval_divide(xx, si_type(-1), xx);
      hyp2F1(result, t1, t2, t3, xx);
      eval_divide(result, x);
      if (!b_neg)
         result.negate();
      eval_ldexp(t1, get_constant_pi<T>(), -1);
      eval_add(result, t1);
      if (b_neg)
         result.negate();
      return;
   }

   // Get initial estimate using standard math function atan.
   fp_type d;
   eval_convert_to(&d, xx);
   result = fp_type(std::atan(d));

   // Newton-Raphson iteration, we should double our precision with each iteration,
   // in practice this seems to not quite work in all cases... so terminate when we
   // have at least 2/3 of the digits correct on the assumption that the correction
   // we've just added will finish the job...

   std::intmax_t current_precision = eval_ilogb(result);
   std::intmax_t target_precision  = std::numeric_limits<number<T> >::is_specialized ?
      current_precision - 1 - (std::numeric_limits<number<T> >::digits * 2) / 3
      : current_precision - 1 - (boost::multiprecision::detail::digits2<number<T> >::value() * 2) / 3;

   T s, c, t;
   while (current_precision > target_precision)
   {
      eval_sin(s, result);
      eval_cos(c, result);
      eval_multiply(t, xx, c);
      eval_subtract(t, s);
      eval_multiply(s, t, c);
      eval_add(result, s);
      current_precision = eval_ilogb(s);
      if (current_precision <= (std::numeric_limits<typename T::exponent_type>::min)() + 1)
         break;
   }
   if (b_neg)
      result.negate();
}

template <class T>
void eval_atan2(T& result, const T& y, const T& x)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The atan2 function is only valid for floating point types.");
   if (&result == &y)
   {
      T temp(y);
      eval_atan2(result, temp, x);
      return;
   }
   else if (&result == &x)
   {
      T temp(x);
      eval_atan2(result, y, temp);
      return;
   }

   using ui_type = typename boost::multiprecision::detail::canonical<std::uint32_t, T>::type;

   switch (eval_fpclassify(y))
   {
   case FP_NAN:
      result = y;
      errno  = EDOM;
      return;
   case FP_ZERO:
   {
      if (eval_signbit(x))
      {
         result = get_constant_pi<T>();
         if (eval_signbit(y))
            result.negate();
      }
      else
      {
         result = y; // Note we allow atan2(0,0) to be +-zero, even though it's mathematically undefined
      }
      return;
   }
   case FP_INFINITE:
   {
      if (eval_fpclassify(x) == FP_INFINITE)
      {
         if (eval_signbit(x))
         {
            // 3Pi/4
            eval_ldexp(result, get_constant_pi<T>(), -2);
            eval_subtract(result, get_constant_pi<T>());
            if (eval_get_sign(y) >= 0)
               result.negate();
         }
         else
         {
            // Pi/4
            eval_ldexp(result, get_constant_pi<T>(), -2);
            if (eval_get_sign(y) < 0)
               result.negate();
         }
      }
      else
      {
         eval_ldexp(result, get_constant_pi<T>(), -1);
         if (eval_get_sign(y) < 0)
            result.negate();
      }
      return;
   }
   }

   switch (eval_fpclassify(x))
   {
   case FP_NAN:
      result = x;
      errno  = EDOM;
      return;
   case FP_ZERO:
   {
      eval_ldexp(result, get_constant_pi<T>(), -1);
      if (eval_get_sign(y) < 0)
         result.negate();
      return;
   }
   case FP_INFINITE:
      if (eval_get_sign(x) > 0)
         result = ui_type(0);
      else
         result = get_constant_pi<T>();
      if (eval_get_sign(y) < 0)
         result.negate();
      return;
   }

   T xx;
   eval_divide(xx, y, x);
   if (eval_get_sign(xx) < 0)
      xx.negate();

   eval_atan(result, xx);

   // Determine quadrant (sign) based on signs of x, y
   const bool y_neg = eval_get_sign(y) < 0;
   const bool x_neg = eval_get_sign(x) < 0;

   if (y_neg != x_neg)
      result.negate();

   if (x_neg)
   {
      if (y_neg)
         eval_subtract(result, get_constant_pi<T>());
      else
         eval_add(result, get_constant_pi<T>());
   }
}
template <class T, class A>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value, void>::type eval_atan2(T& result, const T& x, const A& a)
{
   using canonical_type = typename boost::multiprecision::detail::canonical<A, T>::type         ;
   using cast_type = typename std::conditional<std::is_same<A, canonical_type>::value, T, canonical_type>::type;
   cast_type                                                                      c;
   c = a;
   eval_atan2(result, x, c);
}

template <class T, class A>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value, void>::type eval_atan2(T& result, const A& x, const T& a)
{
   using canonical_type = typename boost::multiprecision::detail::canonical<A, T>::type         ;
   using cast_type = typename std::conditional<std::is_same<A, canonical_type>::value, T, canonical_type>::type;
   cast_type                                                                      c;
   c = x;
   eval_atan2(result, c, a);
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
