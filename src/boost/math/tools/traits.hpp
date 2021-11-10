//  Copyright John Maddock 2007.
//  Copyright Matt Borland 2021.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
This header defines two traits classes, both in namespace boost::math::tools.

is_distribution<D>::value is true iff D has overloaded "cdf" and
"quantile" functions, plus member typedefs value_type and policy_type.  
It's not much of a definitive test frankly,
but if it looks like a distribution and quacks like a distribution
then it must be a distribution.

is_scaled_distribution<D>::value is true iff D is a distribution
as defined above, and has member functions "scale" and "location".

*/

#ifndef BOOST_STATS_IS_DISTRIBUTION_HPP
#define BOOST_STATS_IS_DISTRIBUTION_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <type_traits>

namespace boost{ namespace math{ namespace tools{

namespace detail{

#define BOOST_MATH_HAS_NAMED_TRAIT(trait, name)                         \
template <typename T>                                                   \
class trait                                                             \
{                                                                       \
private:                                                                \
   using yes = char;                                                    \
   struct no { char x[2]; };                                            \
                                                                        \
   template <typename U>                                                \
   static yes test(typename U::name* = nullptr);                        \
                                                                        \
   template <typename U>                                                \
   static no test(...);                                                 \
                                                                        \
public:                                                                 \
   static constexpr bool value = (sizeof(test<T>(0)) == sizeof(char));  \
};

BOOST_MATH_HAS_NAMED_TRAIT(has_value_type, value_type)
BOOST_MATH_HAS_NAMED_TRAIT(has_policy_type, policy_type)
BOOST_MATH_HAS_NAMED_TRAIT(has_backend_type, backend_type)

template <typename D>
char cdf(const D& ...);
template <typename D>
char quantile(const D& ...);

template <typename D>
struct has_cdf
{
   static D d;
   static constexpr bool value = sizeof(cdf(d, 0.0f)) != 1;
};

template <typename D>
struct has_quantile
{
   static D d;
   static constexpr bool value = sizeof(quantile(d, 0.0f)) != 1;
};

template <typename D>
struct is_distribution_imp
{
   static constexpr bool value =
      has_quantile<D>::value 
      && has_cdf<D>::value
      && has_value_type<D>::value
      && has_policy_type<D>::value;
};

template <typename sig, sig val>
struct result_tag{};

template <typename D>
double test_has_location(const volatile result_tag<typename D::value_type (D::*)()const, &D::location>*);
template <typename D>
char test_has_location(...);

template <typename D>
double test_has_scale(const volatile result_tag<typename D::value_type (D::*)()const, &D::scale>*);
template <typename D>
char test_has_scale(...);

template <typename D, bool b>
struct is_scaled_distribution_helper
{
   static constexpr bool value = false;
};

template <typename D>
struct is_scaled_distribution_helper<D, true>
{
   static constexpr bool value = 
      (sizeof(test_has_location<D>(0)) != 1) 
      && 
      (sizeof(test_has_scale<D>(0)) != 1);
};

template <typename D>
struct is_scaled_distribution_imp
{
   static constexpr bool value = (::boost::math::tools::detail::is_scaled_distribution_helper<D, ::boost::math::tools::detail::is_distribution_imp<D>::value>::value);
};

} // namespace detail

template <typename T> struct is_distribution : public std::integral_constant<bool, ::boost::math::tools::detail::is_distribution_imp<T>::value> {};
template <typename T> struct is_scaled_distribution : public std::integral_constant<bool, ::boost::math::tools::detail::is_scaled_distribution_imp<T>::value> {};

}}}

#endif


