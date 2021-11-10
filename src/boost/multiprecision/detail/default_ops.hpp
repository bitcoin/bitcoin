///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011-21 John Maddock.
//  Copyright 2021 Iskandarov Lev. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BIG_NUM_DEF_OPS
#define BOOST_MATH_BIG_NUM_DEF_OPS

#include <boost/core/no_exceptions_support.hpp> // BOOST_TRY
#include <boost/math/policies/error_handling.hpp>
#include <boost/multiprecision/detail/number_base.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/next.hpp>
#include <boost/math/special_functions/hypot.hpp>
#include <cstdint>
#ifndef BOOST_NO_CXX17_HDR_STRING_VIEW
#include <string_view>
#endif

#ifndef INSTRUMENT_BACKEND
#ifndef BOOST_MP_INSTRUMENT
#define INSTRUMENT_BACKEND(x)
#else
#define INSTRUMENT_BACKEND(x) \
   std::cout << BOOST_STRINGIZE(x) << " = " << x.str(0, std::ios_base::scientific) << std::endl;
#endif
#endif

namespace boost {
namespace multiprecision {

namespace detail {

template <class T>
struct is_backend;

template <class To, class From>
void generic_interconvert(To& to, const From& from, const std::integral_constant<int, number_kind_floating_point>& /*to_type*/, const std::integral_constant<int, number_kind_integer>& /*from_type*/);
template <class To, class From>
void generic_interconvert(To& to, const From& from, const std::integral_constant<int, number_kind_integer>& /*to_type*/, const std::integral_constant<int, number_kind_integer>& /*from_type*/);
template <class To, class From>
void generic_interconvert(To& to, const From& from, const std::integral_constant<int, number_kind_floating_point>& /*to_type*/, const std::integral_constant<int, number_kind_floating_point>& /*from_type*/);
template <class To, class From>
void generic_interconvert(To& to, const From& from, const std::integral_constant<int, number_kind_rational>& /*to_type*/, const std::integral_constant<int, number_kind_rational>& /*from_type*/);
template <class To, class From>
void generic_interconvert(To& to, const From& from, const std::integral_constant<int, number_kind_rational>& /*to_type*/, const std::integral_constant<int, number_kind_integer>& /*from_type*/);

template <class Integer>
BOOST_MP_CXX14_CONSTEXPR Integer karatsuba_sqrt(const Integer& x, Integer& r, size_t bits);

} // namespace detail

namespace default_ops {

#ifdef BOOST_MSVC
// warning C4127: conditional expression is constant
// warning C4146: unary minus operator applied to unsigned type, result still unsigned
#pragma warning(push)
#pragma warning(disable : 4127 4146)
#endif
//
// Default versions of mixed arithmetic, these just construct a temporary
// from the arithmetic value and then do the arithmetic on that, two versions
// of each depending on whether the backend can be directly constructed from type V.
//
// Note that we have to provide *all* the template parameters to class number when used in
// enable_if as MSVC-10 won't compile the code if we rely on a computed-default parameter.
// Since the result of the test doesn't depend on whether expression templates are on or off
// we just use et_on everywhere.  We could use a BOOST_WORKAROUND but that just obfuscates the
// code even more....
//
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if< !std::is_convertible<V, T>::value>::type
eval_add(T& result, V const& v)
{
   T t;
   t = v;
   eval_add(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, T>::value>::type
eval_add(T& result, V const& v)
{
   T t(v);
   eval_add(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if< !std::is_convertible<V, T>::value>::type
eval_subtract(T& result, V const& v)
{
   T t;
   t = v;
   eval_subtract(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, T>::value>::type
eval_subtract(T& result, V const& v)
{
   T t(v);
   eval_subtract(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if< !std::is_convertible<V, T>::value>::type
eval_multiply(T& result, V const& v)
{
   T t;
   t = v;
   eval_multiply(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, T>::value>::type
eval_multiply(T& result, V const& v)
{
   T t(v);
   eval_multiply(result, t);
}

template <class T, class U, class V>
BOOST_MP_CXX14_CONSTEXPR void eval_multiply(T& t, const U& u, const V& v);

template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(!std::is_same<T, U>::value && std::is_same<T, V>::value)>::type eval_multiply_add(T& t, const U& u, const V& v)
{
   T z;
   eval_multiply(z, u, v);
   eval_add(t, z);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!std::is_same<T, U>::value && std::is_same<T, V>::value>::type eval_multiply_add(T& t, const U& u, const V& v)
{
   eval_multiply_add(t, v, u);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(!std::is_same<T, U>::value && std::is_same<T, V>::value)>::type eval_multiply_subtract(T& t, const U& u, const V& v)
{
   T z;
   eval_multiply(z, u, v);
   eval_subtract(t, z);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!std::is_same<T, U>::value && std::is_same<T, V>::value>::type eval_multiply_subtract(T& t, const U& u, const V& v)
{
   eval_multiply_subtract(t, v, u);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && !std::is_convertible<V, T>::value>::type
eval_divide(T& result, V const& v)
{
   T t;
   t = v;
   eval_divide(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && std::is_convertible<V, T>::value>::type
eval_divide(T& result, V const& v)
{
   T t(v);
   eval_divide(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && !std::is_convertible<V, T>::value>::type
eval_modulus(T& result, V const& v)
{
   T t;
   t = v;
   eval_modulus(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && std::is_convertible<V, T>::value>::type
eval_modulus(T& result, V const& v)
{
   T t(v);
   eval_modulus(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && !std::is_convertible<V, T>::value>::type
eval_bitwise_and(T& result, V const& v)
{
   T t;
   t = v;
   eval_bitwise_and(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && std::is_convertible<V, T>::value>::type
eval_bitwise_and(T& result, V const& v)
{
   T t(v);
   eval_bitwise_and(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && !std::is_convertible<V, T>::value>::type
eval_bitwise_or(T& result, V const& v)
{
   T t;
   t = v;
   eval_bitwise_or(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && std::is_convertible<V, T>::value>::type
eval_bitwise_or(T& result, V const& v)
{
   T t(v);
   eval_bitwise_or(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && !std::is_convertible<V, T>::value>::type
eval_bitwise_xor(T& result, V const& v)
{
   T t;
   t = v;
   eval_bitwise_xor(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && std::is_convertible<V, T>::value>::type
eval_bitwise_xor(T& result, V const& v)
{
   T t(v);
   eval_bitwise_xor(result, t);
}

template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && !std::is_convertible<V, T>::value>::type
eval_complement(T& result, V const& v)
{
   T t;
   t = v;
   eval_complement(result, t);
}
template <class T, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, number<T, et_on> >::value && std::is_convertible<V, T>::value>::type
eval_complement(T& result, V const& v)
{
   T t(v);
   eval_complement(result, t);
}

//
// Default versions of 3-arg arithmetic functions, these mostly just forward to the 2 arg versions:
//
template <class T, class U, class V>
BOOST_MP_CXX14_CONSTEXPR void eval_add(T& t, const U& u, const V& v);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_add_default(T& t, const T& u, const T& v)
{
   if (&t == &v)
   {
      eval_add(t, u);
   }
   else if (&t == &u)
   {
      eval_add(t, v);
   }
   else
   {
      t = u;
      eval_add(t, v);
   }
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_add_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_add(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_add_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_add(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value>::type eval_add_default(T& t, const U& u, const T& v)
{
   eval_add(t, v, u);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_add_default(T& t, const U& u, const V& v)
{
   BOOST_IF_CONSTEXPR(std::is_same<T, V>::value)
   {
      if ((void*)&t == (void*)&v)
      {
         eval_add(t, u);
      }
      else
      {
         t = u;
         eval_add(t, v);
      }
   }
   else
   {
      t = u;
      eval_add(t, v);
   }
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_add(T& t, const U& u, const V& v)
{
   eval_add_default(t, u, v);
}

template <class T, class U, class V>
void BOOST_MP_CXX14_CONSTEXPR eval_subtract(T& t, const U& u, const V& v);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract_default(T& t, const T& u, const T& v)
{
   if ((&t == &v) && is_signed_number<T>::value)
   {
      eval_subtract(t, u);
      t.negate();
   }
   else if (&t == &u)
   {
      eval_subtract(t, v);
   }
   else
   {
      t = u;
      eval_subtract(t, v);
   }
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_subtract_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_subtract(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_subtract_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_subtract(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && is_signed_number<T>::value>::type eval_subtract_default(T& t, const U& u, const T& v)
{
   eval_subtract(t, v, u);
   t.negate();
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value && is_unsigned_number<T>::value>::type eval_subtract_default(T& t, const U& u, const T& v)
{
   T temp;
   temp = u;
   eval_subtract(t, temp, v);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value && is_unsigned_number<T>::value>::type eval_subtract_default(T& t, const U& u, const T& v)
{
   T temp(u);
   eval_subtract(t, temp, v);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract_default(T& t, const U& u, const V& v)
{
   BOOST_IF_CONSTEXPR(std::is_same<T, V>::value)
   {
      if ((void*)&t == (void*)&v)
      {
         eval_subtract(t, u);
         t.negate();
      }
      else
      {
         t = u;
         eval_subtract(t, v);
      }
   }
   else
   {
      t = u;
      eval_subtract(t, v);
   }
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract(T& t, const U& u, const V& v)
{
   eval_subtract_default(t, u, v);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply_default(T& t, const T& u, const T& v)
{
   if (&t == &v)
   {
      eval_multiply(t, u);
   }
   else if (&t == &u)
   {
      eval_multiply(t, v);
   }
   else
   {
      t = u;
      eval_multiply(t, v);
   }
}
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1900)
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_multiply_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_multiply(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_multiply_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_multiply(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value>::type eval_multiply_default(T& t, const U& u, const T& v)
{
   eval_multiply(t, v, u);
}
#endif
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply_default(T& t, const U& u, const V& v)
{
   BOOST_IF_CONSTEXPR(std::is_same<T, V>::value)
   {
      if ((void*)&t == (void*)&v)
      {
         eval_multiply(t, u);
      }
      else
      {
         t = number<T>::canonical_value(u);
         eval_multiply(t, v);
      }
   }
   else
   {
      t = number<T>::canonical_value(u);
      eval_multiply(t, v);
   }
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply(T& t, const U& u, const V& v)
{
   eval_multiply_default(t, u, v);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply_add(T& t, const T& u, const T& v, const T& x)
{
   if ((void*)&x == (void*)&t)
   {
      T z;
      z = number<T>::canonical_value(x);
      eval_multiply_add(t, u, v, z);
   }
   else
   {
      eval_multiply(t, u, v);
      eval_add(t, x);
   }
}

template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if< !std::is_same<T, U>::value, T>::type make_T(const U& u)
{
   T t;
   t = number<T>::canonical_value(u);
   return t;
}
template <class T>
inline BOOST_MP_CXX14_CONSTEXPR const T& make_T(const T& t)
{
   return t;
}

template <class T, class U, class V, class X>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(!std::is_same<T, U>::value && std::is_same<T, V>::value)>::type eval_multiply_add(T& t, const U& u, const V& v, const X& x)
{
   eval_multiply_add(t, make_T<T>(u), make_T<T>(v), make_T<T>(x));
}
template <class T, class U, class V, class X>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!std::is_same<T, U>::value && std::is_same<T, V>::value>::type eval_multiply_add(T& t, const U& u, const V& v, const X& x)
{
   eval_multiply_add(t, v, u, x);
}
template <class T, class U, class V, class X>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(!std::is_same<T, U>::value && std::is_same<T, V>::value)>::type eval_multiply_subtract(T& t, const U& u, const V& v, const X& x)
{
   if ((void*)&x == (void*)&t)
   {
      T z;
      z = x;
      eval_multiply_subtract(t, u, v, z);
   }
   else
   {
      eval_multiply(t, u, v);
      eval_subtract(t, x);
   }
}
template <class T, class U, class V, class X>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!std::is_same<T, U>::value && std::is_same<T, V>::value>::type eval_multiply_subtract(T& t, const U& u, const V& v, const X& x)
{
   eval_multiply_subtract(t, v, u, x);
}

template <class T, class U, class V>
BOOST_MP_CXX14_CONSTEXPR void eval_divide(T& t, const U& u, const V& v);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_divide_default(T& t, const T& u, const T& v)
{
   if (&t == &u)
      eval_divide(t, v);
   else if (&t == &v)
   {
      T temp;
      eval_divide(temp, u, v);
      temp.swap(t);
   }
   else
   {
      t = u;
      eval_divide(t, v);
   }
}
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1900)
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_divide_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_divide(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_divide_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_divide(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_divide_default(T& t, const U& u, const T& v)
{
   T uu;
   uu = u;
   eval_divide(t, uu, v);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_divide_default(T& t, const U& u, const T& v)
{
   T uu(u);
   eval_divide(t, uu, v);
}
#endif
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_divide_default(T& t, const U& u, const V& v)
{
   BOOST_IF_CONSTEXPR(std::is_same<T, V>::value)
   {
      if ((void*)&t == (void*)&v)
      {
         T temp;
         temp = u;
         eval_divide(temp, v);
         t = temp;
      }
      else
      {
         t = u;
         eval_divide(t, v);
      }
   }
   else
   {
      t = u;
      eval_divide(t, v);
   }
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_divide(T& t, const U& u, const V& v)
{
   eval_divide_default(t, u, v);
}

template <class T, class U, class V>
BOOST_MP_CXX14_CONSTEXPR void eval_modulus(T& t, const U& u, const V& v);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_modulus_default(T& t, const T& u, const T& v)
{
   if (&t == &u)
      eval_modulus(t, v);
   else if (&t == &v)
   {
      T temp;
      eval_modulus(temp, u, v);
      temp.swap(t);
   }
   else
   {
      t = u;
      eval_modulus(t, v);
   }
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_modulus_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_modulus(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_modulus_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_modulus(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_modulus_default(T& t, const U& u, const T& v)
{
   T uu;
   uu = u;
   eval_modulus(t, uu, v);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_modulus_default(T& t, const U& u, const T& v)
{
   T uu(u);
   eval_modulus(t, uu, v);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_modulus_default(T& t, const U& u, const V& v)
{
   BOOST_IF_CONSTEXPR(std::is_same<T, V>::value)
   {
      if ((void*)&t == (void*)&v)
      {
         T temp(u);
         eval_modulus(temp, v);
         t = temp;
      }
      else
      {
         t = u;
         eval_modulus(t, v);
      }
   }
   else
   {
      t = u;
      eval_modulus(t, v);
   }
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_modulus(T& t, const U& u, const V& v)
{
   eval_modulus_default(t, u, v);
}

template <class T, class U, class V>
BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_and(T& t, const U& u, const V& v);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_and_default(T& t, const T& u, const T& v)
{
   if (&t == &v)
   {
      eval_bitwise_and(t, u);
   }
   else if (&t == &u)
   {
      eval_bitwise_and(t, v);
   }
   else
   {
      t = u;
      eval_bitwise_and(t, v);
   }
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if< !std::is_convertible<U, T>::value>::type eval_bitwise_and_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_bitwise_and(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, T>::value>::type eval_bitwise_and_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_bitwise_and(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value>::type eval_bitwise_and_default(T& t, const U& u, const T& v)
{
   eval_bitwise_and(t, v, u);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!std::is_same<T, U>::value || std::is_same<T, V>::value>::type eval_bitwise_and_default(T& t, const U& u, const V& v)
{
   t = u;
   eval_bitwise_and(t, v);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_and(T& t, const U& u, const V& v)
{
   eval_bitwise_and_default(t, u, v);
}

template <class T, class U, class V>
BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_or(T& t, const U& u, const V& v);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_or_default(T& t, const T& u, const T& v)
{
   if (&t == &v)
   {
      eval_bitwise_or(t, u);
   }
   else if (&t == &u)
   {
      eval_bitwise_or(t, v);
   }
   else
   {
      t = u;
      eval_bitwise_or(t, v);
   }
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_bitwise_or_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_bitwise_or(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_bitwise_or_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_bitwise_or(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value>::type eval_bitwise_or_default(T& t, const U& u, const T& v)
{
   eval_bitwise_or(t, v, u);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_or_default(T& t, const U& u, const V& v)
{
   BOOST_IF_CONSTEXPR(std::is_same<T, V>::value)
   {
      if ((void*)&t == (void*)&v)
      {
         eval_bitwise_or(t, u);
      }
      else
      {
         t = u;
         eval_bitwise_or(t, v);
      }
   }
   else
   {
      t = u;
      eval_bitwise_or(t, v);
   }
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_or(T& t, const U& u, const V& v)
{
   eval_bitwise_or_default(t, u, v);
}

template <class T, class U, class V>
BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_xor(T& t, const U& u, const V& v);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_xor_default(T& t, const T& u, const T& v)
{
   if (&t == &v)
   {
      eval_bitwise_xor(t, u);
   }
   else if (&t == &u)
   {
      eval_bitwise_xor(t, v);
   }
   else
   {
      t = u;
      eval_bitwise_xor(t, v);
   }
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && !std::is_convertible<U, T>::value>::type eval_bitwise_xor_default(T& t, const T& u, const U& v)
{
   T vv;
   vv = v;
   eval_bitwise_xor(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value && std::is_convertible<U, T>::value>::type eval_bitwise_xor_default(T& t, const T& u, const U& v)
{
   T vv(v);
   eval_bitwise_xor(t, u, vv);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<U, number<T, et_on> >::value>::type eval_bitwise_xor_default(T& t, const U& u, const T& v)
{
   eval_bitwise_xor(t, v, u);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_xor_default(T& t, const U& u, const V& v)
{
   BOOST_IF_CONSTEXPR(std::is_same<T, V>::value)
   {
      if ((void*)&t == (void*)&v)
      {
         eval_bitwise_xor(t, u);
      }
      else
      {
         t = u;
         eval_bitwise_xor(t, v);
      }
   }
   else
   {
      t = u;
      eval_bitwise_xor(t, v);
   }
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bitwise_xor(T& t, const U& u, const V& v)
{
   eval_bitwise_xor_default(t, u, v);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_increment(T& val)
{
   using ui_type = typename std::tuple_element<0, typename T::unsigned_types>::type;
   eval_add(val, static_cast<ui_type>(1u));
}
template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_decrement(T& val)
{
   using ui_type = typename std::tuple_element<0, typename T::unsigned_types>::type;
   eval_subtract(val, static_cast<ui_type>(1u));
}

template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_left_shift(T& result, const U& arg, const V val)
{
   result = arg;
   eval_left_shift(result, val);
}

template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_right_shift(T& result, const U& arg, const V val)
{
   result = arg;
   eval_right_shift(result, val);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_is_zero(const T& val)
{
   using ui_type = typename std::tuple_element<0, typename T::unsigned_types>::type;
   return val.compare(static_cast<ui_type>(0)) == 0;
}
template <class T>
inline BOOST_MP_CXX14_CONSTEXPR int eval_get_sign(const T& val)
{
   using ui_type = typename std::tuple_element<0, typename T::unsigned_types>::type;
   return val.compare(static_cast<ui_type>(0));
}

template <class T, class V, class U>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components_imp2(T& result, const V& v1, const U& v2, const std::false_type&, const std::false_type&)
{
   using component_number_type = typename component_type<number<T> >::type;

   boost::multiprecision::detail::scoped_precision_options<component_number_type> sp(result);
   (void)sp;

   component_number_type x(v1), y(v2);
   assign_components(result, x.backend(), y.backend());
}
template <class T, class V, class U>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components_imp2(T& result, const V& v1, const U& v2, const std::true_type&, const std::false_type&)
{
   boost::multiprecision::detail::scoped_source_precision<number<V>> scope;
   (void)scope;
   assign_components_imp2(result, number<V>(v1), v2, std::false_type(), std::false_type());
}
template <class T, class V, class U>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components_imp2(T& result, const V& v1, const U& v2, const std::true_type&, const std::true_type&)
{
   boost::multiprecision::detail::scoped_source_precision<number<V>> scope1;
   boost::multiprecision::detail::scoped_source_precision<number<U>> scope2;
   (void)scope1;
   (void)scope2;
   assign_components_imp2(result, number<V>(v1), number<U>(v2), std::false_type(), std::false_type());
}
template <class T, class V, class U>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components_imp2(T& result, const V& v1, const U& v2, const std::false_type&, const std::true_type&)
{
   boost::multiprecision::detail::scoped_source_precision<number<U>> scope;
   (void)scope;
   assign_components_imp2(result, v1, number<U>(v2), std::false_type(), std::false_type());
}


template <class T, class V, class U>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components_imp(T& result, const V& v1, const U& v2, const std::integral_constant<int, number_kind_rational>&)
{
   result = v1;
   T t;
   t = v2;
   eval_divide(result, t);
}

template <class T, class V, class U, int N>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components_imp(T& result, const V& v1, const U& v2, const std::integral_constant<int, N>&)
{
   assign_components_imp2(result, v1, v2, boost::multiprecision::detail::is_backend<V>(), boost::multiprecision::detail::is_backend<U>());
}

template <class T, class V, class U>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components(T& result, const V& v1, const U& v2)
{
   return assign_components_imp(result, v1, v2, typename number_category<T>::type());
}
#ifndef BOOST_NO_CXX17_HDR_STRING_VIEW
template <class Result, class Traits>
inline void assign_from_string_view(Result& result, const std::basic_string_view<char, Traits>& view)
{
   // since most (all?) backends require a const char* to construct from, we just
   // convert to that:
   std::string s(view);
   result = s.c_str();
}
template <class Result, class Traits>
inline void assign_from_string_view(Result& result, const std::basic_string_view<char, Traits>& view_x, const std::basic_string_view<char, Traits>& view_y)
{
   // since most (all?) backends require a const char* to construct from, we just
   // convert to that:
   std::string x(view_x), y(view_y);
   assign_components(result, x.c_str(), y.c_str());
}
#endif
template <class R, int b>
struct has_enough_bits
{
   template <class T>
   struct type : public std::integral_constant<bool, !std::is_same<R, T>::value && (std::numeric_limits<T>::digits >= b)>
   {};
};

template <class R>
struct terminal
{
   BOOST_MP_CXX14_CONSTEXPR terminal(const R& v) : value(v) {}
   BOOST_MP_CXX14_CONSTEXPR terminal() {}
   BOOST_MP_CXX14_CONSTEXPR terminal& operator=(R val)
   {
      value = val;
      return *this;
   }
   R value;
   BOOST_MP_CXX14_CONSTEXPR operator R() const { return value; }
};

template <class Tuple, int i, class T, bool = (i == std::tuple_size<Tuple>::value)>
struct find_index_of_type
{
   static constexpr int value = std::is_same<T, typename std::tuple_element<i, Tuple>::type>::value ? i : find_index_of_type<Tuple, i + 1, T>::value;
};
template <class Tuple, int i, class T>
struct find_index_of_type<Tuple, i, T, true>
{
   static constexpr int value = -1;
};


template <class R, class B>
struct calculate_next_larger_type
{
   // Find which list we're looking through:
   using list_type = typename std::conditional<
       boost::multiprecision::detail::is_signed<R>::value && boost::multiprecision::detail::is_integral<R>::value,
       typename B::signed_types,
       typename std::conditional<
           boost::multiprecision::detail::is_unsigned<R>::value,
           typename B::unsigned_types,
           typename B::float_types>::type>::type;
   static constexpr int start = find_index_of_type<list_type, 0, R>::value;
   static constexpr int index_of_type = boost::multiprecision::detail::find_index_of_large_enough_type<list_type, start == INT_MAX ? 0 : start + 1, std::numeric_limits<R>::digits>::value;
   using type = typename boost::multiprecision::detail::dereference_tuple<index_of_type, list_type, terminal<R> >::type;
};

template <class R, class T>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<R>::value, bool>::type check_in_range(const T& t)
{
   // Can t fit in an R?
   if ((t > 0) && std::numeric_limits<R>::is_specialized && std::numeric_limits<R>::is_bounded && (t > (std::numeric_limits<R>::max)()))
      return true;
   else
      return false;
}

template <class R, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<R>::value>::type eval_convert_to(R* result, const B& backend)
{
   using next_type = typename calculate_next_larger_type<R, B>::type;
   next_type                                               n = next_type();
   eval_convert_to(&n, backend);
   BOOST_IF_CONSTEXPR(!boost::multiprecision::detail::is_unsigned<R>::value && std::numeric_limits<R>::is_specialized && std::numeric_limits<R>::is_bounded)
   {
      if(n > (next_type)(std::numeric_limits<R>::max)())
      {
         *result = (std::numeric_limits<R>::max)();
         return;
      }
   }
   BOOST_IF_CONSTEXPR(std::numeric_limits<R>::is_specialized&& std::numeric_limits<R>::is_bounded)
   {
      if (n < (next_type)(std::numeric_limits<R>::min)())
      {
         *result = (std::numeric_limits<R>::min)();
         return;
      }
   }
   *result = static_cast<R>(n);
}

template <class R, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if< !boost::multiprecision::detail::is_integral<R>::value && !std::is_enum<R>::value>::type eval_convert_to(R* result, const B& backend)
{
   using next_type = typename calculate_next_larger_type<R, B>::type;
   next_type                                               n = next_type();
   eval_convert_to(&n, backend);
   BOOST_IF_CONSTEXPR(std::numeric_limits<R>::is_specialized && std::numeric_limits<R>::is_bounded)
   {
      if ((n > (next_type)(std::numeric_limits<R>::max)() || (n < (next_type) - (std::numeric_limits<R>::max)())))
      {
         *result = n > 0 ? (std::numeric_limits<R>::max)() : -(std::numeric_limits<R>::max)();
      }
      else
         *result = static_cast<R>(n);
   }
   else
      *result = static_cast<R>(n);
}

template <class R, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_enum<R>::value>::type eval_convert_to(R* result, const B& backend)
{
   typename std::underlying_type<R>::type t{};
   eval_convert_to(&t, backend);
   *result = static_cast<R>(t);
}

template <class R, class B>
inline void last_chance_eval_convert_to(terminal<R>* result, const B& backend, const std::integral_constant<bool, false>&)
{
   //
   // We ran out of types to try for the conversion, try
   // a lexical_cast and hope for the best:
   //
   BOOST_IF_CONSTEXPR (std::numeric_limits<R>::is_integer && !std::numeric_limits<R>::is_signed)
      if (eval_get_sign(backend) < 0)
         BOOST_THROW_EXCEPTION(std::range_error("Attempt to convert negative value to an unsigned integer results in undefined behaviour"));
   BOOST_TRY {
      result->value = boost::lexical_cast<R>(backend.str(0, std::ios_base::fmtflags(0)));
   }
   BOOST_CATCH (const bad_lexical_cast&)
   {
      if (eval_get_sign(backend) < 0)
      {
         BOOST_IF_CONSTEXPR(std::numeric_limits<R>::is_integer && !std::numeric_limits<R>::is_signed)
            *result = (std::numeric_limits<R>::max)(); // we should never get here, exception above will be raised.
         else BOOST_IF_CONSTEXPR(std::numeric_limits<R>::is_integer)
            *result = (std::numeric_limits<R>::min)();
         else
            *result = -(std::numeric_limits<R>::max)();
      }
      else
         *result = (std::numeric_limits<R>::max)();
   }
   BOOST_CATCH_END
}

template <class R, class B>
inline void last_chance_eval_convert_to(terminal<R>* result, const B& backend, const std::integral_constant<bool, true>&)
{
   //
   // Last chance conversion to an unsigned integer.
   // We ran out of types to try for the conversion, try
   // a lexical_cast and hope for the best:
   //
   if (eval_get_sign(backend) < 0)
      BOOST_THROW_EXCEPTION(std::range_error("Attempt to convert negative value to an unsigned integer results in undefined behaviour"));
   BOOST_TRY {
      B t(backend);
      R mask = ~static_cast<R>(0u);
      eval_bitwise_and(t, mask);
      result->value = boost::lexical_cast<R>(t.str(0, std::ios_base::fmtflags(0)));
   }
   BOOST_CATCH (const bad_lexical_cast&)
   {
      // We should never really get here...
      *result = (std::numeric_limits<R>::max)();
   }
   BOOST_CATCH_END
}

template <class R, class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_convert_to(terminal<R>* result, const B& backend)
{
   using tag_type = std::integral_constant<bool, boost::multiprecision::detail::is_unsigned<R>::value && number_category<B>::value == number_kind_integer>;
   last_chance_eval_convert_to(result, backend, tag_type());
}

template <class B1, class B2, expression_template_option et>
inline BOOST_MP_CXX14_CONSTEXPR void eval_convert_to(terminal<number<B1, et> >* result, const B2& backend)
{
   //
   // We ran out of types to try for the conversion, try
   // a generic conversion and hope for the best:
   //
   boost::multiprecision::detail::generic_interconvert(result->value.backend(), backend, number_category<B1>(), number_category<B2>());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_convert_to(std::string* result, const B& backend)
{
   *result = backend.str(0, std::ios_base::fmtflags(0));
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_convert_to(std::complex<float>* result, const B& backend)
{
   using scalar_type = typename scalar_result_from_possible_complex<multiprecision::number<B> >::type;
   scalar_type                                                                            re, im;
   eval_real(re.backend(), backend);
   eval_imag(im.backend(), backend);

   *result = std::complex<float>(re.template convert_to<float>(), im.template convert_to<float>());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_convert_to(std::complex<double>* result, const B& backend)
{
   using scalar_type = typename scalar_result_from_possible_complex<multiprecision::number<B> >::type;
   scalar_type                                                                            re, im;
   eval_real(re.backend(), backend);
   eval_imag(im.backend(), backend);

   *result = std::complex<double>(re.template convert_to<double>(), im.template convert_to<double>());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_convert_to(std::complex<long double>* result, const B& backend)
{
   using scalar_type = typename scalar_result_from_possible_complex<multiprecision::number<B> >::type;
   scalar_type                                                                            re, im;
   eval_real(re.backend(), backend);
   eval_imag(im.backend(), backend);

   *result = std::complex<long double>(re.template convert_to<long double>(), im.template convert_to<long double>());
}

//
// Functions:
//
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR void eval_abs(T& result, const U& arg)
{
   using type_list = typename U::signed_types            ;
   using front = typename std::tuple_element<0, type_list>::type;
   result = arg;
   if (arg.compare(front(0)) < 0)
      result.negate();
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR void eval_fabs(T& result, const U& arg)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The fabs function is only valid for floating point types.");
   using type_list = typename U::signed_types            ;
   using front = typename std::tuple_element<0, type_list>::type;
   result = arg;
   if (arg.compare(front(0)) < 0)
      result.negate();
}

template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR int eval_fpclassify(const Backend& arg)
{
   static_assert(number_category<Backend>::value == number_kind_floating_point, "The fpclassify function is only valid for floating point types.");
   return eval_is_zero(arg) ? FP_ZERO : FP_NORMAL;
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_fmod(T& result, const T& a, const T& b)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The fmod function is only valid for floating point types.");
   if ((&result == &a) || (&result == &b))
   {
      T temp;
      eval_fmod(temp, a, b);
      result = temp;
      return;
   }
   switch (eval_fpclassify(a))
   {
   case FP_ZERO:
      result = a;
      return;
   case FP_INFINITE:
   case FP_NAN:
      result = std::numeric_limits<number<T> >::quiet_NaN().backend();
      errno  = EDOM;
      return;
   }
   switch (eval_fpclassify(b))
   {
   case FP_ZERO:
   case FP_NAN:
      result = std::numeric_limits<number<T> >::quiet_NaN().backend();
      errno  = EDOM;
      return;
   }
   T n;
   eval_divide(result, a, b);
   if (eval_get_sign(result) < 0)
      eval_ceil(n, result);
   else
      eval_floor(n, result);
   eval_multiply(n, b);
   eval_subtract(result, a, n);
}
template <class T, class A>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value, void>::type eval_fmod(T& result, const T& x, const A& a)
{
   using canonical_type = typename boost::multiprecision::detail::canonical<A, T>::type         ;
   using cast_type = typename std::conditional<std::is_same<A, canonical_type>::value, T, canonical_type>::type;
   cast_type                                                                      c;
   c = a;
   eval_fmod(result, x, c);
}

template <class T, class A>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value, void>::type eval_fmod(T& result, const A& x, const T& a)
{
   using canonical_type = typename boost::multiprecision::detail::canonical<A, T>::type         ;
   using cast_type = typename std::conditional<std::is_same<A, canonical_type>::value, T, canonical_type>::type;
   cast_type                                                                      c;
   c = x;
   eval_fmod(result, c, a);
}

template <class T>
BOOST_MP_CXX14_CONSTEXPR void eval_round(T& result, const T& a);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_remquo(T& result, const T& a, const T& b, int* pi)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The remquo function is only valid for floating point types.");
   if ((&result == &a) || (&result == &b))
   {
      T temp;
      eval_remquo(temp, a, b, pi);
      result = temp;
      return;
   }
   T n;
   eval_divide(result, a, b);
   eval_round(n, result);
   eval_convert_to(pi, n);
   eval_multiply(n, b);
   eval_subtract(result, a, n);
}
template <class T, class A>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value, void>::type eval_remquo(T& result, const T& x, const A& a, int* pi)
{
   using canonical_type = typename boost::multiprecision::detail::canonical<A, T>::type         ;
   using cast_type = typename std::conditional<std::is_same<A, canonical_type>::value, T, canonical_type>::type;
   cast_type                                                                      c = cast_type();
   c = a;
   eval_remquo(result, x, c, pi);
}
template <class T, class A>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value, void>::type eval_remquo(T& result, const A& x, const T& a, int* pi)
{
   using canonical_type = typename boost::multiprecision::detail::canonical<A, T>::type         ;
   using cast_type = typename std::conditional<std::is_same<A, canonical_type>::value, T, canonical_type>::type;
   cast_type                                                                      c = cast_type();
   c = x;
   eval_remquo(result, c, a, pi);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_remainder(T& result, const U& a, const V& b)
{
   int i(0);
   eval_remquo(result, a, b, &i);
}

template <class B>
BOOST_MP_CXX14_CONSTEXPR bool eval_gt(const B& a, const B& b);
template <class T, class U>
BOOST_MP_CXX14_CONSTEXPR bool eval_gt(const T& a, const U& b);
template <class B>
BOOST_MP_CXX14_CONSTEXPR bool eval_lt(const B& a, const B& b);
template <class T, class U>
BOOST_MP_CXX14_CONSTEXPR bool eval_lt(const T& a, const U& b);

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_fdim(T& result, const T& a, const T& b)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   const ui_type                                                                zero = 0u;
   switch (eval_fpclassify(b))
   {
   case FP_NAN:
   case FP_INFINITE:
      result = zero;
      return;
   }
   switch (eval_fpclassify(a))
   {
   case FP_NAN:
      result = zero;
      return;
   case FP_INFINITE:
      result = a;
      return;
   }
   if (eval_gt(a, b))
   {
      eval_subtract(result, a, b);
   }
   else
      result = zero;
}

template <class T, class A>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value>::type eval_fdim(T& result, const T& a, const A& b)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   using arithmetic_type = typename boost::multiprecision::detail::canonical<A, T>::type       ;
   const ui_type                                                                zero        = 0u;
   arithmetic_type                                                              canonical_b = b;
   switch ((::boost::math::fpclassify)(b))
   {
   case FP_NAN:
   case FP_INFINITE:
      result = zero;
      return;
   }
   switch (eval_fpclassify(a))
   {
   case FP_NAN:
      result = zero;
      return;
   case FP_INFINITE:
      result = a;
      return;
   }
   if (eval_gt(a, canonical_b))
   {
      eval_subtract(result, a, canonical_b);
   }
   else
      result = zero;
}

template <class T, class A>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<A>::value>::type eval_fdim(T& result, const A& a, const T& b)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   using arithmetic_type = typename boost::multiprecision::detail::canonical<A, T>::type       ;
   const ui_type                                                                zero        = 0u;
   arithmetic_type                                                              canonical_a = a;
   switch (eval_fpclassify(b))
   {
   case FP_NAN:
   case FP_INFINITE:
      result = zero;
      return;
   }
   switch ((::boost::math::fpclassify)(a))
   {
   case FP_NAN:
      result = zero;
      return;
   case FP_INFINITE:
      result = std::numeric_limits<number<T> >::infinity().backend();
      return;
   }
   if (eval_gt(canonical_a, b))
   {
      eval_subtract(result, canonical_a, b);
   }
   else
      result = zero;
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_trunc(T& result, const T& a)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The trunc function is only valid for floating point types.");
   switch (eval_fpclassify(a))
   {
   case FP_NAN:
      errno = EDOM;
      // fallthrough...
   case FP_ZERO:
   case FP_INFINITE:
      result = a;
      return;
   }
   if (eval_get_sign(a) < 0)
      eval_ceil(result, a);
   else
      eval_floor(result, a);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_modf(T& result, T const& arg, T* pipart)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   int                                                                          c = eval_fpclassify(arg);
   if (c == (int)FP_NAN)
   {
      if (pipart)
         *pipart = arg;
      result = arg;
      return;
   }
   else if (c == (int)FP_INFINITE)
   {
      if (pipart)
         *pipart = arg;
      result = ui_type(0u);
      return;
   }
   if (pipart)
   {
      eval_trunc(*pipart, arg);
      eval_subtract(result, arg, *pipart);
   }
   else
   {
      T ipart;
      eval_trunc(ipart, arg);
      eval_subtract(result, arg, ipart);
   }
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_round(T& result, const T& a)
{
   static_assert(number_category<T>::value == number_kind_floating_point, "The round function is only valid for floating point types.");
   using fp_type = typename boost::multiprecision::detail::canonical<float, T>::type;
   int                                                                       c = eval_fpclassify(a);
   if (c == (int)FP_NAN)
   {
      result = a;
      errno  = EDOM;
      return;
   }
   if ((c == FP_ZERO) || (c == (int)FP_INFINITE))
   {
      result = a;
   }
   else if (eval_get_sign(a) < 0)
   {
      eval_subtract(result, a, fp_type(0.5f));
      eval_ceil(result, result);
   }
   else
   {
      eval_add(result, a, fp_type(0.5f));
      eval_floor(result, result);
   }
}

template <class B>
BOOST_MP_CXX14_CONSTEXPR void eval_lcm(B& result, const B& a, const B& b);
template <class B>
BOOST_MP_CXX14_CONSTEXPR void eval_gcd(B& result, const B& a, const B& b);

template <class T, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<Arithmetic>::value >::type eval_gcd(T& result, const T& a, const Arithmetic& b)
{
   using si_type = typename boost::multiprecision::detail::canonical<Arithmetic, T>::type;
   using default_ops::eval_gcd;
   T t;
   t = static_cast<si_type>(b);
   eval_gcd(result, a, t);
}
template <class T, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<Arithmetic>::value >::type eval_gcd(T& result, const Arithmetic& a, const T& b)
{
   eval_gcd(result, b, a);
}
template <class T, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<Arithmetic>::value >::type eval_lcm(T& result, const T& a, const Arithmetic& b)
{
   using si_type = typename boost::multiprecision::detail::canonical<Arithmetic, T>::type;
   using default_ops::eval_lcm;
   T t;
   t = static_cast<si_type>(b);
   eval_lcm(result, a, t);
}
template <class T, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<Arithmetic>::value >::type eval_lcm(T& result, const Arithmetic& a, const T& b)
{
   eval_lcm(result, b, a);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR unsigned eval_lsb(const T& val)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   int                                                                          c = eval_get_sign(val);
   if (c == 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("No bits were set in the operand."));
   }
   if (c < 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("Testing individual bits in negative values is not supported - results are undefined."));
   }
   unsigned result = 0;
   T        mask, t;
   mask = ui_type(1);
   do
   {
      eval_bitwise_and(t, mask, val);
      ++result;
      eval_left_shift(mask, 1);
   } while (eval_is_zero(t));

   return --result;
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR int eval_msb(const T& val)
{
   int c = eval_get_sign(val);
   if (c == 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("No bits were set in the operand."));
   }
   if (c < 0)
   {
      BOOST_THROW_EXCEPTION(std::domain_error("Testing individual bits in negative values is not supported - results are undefined."));
   }
   //
   // This implementation is really really rubbish - it does
   // a linear scan for the most-significant-bit.  We should really
   // do a binary search, but as none of our backends actually needs
   // this implementation, we'll leave it for now.  In fact for most
   // backends it's likely that there will always be a more efficient
   // native implementation possible.
   //
   unsigned result = 0;
   T        t(val);
   while (!eval_is_zero(t))
   {
      eval_right_shift(t, 1);
      ++result;
   }
   return --result;
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_bit_test(const T& val, unsigned index)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   T                                                                            mask, t;
   mask = ui_type(1);
   eval_left_shift(mask, index);
   eval_bitwise_and(t, mask, val);
   return !eval_is_zero(t);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bit_set(T& val, unsigned index)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   T                                                                            mask;
   mask = ui_type(1);
   eval_left_shift(mask, index);
   eval_bitwise_or(val, mask);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bit_flip(T& val, unsigned index)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   T                                                                            mask;
   mask = ui_type(1);
   eval_left_shift(mask, index);
   eval_bitwise_xor(val, mask);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_bit_unset(T& val, unsigned index)
{
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned, T>::type;
   T                                                                            mask, t;
   mask = ui_type(1);
   eval_left_shift(mask, index);
   eval_bitwise_and(t, mask, val);
   if (!eval_is_zero(t))
      eval_bitwise_xor(val, mask);
}

template <class Backend>
BOOST_MP_CXX14_CONSTEXPR void eval_qr(const Backend& x, const Backend& y, Backend& q, Backend& r);

template <class Backend>
BOOST_MP_CXX14_CONSTEXPR void eval_karatsuba_sqrt(Backend& result, const Backend& x, Backend& r, Backend& t, size_t bits)
{
   using default_ops::eval_is_zero;
   using default_ops::eval_subtract;
   using default_ops::eval_right_shift;
   using default_ops::eval_left_shift;
   using default_ops::eval_bit_set;
   using default_ops::eval_decrement;
   using default_ops::eval_bitwise_and;
   using default_ops::eval_add;
   using default_ops::eval_qr;

   using small_uint = typename std::tuple_element<0, typename Backend::unsigned_types>::type;

   constexpr small_uint zero = 0u;

   // we can calculate it faster with std::sqrt
#ifdef BOOST_HAS_INT128
   if (bits <= 128)
   {
      unsigned __int128 a{}, b{}, c{};
      eval_convert_to(&a, x);
      c = boost::multiprecision::detail::karatsuba_sqrt(a, b, bits);
      r = number<Backend>::canonical_value(b);
      result = number<Backend>::canonical_value(c);
      return;
   }
#else
   if (bits <= std::numeric_limits<std::uintmax_t>::digits)
   {
      std::uintmax_t a{ 0 }, b{ 0 }, c{ 0 };
      eval_convert_to(&a, x);
      c = boost::multiprecision::detail::karatsuba_sqrt(a, b, bits);
      r = number<Backend>::canonical_value(b);
      result = number<Backend>::canonical_value(c);
      return;
   }
#endif
   // https://hal.inria.fr/file/index/docid/72854/filename/RR-3805.pdf
   std::size_t  b = bits / 4;
   Backend q(x);
   eval_right_shift(q, b * 2);
   Backend s;
   eval_karatsuba_sqrt(s, q, r, t, bits - b * 2);
   t = zero;
   eval_bit_set(t, static_cast<unsigned>(b * 2));
   eval_left_shift(r, b);
   eval_decrement(t);
   eval_bitwise_and(t, x);
   eval_right_shift(t, b);
   eval_add(t, r);
   eval_left_shift(s, 1u);
   eval_qr(t, s, q, r);
   eval_left_shift(r, b);
   t = zero;
   eval_bit_set(t, static_cast<unsigned>(b));
   eval_decrement(t);
   eval_bitwise_and(t, x);
   eval_add(r, t);
   eval_left_shift(s, b - 1);
   eval_add(s, q);
   eval_multiply(q, q);
   // we substract after, so it works for unsigned integers too
   if (r.compare(q) < 0)
   {
      t = s;
      eval_left_shift(t, 1u);
      eval_decrement(t);
      eval_add(r, t);
      eval_decrement(s);
   }
   eval_subtract(r, q);
   result = s;
}

template <class B>
void BOOST_MP_CXX14_CONSTEXPR eval_integer_sqrt_bitwise(B& s, B& r, const B& x)
{
   //
   // This is slow bit-by-bit integer square root, see for example
   // http://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_.28base_2.29
   // There are better methods such as http://hal.inria.fr/docs/00/07/28/54/PDF/RR-3805.pdf
   // and http://hal.inria.fr/docs/00/07/21/13/PDF/RR-4475.pdf which should be implemented
   // at some point.
   //
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned char, B>::type;

   s = ui_type(0u);
   if (eval_get_sign(x) == 0)
   {
      r = ui_type(0u);
      return;
   }
   int g = eval_msb(x);
   if (g <= 1)
   {
      s = ui_type(1);
      eval_subtract(r, x, s);
      return;
   }

   B t;
   r = x;
   g /= 2;
   int org_g = g;
   eval_bit_set(s, g);
   eval_bit_set(t, 2 * g);
   eval_subtract(r, x, t);
   --g;
   if (eval_get_sign(r) == 0)
      return;
   int msbr = eval_msb(r);
   do
   {
      if (msbr >= org_g + g + 1)
      {
         t = s;
         eval_left_shift(t, g + 1);
         eval_bit_set(t, 2 * g);
         if (t.compare(r) <= 0)
         {
            BOOST_ASSERT(g >= 0);
            eval_bit_set(s, g);
            eval_subtract(r, t);
            if (eval_get_sign(r) == 0)
               return;
            msbr = eval_msb(r);
         }
      }
      --g;
   } while (g >= 0);
}

template <class Backend>
BOOST_MP_CXX14_CONSTEXPR void eval_integer_sqrt(Backend& result, Backend& r, const Backend& x)
{
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   // recursive Karatsuba sqrt can cause issues in constexpr context:
   if (BOOST_MP_IS_CONST_EVALUATED(result.size()))
      return eval_integer_sqrt_bitwise(result, r, x);
#endif
   using small_uint = typename std::tuple_element<0, typename Backend::unsigned_types>::type;

   constexpr small_uint zero = 0u;

   if (eval_is_zero(x))
   {
      r = zero;
      result = zero;
      return;
   }
   Backend t;
   eval_karatsuba_sqrt(result, x, r, t, eval_msb(x) + 1);
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_conj(B& result, const B& val)
{
   result = val; // assume non-complex result.
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_proj(B& result, const B& val)
{
   result = val; // assume non-complex result.
}

//
// These have to implemented by the backend, declared here so that our macro generated code compiles OK.
//
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_floor();
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_ceil();
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_trunc();
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_sqrt();
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_ldexp();
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_frexp();
// TODO implement default versions of these:
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_asinh();
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_acosh();
template <class T>
typename std::enable_if<sizeof(T) == 0>::type eval_atanh();

//
// eval_logb and eval_scalbn simply assume base 2 and forward to
// eval_ldexp and eval_frexp:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename B::exponent_type eval_ilogb(const B& val)
{
   static_assert(!std::numeric_limits<number<B> >::is_specialized || (std::numeric_limits<number<B> >::radix == 2), "The default implementation of ilogb requires a base 2 number type");
   typename B::exponent_type e(0);
   switch (eval_fpclassify(val))
   {
   case FP_NAN:
#ifdef FP_ILOGBNAN
      return FP_ILOGBNAN > 0 ? (std::numeric_limits<typename B::exponent_type>::max)() : (std::numeric_limits<typename B::exponent_type>::min)();
#else
      return (std::numeric_limits<typename B::exponent_type>::max)();
#endif
   case FP_INFINITE:
      return (std::numeric_limits<typename B::exponent_type>::max)();
   case FP_ZERO:
      return (std::numeric_limits<typename B::exponent_type>::min)();
   }
   B result;
   eval_frexp(result, val, &e);
   return e - 1;
}

template <class T>
BOOST_MP_CXX14_CONSTEXPR int eval_signbit(const T& val);

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_logb(B& result, const B& val)
{
   switch (eval_fpclassify(val))
   {
   case FP_NAN:
      result = val;
      errno  = EDOM;
      return;
   case FP_ZERO:
      result = std::numeric_limits<number<B> >::infinity().backend();
      result.negate();
      errno = ERANGE;
      return;
   case FP_INFINITE:
      result = val;
      if (eval_signbit(val))
         result.negate();
      return;
   }
   using max_t = typename std::conditional<std::is_same<std::intmax_t, long>::value, boost::long_long_type, std::intmax_t>::type;
   result = static_cast<max_t>(eval_ilogb(val));
}
template <class B, class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_scalbn(B& result, const B& val, A e)
{
   static_assert(!std::numeric_limits<number<B> >::is_specialized || (std::numeric_limits<number<B> >::radix == 2), "The default implementation of scalbn requires a base 2 number type");
   eval_ldexp(result, val, static_cast<typename B::exponent_type>(e));
}
template <class B, class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_scalbln(B& result, const B& val, A e)
{
   eval_scalbn(result, val, e);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR bool is_arg_nan(const T& val, std::integral_constant<bool, true> const&, const std::integral_constant<bool, false>&)
{
   return eval_fpclassify(val) == FP_NAN;
}
template <class T>
inline BOOST_MP_CXX14_CONSTEXPR bool is_arg_nan(const T& val, std::integral_constant<bool, false> const&, const std::integral_constant<bool, true>&)
{
   return (boost::math::isnan)(val);
}
template <class T>
inline BOOST_MP_CXX14_CONSTEXPR bool is_arg_nan(const T&, std::integral_constant<bool, false> const&, const std::integral_constant<bool, false>&)
{
   return false;
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR bool is_arg_nan(const T& val)
{
   return is_arg_nan(val, std::integral_constant<bool, boost::multiprecision::detail::is_backend<T>::value>(), std::is_floating_point<T>());
}

template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_fmax(T& result, const U& a, const V& b)
{
   if (is_arg_nan(a))
      result = number<T>::canonical_value(b);
   else if (is_arg_nan(b))
      result = number<T>::canonical_value(a);
   else if (eval_lt(number<T>::canonical_value(a), number<T>::canonical_value(b)))
      result = number<T>::canonical_value(b);
   else
      result = number<T>::canonical_value(a);
}
template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR void eval_fmin(T& result, const U& a, const V& b)
{
   if (is_arg_nan(a))
      result = number<T>::canonical_value(b);
   else if (is_arg_nan(b))
      result = number<T>::canonical_value(a);
   else if (eval_lt(number<T>::canonical_value(a), number<T>::canonical_value(b)))
      result = number<T>::canonical_value(a);
   else
      result = number<T>::canonical_value(b);
}

template <class R, class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR void eval_hypot(R& result, const T& a, const U& b)
{
   //
   // Normalize x and y, so that both are positive and x >= y:
   //
   R x, y;
   x = number<R>::canonical_value(a);
   y = number<R>::canonical_value(b);
   if (eval_get_sign(x) < 0)
      x.negate();
   if (eval_get_sign(y) < 0)
      y.negate();

   // Special case, see C99 Annex F.
   // The order of the if's is important: do not change!
   int c1 = eval_fpclassify(x);
   int c2 = eval_fpclassify(y);

   if (c1 == FP_ZERO)
   {
      result = y;
      return;
   }
   if (c2 == FP_ZERO)
   {
      result = x;
      return;
   }
   if (c1 == FP_INFINITE)
   {
      result = x;
      return;
   }
   if ((c2 == FP_INFINITE) || (c2 == FP_NAN))
   {
      result = y;
      return;
   }
   if (c1 == FP_NAN)
   {
      result = x;
      return;
   }

   if (eval_gt(y, x))
      x.swap(y);

   eval_multiply(result, x, std::numeric_limits<number<R> >::epsilon().backend());

   if (eval_gt(result, y))
   {
      result = x;
      return;
   }

   R rat;
   eval_divide(rat, y, x);
   eval_multiply(result, rat, rat);
   eval_increment(result);
   eval_sqrt(rat, result);
   eval_multiply(result, rat, x);
}

template <class R, class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_nearbyint(R& result, const T& a)
{
   eval_round(result, a);
}
template <class R, class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_rint(R& result, const T& a)
{
   eval_nearbyint(result, a);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR int eval_signbit(const T& val)
{
   return eval_get_sign(val) < 0 ? 1 : 0;
}

//
// Real and imaginary parts:
//
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_real(To& to, const From& from)
{
   to = from;
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_imag(To& to, const From&)
{
   using ui_type = typename std::tuple_element<0, typename To::unsigned_types>::type;
   to = ui_type(0);
}

} // namespace default_ops
namespace default_ops_adl {

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real_imp(To& to, const From& from)
{
   using to_component_type = typename component_type<number<To> >::type;
   typename to_component_type::backend_type           to_component;
   to_component = from;
   eval_set_real(to, to_component);
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag_imp(To& to, const From& from)
{
   using to_component_type = typename component_type<number<To> >::type;
   typename to_component_type::backend_type           to_component;
   to_component = from;
   eval_set_imag(to, to_component);
}

} // namespace default_ops_adl
namespace default_ops {

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<To>::value == number_kind_complex>::type eval_set_real(To& to, const From& from)
{
   default_ops_adl::eval_set_real_imp(to, from);
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<To>::value != number_kind_complex>::type eval_set_real(To& to, const From& from)
{
   to = from;
}

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag(To& to, const From& from)
{
   default_ops_adl::eval_set_imag_imp(to, from);
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real(T& to, const T& from)
{
   to = from;
}
template <class T>
void BOOST_MP_CXX14_CONSTEXPR eval_set_imag(T&, const T&)
{
   static_assert(sizeof(T) == INT_MAX, "eval_set_imag needs to be specialised for each specific backend");
}

//
// These functions are implemented in separate files, but expanded inline here,
// DO NOT CHANGE THE ORDER OF THESE INCLUDES:
//
#include <boost/multiprecision/detail/functions/constants.hpp>
#include <boost/multiprecision/detail/functions/pow.hpp>
#include <boost/multiprecision/detail/functions/trig.hpp>

} // namespace default_ops

//
// Default versions of floating point classification routines:
//
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR int fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   using multiprecision::default_ops::eval_fpclassify;
   return eval_fpclassify(arg.backend());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR int fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return fpclassify                                                                     BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool isfinite BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   int v = fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(arg);
   return (v != (int)FP_INFINITE) && (v != (int)FP_NAN);
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool isfinite BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return isfinite                                                                       BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool isnan BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   return fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(arg) == (int)FP_NAN;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool isnan BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return isnan                                                                          BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool isinf BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   return fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(arg) == (int)FP_INFINITE;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool isinf BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return isinf                                                                          BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool isnormal BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   return fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(arg) == (int)FP_NORMAL;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool isnormal BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return isnormal                                                                       BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}

// Default versions of sign manipulation functions, if individual backends can do better than this
// (for example with signed zero), then they should overload these functions further:

template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR int sign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   return arg.sign();
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR int sign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return sign                                                                           BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}

template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR int signbit BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   using default_ops::eval_signbit;
   return eval_signbit(arg.backend());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR int signbit BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return signbit                                                                        BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> changesign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   return -arg;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type changesign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return changesign                                                                     BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> copysign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& a, const multiprecision::number<Backend, ExpressionTemplates>& b)
{
   return (boost::multiprecision::signbit)(a) != (boost::multiprecision::signbit)(b) ? (boost::multiprecision::changesign)(a) : a;
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates, class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> copysign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& a, const multiprecision::detail::expression<tag, A1, A2, A3, A4>& b)
{
   return copysign BOOST_PREVENT_MACRO_SUBSTITUTION(a, multiprecision::number<Backend, ExpressionTemplates>(b));
}
template <class tag, class A1, class A2, class A3, class A4, class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> copysign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& a, const multiprecision::number<Backend, ExpressionTemplates>& b)
{
   return copysign BOOST_PREVENT_MACRO_SUBSTITUTION(multiprecision::number<Backend, ExpressionTemplates>(a), b);
}
template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type copysign BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& a, const multiprecision::detail::expression<tagb, A1b, A2b, A3b, A4b>& b)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return copysign                                                                       BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(a), value_type(b));
}
//
// real and imag:
//
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type
real(const multiprecision::number<Backend, ExpressionTemplates>& a)
{
   using default_ops::eval_real;
   using result_type = typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type;
   boost::multiprecision::detail::scoped_default_precision<result_type>                                              precision_guard(a);
   result_type                                                                                                       result;
   eval_real(result.backend(), a.backend());
   return result;
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type
imag(const multiprecision::number<Backend, ExpressionTemplates>& a)
{
   using default_ops::eval_imag;
   using result_type = typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type;
   boost::multiprecision::detail::scoped_default_precision<result_type>                                              precision_guard(a);
   result_type                                                                                                       result;
   eval_imag(result.backend(), a.backend());
   return result;
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename scalar_result_from_possible_complex<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::type
real(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return real(value_type(arg));
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename scalar_result_from_possible_complex<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::type
imag(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return imag(value_type(arg));
}

//
// Complex number functions, these are overloaded at the Backend level, we just provide the
// expression template versions here, plus overloads for non-complex types:
//
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename boost::lazy_enable_if_c<number_category<T>::value == number_kind_complex, component_type<number<T, ExpressionTemplates> > >::type
abs(const number<T, ExpressionTemplates>& v)
{
   return std::move(boost::math::hypot(real(v), imag(v)));
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename boost::lazy_enable_if_c<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_complex, component_type<typename detail::expression<tag, A1, A2, A3, A4>::result_type> >::type
abs(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(abs(static_cast<number_type>(v)));
}

template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_complex, typename scalar_result_from_possible_complex<number<T, ExpressionTemplates> >::type>::type
arg(const number<T, ExpressionTemplates>& v)
{
   return std::move(atan2(imag(v), real(v)));
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_floating_point, typename scalar_result_from_possible_complex<number<T, ExpressionTemplates> >::type>::type
arg(const number<T, ExpressionTemplates>&)
{
   return 0;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_complex || number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_floating_point, typename scalar_result_from_possible_complex<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type>::type
arg(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(arg(static_cast<number_type>(v)));
}

template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename boost::lazy_enable_if_c<number_category<T>::value == number_kind_complex, component_type<number<T, ExpressionTemplates> > >::type
norm(const number<T, ExpressionTemplates>& v)
{
   typename component_type<number<T, ExpressionTemplates> >::type a(real(v)), b(imag(v));
   return std::move(a * a + b * b);
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value != number_kind_complex, typename scalar_result_from_possible_complex<number<T, ExpressionTemplates> >::type>::type
norm(const number<T, ExpressionTemplates>& v)
{
   return v * v;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename scalar_result_from_possible_complex<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type
norm(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(norm(static_cast<number_type>(v)));
}

template <class Backend, expression_template_option ExpressionTemplates>
BOOST_MP_CXX14_CONSTEXPR typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type polar(number<Backend, ExpressionTemplates> const& r, number<Backend, ExpressionTemplates> const& theta)
{
   return typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type(number<Backend, ExpressionTemplates>(r * cos(theta)), number<Backend, ExpressionTemplates>(r * sin(theta)));
}

template <class tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_same<typename detail::expression<tag, A1, A2, A3, A4>::result_type, number<Backend, ExpressionTemplates> >::value,
                     typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type>::type
polar(detail::expression<tag, A1, A2, A3, A4> const& r, number<Backend, ExpressionTemplates> const& theta)
{
   return typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type(number<Backend, ExpressionTemplates>(r * cos(theta)), number<Backend, ExpressionTemplates>(r * sin(theta)));
}

template <class Backend, expression_template_option ExpressionTemplates, class tag, class A1, class A2, class A3, class A4>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_same<typename detail::expression<tag, A1, A2, A3, A4>::result_type, number<Backend, ExpressionTemplates> >::value,
                     typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type>::type
polar(number<Backend, ExpressionTemplates> const& r, detail::expression<tag, A1, A2, A3, A4> const& theta)
{
   return typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type(number<Backend, ExpressionTemplates>(r * cos(theta)), number<Backend, ExpressionTemplates>(r * sin(theta)));
}

template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_same<typename detail::expression<tag, A1, A2, A3, A4>::result_type, typename detail::expression<tagb, A1b, A2b, A3b, A4b>::result_type>::value,
                     typename complex_result_from_scalar<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type>::type
polar(detail::expression<tag, A1, A2, A3, A4> const& r, detail::expression<tagb, A1b, A2b, A3b, A4b> const& theta)
{
   using scalar_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return typename complex_result_from_scalar<scalar_type>::type(scalar_type(r * cos(theta)), scalar_type(r * sin(theta)));
}
//
// We also allow the first argument to polar to be an arithmetic type (probably a literal):
//
template <class Scalar, class Backend, expression_template_option ExpressionTemplates>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<Scalar>::value, typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type>::type
polar(Scalar const& r, number<Backend, ExpressionTemplates> const& theta)
{
   return typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type(number<Backend, ExpressionTemplates>(r * cos(theta)), number<Backend, ExpressionTemplates>(r * sin(theta)));
}

template <class tag, class A1, class A2, class A3, class A4, class Scalar>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<Scalar>::value,
                     typename complex_result_from_scalar<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type>::type
polar(Scalar const& r, detail::expression<tag, A1, A2, A3, A4> const& theta)
{
   using scalar_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return typename complex_result_from_scalar<scalar_type>::type(scalar_type(r * cos(theta)), scalar_type(r * sin(theta)));
}
//
// Single argument overloads:
//
template <class Backend, expression_template_option ExpressionTemplates>
BOOST_MP_CXX14_CONSTEXPR typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type polar(number<Backend, ExpressionTemplates> const& r)
{
   return typename complex_result_from_scalar<number<Backend, ExpressionTemplates> >::type(r);
}

template <class tag, class A1, class A2, class A3, class A4>
BOOST_MP_CXX14_CONSTEXPR typename complex_result_from_scalar<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type
polar(detail::expression<tag, A1, A2, A3, A4> const& r)
{
   return typename complex_result_from_scalar<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type(r);
}

} // namespace multiprecision

namespace math {

//
// Import Math functions here, so they can be found by Boost.Math:
//
using boost::multiprecision::changesign;
using boost::multiprecision::copysign;
using boost::multiprecision::fpclassify;
using boost::multiprecision::isfinite;
using boost::multiprecision::isinf;
using boost::multiprecision::isnan;
using boost::multiprecision::isnormal;
using boost::multiprecision::sign;
using boost::multiprecision::signbit;

} // namespace math

namespace multiprecision {

using c99_error_policy = ::boost::math::policies::policy<
    ::boost::math::policies::domain_error< ::boost::math::policies::errno_on_error>,
    ::boost::math::policies::pole_error< ::boost::math::policies::errno_on_error>,
    ::boost::math::policies::overflow_error< ::boost::math::policies::errno_on_error>,
    ::boost::math::policies::evaluation_error< ::boost::math::policies::errno_on_error>,
    ::boost::math::policies::rounding_error< ::boost::math::policies::errno_on_error> >;

template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value != number_kind_complex, multiprecision::number<Backend, ExpressionTemplates> >::type
    asinh
    BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::asinh(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex, typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::type
    asinh
    BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return asinh(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value != number_kind_complex, multiprecision::number<Backend, ExpressionTemplates> >::type
    acosh
    BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::acosh(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex, typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::type
    acosh
    BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return acosh(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value != number_kind_complex, multiprecision::number<Backend, ExpressionTemplates> >::type
    atanh
    BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::atanh(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex, typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::type
    atanh
    BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return atanh(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::cbrt(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return cbrt(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::erf(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type erf BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return erf(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::erfc(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return erfc(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::expm1(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return expm1(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   multiprecision::number<Backend, ExpressionTemplates>                                    result;
   result = boost::math::lgamma(arg, c99_error_policy());
   if ((boost::multiprecision::isnan)(result) && !(boost::multiprecision::isnan)(arg))
   {
      result = std::numeric_limits<multiprecision::number<Backend, ExpressionTemplates> >::infinity();
      errno  = ERANGE;
   }
   return result;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return lgamma(value_type(arg));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   if ((arg == 0) && std::numeric_limits<multiprecision::number<Backend, ExpressionTemplates> >::has_infinity)
   {
      errno = ERANGE;
      return 1 / arg;
   }
   return boost::math::tgamma(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return tgamma(value_type(arg));
}

template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR long lrint BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   return lround(arg);
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR long lrint BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   return lround(arg);
}
#ifndef BOOST_NO_LONG_LONG
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type llrint BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   return llround(arg);
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type llrint BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   return llround(arg);
}
#endif
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(arg);
   return boost::math::log1p(arg, c99_error_policy());
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(arg);
   return log1p(value_type(arg));
}

template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& a, const multiprecision::number<Backend, ExpressionTemplates>& b)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(a, b);
   return boost::math::nextafter(a, b, c99_error_policy());
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates, class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& a, const multiprecision::detail::expression<tag, A1, A2, A3, A4>& b)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(a, b);
   return nextafter                                                                        BOOST_PREVENT_MACRO_SUBSTITUTION(a, multiprecision::number<Backend, ExpressionTemplates>(b));
}
template <class tag, class A1, class A2, class A3, class A4, class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& a, const multiprecision::number<Backend, ExpressionTemplates>& b)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(a, b);
   return nextafter                                                                        BOOST_PREVENT_MACRO_SUBSTITUTION(multiprecision::number<Backend, ExpressionTemplates>(a), b);
}
template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type nextafter BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& a, const multiprecision::detail::expression<tagb, A1b, A2b, A3b, A4b>& b)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(a, b);
   return nextafter                                                                      BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(a), value_type(b));
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& a, const multiprecision::number<Backend, ExpressionTemplates>& b)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(a, b);
   return boost::math::nextafter(a, b, c99_error_policy());
}
template <class Backend, multiprecision::expression_template_option ExpressionTemplates, class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::number<Backend, ExpressionTemplates>& a, const multiprecision::detail::expression<tag, A1, A2, A3, A4>& b)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(a, b);
   return nexttoward                                                                       BOOST_PREVENT_MACRO_SUBSTITUTION(a, multiprecision::number<Backend, ExpressionTemplates>(b));
}
template <class tag, class A1, class A2, class A3, class A4, class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<Backend, ExpressionTemplates> nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& a, const multiprecision::number<Backend, ExpressionTemplates>& b)
{
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(a, b);
   return nexttoward                                                                       BOOST_PREVENT_MACRO_SUBSTITUTION(multiprecision::number<Backend, ExpressionTemplates>(a), b);
}
template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type nexttoward BOOST_PREVENT_MACRO_SUBSTITUTION(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& a, const multiprecision::detail::expression<tagb, A1b, A2b, A3b, A4b>& b)
{
   using value_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<value_type>                                          precision_guard(a, b);
   return nexttoward                                                                     BOOST_PREVENT_MACRO_SUBSTITUTION(value_type(a), value_type(b));
}

template <class B1, class B2, class B3, expression_template_option ET1, expression_template_option ET2, expression_template_option ET3>
inline BOOST_MP_CXX14_CONSTEXPR number<B1, ET1>& add(number<B1, ET1>& result, const number<B2, ET2>& a, const number<B3, ET3>& b)
{
   static_assert((std::is_convertible<B2, B1>::value), "No conversion to the target of a mixed precision addition exists");
   static_assert((std::is_convertible<B3, B1>::value), "No conversion to the target of a mixed precision addition exists");
   using default_ops::eval_add;
   eval_add(result.backend(), a.backend(), b.backend());
   return result;
}

template <class B1, class B2, class B3, expression_template_option ET1, expression_template_option ET2, expression_template_option ET3>
inline BOOST_MP_CXX14_CONSTEXPR number<B1, ET1>& subtract(number<B1, ET1>& result, const number<B2, ET2>& a, const number<B3, ET3>& b)
{
   static_assert((std::is_convertible<B2, B1>::value), "No conversion to the target of a mixed precision addition exists");
   static_assert((std::is_convertible<B3, B1>::value), "No conversion to the target of a mixed precision addition exists");
   using default_ops::eval_subtract;
   eval_subtract(result.backend(), a.backend(), b.backend());
   return result;
}

template <class B1, class B2, class B3, expression_template_option ET1, expression_template_option ET2, expression_template_option ET3>
inline BOOST_MP_CXX14_CONSTEXPR number<B1, ET1>& multiply(number<B1, ET1>& result, const number<B2, ET2>& a, const number<B3, ET3>& b)
{
   static_assert((std::is_convertible<B2, B1>::value), "No conversion to the target of a mixed precision addition exists");
   static_assert((std::is_convertible<B3, B1>::value), "No conversion to the target of a mixed precision addition exists");
   using default_ops::eval_multiply;
   eval_multiply(result.backend(), a.backend(), b.backend());
   return result;
}

template <class B, expression_template_option ET, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value, number<B, ET>&>::type
add(number<B, ET>& result, const I& a, const I& b)
{
   using default_ops::eval_add;
   using canonical_type = typename detail::canonical<I, B>::type;
   eval_add(result.backend(), static_cast<canonical_type>(a), static_cast<canonical_type>(b));
   return result;
}

template <class B, expression_template_option ET, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value, number<B, ET>&>::type
subtract(number<B, ET>& result, const I& a, const I& b)
{
   using default_ops::eval_subtract;
   using canonical_type = typename detail::canonical<I, B>::type;
   eval_subtract(result.backend(), static_cast<canonical_type>(a), static_cast<canonical_type>(b));
   return result;
}

template <class B, expression_template_option ET, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value, number<B, ET>&>::type
multiply(number<B, ET>& result, const I& a, const I& b)
{
   using default_ops::eval_multiply;
   using canonical_type = typename detail::canonical<I, B>::type;
   eval_multiply(result.backend(), static_cast<canonical_type>(a), static_cast<canonical_type>(b));
   return result;
}

template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<tag, A1, A2, A3, A4>::result_type trunc(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(trunc(number_type(v), pol));
}

template <class Backend, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR number<Backend, ExpressionTemplates> trunc(const number<Backend, ExpressionTemplates>& v, const Policy&)
{
   using default_ops::eval_trunc;
   detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(v);
   number<Backend, ExpressionTemplates>                                                    result;
   eval_trunc(result.backend(), v.backend());
   return result;
}

template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR int itrunc(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                           r(trunc(v, pol));
   if ((r > (std::numeric_limits<int>::max)()) || r < (std::numeric_limits<int>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::itrunc<%1%>(%1%)", 0, number_type(v), 0, pol);
   return r.template convert_to<int>();
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR int itrunc(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   return itrunc(v, boost::math::policies::policy<>());
}
template <class Backend, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR int itrunc(const number<Backend, ExpressionTemplates>& v, const Policy& pol)
{
   number<Backend, ExpressionTemplates> r(trunc(v, pol));
   if ((r > (std::numeric_limits<int>::max)()) || r < (std::numeric_limits<int>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::itrunc<%1%>(%1%)", 0, v, 0, pol);
   return r.template convert_to<int>();
}
template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR int itrunc(const number<Backend, ExpressionTemplates>& v)
{
   return itrunc(v, boost::math::policies::policy<>());
}
template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR long ltrunc(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                           r(trunc(v, pol));
   if ((r > (std::numeric_limits<long>::max)()) || r < (std::numeric_limits<long>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::ltrunc<%1%>(%1%)", 0, number_type(v), 0L, pol);
   return r.template convert_to<long>();
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR long ltrunc(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   return ltrunc(v, boost::math::policies::policy<>());
}
template <class T, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR long ltrunc(const number<T, ExpressionTemplates>& v, const Policy& pol)
{
   number<T, ExpressionTemplates> r(trunc(v, pol));
   if ((r > (std::numeric_limits<long>::max)()) || r < (std::numeric_limits<long>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::ltrunc<%1%>(%1%)", 0, v, 0L, pol);
   return r.template convert_to<long>();
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR long ltrunc(const number<T, ExpressionTemplates>& v)
{
   return ltrunc(v, boost::math::policies::policy<>());
}
#ifndef BOOST_NO_LONG_LONG
template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type lltrunc(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                           r(trunc(v, pol));
   if ((r > (std::numeric_limits<boost::long_long_type>::max)()) || r < (std::numeric_limits<boost::long_long_type>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::lltrunc<%1%>(%1%)", 0, number_type(v), 0LL, pol);
   return r.template convert_to<boost::long_long_type>();
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type lltrunc(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   return lltrunc(v, boost::math::policies::policy<>());
}
template <class T, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type lltrunc(const number<T, ExpressionTemplates>& v, const Policy& pol)
{
   number<T, ExpressionTemplates> r(trunc(v, pol));
   if ((r > (std::numeric_limits<boost::long_long_type>::max)()) || r < (std::numeric_limits<boost::long_long_type>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::lltrunc<%1%>(%1%)", 0, v, 0LL, pol);
   return r.template convert_to<boost::long_long_type>();
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type lltrunc(const number<T, ExpressionTemplates>& v)
{
   return lltrunc(v, boost::math::policies::policy<>());
}
#endif
template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<tag, A1, A2, A3, A4>::result_type round(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(round(static_cast<number_type>(v), pol));
}
template <class T, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR number<T, ExpressionTemplates> round(const number<T, ExpressionTemplates>& v, const Policy&)
{
   using default_ops::eval_round;
   detail::scoped_default_precision<multiprecision::number<T, ExpressionTemplates> > precision_guard(v);
   number<T, ExpressionTemplates>                                                    result;
   eval_round(result.backend(), v.backend());
   return result;
}

template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR int iround(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                           r(round(v, pol));
   if ((r > (std::numeric_limits<int>::max)()) || r < (std::numeric_limits<int>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::iround<%1%>(%1%)", 0, number_type(v), 0, pol);
   return r.template convert_to<int>();
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR int iround(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   return iround(v, boost::math::policies::policy<>());
}
template <class T, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR int iround(const number<T, ExpressionTemplates>& v, const Policy& pol)
{
   number<T, ExpressionTemplates> r(round(v, pol));
   if ((r > (std::numeric_limits<int>::max)()) || r < (std::numeric_limits<int>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::iround<%1%>(%1%)", 0, v, 0, pol);
   return r.template convert_to<int>();
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR int iround(const number<T, ExpressionTemplates>& v)
{
   return iround(v, boost::math::policies::policy<>());
}
template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR long lround(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                           r(round(v, pol));
   if ((r > (std::numeric_limits<long>::max)()) || r < (std::numeric_limits<long>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::lround<%1%>(%1%)", 0, number_type(v), 0L, pol);
   return r.template convert_to<long>();
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR long lround(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   return lround(v, boost::math::policies::policy<>());
}
template <class T, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR long lround(const number<T, ExpressionTemplates>& v, const Policy& pol)
{
   number<T, ExpressionTemplates> r(round(v, pol));
   if ((r > (std::numeric_limits<long>::max)()) || r < (std::numeric_limits<long>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::lround<%1%>(%1%)", 0, v, 0L, pol);
   return r.template convert_to<long>();
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR long lround(const number<T, ExpressionTemplates>& v)
{
   return lround(v, boost::math::policies::policy<>());
}
#ifndef BOOST_NO_LONG_LONG
template <class tag, class A1, class A2, class A3, class A4, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type llround(const detail::expression<tag, A1, A2, A3, A4>& v, const Policy& pol)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                           r(round(v, pol));
   if ((r > (std::numeric_limits<boost::long_long_type>::max)()) || r < (std::numeric_limits<boost::long_long_type>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::iround<%1%>(%1%)", 0, number_type(v), 0LL, pol);
   return r.template convert_to<boost::long_long_type>();
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type llround(const detail::expression<tag, A1, A2, A3, A4>& v)
{
   return llround(v, boost::math::policies::policy<>());
}
template <class T, expression_template_option ExpressionTemplates, class Policy>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type llround(const number<T, ExpressionTemplates>& v, const Policy& pol)
{
   number<T, ExpressionTemplates> r(round(v, pol));
   if ((r > (std::numeric_limits<boost::long_long_type>::max)()) || r < (std::numeric_limits<boost::long_long_type>::min)() || !(boost::math::isfinite)(v))
      return boost::math::policies::raise_rounding_error("boost::multiprecision::iround<%1%>(%1%)", 0, v, 0LL, pol);
   return r.template convert_to<boost::long_long_type>();
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR boost::long_long_type llround(const number<T, ExpressionTemplates>& v)
{
   return llround(v, boost::math::policies::policy<>());
}
#endif
//
// frexp does not return an expression template since we require the
// integer argument to be evaluated even if the returned value is
// not assigned to anything...
//
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_floating_point, number<T, ExpressionTemplates> >::type frexp(const number<T, ExpressionTemplates>& v, short* pint)
{
   using default_ops::eval_frexp;
   detail::scoped_default_precision<multiprecision::number<T, ExpressionTemplates> > precision_guard(v);
   number<T, ExpressionTemplates>                                                    result;
   eval_frexp(result.backend(), v.backend(), pint);
   return result;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_floating_point, typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type
frexp(const detail::expression<tag, A1, A2, A3, A4>& v, short* pint)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(frexp(static_cast<number_type>(v), pint));
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_floating_point, number<T, ExpressionTemplates> >::type frexp(const number<T, ExpressionTemplates>& v, int* pint)
{
   using default_ops::eval_frexp;
   detail::scoped_default_precision<multiprecision::number<T, ExpressionTemplates> > precision_guard(v);
   number<T, ExpressionTemplates>                                                    result;
   eval_frexp(result.backend(), v.backend(), pint);
   return result;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_floating_point, typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type
frexp(const detail::expression<tag, A1, A2, A3, A4>& v, int* pint)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(frexp(static_cast<number_type>(v), pint));
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_floating_point, number<T, ExpressionTemplates> >::type frexp(const number<T, ExpressionTemplates>& v, long* pint)
{
   using default_ops::eval_frexp;
   detail::scoped_default_precision<multiprecision::number<T, ExpressionTemplates> > precision_guard(v);
   number<T, ExpressionTemplates>                                                    result;
   eval_frexp(result.backend(), v.backend(), pint);
   return result;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_floating_point, typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type
frexp(const detail::expression<tag, A1, A2, A3, A4>& v, long* pint)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(frexp(static_cast<number_type>(v), pint));
}
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_floating_point, number<T, ExpressionTemplates> >::type frexp(const number<T, ExpressionTemplates>& v, boost::long_long_type* pint)
{
   using default_ops::eval_frexp;
   detail::scoped_default_precision<multiprecision::number<T, ExpressionTemplates> > precision_guard(v);
   number<T, ExpressionTemplates>                                                    result;
   eval_frexp(result.backend(), v.backend(), pint);
   return result;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_floating_point, typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type
frexp(const detail::expression<tag, A1, A2, A3, A4>& v, boost::long_long_type* pint)
{
   using number_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   return std::move(frexp(static_cast<number_type>(v), pint));
}
//
// modf does not return an expression template since we require the
// second argument to be evaluated even if the returned value is
// not assigned to anything...
//
template <class T, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_floating_point, number<T, ExpressionTemplates> >::type modf(const number<T, ExpressionTemplates>& v, number<T, ExpressionTemplates>* pipart)
{
   using default_ops::eval_modf;
   detail::scoped_default_precision<multiprecision::number<T, ExpressionTemplates> > precision_guard(v);
   number<T, ExpressionTemplates>                                                    result;
   eval_modf(result.backend(), v.backend(), pipart ? &pipart->backend() : 0);
   return result;
}
template <class T, expression_template_option ExpressionTemplates, class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<T>::value == number_kind_floating_point, number<T, ExpressionTemplates> >::type modf(const detail::expression<tag, A1, A2, A3, A4>& v, number<T, ExpressionTemplates>* pipart)
{
   using default_ops::eval_modf;
   detail::scoped_default_precision<multiprecision::number<T, ExpressionTemplates> > precision_guard(v);
   number<T, ExpressionTemplates>                                                    result, arg(v);
   eval_modf(result.backend(), arg.backend(), pipart ? &pipart->backend() : 0);
   return result;
}

//
// Integer square root:
//
template <class B, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer, number<B, ExpressionTemplates> >::type
sqrt(const number<B, ExpressionTemplates>& x)
{
   using default_ops::eval_integer_sqrt;
   number<B, ExpressionTemplates> s, r;
   eval_integer_sqrt(s.backend(), r.backend(), x.backend());
   return s;
}
template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_integer, typename detail::expression<tag, A1, A2, A3, A4>::result_type>::type 
         sqrt(const detail::expression<tag, A1, A2, A3, A4>& arg)
{
   using default_ops::eval_integer_sqrt;
   using result_type = typename detail::expression<tag, A1, A2, A3, A4>::result_type;
   detail::scoped_default_precision<result_type> precision_guard(arg);
   result_type                                   result, v(arg), r;
   eval_integer_sqrt(result.backend(), r.backend(), v.backend());
   return result;
}

//
// fma:
//

namespace default_ops {

struct fma_func
{
   template <class B, class T, class U, class V>
   BOOST_MP_CXX14_CONSTEXPR void operator()(B& result, const T& a, const U& b, const V& c) const
   {
      eval_multiply_add(result, a, b, c);
   }
};

} // namespace default_ops

template <class Backend, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<number<Backend, et_on> >::value == number_kind_floating_point) &&
        (is_number<U>::value || is_number_expression<U>::value || boost::multiprecision::detail::is_arithmetic<U>::value) &&
        (is_number<V>::value || is_number_expression<V>::value || boost::multiprecision::detail::is_arithmetic<V>::value),
    detail::expression<detail::function, default_ops::fma_func, number<Backend, et_on>, U, V> >::type
fma(const number<Backend, et_on>& a, const U& b, const V& c)
{
   return detail::expression<detail::function, default_ops::fma_func, number<Backend, et_on>, U, V>(
       default_ops::fma_func(), a, b, c);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_floating_point) &&
        (is_number<U>::value || is_number_expression<U>::value || boost::multiprecision::detail::is_arithmetic<U>::value) &&
        (is_number<V>::value || is_number_expression<V>::value || boost::multiprecision::detail::is_arithmetic<V>::value),
    detail::expression<detail::function, default_ops::fma_func, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, U, V> >::type
fma(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const U& b, const V& c)
{
   return detail::expression<detail::function, default_ops::fma_func, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, U, V>(
       default_ops::fma_func(), a, b, c);
}

template <class Backend, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<number<Backend, et_off> >::value == number_kind_floating_point) &&
        (is_number<U>::value || is_number_expression<U>::value || boost::multiprecision::detail::is_arithmetic<U>::value) &&
        (is_number<V>::value || is_number_expression<V>::value || boost::multiprecision::detail::is_arithmetic<V>::value),
    number<Backend, et_off> >::type
fma(const number<Backend, et_off>& a, const U& b, const V& c)
{
   using default_ops::eval_multiply_add;
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(a, b, c);
   number<Backend, et_off>                                                    result;
   eval_multiply_add(result.backend(), number<Backend, et_off>::canonical_value(a), number<Backend, et_off>::canonical_value(b), number<Backend, et_off>::canonical_value(c));
   return result;
}

template <class U, class Backend, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<number<Backend, et_on> >::value == number_kind_floating_point) &&
        boost::multiprecision::detail::is_arithmetic<U>::value &&
        (is_number<V>::value || is_number_expression<V>::value || boost::multiprecision::detail::is_arithmetic<V>::value),
    detail::expression<detail::function, default_ops::fma_func, U, number<Backend, et_on>, V> >::type
fma(const U& a, const number<Backend, et_on>& b, const V& c)
{
   return detail::expression<detail::function, default_ops::fma_func, U, number<Backend, et_on>, V>(
       default_ops::fma_func(), a, b, c);
}

template <class U, class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_floating_point) &&
        boost::multiprecision::detail::is_arithmetic<U>::value &&
        (is_number<V>::value || is_number_expression<V>::value || boost::multiprecision::detail::is_arithmetic<V>::value),
    detail::expression<detail::function, default_ops::fma_func, U, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
fma(const U& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b, const V& c)
{
   return detail::expression<detail::function, default_ops::fma_func, U, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(
       default_ops::fma_func(), a, b, c);
}

template <class U, class Backend, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<number<Backend, et_off> >::value == number_kind_floating_point) &&
        boost::multiprecision::detail::is_arithmetic<U>::value &&
        (is_number<V>::value || is_number_expression<V>::value || boost::multiprecision::detail::is_arithmetic<V>::value),
    number<Backend, et_off> >::type
fma(const U& a, const number<Backend, et_off>& b, const V& c)
{
   using default_ops::eval_multiply_add;
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(a, b, c);
   number<Backend, et_off>                                                    result;
   eval_multiply_add(result.backend(), number<Backend, et_off>::canonical_value(a), number<Backend, et_off>::canonical_value(b), number<Backend, et_off>::canonical_value(c));
   return result;
}

template <class U, class V, class Backend>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<number<Backend, et_on> >::value == number_kind_floating_point) &&
        boost::multiprecision::detail::is_arithmetic<U>::value &&
        boost::multiprecision::detail::is_arithmetic<V>::value,
    detail::expression<detail::function, default_ops::fma_func, U, V, number<Backend, et_on> > >::type
fma(const U& a, const V& b, const number<Backend, et_on>& c)
{
   return detail::expression<detail::function, default_ops::fma_func, U, V, number<Backend, et_on> >(
       default_ops::fma_func(), a, b, c);
}

template <class U, class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_floating_point) &&
        boost::multiprecision::detail::is_arithmetic<U>::value &&
        boost::multiprecision::detail::is_arithmetic<V>::value,
    detail::expression<detail::function, default_ops::fma_func, U, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
fma(const U& a, const V& b, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& c)
{
   return detail::expression<detail::function, default_ops::fma_func, U, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(
       default_ops::fma_func(), a, b, c);
}

template <class U, class V, class Backend>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<number<Backend, et_off> >::value == number_kind_floating_point) &&
        boost::multiprecision::detail::is_arithmetic<U>::value &&
        boost::multiprecision::detail::is_arithmetic<V>::value,
        number<Backend, et_off> >::type
fma(const U& a, const V& b, const number<Backend, et_off>& c)
{
   using default_ops::eval_multiply_add;
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(a, b, c);
   number<Backend, et_off>                                                    result;
   eval_multiply_add(result.backend(), number<Backend, et_off>::canonical_value(a), number<Backend, et_off>::canonical_value(b), number<Backend, et_off>::canonical_value(c));
   return result;
}

namespace default_ops {

struct remquo_func
{
   template <class B, class T, class U>
   BOOST_MP_CXX14_CONSTEXPR void operator()(B& result, const T& a, const U& b, int* pi) const
   {
      eval_remquo(result, a, b, pi);
   }
};

} // namespace default_ops

template <class Backend, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    number_category<number<Backend, et_on> >::value == number_kind_floating_point,
    detail::expression<detail::function, default_ops::remquo_func, number<Backend, et_on>, U, int*> >::type
remquo(const number<Backend, et_on>& a, const U& b, int* pi)
{
   return detail::expression<detail::function, default_ops::remquo_func, number<Backend, et_on>, U, int*>(
       default_ops::remquo_func(), a, b, pi);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_floating_point,
    detail::expression<detail::function, default_ops::remquo_func, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, U, int*> >::type
remquo(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const U& b, int* pi)
{
   return detail::expression<detail::function, default_ops::remquo_func, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, U, int*>(
       default_ops::remquo_func(), a, b, pi);
}

template <class U, class Backend>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    (number_category<number<Backend, et_on> >::value == number_kind_floating_point) && !is_number<U>::value && !is_number_expression<U>::value,
    detail::expression<detail::function, default_ops::remquo_func, U, number<Backend, et_on>, int*> >::type
remquo(const U& a, const number<Backend, et_on>& b, int* pi)
{
   return detail::expression<detail::function, default_ops::remquo_func, U, number<Backend, et_on>, int*>(
       default_ops::remquo_func(), a, b, pi);
}

template <class U, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_floating_point) && !is_number<U>::value && !is_number_expression<U>::value,
    detail::expression<detail::function, default_ops::remquo_func, U, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, int*> >::type
remquo(const U& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b, int* pi)
{
   return detail::expression<detail::function, default_ops::remquo_func, U, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, int*>(
       default_ops::remquo_func(), a, b, pi);
}

template <class Backend, class U>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    number_category<number<Backend, et_on> >::value == number_kind_floating_point,
    number<Backend, et_off> >::type
remquo(const number<Backend, et_off>& a, const U& b, int* pi)
{
   using default_ops::eval_remquo;
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(a, b);
   number<Backend, et_off>                                                    result;
   eval_remquo(result.backend(), a.backend(), number<Backend, et_off>::canonical_value(b), pi);
   return result;
}
template <class U, class Backend>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
    (number_category<number<Backend, et_on> >::value == number_kind_floating_point) && !is_number<U>::value && !is_number_expression<U>::value,
    number<Backend, et_off> >::type
remquo(const U& a, const number<Backend, et_off>& b, int* pi)
{
   using default_ops::eval_remquo;
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(a, b);
   number<Backend, et_off>                                                    result;
   eval_remquo(result.backend(), number<Backend, et_off>::canonical_value(a), b.backend(), pi);
   return result;
}

template <class B, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer, number<B, ExpressionTemplates> >::type
sqrt(const number<B, ExpressionTemplates>& x, number<B, ExpressionTemplates>& r)
{
   using default_ops::eval_integer_sqrt;
   detail::scoped_default_precision<multiprecision::number<B, ExpressionTemplates> > precision_guard(x, r);
   number<B, ExpressionTemplates>                                                    s;
   eval_integer_sqrt(s.backend(), r.backend(), x.backend());
   return s;
}
template <class B, expression_template_option ExpressionTemplates, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer, number<B, ExpressionTemplates> >::type
sqrt(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& arg, number<B, ExpressionTemplates>& r)
{
   using default_ops::eval_integer_sqrt;
   detail::scoped_default_precision<multiprecision::number<B, ExpressionTemplates> > precision_guard(r);
   number<B, ExpressionTemplates>                                                    s;
   number<B, ExpressionTemplates>                                                    x(arg);
   eval_integer_sqrt(s.backend(), r.backend(), x.backend());
   return s;
}

// clang-format off
//
// Regrettably, when the argument to a function is an rvalue we must return by value, and not return an 
// expression template, otherwise we can end up with dangling references.  
// See https://github.com/boostorg/multiprecision/issues/175.
//
#define UNARY_OP_FUNCTOR_CXX11_RVALUE(func, category)\
   template <class Backend>                                                                                                                                                                               \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == category, number<Backend, et_on> > ::type                                                                      \
   func(number<Backend, et_on>&& arg)                                                                                                                                                                     \
   {                                                                                                                                                                                                      \
      detail::scoped_default_precision<multiprecision::number<Backend, et_on> > precision_guard(arg);                                                                                                    \
      number<Backend, et_on>                                                    result;                                                                                                                  \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                         \
      BOOST_JOIN(eval_, func)(result.backend(), arg.backend());                                                                                                                                                                  \
      return result;                                                                                                                                                                       \
   }                                                                                                                                                                                                      \

#define BINARY_OP_FUNCTOR_CXX11_RVALUE(func, category)\
   template <class Backend>                                                                                                                                                                                                                                \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == category, number<Backend, et_on> >::type func(number<Backend, et_on>&& arg, const number<Backend, et_on>& a)                                                                                              \
   {                                                                                                                                                                                                                                                       \
      detail::scoped_default_precision<multiprecision::number<Backend, et_on> > precision_guard(arg, a);                                                                                                                                                  \
      number<Backend, et_on>                                                    result;                                                                                                                                                                   \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                          \
      BOOST_JOIN(eval_, func)(result.backend(), arg.backend(), a.backend());                                                                                                                                                                                                      \
      return result;                                                                                                                                                                                                                        \
   }                                                                                                                                                                                                                                                       \
   template <class Backend>                                                                                                                                                                                                                                \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == category, number<Backend, et_on> >::type func(const number<Backend, et_on>& arg, number<Backend, et_on>&& a)                                                                                              \
   {                                                                                                                                                                                                                                                       \
      detail::scoped_default_precision<multiprecision::number<Backend, et_on> > precision_guard(arg, a);                                                                                                                                                  \
      number<Backend, et_on>                                                    result;                                                                                                                                                                   \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                          \
      BOOST_JOIN(eval_, func)(result.backend(), arg.backend(), a.backend());                                                                                                                                                                                                      \
      return result;                                                                                                                                                                                                                        \
   }                                                                                                                                                                                                                                                       \
   template <class Backend>                                                                                                                                                                                                                                \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == category, number<Backend, et_on> >::type func(number<Backend, et_on>&& arg, number<Backend, et_on>&& a)                                                                                              \
   {                                                                                                                                                                                                                                                       \
      detail::scoped_default_precision<multiprecision::number<Backend, et_on> > precision_guard(arg, a);                                                                                                                                                  \
      number<Backend, et_on>                                                    result;                                                                                                                                                                   \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                          \
      BOOST_JOIN(eval_, func)(result.backend(), arg.backend(), a.backend());                                                                                                                                                                                                      \
      return result;                                                                                                                                                                                                                        \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class tag, class A1, class A2, class A3, class A4>                                                                                                                                                                             \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value == category) && (std::is_convertible<typename detail::expression<tag, A1, A2, A3, A4>::result_type, number<Backend, et_on> >::value),                           \
           number<Backend, et_on> > ::type                                                                                                                                                                       \
   func(number<Backend, et_on>&& arg, const detail::expression<tag, A1, A2, A3, A4>& a)                                                                                    \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                      \
             number<Backend, et_on>, detail::expression<tag, A1, A2, A3, A4> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>(), arg, a);                                                                                             \
   }                                                                                                                                                                                                                                                       \
   template <class tag, class A1, class A2, class A3, class A4, class Backend>                                                                                                                                                                             \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value == category) && (std::is_convertible<typename detail::expression<tag, A1, A2, A3, A4>::result_type, number<Backend, et_on> >::value),                           \
           number<Backend, et_on> > ::type                                                                                                                                                                       \
   func(const detail::expression<tag, A1, A2, A3, A4>& arg, number<Backend, et_on>&& a)                                                                                    \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                      \
             detail::expression<tag, A1, A2, A3, A4>, number<Backend, et_on> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>(), arg, a);                                                                                             \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class Arithmetic>                                                                                                                                                                                                              \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                        \
           is_compatible_arithmetic_type<Arithmetic, number<Backend, et_on> >::value && (number_category<Backend>::value == category),                                                                                                                     \
           number<Backend, et_on> >::type                                                                                                                                                                                                     \
   func(number<Backend, et_on>&& arg, const Arithmetic& a)                                                                                                                                               \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>,                                                                                                                                                      \
             number<Backend, et_on>, Arithmetic > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>(), arg, a);                                                                                                                         \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class Arithmetic>                                                                                                                                                                                                              \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                        \
           is_compatible_arithmetic_type<Arithmetic, number<Backend, et_on> >::value && (number_category<Backend>::value == category),                                                                                                                     \
           number<Backend, et_on> > ::type                                                                                                                                                                                                    \
   func(const Arithmetic& arg, number<Backend, et_on>&& a)                                                                                                                                              \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<                                                                                                                                                                                                                           \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                      \
             Arithmetic, number<Backend, et_on> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend > (), arg, a);                                                                                                                          \
   }                                                                                                                                                                                                                                                       \


#define UNARY_OP_FUNCTOR(func, category)                                                                                                                                                                  \
   namespace detail {                                                                                                                                                                                     \
   template <class Backend>                                                                                                                                                                               \
   struct BOOST_JOIN(category, BOOST_JOIN(func, _funct))                                                                                                                                                  \
   {                                                                                                                                                                                                      \
      BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, const Backend& arg) const                                                                                                                                          \
      {                                                                                                                                                                                                   \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                      \
         BOOST_JOIN(eval_, func)                                                                                                                                                                          \
         (result, arg);                                                                                                                                                                                   \
      }                                                                                                                                                                                                   \
      template <class U>                                                                                                                                                                                  \
      BOOST_MP_CXX14_CONSTEXPR void operator()(U& result, const Backend& arg) const                                                                                                                                                \
      {                                                                                                                                                                                                   \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                      \
         Backend temp;                                                                                                                                                                                    \
         BOOST_JOIN(eval_, func)                                                                                                                                                                          \
         (temp, arg);                                                                                                                                                                                     \
         result = std::move(temp);                                                                                                                                                                                   \
      }                                                                                                                                                                                                   \
   };                                                                                                                                                                                                     \
   }                                                                                                                                                                                                      \
                                                                                                                                                                                                          \
   template <class tag, class A1, class A2, class A3, class A4>                                                                                                                                           \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<detail::expression<tag, A1, A2, A3, A4> >::value == category,                                                                     \
                                                        detail::expression<detail::function,                                                                                                              \
                                                        detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,            \
                                                        detail::expression<tag, A1, A2, A3, A4> > > ::type                                                                                                \
   func(const detail::expression<tag, A1, A2, A3, A4>& arg)                                                                    \
   {                                                                                                                                                                                                      \
      return detail::expression<                                                                                                                                                                          \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                               \
             detail::expression<tag, A1, A2, A3, A4> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type > (), arg); \
   }                                                                                                                                                                                                      \
   template <class Backend>                                                                                                                                                                               \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == category,                                                                                                      \
          detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>, number<Backend, et_on> > > ::type                                                       \
   func(const number<Backend, et_on>& arg)                                                                                                                                                                \
   {                                                                                                                                                                                                      \
      return detail::expression<                                                                                                                                                                          \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                     \
             number<Backend, et_on> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend > (), arg);                                                                                        \
   }                                                                                                                                                                                                      \
   template <class Backend>                                                                                                                                                                               \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                    \
       boost::multiprecision::number_category<Backend>::value == category,                                                                                                                                \
       number<Backend, et_off> >::type                                                                                                                                                                    \
   func(const number<Backend, et_off>& arg)                                                                                                                                                               \
   {                                                                                                                                                                                                      \
      detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg);                                                                                                    \
      number<Backend, et_off>                                                    result;                                                                                                                  \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                         \
      BOOST_JOIN(eval_, func)(result.backend(), arg.backend());                                                                                                                                                                  \
      return result;                                                                                                                                                                       \
   }\
   UNARY_OP_FUNCTOR_CXX11_RVALUE(func, category)\

#define BINARY_OP_FUNCTOR(func, category)                                                                                                                                                                                                                  \
   namespace detail {                                                                                                                                                                                                                                      \
   template <class Backend>                                                                                                                                                                                                                                \
   struct BOOST_JOIN(category, BOOST_JOIN(func, _funct))                                                                                                                                                                                                   \
   {                                                                                                                                                                                                                                                       \
      BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, const Backend& arg, const Backend& a) const                                                                                                                                                                         \
      {                                                                                                                                                                                                                                                    \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                       \
         BOOST_JOIN(eval_, func)                                                                                                                                                                                                                           \
         (result, arg, a);                                                                                                                                                                                                                                 \
      }                                                                                                                                                                                                                                                    \
      template <class Arithmetic>                                                                                                                                                                                                                          \
      BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, const Backend& arg, const Arithmetic& a) const                                                                                                                                                                      \
      {                                                                                                                                                                                                                                                    \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                       \
         BOOST_JOIN(eval_, func)                                                                                                                                                                                                                           \
         (result, arg, number<Backend>::canonical_value(a));                                                                                                                                                                                               \
      }                                                                                                                                                                                                                                                    \
      template <class Arithmetic>                                                                                                                                                                                                                          \
      BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, const Arithmetic& arg, const Backend& a) const                                                                                                                                                                      \
      {                                                                                                                                                                                                                                                    \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                       \
         BOOST_JOIN(eval_, func)                                                                                                                                                                                                                           \
         (result, number<Backend>::canonical_value(arg), a);                                                                                                                                                                                               \
      }                                                                                                                                                                                                                                                    \
      template <class U>                                                                                                                                                                                                                                   \
      BOOST_MP_CXX14_CONSTEXPR void operator()(U& result, const Backend& arg, const Backend& a) const                                                                                                                                                                               \
      {                                                                                                                                                                                                                                                    \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                       \
         Backend r;                                                                                                                                                                                                                                        \
         BOOST_JOIN(eval_, func)                                                                                                                                                                                                                           \
         (r, arg, a);                                                                                                                                                                                                                                      \
         result = std::move(r);                                                                                                                                                                                                                                       \
      }                                                                                                                                                                                                                                                    \
      template <class U, class Arithmetic>                                                                                                                                                                                                                 \
      BOOST_MP_CXX14_CONSTEXPR void operator()(U& result, const Backend& arg, const Arithmetic& a) const                                                                                                                                                                            \
      {                                                                                                                                                                                                                                                    \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                       \
         Backend r;                                                                                                                                                                                                                                        \
         BOOST_JOIN(eval_, func)                                                                                                                                                                                                                           \
         (r, arg, number<Backend>::canonical_value(a));                                                                                                                                                                                                    \
         result = std::move(r);                                                                                                                                                                                                                                       \
      }                                                                                                                                                                                                                                                    \
      template <class U, class Arithmetic>                                                                                                                                                                                                                 \
      BOOST_MP_CXX14_CONSTEXPR void operator()(U& result, const Arithmetic& arg, const Backend& a) const                                                                                                                                                                            \
      {                                                                                                                                                                                                                                                    \
         using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                       \
         Backend r;                                                                                                                                                                                                                                        \
         BOOST_JOIN(eval_, func)                                                                                                                                                                                                                           \
         (r, number<Backend>::canonical_value(arg), a);                                                                                                                                                                                                    \
         result = std::move(r);                                                                                                                                                                                                                                       \
      }                                                                                                                                                                                                                                                    \
   };                                                                                                                                                                                                                                                      \
   }                                                                                                                                                                                                                                                       \
   template <class Backend>                                                                                                                                                                                                                                \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == category, detail::expression<detail::function, \
         detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>, number<Backend, et_on>, number<Backend, et_on> > > ::type                                                                                                                                                                \
   func(const number<Backend, et_on>& arg, const number<Backend, et_on>& a)                                                                                              \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                      \
             number<Backend, et_on>, number<Backend, et_on> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>(), arg, a);                                                                                                              \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class tag, class A1, class A2, class A3, class A4>                                                                                                                                                                             \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value == category) && (std::is_convertible<typename detail::expression<tag, A1, A2, A3, A4>::result_type, number<Backend, et_on> >::value),                           \
           detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>, number<Backend, et_on>, detail::expression<tag, A1, A2, A3, A4> > > ::type                                                                                                                                                                       \
   func(const number<Backend, et_on>& arg, const detail::expression<tag, A1, A2, A3, A4>& a)                                                                                    \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                      \
             number<Backend, et_on>, detail::expression<tag, A1, A2, A3, A4> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>(), arg, a);                                                                                             \
   }                                                                                                                                                                                                                                                       \
   template <class tag, class A1, class A2, class A3, class A4, class Backend>                                                                                                                                                                             \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value == category) && (std::is_convertible<typename detail::expression<tag, A1, A2, A3, A4>::result_type, number<Backend, et_on> >::value),                           \
           detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>, detail::expression<tag, A1, A2, A3, A4>, number<Backend, et_on> > > ::type                                                                                                                                                                       \
   func(const detail::expression<tag, A1, A2, A3, A4>& arg, const number<Backend, et_on>& a)                                                                                    \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                      \
             detail::expression<tag, A1, A2, A3, A4>, number<Backend, et_on> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>(), arg, a);                                                                                             \
   }                                                                                                                                                                                                                                                       \
   template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b>                                                                                                                                    \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<detail::expression<tag, A1, A2, A3, A4> >::value == category) && (number_category<detail::expression<tagb, A1b, A2b, A3b, A4b> >::value == category),                                                                          \
           detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                                                                  \
           detail::expression<tag, A1, A2, A3, A4>, detail::expression<tagb, A1b, A2b, A3b, A4b> > > ::type                                                                                                                                                 \
   func(const detail::expression<tag, A1, A2, A3, A4>& arg, const detail::expression<tagb, A1b, A2b, A3b, A4b>& a)                                        \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                                                                \
             detail::expression<tag, A1, A2, A3, A4>, detail::expression<tagb, A1b, A2b, A3b, A4b> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>(), arg, a); \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class Arithmetic>                                                                                                                                                                                                              \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                        \
           is_compatible_arithmetic_type<Arithmetic, number<Backend, et_on> >::value && (number_category<Backend>::value == category),                                                                                                                     \
           detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                        \
           number<Backend, et_on>, Arithmetic> > ::type                                                                                                                                                                                                     \
   func(const number<Backend, et_on>& arg, const Arithmetic& a)                                                                                                                                               \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>,                                                                                                                                                      \
             number<Backend, et_on>, Arithmetic > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct))<Backend>(), arg, a);                                                                                                                         \
   }                                                                                                                                                                                                                                                       \
   template <class tag, class A1, class A2, class A3, class A4, class Arithmetic>                                                                                                                                                                          \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                        \
           is_compatible_arithmetic_type<Arithmetic, typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value && (number_category<detail::expression<tag, A1, A2, A3, A4> >::value == category),                                              \
           detail::expression<                                                                                                                                                                                                                             \
               detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                                                                  \
           detail::expression<tag, A1, A2, A3, A4>, Arithmetic> > ::type                                                                                                                                                                                    \
   func(const detail::expression<tag, A1, A2, A3, A4>& arg, const Arithmetic& a)                                                                                                             \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<                                                                                                                                                                                                                           \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                                                                \
             detail::expression<tag, A1, A2, A3, A4>, Arithmetic > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type > (), arg, a);                                  \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class Arithmetic>                                                                                                                                                                                                              \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                        \
           is_compatible_arithmetic_type<Arithmetic, number<Backend, et_on> >::value && (number_category<Backend>::value == category),                                                                                                                     \
           detail::expression<                                                                                                                                                                                                                             \
               detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                        \
           Arithmetic, number<Backend, et_on> > > ::type                                                                                                                                                                                                    \
   func(const Arithmetic& arg, const number<Backend, et_on>& a)                                                                                                                                              \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<                                                                                                                                                                                                                           \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                                                      \
             Arithmetic, number<Backend, et_on> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend > (), arg, a);                                                                                                                          \
   }                                                                                                                                                                                                                                                       \
   template <class tag, class A1, class A2, class A3, class A4, class Arithmetic>                                                                                                                                                                          \
       inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                        \
           is_compatible_arithmetic_type<Arithmetic, typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value && (number_category<detail::expression<tag, A1, A2, A3, A4> >::value == category),                                              \
           detail::expression<                                                                                                                                                                                                                             \
               detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                                                                  \
           Arithmetic, detail::expression<tag, A1, A2, A3, A4> > > ::type                                                                                                                                                                                   \
   func(const Arithmetic& arg, const detail::expression<tag, A1, A2, A3, A4>& a)                                                                                                            \
   {                                                                                                                                                                                                                                                       \
      return detail::expression<                                                                                                                                                                                                                           \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                                                                \
             Arithmetic, detail::expression<tag, A1, A2, A3, A4> > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type > (), arg, a);                                   \
   }                                                                                                                                                                                                                                                       \
   template <class Backend>                                                                                                                                                                                                                                \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value == category), number<Backend, et_off> >::type                                                                                                                                                                                             \
   func(const number<Backend, et_off>& arg, const number<Backend, et_off>& a)                                                                                                                                                                              \
   {                                                                                                                                                                                                                                                       \
      detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg, a);                                                                                                                                                  \
      number<Backend, et_off>                                                    result;                                                                                                                                                                   \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                          \
      BOOST_JOIN(eval_, func)(result.backend(), arg.backend(), a.backend());                                                                                                                                                                                                      \
      return result;                                                                                                                                                                                                                        \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class Arithmetic>                                                                                                                                                                                                              \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                            \
       is_compatible_arithmetic_type<Arithmetic, number<Backend, et_off> >::value && (number_category<Backend>::value == category),                                                                                                                        \
       number<Backend, et_off> >::type                                                                                                                                                                                                                     \
   func(const number<Backend, et_off>& arg, const Arithmetic& a)                                                                                                                                                                                           \
   {                                                                                                                                                                                                                                                       \
      detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg);                                                                                                                                                     \
      number<Backend, et_off>                                                    result;                                                                                                                                                                   \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                          \
      BOOST_JOIN(eval_, func)                                                                                                                                                                                                                              \
      (result.backend(), arg.backend(), number<Backend, et_off>::canonical_value(a));                                                                                                                                                                      \
      return result;                                                                                                                                                                                                                        \
   }                                                                                                                                                                                                                                                       \
   template <class Backend, class Arithmetic>                                                                                                                                                                                                              \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                                                            \
       is_compatible_arithmetic_type<Arithmetic, number<Backend, et_off> >::value && (number_category<Backend>::value == category),                                                                                                                        \
       number<Backend, et_off> >::type                                                                                                                                                                                                                     \
   func(const Arithmetic& a, const number<Backend, et_off>& arg)                                                                                                                                                                                           \
   {                                                                                                                                                                                                                                                       \
      detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg);                                                                                                                                                     \
      number<Backend, et_off>                                                    result;                                                                                                                                                                   \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                                                          \
      BOOST_JOIN(eval_, func)                                                                                                                                                                                                                              \
      (result.backend(), number<Backend, et_off>::canonical_value(a), arg.backend());                                                                                                                                                                      \
      return result;                                                                                                                                                                                                                        \
   }\
   BINARY_OP_FUNCTOR_CXX11_RVALUE(func, category)

#define HETERO_BINARY_OP_FUNCTOR_B(func, Arg2, category)                                                                                                                                                            \
   template <class tag, class A1, class A2, class A3, class A4>                                                                                                                                                     \
       inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                 \
           (number_category<detail::expression<tag, A1, A2, A3, A4> >::value == category),                                                                                                                          \
           detail::expression<                                                                                                                                                                                      \
               detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                           \
           detail::expression<tag, A1, A2, A3, A4>, Arg2> > ::type                                                                                                                                                   \
                                                           func(const detail::expression<tag, A1, A2, A3, A4>& arg, Arg2 const& a)                                                                                  \
   {                                                                                                                                                                                                                \
      return detail::expression<                                                                                                                                                                                    \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>,                                         \
             detail::expression<tag, A1, A2, A3, A4>, Arg2 > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type > (), arg, a); \
   }                                                                                                                                                                                                                \
   template <class Backend>                                                                                                                                                                                         \
       inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                 \
           (number_category<Backend>::value == category),                                                                                                                                                           \
           detail::expression<                                                                                                                                                                                      \
               detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                                 \
           number<Backend, et_on>, Arg2> > ::type                                                                                                                                                                    \
                                          func(const number<Backend, et_on>& arg, Arg2 const& a)                                                                                                                    \
   {                                                                                                                                                                                                                \
      return detail::expression<                                                                                                                                                                                    \
                 detail::function, detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend>,                                                                                                               \
             number<Backend, et_on>, Arg2 > (detail::BOOST_JOIN(category, BOOST_JOIN(func, _funct)) < Backend > (), arg, a);                                                                                        \
   }                                                                                                                                                                                                                \
   template <class Backend>                                                                                                                                                                                         \
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<                                                                                                                                                                                     \
       (number_category<Backend>::value == category),                                                                                                                                                               \
       number<Backend, et_off> >::type                                                                                                                                                                              \
   func(const number<Backend, et_off>& arg, Arg2 const& a)                                                                                                                                                          \
   {                                                                                                                                                                                                                \
      detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg, a);                                                                                                           \
      number<Backend, et_off>                                                    result;                                                                                                                            \
      using default_ops::BOOST_JOIN(eval_, func);                                                                                                                                                                   \
      BOOST_JOIN(eval_, func)                                                                                                                                                                                       \
      (result.backend(), arg.backend(), a);                                                                                                                                                                         \
      return result;                                                                                                                                                                                 \
   }

#define HETERO_BINARY_OP_FUNCTOR(func, Arg2, category)                  \
   namespace detail {                                                   \
   template <class Backend>                                             \
   struct BOOST_JOIN(category, BOOST_JOIN(func, _funct))                \
   {                                                                    \
      template <class Arg>                                              \
      BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, Backend const& arg, Arg a) const \
      {                                                                 \
         using default_ops::BOOST_JOIN(eval_, func);                    \
         BOOST_JOIN(eval_, func)                                        \
         (result, arg, a);                                              \
      }                                                                 \
      template <class U, class Arg>                                              \
      BOOST_MP_CXX14_CONSTEXPR void operator()(U& result, Backend const& arg, Arg a) const \
      {                                                                 \
         using default_ops::BOOST_JOIN(eval_, func);                    \
         Backend temp;                                                  \
         BOOST_JOIN(eval_, func)                                        \
         (temp, arg, a);                                                \
         result = std::move(temp);                                  \
      }                                                                 \
   };                                                                   \
   }                                                                    \
                                                                        \
   HETERO_BINARY_OP_FUNCTOR_B(func, Arg2, category)

// clang-format on

namespace detail {
template <class Backend>
struct abs_funct
{
   BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, const Backend& arg) const
   {
      using default_ops::eval_abs;
      eval_abs(result, arg);
   }
};
template <class Backend>
struct conj_funct
{
   BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, const Backend& arg) const
   {
      using default_ops::eval_conj;
      eval_conj(result, arg);
   }
};
template <class Backend>
struct proj_funct
{
   BOOST_MP_CXX14_CONSTEXPR void operator()(Backend& result, const Backend& arg) const
   {
      using default_ops::eval_proj;
      eval_proj(result, arg);
   }
};

} // namespace detail

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex,
                                    detail::expression<
                                        detail::function, detail::abs_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>, detail::expression<tag, A1, A2, A3, A4> > >::type
abs(const detail::expression<tag, A1, A2, A3, A4>& arg)
{
   return detail::expression<
       detail::function, detail::abs_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>, detail::expression<tag, A1, A2, A3, A4> >(
       detail::abs_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>(), arg);
}
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value != number_kind_complex,
                             detail::expression<
                                 detail::function, detail::abs_funct<Backend>, number<Backend, et_on> > >::type
abs(const number<Backend, et_on>& arg)
{
   return detail::expression<
       detail::function, detail::abs_funct<Backend>, number<Backend, et_on> >(
       detail::abs_funct<Backend>(), arg);
}
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value != number_kind_complex, number<Backend, et_off> >::type
abs(const number<Backend, et_off>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg);
   number<Backend, et_off>                                                    result;
   using default_ops::eval_abs;
   eval_abs(result.backend(), arg.backend());
   return result;
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<
    detail::function, detail::conj_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>, detail::expression<tag, A1, A2, A3, A4> >
conj(const detail::expression<tag, A1, A2, A3, A4>& arg)
{
   return detail::expression<
       detail::function, detail::conj_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>, detail::expression<tag, A1, A2, A3, A4> >(
       detail::conj_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>(), arg);
}
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<
    detail::function, detail::conj_funct<Backend>, number<Backend, et_on> >
conj(const number<Backend, et_on>& arg)
{
   return detail::expression<
       detail::function, detail::conj_funct<Backend>, number<Backend, et_on> >(
       detail::conj_funct<Backend>(), arg);
}
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR number<Backend, et_off>
conj(const number<Backend, et_off>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg);
   number<Backend, et_off>                                                    result;
   using default_ops::eval_conj;
   eval_conj(result.backend(), arg.backend());
   return result;
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<
    detail::function, detail::proj_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>, detail::expression<tag, A1, A2, A3, A4> >
proj(const detail::expression<tag, A1, A2, A3, A4>& arg)
{
   return detail::expression<
       detail::function, detail::proj_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>, detail::expression<tag, A1, A2, A3, A4> >(
       detail::proj_funct<typename detail::backend_type<detail::expression<tag, A1, A2, A3, A4> >::type>(), arg);
}
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<
    detail::function, detail::proj_funct<Backend>, number<Backend, et_on> >
proj(const number<Backend, et_on>& arg)
{
   return detail::expression<
       detail::function, detail::proj_funct<Backend>, number<Backend, et_on> >(
       detail::proj_funct<Backend>(), arg);
}
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR number<Backend, et_off>
proj(const number<Backend, et_off>& arg)
{
   detail::scoped_default_precision<multiprecision::number<Backend, et_off> > precision_guard(arg);
   number<Backend, et_off>                                                    result;
   using default_ops::eval_proj;
   eval_proj(result.backend(), arg.backend());
   return result;
}

UNARY_OP_FUNCTOR(fabs, number_kind_floating_point)
UNARY_OP_FUNCTOR(sqrt, number_kind_floating_point)
UNARY_OP_FUNCTOR(floor, number_kind_floating_point)
UNARY_OP_FUNCTOR(ceil, number_kind_floating_point)
UNARY_OP_FUNCTOR(trunc, number_kind_floating_point)
UNARY_OP_FUNCTOR(round, number_kind_floating_point)
UNARY_OP_FUNCTOR(exp, number_kind_floating_point)
UNARY_OP_FUNCTOR(exp2, number_kind_floating_point)
UNARY_OP_FUNCTOR(log, number_kind_floating_point)
UNARY_OP_FUNCTOR(log10, number_kind_floating_point)
UNARY_OP_FUNCTOR(cos, number_kind_floating_point)
UNARY_OP_FUNCTOR(sin, number_kind_floating_point)
UNARY_OP_FUNCTOR(tan, number_kind_floating_point)
UNARY_OP_FUNCTOR(asin, number_kind_floating_point)
UNARY_OP_FUNCTOR(acos, number_kind_floating_point)
UNARY_OP_FUNCTOR(atan, number_kind_floating_point)
UNARY_OP_FUNCTOR(cosh, number_kind_floating_point)
UNARY_OP_FUNCTOR(sinh, number_kind_floating_point)
UNARY_OP_FUNCTOR(tanh, number_kind_floating_point)
UNARY_OP_FUNCTOR(log2, number_kind_floating_point)
UNARY_OP_FUNCTOR(nearbyint, number_kind_floating_point)
UNARY_OP_FUNCTOR(rint, number_kind_floating_point)

HETERO_BINARY_OP_FUNCTOR(ldexp, short, number_kind_floating_point)
//HETERO_BINARY_OP_FUNCTOR(frexp, short*, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(ldexp, int, number_kind_floating_point)
//HETERO_BINARY_OP_FUNCTOR_B(frexp, int*, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(ldexp, long, number_kind_floating_point)
//HETERO_BINARY_OP_FUNCTOR_B(frexp, long*, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(ldexp, boost::long_long_type, number_kind_floating_point)
//HETERO_BINARY_OP_FUNCTOR_B(frexp, boost::long_long_type*, number_kind_floating_point)
BINARY_OP_FUNCTOR(pow, number_kind_floating_point)
BINARY_OP_FUNCTOR(fmod, number_kind_floating_point)
BINARY_OP_FUNCTOR(fmax, number_kind_floating_point)
BINARY_OP_FUNCTOR(fmin, number_kind_floating_point)
BINARY_OP_FUNCTOR(atan2, number_kind_floating_point)
BINARY_OP_FUNCTOR(fdim, number_kind_floating_point)
BINARY_OP_FUNCTOR(hypot, number_kind_floating_point)
BINARY_OP_FUNCTOR(remainder, number_kind_floating_point)

UNARY_OP_FUNCTOR(logb, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR(scalbn, short, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR(scalbln, short, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(scalbn, int, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(scalbln, int, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(scalbn, long, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(scalbln, long, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(scalbn, boost::long_long_type, number_kind_floating_point)
HETERO_BINARY_OP_FUNCTOR_B(scalbln, boost::long_long_type, number_kind_floating_point)

//
// Complex functions:
//
UNARY_OP_FUNCTOR(exp, number_kind_complex)
UNARY_OP_FUNCTOR(log, number_kind_complex)
UNARY_OP_FUNCTOR(log10, number_kind_complex)
BINARY_OP_FUNCTOR(pow, number_kind_complex)
UNARY_OP_FUNCTOR(sqrt, number_kind_complex)
UNARY_OP_FUNCTOR(sin, number_kind_complex)
UNARY_OP_FUNCTOR(cos, number_kind_complex)
UNARY_OP_FUNCTOR(tan, number_kind_complex)
UNARY_OP_FUNCTOR(asin, number_kind_complex)
UNARY_OP_FUNCTOR(acos, number_kind_complex)
UNARY_OP_FUNCTOR(atan, number_kind_complex)
UNARY_OP_FUNCTOR(sinh, number_kind_complex)
UNARY_OP_FUNCTOR(cosh, number_kind_complex)
UNARY_OP_FUNCTOR(tanh, number_kind_complex)
UNARY_OP_FUNCTOR(asinh, number_kind_complex)
UNARY_OP_FUNCTOR(acosh, number_kind_complex)
UNARY_OP_FUNCTOR(atanh, number_kind_complex)

//
// Integer functions:
//
BINARY_OP_FUNCTOR(gcd, number_kind_integer)
BINARY_OP_FUNCTOR(lcm, number_kind_integer)
HETERO_BINARY_OP_FUNCTOR(pow, unsigned, number_kind_integer)

#undef BINARY_OP_FUNCTOR
#undef UNARY_OP_FUNCTOR

//
// ilogb:
//
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_floating_point, typename Backend::exponent_type>::type
ilogb(const multiprecision::number<Backend, ExpressionTemplates>& val)
{
   using default_ops::eval_ilogb;
   return eval_ilogb(val.backend());
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<detail::expression<tag, A1, A2, A3, A4> >::value == number_kind_floating_point, typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type::backend_type::exponent_type>::type
ilogb(const detail::expression<tag, A1, A2, A3, A4>& val)
{
   using default_ops::eval_ilogb;
   typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type arg(val);
   return eval_ilogb(arg.backend());
}

} //namespace multiprecision

namespace math {
//
// Overload of Boost.Math functions that find the wrong overload when used with number:
//
namespace detail {
template <class T>
T sinc_pi_imp(T);
template <class T>
T sinhc_pi_imp(T);
} // namespace detail
template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline multiprecision::number<Backend, ExpressionTemplates> sinc_pi(const multiprecision::number<Backend, ExpressionTemplates>& x)
{
   boost::multiprecision::detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(x);
   return std::move(detail::sinc_pi_imp(x));
}

template <class Backend, multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline multiprecision::number<Backend, ExpressionTemplates> sinc_pi(const multiprecision::number<Backend, ExpressionTemplates>& x, const Policy&)
{
   boost::multiprecision::detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(x);
   return std::move(detail::sinc_pi_imp(x));
}

template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline multiprecision::number<Backend, ExpressionTemplates> sinhc_pi(const multiprecision::number<Backend, ExpressionTemplates>& x)
{
   boost::multiprecision::detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(x);
   return std::move(detail::sinhc_pi_imp(x));
}

template <class Backend, multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline multiprecision::number<Backend, ExpressionTemplates> sinhc_pi(const multiprecision::number<Backend, ExpressionTemplates>& x, const Policy&)
{
   boost::multiprecision::detail::scoped_default_precision<multiprecision::number<Backend, ExpressionTemplates> > precision_guard(x);
   return std::move(boost::math::sinhc_pi(x));
}

using boost::multiprecision::gcd;
using boost::multiprecision::lcm;

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
} // namespace math

namespace integer {

using boost::multiprecision::gcd;
using boost::multiprecision::lcm;

} // namespace integer

} // namespace boost

//
// This has to come last of all:
//
#include <boost/multiprecision/detail/no_et_ops.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
//
// min/max overloads:
//
#include <boost/multiprecision/detail/min_max.hpp>

#endif
