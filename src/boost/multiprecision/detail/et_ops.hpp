///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_ET_OPS_HPP
#define BOOST_MP_ET_OPS_HPP

namespace boost { namespace multiprecision {

//
// Non-member operators for number:
//
// Unary operators first.
// Note that these *must* return by value, even though that's somewhat against
// existing practice.  The issue is that in C++11 land one could easily and legitimately
// write:
//    auto x = +1234_my_user_defined_suffix;
// which would result in a dangling-reference-to-temporary if unary + returned a reference
// to it's argument.  While return-by-value is obviously inefficient in other situations
// the reality is that no one ever uses unary operator+ anyway...!
//
template <class B, expression_template_option ExpressionTemplates>
inline constexpr const number<B, ExpressionTemplates> operator+(const number<B, ExpressionTemplates>& v) { return v; }
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline constexpr const detail::expression<tag, Arg1, Arg2, Arg3, Arg4> operator+(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& v) { return v; }
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, number<B, et_on> > operator-(const number<B, et_on>& v)
{
   static_assert(is_signed_number<B>::value, "Negating an unsigned type results in ill-defined behavior.");
   return detail::expression<detail::negate, number<B, et_on> >(v);
}
// rvalue ops:
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on> operator-(number<B, et_on>&& v)
{
   static_assert(is_signed_number<B>::value, "Negating an unsigned type results in ill-defined behavior.");
   return detail::expression<detail::negate, number<B, et_on> >(v);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > operator-(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& v)
{
   static_assert((is_signed_number<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value), "Negating an unsigned type results in ill-defined behavior.");
   return detail::expression<detail::negate, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(v);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::complement_immediates, number<B, et_on> > >::type
operator~(const number<B, et_on>& v) { return detail::expression<detail::complement_immediates, number<B, et_on> >(v); }

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            number<B, et_on> >::type
operator~(number<B, et_on>&& v) { return detail::expression<detail::complement_immediates, number<B, et_on> >(v); }

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer,
                            detail::expression<detail::bitwise_complement, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator~(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& v) { return detail::expression<detail::bitwise_complement, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(v); }
//
// Then addition:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >
operator+(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator+(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator+(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator+(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, detail::expression<detail::add_immediates, number<B, et_on>, V> >::type
operator+(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, V>(a, b);
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator+(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, V>(a, b);
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::add_immediates, V, number<B, et_on> > >::type
operator+(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::add_immediates, V, number<B, et_on> >(a, b);
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator+(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::add_immediates, V, number<B, et_on> >(a, b);
}

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::plus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >
operator+(const number<B, ET>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::plus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::plus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type
operator+(number<B, ET>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::plus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >
operator+(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >::result_type
operator+(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >
operator+(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator+(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::plus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::plus, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator+(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::plus, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
//
// Fused multiply add:
//
template <class V, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::result_type>::value,
                          detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V> >::type
operator+(const V& a, const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V>(b.left(), b.right(), a);
}
template <class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::result_type>::value,
                          detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V> >::type
operator+(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V>(a.left(), a.right(), b);
}
template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >
operator+(const number<B, ET>& a, const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(b.left(), b.right(), a);
}

template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >::result_type
operator+(number<B, ET>&& a, const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(b.left(), b.right(), a);
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >
operator+(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(a.left(), a.right(), b);
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >::result_type
operator+(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::multiply_add, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(a.left(), a.right(), b);
}

//
// Fused multiply subtract:
//
template <class V, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::result_type>::value,
                          detail::expression<detail::negate, detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V> > >::type
operator-(const V& a, const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V> >(detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V>(b.left(), b.right(), a));
}
template <class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::result_type>::value,
                          detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V> >::type
operator-(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, V>(a.left(), a.right(), b);
}
template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> > >
operator-(const number<B, ET>& a, const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> > >(detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(b.left(), b.right(), a));
}

template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> > >::result_type
operator-(number<B, ET>&& a, const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> > >(detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(b.left(), b.right(), a));
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >
operator-(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(a.left(), a.right(), b);
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >::result_type
operator-(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::multiply_subtract, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::left_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::right_type, number<B, ET> >(a.left(), a.right(), b);
}

//
// Repeat operator for negated arguments: propagate the negation to the top level to avoid temporaries:
//
template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::minus, number<B, ET>, Arg1>
operator+(const number<B, ET>& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::minus, number<B, ET>, Arg1>(a, b.left_ref());
}

template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::minus, number<B, ET>, Arg1>::result_type
operator+(number<B, ET>&& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::minus, number<B, ET>, Arg1>(a, b.left_ref());
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::minus, number<B, ET>, Arg1>
operator+(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::minus, number<B, ET>, Arg1>(b, a.left_ref());
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::minus, number<B, ET>, Arg1>::result_type
operator+(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::minus, number<B, ET>, Arg1>(b, a.left_ref());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >
operator+(const number<B, et_on>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >::result_type
operator+(number<B, et_on>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >
operator+(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B, et_on>& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(b, a.left_ref());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >::result_type
operator+(const detail::expression<detail::negate, number<B, et_on> >& a, number<B, et_on>&& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(b, a.left_ref());
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::subtract_immediates, V, number<B, et_on> > >::type
operator+(const detail::expression<detail::negate, number<B, et_on> >& a, const V& b)
{
   return detail::expression<detail::subtract_immediates, V, number<B, et_on> >(b, a.left_ref());
}
template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> > >::type
operator+(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B2, ET>& b)
{
   return detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> >(b, a.left_ref());
}

template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, typename detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> >::result_type>::type
operator+(const detail::expression<detail::negate, number<B, et_on> >& a, number<B2, ET>&& b)
{
   return detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> >(b, a.left_ref());
}

template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> > >::type
operator+(const number<B2, ET>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> >(a, b.left_ref());
}

template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, typename detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> >::result_type>::type
operator+(number<B2, ET>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::subtract_immediates, number<B2, ET>, number<B, et_on> >(a, b.left_ref());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> > >
operator+(const detail::expression<detail::negate, number<B, et_on> >& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> > >(detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(a.left_ref(), b.left_ref()));
}
//
// Subtraction:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >
operator-(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator-(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator-(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator-(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, detail::expression<detail::subtract_immediates, number<B, et_on>, V> >::type
operator-(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, V>(a, b);
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator-(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::subtract_immediates, number<B, et_on>, V>(a, b);
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::subtract_immediates, V, number<B, et_on> > >::type
operator-(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::subtract_immediates, V, number<B, et_on> >(a, b);
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator-(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::subtract_immediates, V, number<B, et_on> >(a, b);
}

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::minus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >
operator-(const number<B, ET>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::minus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::minus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type
operator-(number<B, ET>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::minus, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >
operator-(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >::result_type
operator-(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >
operator-(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator-(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::minus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::minus, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator-(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::minus, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
//
// Repeat operator for negated arguments: propagate the negation to the top level to avoid temporaries:
//
template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::plus, number<B, ET>, Arg1>
operator-(const number<B, ET>& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::plus, number<B, ET>, Arg1>(a, b.left_ref());
}

template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::plus, number<B, ET>, Arg1>::result_type
operator-(number<B, ET>&& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::plus, number<B, ET>, Arg1>(a, b.left_ref());
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::plus, number<B, ET>, Arg1> >
operator-(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::plus, number<B, ET>, Arg1> >(
       detail::expression<detail::plus, number<B, ET>, Arg1>(b, a.left_ref()));
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::plus, number<B, ET>, Arg1> >::result_type
operator-(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::plus, number<B, ET>, Arg1> >(
       detail::expression<detail::plus, number<B, ET>, Arg1>(b, a.left_ref()));
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >
operator-(const number<B, et_on>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >::result_type
operator-(number<B, et_on>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref());
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> > >
operator-(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B, et_on>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(b, a.left_ref()));
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> > >::result_type
operator-(const detail::expression<detail::negate, number<B, et_on> >& a, number<B, et_on>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::add_immediates, number<B, et_on>, number<B, et_on> >(b, a.left_ref()));
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, V> > >::type
operator-(const detail::expression<detail::negate, number<B, et_on> >& a, const V& b)
{
   return detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, V> >(detail::expression<detail::add_immediates, number<B, et_on>, V>(a.left_ref(), b));
}
template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B2, ET> > > >::type
operator-(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B2, ET>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B2, ET> > >(detail::expression<detail::add_immediates, number<B, et_on>, number<B2, ET> >(a.left_ref(), b));
}

template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, typename detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B2, ET> > >::result_type>::type
operator-(const detail::expression<detail::negate, number<B, et_on> >& a, number<B2, ET>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::add_immediates, number<B, et_on>, number<B2, ET> > >(detail::expression<detail::add_immediates, number<B, et_on>, number<B2, ET> >(a.left_ref(), b));
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::add_immediates, V, number<B, et_on> > >::type
operator-(const V& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::add_immediates, V, number<B, et_on> >(a, b.left_ref());
}
template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::add_immediates, number<B2, ET>, number<B, et_on> > >::type
operator-(const number<B2, ET>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::add_immediates, number<B2, ET>, number<B, et_on> >(a, b.left_ref());
}

template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, typename detail::expression<detail::add_immediates, number<B2, ET>, number<B, et_on> >::result_type>::type
operator-(number<B2, ET>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::add_immediates, number<B2, ET>, number<B, et_on> >(a, b.left_ref());
}

//
// Multiplication:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >
operator*(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator*(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator*(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator*(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, detail::expression<detail::multiply_immediates, number<B, et_on>, V> >::type
operator*(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::multiply_immediates, number<B, et_on>, V>(a, b);
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator*(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::multiply_immediates, number<B, et_on>, V>(a, b);
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::multiply_immediates, V, number<B, et_on> > >::type
operator*(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::multiply_immediates, V, number<B, et_on> >(a, b);
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator*(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::multiply_immediates, V, number<B, et_on> >(a, b);
}

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::multiplies, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >
operator*(const number<B, ET>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::multiplies, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}

template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::multiplies, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type
operator*(number<B, ET>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::multiplies, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >
operator*(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >::result_type
operator*(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >
operator*(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator*(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::multiplies, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::multiplies, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator*(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::multiplies, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
//
// Repeat operator for negated arguments: propagate the negation to the top level to avoid temporaries:
//
template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >
operator*(const number<B, ET>& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >(
       detail::expression<detail::multiplies, number<B, ET>, Arg1>(a, b.left_ref()));
}

template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >::result_type
operator*(number<B, ET>&& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >(
       detail::expression<detail::multiplies, number<B, ET>, Arg1>(a, b.left_ref()));
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >
operator*(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >(
       detail::expression<detail::multiplies, number<B, ET>, Arg1>(b, a.left_ref()));
}

template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >::result_type
operator*(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiplies, number<B, ET>, Arg1> >(
       detail::expression<detail::multiplies, number<B, ET>, Arg1>(b, a.left_ref()));
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >
operator*(const number<B, et_on>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref()));
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >::result_type
operator*(number<B, et_on>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref()));
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >
operator*(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B, et_on>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(b, a.left_ref()));
}

template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >::result_type
operator*(const detail::expression<detail::negate, number<B, et_on> >& a, number<B, et_on>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B, et_on> >(b, a.left_ref()));
}

template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, V> > >::type
operator*(const detail::expression<detail::negate, number<B, et_on> >& a, const V& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, V> >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, V>(a.left_ref(), b));
}
template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > > >::type
operator*(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B2, ET>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> >(a.left_ref(), b));
}

template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, typename detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > >::result_type>::type
operator*(const detail::expression<detail::negate, number<B, et_on> >& a, number<B2, ET>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> >(a.left_ref(), b));
}

template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, V> > >::type
operator*(const V& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, V> >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, V>(b.left_ref(), a));
}
template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > > >::type
operator*(const number<B2, ET>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> >(b.left_ref(), a));
}

template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, typename detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > >::result_type>::type
operator*(number<B2, ET>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> > >(
       detail::expression<detail::multiply_immediates, number<B, et_on>, number<B2, ET> >(b.left_ref(), a));
}

//
// Division:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >
operator/(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator/(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator/(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR number<B, et_on>
operator/(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, detail::expression<detail::divide_immediates, number<B, et_on>, V> >::type
operator/(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::divide_immediates, number<B, et_on>, V>(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && !is_equivalent_number_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator/(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::divide_immediates, number<B, et_on>, V>(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::divide_immediates, V, number<B, et_on> > >::type
operator/(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::divide_immediates, V, number<B, et_on> >(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, number<B, et_on> >::type
operator/(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::divide_immediates, V, number<B, et_on> >(a, b);
}
template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::divides, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >
operator/(const number<B, ET>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::divides, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class B, expression_template_option ET, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::divides, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type
operator/(number<B, ET>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::divides, number<B, ET>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >
operator/(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >::result_type
operator/(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, ET> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >
operator/(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator/(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::divides, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value, detail::expression<detail::divides, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator/(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::divides, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
//
// Repeat operator for negated arguments: propagate the negation to the top level to avoid temporaries:
//
template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::divides, number<B, ET>, Arg1> >
operator/(const number<B, ET>& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divides, number<B, ET>, Arg1> >(
       detail::expression<detail::divides, number<B, ET>, Arg1>(a, b.left_ref()));
}
template <class B, expression_template_option ET, class Arg1, class Arg2, class Arg3, class Arg4>
inline typename detail::expression<detail::negate, detail::expression<detail::divides, number<B, ET>, Arg1> >::result_type
operator/(number<B, ET>&& a, const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divides, number<B, ET>, Arg1> >(
       detail::expression<detail::divides, number<B, ET>, Arg1>(a, b.left_ref()));
}
template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::divides, Arg1, number<B, ET> > >
operator/(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, const number<B, ET>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divides, Arg1, number<B, ET> > >(
       detail::expression<detail::divides, Arg1, number<B, ET> >(a.left_ref(), b));
}
template <class Arg1, class Arg2, class Arg3, class Arg4, class B, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::divides, Arg1, number<B, ET> > >::result_type
operator/(const detail::expression<detail::negate, Arg1, Arg2, Arg3, Arg4>& a, number<B, ET>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divides, Arg1, number<B, ET> > >(
       detail::expression<detail::divides, Arg1, number<B, ET> >(a.left_ref(), b));
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >
operator/(const number<B, et_on>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref()));
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >::result_type
operator/(number<B, et_on>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a, b.left_ref()));
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >
operator/(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B, et_on>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a.left_ref(), b));
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >::result_type
operator/(const detail::expression<detail::negate, number<B, et_on> >& a, number<B, et_on>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> > >(
       detail::expression<detail::divide_immediates, number<B, et_on>, number<B, et_on> >(a.left_ref(), b));
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, V> > >::type
operator/(const detail::expression<detail::negate, number<B, et_on> >& a, const V& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, V> >(
       detail::expression<detail::divide_immediates, number<B, et_on>, V>(a.left_ref(), b));
}
template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B2, ET> > > >::type
operator/(const detail::expression<detail::negate, number<B, et_on> >& a, const number<B2, ET>& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B2, ET> > >(
       detail::expression<detail::divide_immediates, number<B, et_on>, number<B2, ET> >(a.left_ref(), b));
}
template <class B, class B2, expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, typename detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B2, ET> > >::result_type>::type
operator/(const detail::expression<detail::negate, number<B, et_on> >& a, number<B2, ET>&& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B, et_on>, number<B2, ET> > >(
       detail::expression<detail::divide_immediates, number<B, et_on>, number<B2, ET> >(a.left_ref(), b));
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::divide_immediates, V, number<B, et_on> > > >::type
operator/(const V& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, V, number<B, et_on> > >(
       detail::expression<detail::divide_immediates, V, number<B, et_on> >(a, b.left_ref()));
}
template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, number<B, et_on> >::value, detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B2, ET>, number<B, et_on> > > >::type
operator/(const number<B2, ET>& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B2, ET>, number<B, et_on> > >(
       detail::expression<detail::divide_immediates, number<B2, ET>, number<B, et_on> >(a, b.left_ref()));
}
template <class B2, expression_template_option ET, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<number<B2, ET>, typename detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B2, ET>, number<B, et_on> > >::result_type>::value, number<B, et_on> >::type
operator/(number<B2, ET>&& a, const detail::expression<detail::negate, number<B, et_on> >& b)
{
   return detail::expression<detail::negate, detail::expression<detail::divide_immediates, number<B2, ET>, number<B, et_on> > >(
       detail::expression<detail::divide_immediates, number<B2, ET>, number<B, et_on> >(a, b.left_ref()));
}
//
// Modulus:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::modulus_immediates, number<B, et_on>, number<B, et_on> > >::type
operator%(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::modulus_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator%(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::modulus_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator%(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::modulus_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator%(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::modulus_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer) && !is_equivalent_number_type<V, number<B, et_on> >::value,
                            detail::expression<detail::modulus_immediates, number<B, et_on>, V> >::type
operator%(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::modulus_immediates, number<B, et_on>, V>(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer) && !is_equivalent_number_type<V, number<B, et_on> >::value,
   number<B, et_on> >::type
operator%(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::modulus_immediates, number<B, et_on>, V>(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::modulus_immediates, V, number<B, et_on> > >::type
operator%(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::modulus_immediates, V, number<B, et_on> >(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator%(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::modulus_immediates, V, number<B, et_on> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::modulus, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator%(const number<B, et_on>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::modulus, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::modulus, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type >::type
operator%(number<B, et_on>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::modulus, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> > >::type
operator%(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >::result_type >::type
operator%(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer,
                            detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> > >::type
operator%(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator%(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::modulus, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::modulus, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator%(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::modulus, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
//
// Left shift:
//
template <class B, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value && (number_category<B>::value == number_kind_integer), detail::expression<detail::shift_left, number<B, et_on>, I> >::type
operator<<(const number<B, et_on>& a, const I& b)
{
   return detail::expression<detail::shift_left, number<B, et_on>, I>(a, b);
}
template <class B, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value && (number_category<B>::value == number_kind_integer), number<B, et_on> >::type
operator<<(number<B, et_on>&& a, const I& b)
{
   return detail::expression<detail::shift_left, number<B, et_on>, I>(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::shift_left, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, I> >::type
operator<<(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const I& b)
{
   return detail::expression<detail::shift_left, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, I>(a, b);
}
//
// Right shift:
//
template <class B, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::shift_right, number<B, et_on>, I> >::type
operator>>(const number<B, et_on>& a, const I& b)
{
   return detail::expression<detail::shift_right, number<B, et_on>, I>(a, b);
}
template <class B, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator>>(number<B, et_on>&& a, const I& b)
{
   return detail::expression<detail::shift_right, number<B, et_on>, I>(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class I>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<I>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::shift_right, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, I> >::type
operator>>(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const I& b)
{
   return detail::expression<detail::shift_right, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, I>(a, b);
}
//
// Bitwise AND:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_and_immediates, number<B, et_on>, number<B, et_on> > >::type
operator&(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_and_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator&(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_and_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator&(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_and_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator&(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_and_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::bitwise_and_immediates, number<B, et_on>, V> >::type
operator&(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::bitwise_and_immediates, number<B, et_on>, V>(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator&(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::bitwise_and_immediates, number<B, et_on>, V>(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::bitwise_and_immediates, V, number<B, et_on> > >::type
operator&(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_and_immediates, V, number<B, et_on> >(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator&(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_and_immediates, V, number<B, et_on> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_and, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator&(const number<B, et_on>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_and, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::bitwise_and, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type >::type
operator&(number<B, et_on>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_and, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> > >::type
operator&(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >::result_type >::type
operator&(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer,
                            detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> > >::type
operator&(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator&(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::bitwise_and, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::bitwise_and, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator&(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_and, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
//
// Bitwise OR:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_or_immediates, number<B, et_on>, number<B, et_on> > >::type
operator|(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_or_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator|(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_or_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator|(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_or_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator|(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_or_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::bitwise_or_immediates, number<B, et_on>, V> >::type
operator|(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::bitwise_or_immediates, number<B, et_on>, V>(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator|(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::bitwise_or_immediates, number<B, et_on>, V>(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::bitwise_or_immediates, V, number<B, et_on> > >::type
operator|(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_or_immediates, V, number<B, et_on> >(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator|(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_or_immediates, V, number<B, et_on> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_or, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator|(const number<B, et_on>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_or, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::bitwise_or, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type>::type
operator|(number<B, et_on>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_or, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> > >::type
operator|(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >::result_type>::type
operator|(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer,
                            detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> > >::type
operator|(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator|(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::bitwise_or, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::bitwise_or, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator|(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_or, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
//
// Bitwise XOR:
//
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, number<B, et_on> > >::type
operator^(const number<B, et_on>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator^(number<B, et_on>&& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator^(const number<B, et_on>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   number<B, et_on> >::type
operator^(number<B, et_on>&& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, number<B, et_on> >(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, V> >::type
operator^(const number<B, et_on>& a, const V& b)
{
   return detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, V>(a, b);
}
template <class B, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator^(number<B, et_on>&& a, const V& b)
{
   return detail::expression<detail::bitwise_xor_immediates, number<B, et_on>, V>(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
                            detail::expression<detail::bitwise_xor_immediates, V, number<B, et_on> > >::type
operator^(const V& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_xor_immediates, V, number<B, et_on> >(a, b);
}
template <class V, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, number<B, et_on> >::value && (number_category<B>::value == number_kind_integer),
   number<B, et_on> >::type
operator^(const V& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_xor_immediates, V, number<B, et_on> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_xor, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator^(const number<B, et_on>& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_xor, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class B, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::bitwise_xor, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >::result_type>::type
operator^(number<B, et_on>&& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_xor, number<B, et_on>, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
                            detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> > >::type
operator^(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const number<B, et_on>& b)
{
   return detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class B>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<B>::value == number_kind_integer,
   typename detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >::result_type>::type
operator^(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, number<B, et_on>&& b)
{
   return detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, number<B, et_on> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tag2, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer,
                            detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> > >::type
operator^(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b>& b)
{
   return detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, detail::expression<tag2, Arg1b, Arg2b, Arg3b, Arg4b> >(a, b);
}
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class V>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer),
                            detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V> >::type
operator^(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& a, const V& b)
{
   return detail::expression<detail::bitwise_xor, detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, V>(a, b);
}
template <class V, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<is_compatible_arithmetic_type<V, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value && (number_category<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value == number_kind_integer), detail::expression<detail::bitwise_xor, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > >::type
operator^(const V& a, const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& b)
{
   return detail::expression<detail::bitwise_xor, V, detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >(a, b);
}

}} // namespace boost::multiprecision

#endif
