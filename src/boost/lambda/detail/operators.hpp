// Boost Lambda Library - operators.hpp --------------------------------------

// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// ---------------------------------------------------------------

#ifndef BOOST_LAMBDA_OPERATORS_HPP
#define BOOST_LAMBDA_OPERATORS_HPP

#include "boost/lambda/detail/is_instance_of.hpp"

namespace boost { 
namespace lambda {

#if defined BOOST_LAMBDA_BE1
#error "Multiple defines of BOOST_LAMBDA_BE1"
#endif

  // For all BOOSTA_LAMBDA_BE* macros:

  // CONSTA must be either 'A' or 'const A'
  // CONSTB must be either 'B' or 'const B'

  // It is stupid to have the names A and B as macro arguments, but it avoids
  // the need to pass in emtpy macro arguments, which gives warnings on some
  // compilers

#define BOOST_LAMBDA_BE1(OPER_NAME, ACTION, CONSTA, CONSTB, CONVERSION)      \
template<class Arg, class B>                                                 \
inline const                                                                 \
lambda_functor<                                                              \
  lambda_functor_base<                                                       \
    ACTION,                                                                  \
    tuple<lambda_functor<Arg>, typename const_copy_argument <CONSTB>::type>  \
  >                                                                          \
>                                                                            \
OPER_NAME (const lambda_functor<Arg>& a, CONSTB& b) {                      \
  return                                                                     \
    lambda_functor_base<                                                     \
      ACTION,                                                                \
      tuple<lambda_functor<Arg>, typename const_copy_argument <CONSTB>::type>\
    >                                                                        \
   (tuple<lambda_functor<Arg>, typename const_copy_argument <CONSTB>::type>(a, b)); \
}


#if defined BOOST_LAMBDA_BE2
#error "Multiple defines of BOOST_LAMBDA_BE2"
#endif

#define BOOST_LAMBDA_BE2(OPER_NAME, ACTION, CONSTA, CONSTB, CONVERSION)      \
template<class A, class Arg>                                                 \
inline const                                                                 \
lambda_functor<                                                              \
  lambda_functor_base<                                                       \
    ACTION,                                                                  \
    tuple<typename CONVERSION <CONSTA>::type, lambda_functor<Arg> >        \
  >                                                                          \
>                                                                            \
OPER_NAME (CONSTA& a, const lambda_functor<Arg>& b) {                      \
  return                                                                     \
    lambda_functor_base<                                                     \
      ACTION,                                                                \
      tuple<typename CONVERSION <CONSTA>::type, lambda_functor<Arg> >      \
    >                                                                        \
  (tuple<typename CONVERSION <CONSTA>::type, lambda_functor<Arg> >(a, b)); \
}


#if defined BOOST_LAMBDA_BE3
#error "Multiple defines of BOOST_LAMBDA_BE3"
#endif

#define BOOST_LAMBDA_BE3(OPER_NAME, ACTION, CONSTA, CONSTB, CONVERSION)    \
template<class ArgA, class ArgB>                                           \
inline const                                                               \
lambda_functor<                                                            \
  lambda_functor_base<                                                     \
    ACTION,                                                                \
    tuple<lambda_functor<ArgA>, lambda_functor<ArgB> >                     \
  >                                                                        \
>                                                                          \
OPER_NAME (const lambda_functor<ArgA>& a, const lambda_functor<ArgB>& b) { \
  return                                                                   \
    lambda_functor_base<                                                   \
      ACTION,                                                              \
      tuple<lambda_functor<ArgA>, lambda_functor<ArgB> >                   \
    >                                                                      \
  (tuple<lambda_functor<ArgA>, lambda_functor<ArgB> >(a, b));              \
}

#if defined BOOST_LAMBDA_BE
#error "Multiple defines of BOOST_LAMBDA_BE"
#endif

#define BOOST_LAMBDA_BE(OPER_NAME, ACTION, CONSTA, CONSTB, CONST_CONVERSION) \
BOOST_LAMBDA_BE1(OPER_NAME, ACTION, CONSTA, CONSTB, CONST_CONVERSION)        \
BOOST_LAMBDA_BE2(OPER_NAME, ACTION, CONSTA, CONSTB, CONST_CONVERSION)        \
BOOST_LAMBDA_BE3(OPER_NAME, ACTION, CONSTA, CONSTB, CONST_CONVERSION)

#define BOOST_LAMBDA_EMPTY() 

BOOST_LAMBDA_BE(operator+, arithmetic_action<plus_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator-, arithmetic_action<minus_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator*, arithmetic_action<multiply_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator/, arithmetic_action<divide_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator%, arithmetic_action<remainder_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator<<, bitwise_action<leftshift_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator>>, bitwise_action<rightshift_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator&, bitwise_action<and_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator|, bitwise_action<or_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator^, bitwise_action<xor_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator&&, logical_action<and_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator||, logical_action<or_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator<, relational_action<less_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator>, relational_action<greater_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator<=, relational_action<lessorequal_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator>=, relational_action<greaterorequal_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator==, relational_action<equal_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE(operator!=, relational_action<notequal_action>, const A, const B, const_copy_argument)

BOOST_LAMBDA_BE(operator+=, arithmetic_assignment_action<plus_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator-=, arithmetic_assignment_action<minus_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator*=, arithmetic_assignment_action<multiply_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator/=, arithmetic_assignment_action<divide_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator%=, arithmetic_assignment_action<remainder_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator<<=, bitwise_assignment_action<leftshift_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator>>=, bitwise_assignment_action<rightshift_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator&=, bitwise_assignment_action<and_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator|=, bitwise_assignment_action<or_action>, A, const B, reference_argument)
BOOST_LAMBDA_BE(operator^=, bitwise_assignment_action<xor_action>, A, const B, reference_argument)


// A special trick for comma operator for correct preprocessing
#if defined BOOST_LAMBDA_COMMA_OPERATOR_NAME
#error "Multiple defines of BOOST_LAMBDA_COMMA_OPERATOR_NAME"
#endif

#define BOOST_LAMBDA_COMMA_OPERATOR_NAME operator,

BOOST_LAMBDA_BE1(BOOST_LAMBDA_COMMA_OPERATOR_NAME, other_action<comma_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE2(BOOST_LAMBDA_COMMA_OPERATOR_NAME, other_action<comma_action>, const A, const B, const_copy_argument)
BOOST_LAMBDA_BE3(BOOST_LAMBDA_COMMA_OPERATOR_NAME, other_action<comma_action>, const A, const B, const_copy_argument)



namespace detail {

// special cases for ostream& << Any and istream& >> Any ---------------
// the actual stream classes may vary and thus a specialisation for, 
// say ostream& does not match (the general case above is chosen). 
// Therefore we specialise for non-const reference:
// if the left argument is a stream, we store the stream as reference
// if it is something else, we store a const plain by default

// Note that the overloading is const vs. non-const first argument


template<class T> struct convert_ostream_to_ref_others_to_c_plain_by_default {
  typedef typename detail::IF<
                       is_instance_of_2<
                         T, std::basic_ostream
                       >::value,
                       T&,
                       typename const_copy_argument <T>::type
                     >::RET type;
};

template<class T> struct convert_istream_to_ref_others_to_c_plain_by_default {
  typedef typename detail::IF<
                       is_instance_of_2<
                         T, std::basic_istream
                       >::value,
                       T&,
                       typename const_copy_argument <T>::type
                     >::RET type;
};

} // detail

BOOST_LAMBDA_BE2(operator<<, bitwise_action< leftshift_action>, A, const B, detail::convert_ostream_to_ref_others_to_c_plain_by_default)
BOOST_LAMBDA_BE2(operator>>, bitwise_action< rightshift_action>, A, const B, detail::convert_istream_to_ref_others_to_c_plain_by_default)      


// special case for io_manipulators.
// function references cannot be given as arguments to lambda operator
// expressions in general. With << and >> the use of manipulators is
// so common, that specializations are provided to make them work.

template<class Arg, class Ret, class ManipArg>
inline const 
lambda_functor<
  lambda_functor_base<
    bitwise_action<leftshift_action>,
    tuple<lambda_functor<Arg>, Ret(&)(ManipArg)> 
  > 
>
operator<<(const lambda_functor<Arg>& a, Ret(&b)(ManipArg))
{
  return 
      lambda_functor_base<
        bitwise_action<leftshift_action>,
        tuple<lambda_functor<Arg>, Ret(&)(ManipArg)>
      > 
    ( tuple<lambda_functor<Arg>, Ret(&)(ManipArg)>(a, b) );
}

template<class Arg, class Ret, class ManipArg>
inline const 
lambda_functor<
  lambda_functor_base<
    bitwise_action<rightshift_action>,
    tuple<lambda_functor<Arg>, Ret(&)(ManipArg)>
  > 
>
operator>>(const lambda_functor<Arg>& a, Ret(&b)(ManipArg))
{
  return 
      lambda_functor_base<
        bitwise_action<rightshift_action>,
        tuple<lambda_functor<Arg>, Ret(&)(ManipArg)>
      > 
    ( tuple<lambda_functor<Arg>, Ret(&)(ManipArg)>(a, b) );
}


// (+ and -) take their arguments as const references. 
// This has consquences with pointer artihmetic
// E.g int a[]; ... *a = 1 works but not *(a+1) = 1. 
// the result of a+1 would be const
// To make the latter work too, 
// non-const arrays are taken as non-const and stored as non-const as well.
#if defined  BOOST_LAMBDA_PTR_ARITHMETIC_E1
#error "Multiple defines of  BOOST_LAMBDA_PTR_ARITHMETIC_E1"
#endif

#define BOOST_LAMBDA_PTR_ARITHMETIC_E1(OPER_NAME, ACTION, CONSTB)           \
template<class Arg, int N, class B>                                         \
inline const                                                                \
lambda_functor<                                                             \
  lambda_functor_base<ACTION, tuple<lambda_functor<Arg>, CONSTB(&)[N]> >    \
>                                                                           \
OPER_NAME (const lambda_functor<Arg>& a, CONSTB(&b)[N])                     \
{                                                                           \
  return                                                                    \
    lambda_functor_base<ACTION, tuple<lambda_functor<Arg>, CONSTB(&)[N]> >  \
  (tuple<lambda_functor<Arg>, CONSTB(&)[N]>(a, b));                         \
}


#if defined  BOOST_LAMBDA_PTR_ARITHMETIC_E2
#error "Multiple defines of  BOOST_LAMBDA_PTR_ARITHMETIC_E2"
#endif

#define BOOST_LAMBDA_PTR_ARITHMETIC_E2(OPER_NAME, ACTION, CONSTA)           \
template<int N, class A, class Arg>                                         \
inline const                                                                \
lambda_functor<                                                             \
  lambda_functor_base<ACTION, tuple<CONSTA(&)[N], lambda_functor<Arg> > >   \
>                                                                           \
OPER_NAME (CONSTA(&a)[N], const lambda_functor<Arg>& b)                     \
{                                                                           \
  return                                                                    \
    lambda_functor_base<ACTION, tuple<CONSTA(&)[N], lambda_functor<Arg> > > \
    (tuple<CONSTA(&)[N], lambda_functor<Arg> >(a, b));                      \
}


BOOST_LAMBDA_PTR_ARITHMETIC_E1(operator+, arithmetic_action<plus_action>, B)
BOOST_LAMBDA_PTR_ARITHMETIC_E2(operator+, arithmetic_action<plus_action>, A)
BOOST_LAMBDA_PTR_ARITHMETIC_E1(operator+, arithmetic_action<plus_action>,const B)
BOOST_LAMBDA_PTR_ARITHMETIC_E2(operator+, arithmetic_action<plus_action>,const A)


//BOOST_LAMBDA_PTR_ARITHMETIC_E1(operator-, arithmetic_action<minus_action>)
// This is not needed, since the result of ptr-ptr is an rvalue anyway

BOOST_LAMBDA_PTR_ARITHMETIC_E2(operator-, arithmetic_action<minus_action>, A)
BOOST_LAMBDA_PTR_ARITHMETIC_E2(operator-, arithmetic_action<minus_action>, const A)


#undef BOOST_LAMBDA_BE1
#undef BOOST_LAMBDA_BE2
#undef BOOST_LAMBDA_BE3
#undef BOOST_LAMBDA_BE
#undef BOOST_LAMBDA_COMMA_OPERATOR_NAME

#undef BOOST_LAMBDA_PTR_ARITHMETIC_E1
#undef BOOST_LAMBDA_PTR_ARITHMETIC_E2


// ---------------------------------------------------------------------
// unary operators -----------------------------------------------------
// ---------------------------------------------------------------------

#if defined BOOST_LAMBDA_UE
#error "Multiple defines of BOOST_LAMBDA_UE"
#endif

#define BOOST_LAMBDA_UE(OPER_NAME, ACTION)                                 \
template<class Arg>                                                        \
inline const                                                               \
lambda_functor<lambda_functor_base<ACTION, tuple<lambda_functor<Arg> > > > \
OPER_NAME (const lambda_functor<Arg>& a)                                   \
{                                                                          \
  return                                                                   \
    lambda_functor_base<ACTION, tuple<lambda_functor<Arg> > >              \
    ( tuple<lambda_functor<Arg> >(a) );                                    \
}


BOOST_LAMBDA_UE(operator+, unary_arithmetic_action<plus_action>)
BOOST_LAMBDA_UE(operator-, unary_arithmetic_action<minus_action>)
BOOST_LAMBDA_UE(operator~, bitwise_action<not_action>)
BOOST_LAMBDA_UE(operator!, logical_action<not_action>)
BOOST_LAMBDA_UE(operator++, pre_increment_decrement_action<increment_action>)
BOOST_LAMBDA_UE(operator--, pre_increment_decrement_action<decrement_action>)
BOOST_LAMBDA_UE(operator*, other_action<contentsof_action>)
BOOST_LAMBDA_UE(operator&, other_action<addressof_action>)

#if defined BOOST_LAMBDA_POSTFIX_UE
#error "Multiple defines of BOOST_LAMBDA_POSTFIX_UE"
#endif

#define BOOST_LAMBDA_POSTFIX_UE(OPER_NAME, ACTION)                         \
template<class Arg>                                                        \
inline const                                                               \
lambda_functor<lambda_functor_base<ACTION, tuple<lambda_functor<Arg> > > > \
OPER_NAME (const lambda_functor<Arg>& a, int)                              \
{                                                                          \
  return                                                                   \
    lambda_functor_base<ACTION, tuple<lambda_functor<Arg> > >              \
    ( tuple<lambda_functor<Arg> >(a) );                                    \
}


BOOST_LAMBDA_POSTFIX_UE(operator++, post_increment_decrement_action<increment_action>)
BOOST_LAMBDA_POSTFIX_UE(operator--, post_increment_decrement_action<decrement_action>)

#undef BOOST_LAMBDA_UE
#undef BOOST_LAMBDA_POSTFIX_UE

} // namespace lambda
} // namespace boost

#endif
