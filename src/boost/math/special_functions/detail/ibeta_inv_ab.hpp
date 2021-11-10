//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// This is not a complete header file, it is included by beta.hpp
// after it has defined it's definitions.  This inverts the incomplete
// beta functions ibeta and ibetac on the first parameters "a"
// and "b" using a generic root finding algorithm (TOMS Algorithm 748).
//

#ifndef BOOST_MATH_SP_DETAIL_BETA_INV_AB
#define BOOST_MATH_SP_DETAIL_BETA_INV_AB

#ifdef _MSC_VER
#pragma once
#endif

#include <cstdint>
#include <utility>
#include <boost/math/tools/toms748_solve.hpp>

namespace boost{ namespace math{ namespace detail{

template <class T, class Policy>
struct beta_inv_ab_t
{
   beta_inv_ab_t(T b_, T z_, T p_, bool invert_, bool swap_ab_) : b(b_), z(z_), p(p_), invert(invert_), swap_ab(swap_ab_) {}
   T operator()(T a)
   {
      return invert ? 
         p - boost::math::ibetac(swap_ab ? b : a, swap_ab ? a : b, z, Policy()) 
         : boost::math::ibeta(swap_ab ? b : a, swap_ab ? a : b, z, Policy()) - p;
   }
private:
   T b, z, p;
   bool invert, swap_ab;
};

template <class T, class Policy>
T inverse_negative_binomial_cornish_fisher(T n, T sf, T sfc, T p, T q, const Policy& pol)
{
   BOOST_MATH_STD_USING
   // mean:
   T m = n * (sfc) / sf;
   T t = sqrt(n * (sfc));
   // standard deviation:
   T sigma = t / sf;
   // skewness
   T sk = (1 + sfc) / t;
   // kurtosis:
   T k = (6 - sf * (5+sfc)) / (n * (sfc));
   // Get the inverse of a std normal distribution:
   T x = boost::math::erfc_inv(p > q ? 2 * q : 2 * p, pol) * constants::root_two<T>();
   // Set the sign:
   if(p < 0.5)
      x = -x;
   T x2 = x * x;
   // w is correction term due to skewness
   T w = x + sk * (x2 - 1) / 6;
   //
   // Add on correction due to kurtosis.
   //
   if(n >= 10)
      w += k * x * (x2 - 3) / 24 + sk * sk * x * (2 * x2 - 5) / -36;

   w = m + sigma * w;
   if(w < tools::min_value<T>())
      return tools::min_value<T>();
   return w;
}

template <class T, class Policy>
T ibeta_inv_ab_imp(const T& b, const T& z, const T& p, const T& q, bool swap_ab, const Policy& pol)
{
   BOOST_MATH_STD_USING  // for ADL of std lib math functions
   //
   // Special cases first:
   //
   BOOST_MATH_INSTRUMENT_CODE("b = " << b << " z = " << z << " p = " << p << " q = " << " swap = " << swap_ab);
   if(p == 0)
   {
      return swap_ab ? tools::min_value<T>() : tools::max_value<T>();
   }
   if(q == 0)
   {
      return swap_ab ? tools::max_value<T>() : tools::min_value<T>();
   }
   //
   // Function object, this is the functor whose root
   // we have to solve:
   //
   beta_inv_ab_t<T, Policy> f(b, z, (p < q) ? p : q, (p < q) ? false : true, swap_ab);
   //
   // Tolerance: full precision.
   //
   tools::eps_tolerance<T> tol(policies::digits<T, Policy>());
   //
   // Now figure out a starting guess for what a may be, 
   // we'll start out with a value that'll put p or q
   // right bang in the middle of their range, the functions
   // are quite sensitive so we should need too many steps
   // to bracket the root from there:
   //
   T guess = 0;
   T factor = 5;
   //
   // Convert variables to parameters of a negative binomial distribution:
   //
   T n = b;
   T sf = swap_ab ? z : 1-z;
   T sfc = swap_ab ? 1-z : z;
   T u = swap_ab ? p : q;
   T v = swap_ab ? q : p;
   if(u <= pow(sf, n))
   {
      //
      // Result is less than 1, negative binomial approximation
      // is useless....
      //
      if((p < q) != swap_ab)
      {
         guess = (std::min)(T(b * 2), T(1));
      }
      else
      {
         guess = (std::min)(T(b / 2), T(1));
      }
   }
   if(n * n * n * u * sf > 0.005)
      guess = 1 + inverse_negative_binomial_cornish_fisher(n, sf, sfc, u, v, pol);

   if(guess < 10)
   {
      //
      // Negative binomial approximation not accurate in this area:
      //
      if((p < q) != swap_ab)
      {
         guess = (std::min)(T(b * 2), T(10));
      }
      else
      {
         guess = (std::min)(T(b / 2), T(10));
      }
   }
   else
      factor = (v < sqrt(tools::epsilon<T>())) ? 2 : (guess < 20 ? 1.2f : 1.1f);
   BOOST_MATH_INSTRUMENT_CODE("guess = " << guess);
   //
   // Max iterations permitted:
   //
   std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
   std::pair<T, T> r = bracket_and_solve_root(f, guess, factor, swap_ab ? true : false, tol, max_iter, pol);
   if(max_iter >= policies::get_max_root_iterations<Policy>())
      return policies::raise_evaluation_error<T>("boost::math::ibeta_invab_imp<%1%>(%1%,%1%,%1%)", "Unable to locate the root within a reasonable number of iterations, closest approximation so far was %1%", r.first, pol);
   return (r.first + r.second) / 2;
}

} // namespace detail

template <class RT1, class RT2, class RT3, class Policy>
typename tools::promote_args<RT1, RT2, RT3>::type 
      ibeta_inva(RT1 b, RT2 x, RT3 p, const Policy& pol)
{
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   static const char* function = "boost::math::ibeta_inva<%1%>(%1%,%1%,%1%)";
   if(p == 0)
   {
      return policies::raise_overflow_error<result_type>(function, 0, Policy());
   }
   if(p == 1)
   {
      return tools::min_value<result_type>();
   }

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::ibeta_inv_ab_imp(
         static_cast<value_type>(b), 
         static_cast<value_type>(x), 
         static_cast<value_type>(p), 
         static_cast<value_type>(1 - static_cast<value_type>(p)), 
         false, pol), 
      function);
}

template <class RT1, class RT2, class RT3, class Policy>
typename tools::promote_args<RT1, RT2, RT3>::type 
      ibetac_inva(RT1 b, RT2 x, RT3 q, const Policy& pol)
{
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   static const char* function = "boost::math::ibetac_inva<%1%>(%1%,%1%,%1%)";
   if(q == 1)
   {
      return policies::raise_overflow_error<result_type>(function, 0, Policy());
   }
   if(q == 0)
   {
      return tools::min_value<result_type>();
   }

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::ibeta_inv_ab_imp(
         static_cast<value_type>(b), 
         static_cast<value_type>(x), 
         static_cast<value_type>(1 - static_cast<value_type>(q)), 
         static_cast<value_type>(q), 
         false, pol),
      function);
}

template <class RT1, class RT2, class RT3, class Policy>
typename tools::promote_args<RT1, RT2, RT3>::type 
      ibeta_invb(RT1 a, RT2 x, RT3 p, const Policy& pol)
{
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   static const char* function = "boost::math::ibeta_invb<%1%>(%1%,%1%,%1%)";
   if(p == 0)
   {
      return tools::min_value<result_type>();
   }
   if(p == 1)
   {
      return policies::raise_overflow_error<result_type>(function, 0, Policy());
   }

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::ibeta_inv_ab_imp(
         static_cast<value_type>(a), 
         static_cast<value_type>(x), 
         static_cast<value_type>(p), 
         static_cast<value_type>(1 - static_cast<value_type>(p)), 
         true, pol),
      function);
}

template <class RT1, class RT2, class RT3, class Policy>
typename tools::promote_args<RT1, RT2, RT3>::type 
      ibetac_invb(RT1 a, RT2 x, RT3 q, const Policy& pol)
{
   static const char* function = "boost::math::ibeta_invb<%1%>(%1%, %1%, %1%)";
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   if(q == 1)
   {
      return tools::min_value<result_type>();
   }
   if(q == 0)
   {
      return policies::raise_overflow_error<result_type>(function, 0, Policy());
   }

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::ibeta_inv_ab_imp(
         static_cast<value_type>(a), 
         static_cast<value_type>(x), 
         static_cast<value_type>(1 - static_cast<value_type>(q)), 
         static_cast<value_type>(q),
         true, pol),
         function);
}

template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type 
         ibeta_inva(RT1 b, RT2 x, RT3 p)
{
   return boost::math::ibeta_inva(b, x, p, policies::policy<>());
}

template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type 
         ibetac_inva(RT1 b, RT2 x, RT3 q)
{
   return boost::math::ibetac_inva(b, x, q, policies::policy<>());
}

template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type 
         ibeta_invb(RT1 a, RT2 x, RT3 p)
{
   return boost::math::ibeta_invb(a, x, p, policies::policy<>());
}

template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type 
         ibetac_invb(RT1 a, RT2 x, RT3 q)
{
   return boost::math::ibetac_invb(a, x, q, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_SP_DETAIL_BETA_INV_AB



