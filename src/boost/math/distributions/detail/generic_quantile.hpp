//  Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DISTIBUTIONS_DETAIL_GENERIC_QUANTILE_HPP
#define BOOST_MATH_DISTIBUTIONS_DETAIL_GENERIC_QUANTILE_HPP

namespace boost{ namespace math{ namespace detail{

template <class Dist>
struct generic_quantile_finder
{
   typedef typename Dist::value_type value_type;
   typedef typename Dist::policy_type policy_type;

   generic_quantile_finder(const Dist& d, value_type t, bool c)
      : dist(d), target(t), comp(c) {}

   value_type operator()(const value_type& x)
   {
      return comp ?
         value_type(target - cdf(complement(dist, x)))
         : value_type(cdf(dist, x) - target);
   }

private:
   Dist dist;
   value_type target;
   bool comp;
};

template <class T, class Policy>
inline T check_range_result(const T& x, const Policy& pol, const char* function)
{
   if((x >= 0) && (x < tools::min_value<T>()))
      return policies::raise_underflow_error<T>(function, 0, pol);
   if(x <= -tools::max_value<T>())
      return -policies::raise_overflow_error<T>(function, 0, pol);
   if(x >= tools::max_value<T>())
      return policies::raise_overflow_error<T>(function, 0, pol);
   return x;
}

template <class Dist>
typename Dist::value_type generic_quantile(const Dist& dist, const typename Dist::value_type& p, const typename Dist::value_type& guess, bool comp, const char* function)
{
   typedef typename Dist::value_type value_type;
   typedef typename Dist::policy_type policy_type;
   typedef typename policies::normalise<
      policy_type, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   //
   // Special cases first:
   //
   if(p == 0)
   {
      return comp
      ? check_range_result(range(dist).second, forwarding_policy(), function)
      : check_range_result(range(dist).first, forwarding_policy(), function);
   }
   if(p == 1)
   {
      return !comp
      ? check_range_result(range(dist).second, forwarding_policy(), function)
      : check_range_result(range(dist).first, forwarding_policy(), function);
   }

   generic_quantile_finder<Dist> f(dist, p, comp);
   tools::eps_tolerance<value_type> tol(policies::digits<value_type, forwarding_policy>() - 3);
   std::uintmax_t max_iter = policies::get_max_root_iterations<forwarding_policy>();
   std::pair<value_type, value_type> ir = tools::bracket_and_solve_root(
      f, guess, value_type(2), true, tol, max_iter, forwarding_policy());
   value_type result = ir.first + (ir.second - ir.first) / 2;
   if(max_iter >= policies::get_max_root_iterations<forwarding_policy>())
   {
      return policies::raise_evaluation_error<value_type>(function, "Unable to locate solution in a reasonable time:"
         " either there is no answer to quantile"
         " or the answer is infinite.  Current best guess is %1%", result, forwarding_policy());
   }
   return result;
}

}}} // namespaces

#endif // BOOST_MATH_DISTIBUTIONS_DETAIL_GENERIC_QUANTILE_HPP

