///////////////////////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_UBLAS_HPP
#define BOOST_MP_UBLAS_HPP

namespace boost { namespace numeric { namespace ublas {

template <class V>
class sparse_vector_element;

template <class V, class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline bool operator==(const sparse_vector_element<V>& a, const ::boost::multiprecision::number<Backend, ExpressionTemplates>& b)
{
   using ref_type = typename sparse_vector_element<V>::const_reference;
   return static_cast<ref_type>(a) == b;
}

template <class X, class Y>
struct promote_traits;

template <class Backend1, boost::multiprecision::expression_template_option ExpressionTemplates1, class Backend2, boost::multiprecision::expression_template_option ExpressionTemplates2>
struct promote_traits<boost::multiprecision::number<Backend1, ExpressionTemplates1>, boost::multiprecision::number<Backend2, ExpressionTemplates2> >
{
   using number1_t = boost::multiprecision::number<Backend1, ExpressionTemplates1>;
   using number2_t = boost::multiprecision::number<Backend2, ExpressionTemplates2>;
   using promote_type = typename std::conditional<
       std::is_convertible<number1_t, number2_t>::value && !std::is_convertible<number2_t, number1_t>::value,
       number2_t, number1_t>::type;
};

template <class Backend1, boost::multiprecision::expression_template_option ExpressionTemplates1, class Arithmetic>
struct promote_traits<boost::multiprecision::number<Backend1, ExpressionTemplates1>, Arithmetic>
{
   using promote_type = boost::multiprecision::number<Backend1, ExpressionTemplates1>;
};

template <class Arithmetic, class Backend1, boost::multiprecision::expression_template_option ExpressionTemplates1>
struct promote_traits<Arithmetic, boost::multiprecision::number<Backend1, ExpressionTemplates1> >
{
   using promote_type = boost::multiprecision::number<Backend1, ExpressionTemplates1>;
};

template <class Backend1, boost::multiprecision::expression_template_option ExpressionTemplates1, class tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct promote_traits<boost::multiprecision::number<Backend1, ExpressionTemplates1>, boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4> >
{
   using number1_t = boost::multiprecision::number<Backend1, ExpressionTemplates1>         ;
   using expression_type = boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>;
   using number2_t = typename expression_type::result_type                                 ;
   using promote_type = typename promote_traits<number1_t, number2_t>::promote_type           ;
};

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class Backend1, boost::multiprecision::expression_template_option ExpressionTemplates1>
struct promote_traits<boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, boost::multiprecision::number<Backend1, ExpressionTemplates1> >
{
   using number1_t = boost::multiprecision::number<Backend1, ExpressionTemplates1>         ;
   using expression_type = boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>;
   using number2_t = typename expression_type::result_type                                 ;
   using promote_type = typename promote_traits<number1_t, number2_t>::promote_type           ;
};

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4, class tagb, class Arg1b, class Arg2b, class Arg3b, class Arg4b>
struct promote_traits<boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>, boost::multiprecision::detail::expression<tagb, Arg1b, Arg2b, Arg3b, Arg4b> >
{
   using expression1_t = boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>     ;
   using number1_t = typename expression1_t::result_type                                        ;
   using expression2_t = boost::multiprecision::detail::expression<tagb, Arg1b, Arg2b, Arg3b, Arg4b>;
   using number2_t = typename expression2_t::result_type                                        ;
};

}}} // namespace boost::numeric::ublas

#endif
