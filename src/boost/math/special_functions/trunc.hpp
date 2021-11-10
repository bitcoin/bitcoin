//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TRUNC_HPP
#define BOOST_MATH_TRUNC_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <type_traits>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace boost{ namespace math{ namespace detail{

template <class T, class Policy>
inline typename tools::promote_args<T>::type trunc(const T& v, const Policy& pol, const std::false_type&)
{
   BOOST_MATH_STD_USING
   typedef typename tools::promote_args<T>::type result_type;
   if(!(boost::math::isfinite)(v))
      return policies::raise_rounding_error("boost::math::trunc<%1%>(%1%)", 0, static_cast<result_type>(v), static_cast<result_type>(v), pol);
   return (v >= 0) ? static_cast<result_type>(floor(v)) : static_cast<result_type>(ceil(v));
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type trunc(const T& v, const Policy&, const std::true_type&)
{
   return v;
}

}

template <class T, class Policy>
inline typename tools::promote_args<T>::type trunc(const T& v, const Policy& pol)
{
   return detail::trunc(v, pol, std::integral_constant<bool, detail::is_integer_for_rounding<T>::value>());
}
template <class T>
inline typename tools::promote_args<T>::type trunc(const T& v)
{
   return trunc(v, policies::policy<>());
}
//
// The following functions will not compile unless T has an
// implicit conversion to the integer types.  For user-defined
// number types this will likely not be the case.  In that case
// these functions should either be specialized for the UDT in
// question, or else overloads should be placed in the same
// namespace as the UDT: these will then be found via argument
// dependent lookup.  See our concept archetypes for examples.
//
// Non-standard numeric limits syntax "(std::numeric_limits<int>::max)()" 
// is to avoid macro substiution from MSVC
// https://stackoverflow.com/questions/27442885/syntax-error-with-stdnumeric-limitsmax
//
template <class T, class Policy>
inline int itrunc(const T& v, const Policy& pol)
{
   BOOST_MATH_STD_USING
   typedef typename tools::promote_args<T>::type result_type;
   result_type r = boost::math::trunc(v, pol);
   if(r > static_cast<result_type>((std::numeric_limits<int>::max)()) || r < static_cast<result_type>((std::numeric_limits<int>::min)()))
      return static_cast<int>(policies::raise_rounding_error("boost::math::itrunc<%1%>(%1%)", 0, static_cast<result_type>(v), 0, pol));
   return static_cast<int>(r);
}
template <class T>
inline int itrunc(const T& v)
{
   return itrunc(v, policies::policy<>());
}

template <class T, class Policy>
inline long ltrunc(const T& v, const Policy& pol)
{
   BOOST_MATH_STD_USING
   typedef typename tools::promote_args<T>::type result_type;
   result_type r = boost::math::trunc(v, pol);
   if(r > static_cast<result_type>((std::numeric_limits<long>::max)()) || r < static_cast<result_type>((std::numeric_limits<long>::min)()))
      return static_cast<long>(policies::raise_rounding_error("boost::math::ltrunc<%1%>(%1%)", 0, static_cast<result_type>(v), 0L, pol));
   return static_cast<long>(r);
}
template <class T>
inline long ltrunc(const T& v)
{
   return ltrunc(v, policies::policy<>());
}

template <class T, class Policy>
inline long long lltrunc(const T& v, const Policy& pol)
{
   BOOST_MATH_STD_USING
   typedef typename tools::promote_args<T>::type result_type;
   result_type r = boost::math::trunc(v, pol);
   if(r > static_cast<result_type>((std::numeric_limits<long long>::max)()) || 
      r < static_cast<result_type>((std::numeric_limits<long long>::min)()))
   {
      return static_cast<long long>(policies::raise_rounding_error("boost::math::lltrunc<%1%>(%1%)", 0, v, static_cast<long long>(0), pol));
   }
   return static_cast<long long>(r);
}
template <class T>
inline long long lltrunc(const T& v)
{
   return lltrunc(v, policies::policy<>());
}

template <class T, class Policy>
inline typename std::enable_if<std::is_constructible<int, T>::value, int>::type
   iconvert(const T& v, const Policy&) 
{
   return static_cast<int>(v);
}

template <class T, class Policy>
inline typename std::enable_if<!std::is_constructible<int, T>::value, int>::type
   iconvert(const T& v, const Policy& pol) 
{
   using boost::math::itrunc;
   return itrunc(v, pol);
}

template <class T, class Policy>
inline typename std::enable_if<std::is_constructible<long, T>::value, long>::type
   lconvert(const T& v, const Policy&) 
{
   return static_cast<long>(v);
}

template <class T, class Policy>
inline typename std::enable_if<!std::is_constructible<long, T>::value, long>::type
   lconvert(const T& v, const Policy& pol) 
{
   using boost::math::ltrunc;
   return ltrunc(v, pol);
}

template <class T, class Policy>
inline typename std::enable_if<std::is_constructible<long long, T>::value, long long>::type
   llconvertert(const T& v, const Policy&) 
{
   return static_cast<long long>(v);
}

template <class T, class Policy>
inline typename std::enable_if<!std::is_constructible<long long, T>::value, long long>::type
   llconvertert(const T& v, const Policy& pol) 
{
   using boost::math::lltrunc;
   return lltrunc(v, pol);
}

}} // namespaces

#endif // BOOST_MATH_TRUNC_HPP
