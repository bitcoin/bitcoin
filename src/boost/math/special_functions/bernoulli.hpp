
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2013 Nikhar Agrawal
//  Copyright 2013 Christopher Kormanyos
//  Copyright 2013 John Maddock
//  Copyright 2013 Paul Bristow
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_BERNOULLI_B2N_2013_05_30_HPP_
#define _BOOST_BERNOULLI_B2N_2013_05_30_HPP_

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/detail/unchecked_bernoulli.hpp>
#include <boost/math/special_functions/detail/bernoulli_details.hpp>

namespace boost { namespace math { 
   
namespace detail {

template <class T, class OutputIterator, class Policy, int N>
OutputIterator bernoulli_number_imp(OutputIterator out, std::size_t start, std::size_t n, const Policy& pol, const std::integral_constant<int, N>& tag)
{
   for(std::size_t i = start; (i <= max_bernoulli_b2n<T>::value) && (i < start + n); ++i)
   {
      *out = unchecked_bernoulli_imp<T>(i, tag);
      ++out;
   }
   
   for(std::size_t i = (std::max)(static_cast<std::size_t>(max_bernoulli_b2n<T>::value + 1), start); i < start + n; ++i)
   {
      // We must overflow:
      *out = (i & 1 ? 1 : -1) * policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(n)", 0, T(i), pol);
      ++out;
   }
   return out;
}

template <class T, class OutputIterator, class Policy>
OutputIterator bernoulli_number_imp(OutputIterator out, std::size_t start, std::size_t n, const Policy& pol, const std::integral_constant<int, 0>& tag)
{
   for(std::size_t i = start; (i <= max_bernoulli_b2n<T>::value) && (i < start + n); ++i)
   {
      *out = unchecked_bernoulli_imp<T>(i, tag);
      ++out;
   }
   //
   // Short circuit return so we don't grab the mutex below unless we have to:
   //
   if(start + n <= max_bernoulli_b2n<T>::value)
      return out;

   return get_bernoulli_numbers_cache<T, Policy>().copy_bernoulli_numbers(out, start, n, pol);
}

} // namespace detail

template <class T, class Policy>
inline T bernoulli_b2n(const int i, const Policy &pol)
{
   typedef std::integral_constant<int, detail::bernoulli_imp_variant<T>::value> tag_type;
   if(i < 0)
      return policies::raise_domain_error<T>("boost::math::bernoulli_b2n<%1%>", "Index should be >= 0 but got %1%", T(i), pol);

   T result = static_cast<T>(0); // The = 0 is just to silence compiler warnings :-(
   boost::math::detail::bernoulli_number_imp<T>(&result, static_cast<std::size_t>(i), 1u, pol, tag_type());
   return result;
}

template <class T>
inline T bernoulli_b2n(const int i)
{
   return boost::math::bernoulli_b2n<T>(i, policies::policy<>());
}

template <class T, class OutputIterator, class Policy>
inline OutputIterator bernoulli_b2n(const int start_index,
                                    const unsigned number_of_bernoullis_b2n,
                                    OutputIterator out_it,
                                    const Policy& pol)
{
   typedef std::integral_constant<int, detail::bernoulli_imp_variant<T>::value> tag_type;
   if(start_index < 0)
   {
      *out_it = policies::raise_domain_error<T>("boost::math::bernoulli_b2n<%1%>", "Index should be >= 0 but got %1%", T(start_index), pol);
      return ++out_it;
   }

   return boost::math::detail::bernoulli_number_imp<T>(out_it, start_index, number_of_bernoullis_b2n, pol, tag_type());
}

template <class T, class OutputIterator>
inline OutputIterator bernoulli_b2n(const int start_index,
                                    const unsigned number_of_bernoullis_b2n,
                                    OutputIterator out_it)
{
   return boost::math::bernoulli_b2n<T, OutputIterator>(start_index, number_of_bernoullis_b2n, out_it, policies::policy<>());
}

template <class T, class Policy>
inline T tangent_t2n(const int i, const Policy &pol)
{
   if(i < 0)
      return policies::raise_domain_error<T>("boost::math::tangent_t2n<%1%>", "Index should be >= 0 but got %1%", T(i), pol);

   T result;
   boost::math::detail::get_bernoulli_numbers_cache<T, Policy>().copy_tangent_numbers(&result, i, 1, pol);
   return result;
}

template <class T>
inline T tangent_t2n(const int i)
{
   return boost::math::tangent_t2n<T>(i, policies::policy<>());
}

template <class T, class OutputIterator, class Policy>
inline OutputIterator tangent_t2n(const int start_index,
                                    const unsigned number_of_tangent_t2n,
                                    OutputIterator out_it,
                                    const Policy& pol)
{
   if(start_index < 0)
   {
      *out_it = policies::raise_domain_error<T>("boost::math::tangent_t2n<%1%>", "Index should be >= 0 but got %1%", T(start_index), pol);
      return ++out_it;
   }

   return boost::math::detail::get_bernoulli_numbers_cache<T, Policy>().copy_tangent_numbers(out_it, start_index, number_of_tangent_t2n, pol);
}

template <class T, class OutputIterator>
inline OutputIterator tangent_t2n(const int start_index,
                                    const unsigned number_of_tangent_t2n,
                                    OutputIterator out_it)
{
   return boost::math::tangent_t2n<T, OutputIterator>(start_index, number_of_tangent_t2n, out_it, policies::policy<>());
}

} } // namespace boost::math

#endif // _BOOST_BERNOULLI_B2N_2013_05_30_HPP_
