///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MP_INT_FUNC_HPP
#define BOOST_MP_INT_FUNC_HPP

#include <boost/multiprecision/number.hpp>

namespace boost { namespace multiprecision {

namespace default_ops {

template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR void eval_qr(const Backend& x, const Backend& y, Backend& q, Backend& r)
{
   eval_divide(q, x, y);
   eval_modulus(r, x, y);
}

template <class Backend, class Integer>
inline BOOST_MP_CXX14_CONSTEXPR Integer eval_integer_modulus(const Backend& x, Integer val)
{
   BOOST_MP_USING_ABS
   using default_ops::eval_convert_to;
   using default_ops::eval_modulus;
   using int_type = typename boost::multiprecision::detail::canonical<Integer, Backend>::type;
   Backend                                                                           t;
   eval_modulus(t, x, static_cast<int_type>(val));
   Integer result(0);
   eval_convert_to(&result, t);
   return abs(result);
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_gcd(B& result, const B& a, const B& b)
{
   using default_ops::eval_get_sign;
   using default_ops::eval_is_zero;
   using default_ops::eval_lsb;

   int shift(0);

   B u(a), v(b);

   int s = eval_get_sign(u);

   /* GCD(0,x) := x */
   if (s < 0)
   {
      u.negate();
   }
   else if (s == 0)
   {
      result = v;
      return;
   }
   s = eval_get_sign(v);
   if (s < 0)
   {
      v.negate();
   }
   else if (s == 0)
   {
      result = u;
      return;
   }

   /* Let shift := lg K, where K is the greatest power of 2
   dividing both u and v. */

   unsigned us = eval_lsb(u);
   unsigned vs = eval_lsb(v);
   shift       = (std::min)(us, vs);
   eval_right_shift(u, us);
   eval_right_shift(v, vs);

   do
   {
      /* Now u and v are both odd, so diff(u, v) is even.
      Let u = min(u, v), v = diff(u, v)/2. */
      s = u.compare(v);
      if (s > 0)
         u.swap(v);
      if (s == 0)
         break;
      eval_subtract(v, u);
      vs = eval_lsb(v);
      eval_right_shift(v, vs);
   } while (true);

   result = u;
   eval_left_shift(result, shift);
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR void eval_lcm(B& result, const B& a, const B& b)
{
   using ui_type = typename std::tuple_element<0, typename B::unsigned_types>::type;
   B                                                             t;
   eval_gcd(t, a, b);

   if (eval_is_zero(t))
   {
      result = static_cast<ui_type>(0);
   }
   else
   {
      eval_divide(result, a, t);
      eval_multiply(result, b);
   }
   if (eval_get_sign(result) < 0)
      result.negate();
}

} // namespace default_ops

template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer>::type
divide_qr(const number<Backend, ExpressionTemplates>& x, const number<Backend, ExpressionTemplates>& y,
          number<Backend, ExpressionTemplates>& q, number<Backend, ExpressionTemplates>& r)
{
   using default_ops::eval_qr;
   eval_qr(x.backend(), y.backend(), q.backend(), r.backend());
}

template <class Backend, expression_template_option ExpressionTemplates, class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer>::type
divide_qr(const number<Backend, ExpressionTemplates>& x, const multiprecision::detail::expression<tag, A1, A2, A3, A4>& y,
          number<Backend, ExpressionTemplates>& q, number<Backend, ExpressionTemplates>& r)
{
   divide_qr(x, number<Backend, ExpressionTemplates>(y), q, r);
}

template <class tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer>::type
divide_qr(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& x, const number<Backend, ExpressionTemplates>& y,
          number<Backend, ExpressionTemplates>& q, number<Backend, ExpressionTemplates>& r)
{
   divide_qr(number<Backend, ExpressionTemplates>(x), y, q, r);
}

template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer>::type
divide_qr(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& x, const multiprecision::detail::expression<tagb, A1b, A2b, A3b, A4b>& y,
          number<Backend, ExpressionTemplates>& q, number<Backend, ExpressionTemplates>& r)
{
   divide_qr(number<Backend, ExpressionTemplates>(x), number<Backend, ExpressionTemplates>(y), q, r);
}

template <class Backend, expression_template_option ExpressionTemplates, class Integer>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<Integer>::value && (number_category<Backend>::value == number_kind_integer), Integer>::type
integer_modulus(const number<Backend, ExpressionTemplates>& x, Integer val)
{
   using default_ops::eval_integer_modulus;
   return eval_integer_modulus(x.backend(), val);
}

template <class tag, class A1, class A2, class A3, class A4, class Integer>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<Integer>::value && (number_category<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_integer), Integer>::type
integer_modulus(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& x, Integer val)
{
   using result_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   return integer_modulus(result_type(x), val);
}

template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer, unsigned>::type
lsb(const number<Backend, ExpressionTemplates>& x)
{
   using default_ops::eval_lsb;
   return eval_lsb(x.backend());
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_integer, unsigned>::type
lsb(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& x)
{
   using number_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                                           n(x);
   using default_ops::eval_lsb;
   return eval_lsb(n.backend());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer, unsigned>::type
msb(const number<Backend, ExpressionTemplates>& x)
{
   using default_ops::eval_msb;
   return eval_msb(x.backend());
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_integer, unsigned>::type
msb(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& x)
{
   using number_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                                           n(x);
   using default_ops::eval_msb;
   return eval_msb(n.backend());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer, bool>::type
bit_test(const number<Backend, ExpressionTemplates>& x, unsigned index)
{
   using default_ops::eval_bit_test;
   return eval_bit_test(x.backend(), index);
}

template <class tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type>::value == number_kind_integer, bool>::type
bit_test(const multiprecision::detail::expression<tag, A1, A2, A3, A4>& x, unsigned index)
{
   using number_type = typename multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
   number_type                                                                           n(x);
   using default_ops::eval_bit_test;
   return eval_bit_test(n.backend(), index);
}

template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer, number<Backend, ExpressionTemplates>&>::type
bit_set(number<Backend, ExpressionTemplates>& x, unsigned index)
{
   using default_ops::eval_bit_set;
   eval_bit_set(x.backend(), index);
   return x;
}

template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer, number<Backend, ExpressionTemplates>&>::type
bit_unset(number<Backend, ExpressionTemplates>& x, unsigned index)
{
   using default_ops::eval_bit_unset;
   eval_bit_unset(x.backend(), index);
   return x;
}

template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<Backend>::value == number_kind_integer, number<Backend, ExpressionTemplates>&>::type
bit_flip(number<Backend, ExpressionTemplates>& x, unsigned index)
{
   using default_ops::eval_bit_flip;
   eval_bit_flip(x.backend(), index);
   return x;
}

namespace default_ops {

//
// Within powm, we need a type with twice as many digits as the argument type, define
// a traits class to obtain that type:
//
template <class Backend>
struct double_precision_type
{
   using type = Backend;
};

//
// If the exponent is a signed integer type, then we need to
// check the value is positive:
//
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR void check_sign_of_backend(const Backend& v, const std::integral_constant<bool, true>)
{
   if (eval_get_sign(v) < 0)
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("powm requires a positive exponent."));
   }
}
template <class Backend>
inline BOOST_MP_CXX14_CONSTEXPR void check_sign_of_backend(const Backend&, const std::integral_constant<bool, false>) {}
//
// Calculate (a^p)%c:
//
template <class Backend>
BOOST_MP_CXX14_CONSTEXPR void eval_powm(Backend& result, const Backend& a, const Backend& p, const Backend& c)
{
   using default_ops::eval_bit_test;
   using default_ops::eval_get_sign;
   using default_ops::eval_modulus;
   using default_ops::eval_multiply;
   using default_ops::eval_right_shift;

   using double_type = typename double_precision_type<Backend>::type                                      ;
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned char, double_type>::type;

   check_sign_of_backend(p, std::integral_constant<bool, std::numeric_limits<number<Backend> >::is_signed>());

   double_type x, y(a), b(p), t;
   x = ui_type(1u);

   while (eval_get_sign(b) > 0)
   {
      if (eval_bit_test(b, 0))
      {
         eval_multiply(t, x, y);
         eval_modulus(x, t, c);
      }
      eval_multiply(t, y, y);
      eval_modulus(y, t, c);
      eval_right_shift(b, ui_type(1));
   }
   Backend x2(x);
   eval_modulus(result, x2, c);
}

template <class Backend, class Integer>
BOOST_MP_CXX14_CONSTEXPR void eval_powm(Backend& result, const Backend& a, const Backend& p, Integer c)
{
   using double_type = typename double_precision_type<Backend>::type                                      ;
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned char, double_type>::type;
   using i1_type = typename boost::multiprecision::detail::canonical<Integer, double_type>::type      ;
   using i2_type = typename boost::multiprecision::detail::canonical<Integer, Backend>::type          ;

   using default_ops::eval_bit_test;
   using default_ops::eval_get_sign;
   using default_ops::eval_modulus;
   using default_ops::eval_multiply;
   using default_ops::eval_right_shift;

   check_sign_of_backend(p, std::integral_constant<bool, std::numeric_limits<number<Backend> >::is_signed>());

   if (eval_get_sign(p) < 0)
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("powm requires a positive exponent."));
   }

   double_type x, y(a), b(p), t;
   x = ui_type(1u);

   while (eval_get_sign(b) > 0)
   {
      if (eval_bit_test(b, 0))
      {
         eval_multiply(t, x, y);
         eval_modulus(x, t, static_cast<i1_type>(c));
      }
      eval_multiply(t, y, y);
      eval_modulus(y, t, static_cast<i1_type>(c));
      eval_right_shift(b, ui_type(1));
   }
   Backend x2(x);
   eval_modulus(result, x2, static_cast<i2_type>(c));
}

template <class Backend, class Integer>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_unsigned<Integer>::value >::type eval_powm(Backend& result, const Backend& a, Integer b, const Backend& c)
{
   using double_type = typename double_precision_type<Backend>::type                                      ;
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned char, double_type>::type;

   using default_ops::eval_bit_test;
   using default_ops::eval_get_sign;
   using default_ops::eval_modulus;
   using default_ops::eval_multiply;
   using default_ops::eval_right_shift;

   double_type x, y(a), t;
   x = ui_type(1u);

   while (b > 0)
   {
      if (b & 1)
      {
         eval_multiply(t, x, y);
         eval_modulus(x, t, c);
      }
      eval_multiply(t, y, y);
      eval_modulus(y, t, c);
      b >>= 1;
   }
   Backend x2(x);
   eval_modulus(result, x2, c);
}

template <class Backend, class Integer>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_signed<Integer>::value && boost::multiprecision::detail::is_integral<Integer>::value>::type eval_powm(Backend& result, const Backend& a, Integer b, const Backend& c)
{
   if (b < 0)
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("powm requires a positive exponent."));
   }
   eval_powm(result, a, static_cast<typename boost::multiprecision::detail::make_unsigned<Integer>::type>(b), c);
}

template <class Backend, class Integer1, class Integer2>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_unsigned<Integer1>::value >::type eval_powm(Backend& result, const Backend& a, Integer1 b, Integer2 c)
{
   using double_type = typename double_precision_type<Backend>::type                                      ;
   using ui_type = typename boost::multiprecision::detail::canonical<unsigned char, double_type>::type;
   using i1_type = typename boost::multiprecision::detail::canonical<Integer1, double_type>::type     ;
   using i2_type = typename boost::multiprecision::detail::canonical<Integer2, Backend>::type         ;

   using default_ops::eval_bit_test;
   using default_ops::eval_get_sign;
   using default_ops::eval_modulus;
   using default_ops::eval_multiply;
   using default_ops::eval_right_shift;

   double_type x, y(a), t;
   x = ui_type(1u);

   while (b > 0)
   {
      if (b & 1)
      {
         eval_multiply(t, x, y);
         eval_modulus(x, t, static_cast<i1_type>(c));
      }
      eval_multiply(t, y, y);
      eval_modulus(y, t, static_cast<i1_type>(c));
      b >>= 1;
   }
   Backend x2(x);
   eval_modulus(result, x2, static_cast<i2_type>(c));
}

template <class Backend, class Integer1, class Integer2>
BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_signed<Integer1>::value && boost::multiprecision::detail::is_integral<Integer1>::value>::type eval_powm(Backend& result, const Backend& a, Integer1 b, Integer2 c)
{
   if (b < 0)
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("powm requires a positive exponent."));
   }
   eval_powm(result, a, static_cast<typename boost::multiprecision::detail::make_unsigned<Integer1>::type>(b), c);
}

struct powm_func
{
   template <class T, class U, class V>
   BOOST_MP_CXX14_CONSTEXPR void operator()(T& result, const T& b, const U& p, const V& m) const
   {
      eval_powm(result, b, p, m);
   }
   template <class R, class T, class U, class V>
   BOOST_MP_CXX14_CONSTEXPR void operator()(R& result, const T& b, const U& p, const V& m) const
   {
      T temp;
      eval_powm(temp, b, p, m);
      result = std::move(temp);
   }
};

} // namespace default_ops

template <class T, class U, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<
        (number_category<T>::value == number_kind_integer) &&
        (is_number<T>::value || is_number_expression<T>::value) &&
        (is_number<U>::value || is_number_expression<U>::value || boost::multiprecision::detail::is_integral<U>::value) &&
        (is_number<V>::value || is_number_expression<V>::value || boost::multiprecision::detail::is_integral<V>::value),
    typename std::conditional<
        is_no_et_number<T>::value,
        T,
        typename std::conditional<
            is_no_et_number<U>::value,
            U,
            typename std::conditional<
                is_no_et_number<V>::value,
                V,
                detail::expression<detail::function, default_ops::powm_func, T, U, V> >::type>::type>::type>::type
powm(const T& b, const U& p, const V& mod)
{
   return detail::expression<detail::function, default_ops::powm_func, T, U, V>(
       default_ops::powm_func(), b, p, mod);
}

}} // namespace boost::multiprecision

#endif
