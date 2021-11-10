///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_COMPARE_HPP
#define BOOST_MP_COMPARE_HPP

#include <boost/multiprecision/traits/is_backend.hpp>

//
// Comparison operators for number.
//

namespace boost { namespace multiprecision {

namespace default_ops {

//
// The dispatching mechanism used here to deal with differently typed arguments
// could be better replaced with enable_if overloads, but that breaks MSVC-12
// under strange and hard to reproduce circumstances.
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_eq(const B& a, const B& b)
{
   return a.compare(b) == 0;
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_eq_imp(const T& a, const U& b, const std::integral_constant<bool, true>&)
{
   typename boost::multiprecision::detail::number_from_backend<T, U>::type t(b);
   return eval_eq(a, t.backend());
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_eq_imp(const T& a, const U& b, const std::integral_constant<bool, false>&)
{
   typename boost::multiprecision::detail::number_from_backend<U, T>::type t(a);
   return eval_eq(t.backend(), b);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_eq(const T& a, const U& b)
{
   using tag_type = std::integral_constant<bool, boost::multiprecision::detail::is_first_backend<T, U>::value>;
   return eval_eq_imp(a, b, tag_type());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_lt(const B& a, const B& b)
{
   return a.compare(b) < 0;
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_lt_imp(const T& a, const U& b, const std::integral_constant<bool, true>&)
{
   typename boost::multiprecision::detail::number_from_backend<T, U>::type t(b);
   return eval_lt(a, t.backend());
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_lt_imp(const T& a, const U& b, const std::integral_constant<bool, false>&)
{
   typename boost::multiprecision::detail::number_from_backend<U, T>::type t(a);
   return eval_lt(t.backend(), b);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_lt(const T& a, const U& b)
{
   using tag_type = std::integral_constant<bool, boost::multiprecision::detail::is_first_backend<T, U>::value>;
   return eval_lt_imp(a, b, tag_type());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_gt(const B& a, const B& b)
{
   return a.compare(b) > 0;
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_gt_imp(const T& a, const U& b, const std::integral_constant<bool, true>&)
{
   typename boost::multiprecision::detail::number_from_backend<T, U>::type t(b);
   return eval_gt(a, t.backend());
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_gt_imp(const T& a, const U& b, const std::integral_constant<bool, false>&)
{
   typename boost::multiprecision::detail::number_from_backend<U, T>::type t(a);
   return eval_gt(t.backend(), b);
}
template <class T, class U>
inline BOOST_MP_CXX14_CONSTEXPR bool eval_gt(const T& a, const U& b)
{
   using tag_type = std::integral_constant<bool, boost::multiprecision::detail::is_first_backend<T, U>::value>;
   return eval_gt_imp(a, b, tag_type());
}

} // namespace default_ops

namespace detail {

template <class Num, class Val>
struct is_valid_mixed_compare : public std::integral_constant<bool, false>
{};

template <class B, expression_template_option ET, class Val>
struct is_valid_mixed_compare<number<B, ET>, Val> : public std::is_convertible<Val, number<B, ET> >
{};

template <class B, expression_template_option ET>
struct is_valid_mixed_compare<number<B, ET>, number<B, ET> > : public std::integral_constant<bool, false>
{};

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct is_valid_mixed_compare<number<B, ET>, expression<tag, Arg1, Arg2, Arg3, Arg4> >
    : public std::is_convertible<expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >
{};

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
struct is_valid_mixed_compare<expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >
    : public std::is_convertible<expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >
{};

template <class Backend, expression_template_option ExpressionTemplates>
inline constexpr typename std::enable_if<number_category<Backend>::value != number_kind_floating_point, bool>::type is_unordered_value(const number<Backend, ExpressionTemplates>&)
{
   return false;
}
template <class Backend, expression_template_option ExpressionTemplates>
inline
#if !BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40700)
    constexpr
#endif
    typename std::enable_if<number_category<Backend>::value == number_kind_floating_point, bool>::type
    is_unordered_value(const number<Backend, ExpressionTemplates>& a)
{
   using default_ops::eval_fpclassify;
   return eval_fpclassify(a.backend()) == FP_NAN;
}

template <class Arithmetic>
inline constexpr typename std::enable_if<number_category<Arithmetic>::value != number_kind_floating_point, bool>::type is_unordered_value(const Arithmetic&)
{
   return false;
}
template <class Arithmetic>
inline 
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
    BOOST_MP_CXX14_CONSTEXPR 
#endif
   typename std::enable_if < number_category < Arithmetic> ::value == number_kind_floating_point, bool> ::type
    is_unordered_value(const Arithmetic& a)
{
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   if (BOOST_MP_IS_CONST_EVALUATED(a))
   {
      return a != a;
   }
   else
#endif
   {
      return (boost::math::isnan)(a);
   }
}

template <class T, class U>
inline constexpr bool is_unordered_comparison(const T& a, const U& b)
{
   return is_unordered_value(a) || is_unordered_value(b);
}

} // namespace detail

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool operator==(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b)
{
   using default_ops::eval_eq;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_eq(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && !is_number_expression<Arithmetic>::value, bool>::type
operator==(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_eq;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_eq(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && !is_number_expression<Arithmetic>::value, bool>::type
operator==(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_eq;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_eq(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
operator==(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(b);
   if (detail::is_unordered_comparison(a, t))
      return false;
   return eval_eq(t.backend(), result_type::canonical_value(a));
}
template <class Backend, expression_template_option ExpressionTemplates, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool operator==(const number<Backend, ExpressionTemplates>& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(b);
   if (detail::is_unordered_comparison(a, t))
      return false;
   return eval_eq(t.backend(), a.backend());
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
operator==(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(a);
   if (detail::is_unordered_comparison(t, b))
      return false;
   return eval_eq(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool operator==(const detail::expression<Tag, A1, A2, A3, A4>& a, const number<Backend, ExpressionTemplates>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(a);
   if (detail::is_unordered_comparison(t, b))
      return false;
   return eval_eq(t.backend(), b.backend());
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
operator==(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_eq;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   if (detail::is_unordered_comparison(t, t2))
      return false;
   return eval_eq(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool operator!=(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b)
{
   using default_ops::eval_eq;
   if (detail::is_unordered_comparison(a, b))
      return true;
   return !eval_eq(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && !is_number_expression<Arithmetic>::value, bool>::type
operator!=(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_eq;
   if (detail::is_unordered_comparison(a, b))
      return true;
   return !eval_eq(a.backend(), number<Backend, et_on>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && !is_number_expression<Arithmetic>::value, bool>::type
operator!=(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_eq;
   if (detail::is_unordered_comparison(a, b))
      return true;
   return !eval_eq(b.backend(), number<Backend, et_on>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
operator!=(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(b);
   if (detail::is_unordered_comparison(a, t))
      return true;
   return !eval_eq(t.backend(), result_type::canonical_value(a));
}
template <class Backend, expression_template_option ExpressionTemplates, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool operator!=(const number<Backend, ExpressionTemplates>& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(b);
   if (detail::is_unordered_comparison(a, t))
      return true;
   return !eval_eq(t.backend(), a.backend());
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
operator!=(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(a);
   if (detail::is_unordered_comparison(t, b))
      return true;
   return !eval_eq(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool operator!=(const detail::expression<Tag, A1, A2, A3, A4>& a, const number<Backend, ExpressionTemplates>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_eq;
   result_type t(a);
   if (detail::is_unordered_comparison(t, b))
      return true;
   return !eval_eq(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
operator!=(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_eq;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   if (detail::is_unordered_comparison(t, t2))
      return true;
   return !eval_eq(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value != number_kind_complex) && (number_category<Backend2>::value != number_kind_complex), bool>::type
operator<(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b)
{
   using default_ops::eval_lt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_lt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator<(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_lt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_lt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator<(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_gt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_gt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator<(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(b);
   if (detail::is_unordered_comparison(a, t))
      return false;
   return eval_gt(t.backend(), result_type::canonical_value(a));
}
template <class Backend, expression_template_option ExpressionTemplates, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool operator<(const number<Backend, ExpressionTemplates>& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(b);
   return a < t;
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator<(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   result_type t(a);
   if (detail::is_unordered_comparison(t, b))
      return false;
   return eval_lt(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool operator<(const detail::expression<Tag, A1, A2, A3, A4>& a, const number<Backend, ExpressionTemplates>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   result_type t(a);
   return t < b;
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator<(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_lt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   if (detail::is_unordered_comparison(t, t2))
      return false;
   return eval_lt(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value != number_kind_complex) && (number_category<Backend2>::value != number_kind_complex), bool>::type
operator>(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b)
{
   using default_ops::eval_gt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_gt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator>(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_gt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_gt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator>(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_lt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return eval_lt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator>(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   result_type t(b);
   return a > t;
}
template <class Backend, expression_template_option ExpressionTemplates, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool operator>(const number<Backend, ExpressionTemplates>& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   result_type t(b);
   return a > t;
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator>(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(a);
   return t > b;
}
template <class Tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool operator>(const detail::expression<Tag, A1, A2, A3, A4>& a, const number<Backend, ExpressionTemplates>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(a);
   return t > b;
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator>(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_gt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   return t > t2;
}

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value != number_kind_complex) && (number_category<Backend2>::value != number_kind_complex), bool>::type
operator<=(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b)
{
   using default_ops::eval_gt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return !eval_gt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator<=(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_gt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return !eval_gt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator<=(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_lt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return !eval_lt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator<=(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   if (detail::is_unordered_value(a) || detail::is_unordered_value(b))
      return false;
   result_type t(b);
   if (detail::is_unordered_comparison(a, t))
      return false;
   return !eval_lt(t.backend(), result_type::canonical_value(a));
}
template <class Backend, expression_template_option ExpressionTemplates, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool operator<=(const number<Backend, ExpressionTemplates>& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   if (detail::is_unordered_value(a) || detail::is_unordered_value(b))
      return false;
   result_type t(b);
   return a <= t;
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator<=(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(a);
   if (detail::is_unordered_comparison(t, b))
      return false;
   return !eval_gt(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool operator<=(const detail::expression<Tag, A1, A2, A3, A4>& a, const number<Backend, ExpressionTemplates>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(a);
   return t <= b;
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator<=(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_gt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   if (detail::is_unordered_comparison(t, t2))
      return false;
   return !eval_gt(t.backend(), t2.backend());
}

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(number_category<Backend>::value != number_kind_complex) && (number_category<Backend2>::value != number_kind_complex), bool>::type
operator>=(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b)
{
   using default_ops::eval_lt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return !eval_lt(a.backend(), b.backend());
}
template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator>=(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   using default_ops::eval_lt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return !eval_lt(a.backend(), number<Backend, ExpressionTemplates>::canonical_value(b));
}
template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value && (number_category<Backend>::value != number_kind_complex) && !is_number_expression<Arithmetic>::value, bool>::type
operator>=(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   using default_ops::eval_gt;
   if (detail::is_unordered_comparison(a, b))
      return false;
   return !eval_gt(b.backend(), number<Backend, ExpressionTemplates>::canonical_value(a));
}
template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator>=(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(b);
   if (detail::is_unordered_comparison(a, t))
      return false;
   return !eval_gt(t.backend(), result_type::canonical_value(a));
}
template <class Backend, expression_template_option ExpressionTemplates, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR bool operator>=(const number<Backend, ExpressionTemplates>& a, const detail::expression<Tag, A1, A2, A3, A4>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_gt;
   result_type t(b);
   return a >= t;
}
template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator>=(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   result_type t(a);
   if (detail::is_unordered_comparison(t, b))
      return false;
   return !eval_lt(t.backend(), result_type::canonical_value(b));
}
template <class Tag, class A1, class A2, class A3, class A4, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR bool operator>=(const detail::expression<Tag, A1, A2, A3, A4>& a, const number<Backend, ExpressionTemplates>& b)
{
   using result_type = typename detail::expression<Tag, A1, A2, A3, A4>::result_type;
   using default_ops::eval_lt;
   result_type t(a);
   return t >= b;
}
template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value && (number_category<typename detail::expression<Tag, A1, A2, A3, A4>::result_type>::value != number_kind_complex), bool>::type
operator>=(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b)
{
   using default_ops::eval_lt;
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      t(a);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type t2(b);
   if (detail::is_unordered_comparison(t, t2))
      return false;
   return !eval_lt(t.backend(), t2.backend());
}

//
// C99 comparison macros as functions:
//
template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool isgreater BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b) { return a > b; }

template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b) { return a > b; }

template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b) { return a > b; }

template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b) { return a > b; }

template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b) { return a > b; }

template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
    isgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b) { return a > b; }

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b) { return a >= b; }

template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isgreaterequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b) { return a >= b; }

template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isgreaterequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b) { return a >= b; }

template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isgreaterequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b) { return a >= b; }

template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isgreaterequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b) { return a >= b; }

template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
    isgreaterequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b) { return a >= b; }

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool islessequal BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b) { return a <= b; }

template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    islessequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b) { return a <= b; }

template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    islessequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b) { return a <= b; }

template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    islessequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b) { return a <= b; }

template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    islessequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b) { return a <= b; }

template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
    islessequal
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b) { return a <= b; }

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool isless BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b) { return a < b; }

template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isless
    BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b) { return a < b; }

template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isless
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b) { return a < b; }

template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isless
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& b) { return a < b; }

template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isless
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const Arithmetic& b) { return a < b; }

template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
    isless
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& a, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& b) { return a < b; }

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b)
{
   if (detail::is_unordered_comparison(a, b))
      return false;
   return a != b;
}

template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    islessgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b)
{
   if (detail::is_unordered_comparison(a, b))
      return false;
   return a != b;
}

template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    islessgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b)
{
   if (detail::is_unordered_comparison(a, b))
      return false;
   return a != b;
}

template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    islessgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& bb)
{
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type b(bb);
   return islessgreater                                          BOOST_PREVENT_MACRO_SUBSTITUTION(a, b);
}

template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    islessgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& aa, const Arithmetic& b)
{
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type a(aa);
   return islessgreater                                          BOOST_PREVENT_MACRO_SUBSTITUTION(a, b);
}

template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
    islessgreater
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& aa, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& bb)
{
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      a(aa);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type b(bb);
   return islessgreater                                               BOOST_PREVENT_MACRO_SUBSTITUTION(a, b);
}

template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
inline BOOST_MP_CXX14_CONSTEXPR bool isunordered BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const number<Backend2, ExpressionTemplates2>& b) { return detail::is_unordered_comparison(a, b); }

template <class Backend, expression_template_option ExpressionTemplates, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isunordered
    BOOST_PREVENT_MACRO_SUBSTITUTION(const number<Backend, ExpressionTemplates>& a, const Arithmetic& b) { return detail::is_unordered_comparison(a, b); }

template <class Arithmetic, class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<number<Backend, ExpressionTemplates>, Arithmetic>::value, bool>::type
    isunordered
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const number<Backend, ExpressionTemplates>& b) { return detail::is_unordered_comparison(a, b); }

template <class Arithmetic, class Tag, class A1, class A2, class A3, class A4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isunordered
    BOOST_PREVENT_MACRO_SUBSTITUTION(const Arithmetic& a, const detail::expression<Tag, A1, A2, A3, A4>& bb)
{
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type b(bb);
   return detail::is_unordered_comparison(a, b);
}

template <class Tag, class A1, class A2, class A3, class A4, class Arithmetic>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<detail::is_valid_mixed_compare<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, Arithmetic>::value, bool>::type
    isunordered
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& aa, const Arithmetic& b)
{
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type a(aa);
   return detail::is_unordered_comparison(a, b);
}

template <class Tag, class A1, class A2, class A3, class A4, class Tagb, class A1b, class A2b, class A3b, class A4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_equivalent_number_type<typename detail::expression<Tag, A1, A2, A3, A4>::result_type, typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type>::value, bool>::type
    isunordered
    BOOST_PREVENT_MACRO_SUBSTITUTION(const detail::expression<Tag, A1, A2, A3, A4>& aa, const detail::expression<Tagb, A1b, A2b, A3b, A4b>& bb)
{
   typename detail::expression<Tag, A1, A2, A3, A4>::result_type      a(aa);
   typename detail::expression<Tagb, A1b, A2b, A3b, A4b>::result_type b(bb);
   return detail::is_unordered_comparison(a, b);
}

}} // namespace boost::multiprecision

#endif // BOOST_MP_COMPARE_HPP
