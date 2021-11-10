//  return_type_traits.hpp -- Boost Lambda Library ---------------------------

// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org


#ifndef BOOST_LAMBDA_RETURN_TYPE_TRAITS_HPP
#define BOOST_LAMBDA_RETURN_TYPE_TRAITS_HPP

#include "boost/mpl/has_xxx.hpp"

#include <cstddef> // needed for the ptrdiff_t

namespace boost { 
namespace lambda {

// Much of the type deduction code for standard arithmetic types 
// from Gary Powell

  // different arities:
template <class Act, class A1> struct return_type_1; // 1-ary actions
template <class Act, class A1, class A2> struct return_type_2; // 2-ary
template <class Act, class Args> struct return_type_N; // >3- ary

template <class Act, class A1> struct return_type_1_prot;
template <class Act, class A1, class A2> struct return_type_2_prot; // 2-ary
template <class Act, class A1> struct return_type_N_prot; // >3-ary


namespace detail {

template<class> class return_type_deduction_failure {};

  // In some cases return type deduction should fail (an invalid lambda 
  // expression). Sometimes the lambda expression can be ok, the return type
  // just is not deducible (user defined operators). Then return type deduction
  // should never be entered at all, and the use of ret<> does this.
  // However, for nullary lambda functors, return type deduction is always
  // entered, and there seems to be no way around this.

  // (the return type is part of the prototype of the non-template
  // operator()(). The prototype is instantiated, even though the body 
  // is not.) 
 
  // So, in the case the return type deduction should fail, it should not
  // fail directly, but rather result in a valid but wrong return type,
  // causing a compile time error only if the function is really called.



} // end detail



// return_type_X_prot classes --------------------------------------------
// These classes are the first layer that gets instantiated from the 
// lambda_functor_base sig templates. It will check whether 
// the action is protectable and one of arguments is "protected" or its
// evaluation will otherwise result in another lambda functor.
// If this is a case, the result type will be another lambda functor.

// The arguments are always non-reference types, except for comma action
// where the right argument can be a reference too. This is because it 
// matters (in the builtin case) whether the argument is an lvalue or 
// rvalue: int i; i, 1 -> rvalue; 1, i -> lvalue

template <class Act, class A> struct return_type_1_prot {
public:
  typedef typename 
    detail::IF<
      is_protectable<Act>::value && is_lambda_functor<A>::value,
      lambda_functor<
        lambda_functor_base< 
          Act, 
          tuple<typename detail::remove_reference_and_cv<A>::type>
        >
      >,
      typename return_type_1<Act, A>::type
    >::RET type;  
};

  // take care of the unavoidable instantiation for nullary case
template<class Act> struct return_type_1_prot<Act, null_type> {
  typedef null_type type;
};
 
// Unary actions (result from unary operators)
// do not have a default return type.
template<class Act, class A> struct return_type_1 { 
   typedef typename 
     detail::return_type_deduction_failure<return_type_1> type;
};


namespace detail {

  template <class T>
  class protect_conversion {
      typedef typename boost::remove_reference<T>::type non_ref_T;
    public:

  // add const to rvalues, so that all rvalues are stored as const in 
  // the args tuple
    typedef typename detail::IF_type<
      boost::is_reference<T>::value && !boost::is_const<non_ref_T>::value,
      detail::identity_mapping<T>,
      const_copy_argument<non_ref_T> // handles funtion and array 
    >::type type;                      // types correctly
  };

} // end detail

template <class Act, class A, class B> struct return_type_2_prot {

// experimental feature
  // We may have a lambda functor as a result type of a subexpression 
  // (if protect) has  been used.
  // Thus, if one of the parameter types is a lambda functor, the result
  // is a lambda functor as well. 
  // We need to make a conservative choise here.
  // The resulting lambda functor stores all const reference arguments as
  // const copies. References to non-const are stored as such.
  // So if the source of the argument is a const open argument, a bound
  // argument stored as a const reference, or a function returning a 
  // const reference, that information is lost. There is no way of 
  // telling apart 'real const references' from just 'LL internal
  // const references' (or it would be really hard)

  // The return type is a subclass of lambda_functor, which has a converting 
  // copy constructor. It can copy any lambda functor, that has the same 
  // action type and code, and a copy compatible argument tuple.


  typedef typename boost::remove_reference<A>::type non_ref_A;
  typedef typename boost::remove_reference<B>::type non_ref_B;

typedef typename 
  detail::IF<
    is_protectable<Act>::value &&
      (is_lambda_functor<A>::value || is_lambda_functor<B>::value),
    lambda_functor<
      lambda_functor_base< 
        Act, 
        tuple<typename detail::protect_conversion<A>::type, 
              typename detail::protect_conversion<B>::type>
      >
    >,
    typename return_type_2<Act, non_ref_A, non_ref_B>::type
  >::RET type;
};

  // take care of the unavoidable instantiation for nullary case
template<class Act> struct return_type_2_prot<Act, null_type, null_type> {
  typedef null_type type;
};
  // take care of the unavoidable instantiation for nullary case
template<class Act, class Other> struct return_type_2_prot<Act, Other, null_type> {
  typedef null_type type;
};
  // take care of the unavoidable instantiation for nullary case
template<class Act, class Other> struct return_type_2_prot<Act, null_type, Other> {
  typedef null_type type;
};

  // comma is a special case, as the user defined operator can return
  // an lvalue (reference) too, hence it must be handled at this level.
template<class A, class B> 
struct return_type_2_comma
{
  typedef typename boost::remove_reference<A>::type non_ref_A;
  typedef typename boost::remove_reference<B>::type non_ref_B;

typedef typename 
  detail::IF<
    is_protectable<other_action<comma_action> >::value && // it is protectable
    (is_lambda_functor<A>::value || is_lambda_functor<B>::value),
    lambda_functor<
      lambda_functor_base< 
        other_action<comma_action>, 
        tuple<typename detail::protect_conversion<A>::type, 
              typename detail::protect_conversion<B>::type>
      >
    >,
    typename 
      return_type_2<other_action<comma_action>, non_ref_A, non_ref_B>::type
  >::RET type1;

   // if no user defined return_type_2 (or plain_return_type_2) specialization
  // matches, then return the righthand argument
  typedef typename 
    detail::IF<
      boost::is_same<type1, detail::unspecified>::value, 
      B,
      type1
    >::RET type;

};


  // currently there are no protectable actions with > 2 args

template<class Act, class Args> struct return_type_N_prot {
  typedef typename return_type_N<Act, Args>::type type;
};

  // take care of the unavoidable instantiation for nullary case
template<class Act> struct return_type_N_prot<Act, null_type> {
  typedef null_type type;
};

// handle different kind of actions ------------------------

  // use the return type given in the bind invocation as bind<Ret>(...)
template<int I, class Args, class Ret> 
struct return_type_N<function_action<I, Ret>, Args> { 
  typedef Ret type;
};

// ::result_type support

namespace detail
{

BOOST_MPL_HAS_XXX_TRAIT_DEF(result_type)

template<class F> struct get_result_type
{
  typedef typename F::result_type type;
};

template<class F, class A> struct get_sig
{
  typedef typename function_adaptor<F>::template sig<A>::type type;
};

} // namespace detail

  // Ret is detail::unspecified, so try to deduce return type
template<int I, class Args> 
struct return_type_N<function_action<I, detail::unspecified>, Args > { 

  // in the case of function action, the first element in Args is 
  // some type of function
  typedef typename Args::head_type Func;
  typedef typename detail::remove_reference_and_cv<Func>::type plain_Func;

public: 
  // pass the function to function_adaptor, and get the return type from 
  // that
  typedef typename detail::IF<
    detail::has_result_type<plain_Func>::value,
    detail::get_result_type<plain_Func>,
    detail::get_sig<plain_Func, Args>
  >::RET::type type;
};


} // namespace lambda
} // namespace boost

#endif



