// Copyright John Maddock 2008.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DISTRIBUTIONS_DETAIL_MODE_HPP
#define BOOST_MATH_DISTRIBUTIONS_DETAIL_MODE_HPP

#include <boost/math/tools/minima.hpp> // function minimization for mode
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/distributions/fwd.hpp>

namespace boost{ namespace math{ namespace detail{

template <class Dist>
struct pdf_minimizer
{
   pdf_minimizer(const Dist& d)
      : dist(d) {}

   typename Dist::value_type operator()(const typename Dist::value_type& x)
   {
      return -pdf(dist, x);
   }
private:
   Dist dist;
};

template <class Dist>
typename Dist::value_type generic_find_mode(const Dist& dist, typename Dist::value_type guess, const char* function, typename Dist::value_type step = 0)
{
   BOOST_MATH_STD_USING
   typedef typename Dist::value_type value_type;
   typedef typename Dist::policy_type policy_type;
   //
   // Need to begin by bracketing the maxima of the PDF:
   //
   value_type maxval;
   value_type upper_bound = guess;
   value_type lower_bound;
   value_type v = pdf(dist, guess);
   if(v == 0)
   {
      //
      // Oops we don't know how to handle this, or even in which
      // direction we should move in, treat as an evaluation error:
      //
      return policies::raise_evaluation_error(
         function, 
         "Could not locate a starting location for the search for the mode, original guess was %1%", guess, policy_type());
   }
   do
   {
      maxval = v;
      if(step != 0)
         upper_bound += step;
      else
         upper_bound *= 2;
      v = pdf(dist, upper_bound);
   }while(maxval < v);

   lower_bound = upper_bound;
   do
   {
      maxval = v;
      if(step != 0)
         lower_bound -= step;
      else
         lower_bound /= 2;
      v = pdf(dist, lower_bound);
   }while(maxval < v);

   std::uintmax_t max_iter = policies::get_max_root_iterations<policy_type>();

   value_type result = tools::brent_find_minima(
      pdf_minimizer<Dist>(dist), 
      lower_bound, 
      upper_bound, 
      policies::digits<value_type, policy_type>(), 
      max_iter).first;
   if(max_iter >= policies::get_max_root_iterations<policy_type>())
   {
      return policies::raise_evaluation_error<value_type>(
         function, 
         "Unable to locate solution in a reasonable time:"
         " either there is no answer to the mode of the distribution"
         " or the answer is infinite.  Current best guess is %1%", result, policy_type());
   }
   return result;
}
//
// As above,but confined to the interval [0,1]:
//
template <class Dist>
typename Dist::value_type generic_find_mode_01(const Dist& dist, typename Dist::value_type guess, const char* function)
{
   BOOST_MATH_STD_USING
   typedef typename Dist::value_type value_type;
   typedef typename Dist::policy_type policy_type;
   //
   // Need to begin by bracketing the maxima of the PDF:
   //
   value_type maxval;
   value_type upper_bound = guess;
   value_type lower_bound;
   value_type v = pdf(dist, guess);
   do
   {
      maxval = v;
      upper_bound = 1 - (1 - upper_bound) / 2;
      if(upper_bound == 1)
         return 1;
      v = pdf(dist, upper_bound);
   }while(maxval < v);

   lower_bound = upper_bound;
   do
   {
      maxval = v;
      lower_bound /= 2;
      if(lower_bound < tools::min_value<value_type>())
         return 0;
      v = pdf(dist, lower_bound);
   }while(maxval < v);

   std::uintmax_t max_iter = policies::get_max_root_iterations<policy_type>();

   value_type result = tools::brent_find_minima(
      pdf_minimizer<Dist>(dist), 
      lower_bound, 
      upper_bound, 
      policies::digits<value_type, policy_type>(), 
      max_iter).first;
   if(max_iter >= policies::get_max_root_iterations<policy_type>())
   {
      return policies::raise_evaluation_error<value_type>(
         function, 
         "Unable to locate solution in a reasonable time:"
         " either there is no answer to the mode of the distribution"
         " or the answer is infinite.  Current best guess is %1%", result, policy_type());
   }
   return result;
}

}}} // namespaces

#endif // BOOST_MATH_DISTRIBUTIONS_DETAIL_MODE_HPP
