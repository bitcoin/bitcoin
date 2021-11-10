// - lambda_traits.hpp --- Boost Lambda Library ----------------------------
//
// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// -------------------------------------------------------------------------

#ifndef BOOST_LAMBDA_LAMBDA_TRAITS_HPP
#define BOOST_LAMBDA_LAMBDA_TRAITS_HPP

#include "boost/type_traits/transform_traits.hpp"
#include "boost/type_traits/cv_traits.hpp"
#include "boost/type_traits/function_traits.hpp"
#include "boost/type_traits/object_traits.hpp"
#include "boost/tuple/tuple.hpp"

namespace boost {
namespace lambda {

// -- if construct ------------------------------------------------
// Proposed by Krzysztof Czarnecki and Ulrich Eisenecker

namespace detail {

template <bool If, class Then, class Else> struct IF { typedef Then RET; };

template <class Then, class Else> struct IF<false, Then, Else> {
  typedef Else RET;
};


// An if construct that doesn't instantiate the non-matching template:

// Called as: 
//  IF_type<condition, A, B>::type 
// The matching template must define the typeded 'type'
// I.e. A::type if condition is true, B::type if condition is false
// Idea from Vesa Karvonen (from C&E as well I guess)
template<class T>
struct IF_type_
{
  typedef typename T::type type;
};


template<bool C, class T, class E>
struct IF_type
{
  typedef typename
    IF_type_<typename IF<C, T, E>::RET >::type type;
};

// helper that can be used to give typedef T to some type
template <class T> struct identity_mapping { typedef T type; };

// An if construct for finding an integral constant 'value'
// Does not instantiate the non-matching branch
// Called as IF_value<condition, A, B>::value
// If condition is true A::value must be defined, otherwise B::value

template<class T>
struct IF_value_
{
  BOOST_STATIC_CONSTANT(int, value = T::value);
};


template<bool C, class T, class E>
struct IF_value
{
  BOOST_STATIC_CONSTANT(int, value = (IF_value_<typename IF<C, T, E>::RET>::value));
};


// --------------------------------------------------------------

// removes reference from other than function types:
template<class T> class remove_reference_if_valid
{

  typedef typename boost::remove_reference<T>::type plainT;
public:
  typedef typename IF<
    boost::is_function<plainT>::value,
    T,
    plainT
  >::RET type;

};


template<class T> struct remove_reference_and_cv {
   typedef typename boost::remove_cv<
     typename boost::remove_reference<T>::type
   >::type type;
};


   
// returns a reference to the element of tuple T
template<int N, class T> struct tuple_element_as_reference {   
  typedef typename
     boost::tuples::access_traits<
       typename boost::tuples::element<N, T>::type
     >::non_const_type type;
};

// returns the cv and reverence stripped type of a tuple element
template<int N, class T> struct tuple_element_stripped {   
  typedef typename
     remove_reference_and_cv<
       typename boost::tuples::element<N, T>::type
     >::type type;
};

// is_lambda_functor -------------------------------------------------   

template <class T> struct is_lambda_functor_ {
  BOOST_STATIC_CONSTANT(bool, value = false);
};
   
template <class Arg> struct is_lambda_functor_<lambda_functor<Arg> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
   
} // end detail

   
template <class T> struct is_lambda_functor {
  BOOST_STATIC_CONSTANT(bool, 
     value = 
       detail::is_lambda_functor_<
         typename detail::remove_reference_and_cv<T>::type
       >::value);
};
   

namespace detail {

// -- parameter_traits_ ---------------------------------------------

// An internal parameter type traits class that respects
// the reference_wrapper class.

// The conversions performed are:
// references -> compile_time_error
// T1 -> T2, 
// reference_wrapper<T> -> T&
// const array -> ref to const array
// array -> ref to array
// function -> ref to function

// ------------------------------------------------------------------------

template<class T1, class T2> 
struct parameter_traits_ {
  typedef T2 type;
};

// Do not instantiate with reference types
template<class T, class Any> struct parameter_traits_<T&, Any> {
  typedef typename 
    generate_error<T&>::
      parameter_traits_class_instantiated_with_reference_type type;
};

// Arrays can't be stored as plain types; convert them to references
template<class T, int n, class Any> struct parameter_traits_<T[n], Any> {
  typedef T (&type)[n];
};
   
template<class T, int n, class Any> 
struct parameter_traits_<const T[n], Any> {
  typedef const T (&type)[n];
};

template<class T, int n, class Any> 
struct parameter_traits_<volatile T[n], Any> {
  typedef volatile  T (&type)[n];
};
template<class T, int n, class Any> 
struct parameter_traits_<const volatile T[n], Any> {
  typedef const volatile T (&type)[n];
};


template<class T, class Any> 
struct parameter_traits_<boost::reference_wrapper<T>, Any >{
  typedef T& type;
};

template<class T, class Any> 
struct parameter_traits_<const boost::reference_wrapper<T>, Any >{
  typedef T& type;
};

template<class T, class Any> 
struct parameter_traits_<volatile boost::reference_wrapper<T>, Any >{
  typedef T& type;
};

template<class T, class Any> 
struct parameter_traits_<const volatile boost::reference_wrapper<T>, Any >{
  typedef T& type;
};

template<class Any>
struct parameter_traits_<void, Any> {
  typedef void type;
};

template<class Arg, class Any>
struct parameter_traits_<lambda_functor<Arg>, Any > {
  typedef lambda_functor<Arg> type;
};

template<class Arg, class Any>
struct parameter_traits_<const lambda_functor<Arg>, Any > {
  typedef lambda_functor<Arg> type;
};

// Are the volatile versions needed?
template<class Arg, class Any>
struct parameter_traits_<volatile lambda_functor<Arg>, Any > {
  typedef lambda_functor<Arg> type;
};

template<class Arg, class Any>
struct parameter_traits_<const volatile lambda_functor<Arg>, Any > {
  typedef lambda_functor<Arg> type;
};

} // end namespace detail


// ------------------------------------------------------------------------
// traits classes for lambda expressions (bind functions, operators ...)   

// must be instantiated with non-reference types

// The default is const plain type -------------------------
// const T -> const T, 
// T -> const T, 
// references -> compile_time_error
// reference_wrapper<T> -> T&
// array -> const ref array
template<class T>
struct const_copy_argument {
  typedef typename 
    detail::parameter_traits_<
      T,
      typename detail::IF<boost::is_function<T>::value, T&, const T>::RET
    >::type type;
};

// T may be a function type. Without the IF test, const would be added 
// to a function type, which is illegal.

// all arrays are converted to const.
// This traits template is used for 'const T&' parameter passing 
// and thus the knowledge of the potential 
// non-constness of an actual argument is lost.   
template<class T, int n>  struct const_copy_argument <T[n]> {
  typedef const T (&type)[n];
};
template<class T, int n>  struct const_copy_argument <volatile T[n]> {
     typedef const volatile T (&type)[n];
};
   
template<class T>
struct const_copy_argument<T&> {};
// do not instantiate with references
  //  typedef typename detail::generate_error<T&>::references_not_allowed type;


template<>
struct const_copy_argument<void> {
  typedef void type;
};

template<>
struct const_copy_argument<void const> {
  typedef void type;
};


// Does the same as const_copy_argument, but passes references through as such
template<class T>
struct bound_argument_conversion {
  typedef typename const_copy_argument<T>::type type; 
};

template<class T>
struct bound_argument_conversion<T&> {
  typedef T& type; 
};
   
// The default is non-const reference -------------------------
// const T -> const T&, 
// T -> T&, 
// references -> compile_time_error
// reference_wrapper<T> -> T&
template<class T>
struct reference_argument {
  typedef typename detail::parameter_traits_<T, T&>::type type; 
};

template<class T>
struct reference_argument<T&> {
  typedef typename detail::generate_error<T&>::references_not_allowed type; 
};

template<class Arg>
struct reference_argument<lambda_functor<Arg> > {
  typedef lambda_functor<Arg> type;
};

template<class Arg>
struct reference_argument<const lambda_functor<Arg> > {
  typedef lambda_functor<Arg> type;
};

// Are the volatile versions needed?
template<class Arg>
struct reference_argument<volatile lambda_functor<Arg> > {
  typedef lambda_functor<Arg> type;
};

template<class Arg>
struct reference_argument<const volatile lambda_functor<Arg> > {
  typedef lambda_functor<Arg> type;
};

template<>
struct reference_argument<void> {
  typedef void type;
};

namespace detail {
   
// Array to pointer conversion
template <class T>
struct array_to_pointer { 
  typedef T type;
};

template <class T, int N>
struct array_to_pointer <const T[N]> { 
  typedef const T* type;
};
template <class T, int N>
struct array_to_pointer <T[N]> { 
  typedef T* type;
};

template <class T, int N>
struct array_to_pointer <const T (&) [N]> { 
  typedef const T* type;
};
template <class T, int N>
struct array_to_pointer <T (&) [N]> { 
  typedef T* type;
};


// ---------------------------------------------------------------------------
// The call_traits for bind
// Respects the reference_wrapper class.

// These templates are used outside of bind functions as well.
// the bind_tuple_mapper provides a shorter notation for default
// bound argument storing semantics, if all arguments are treated
// uniformly.

// from template<class T> foo(const T& t) : bind_traits<const T>::type
// from template<class T> foo(T& t) : bind_traits<T>::type

// Conversions:
// T -> const T,
// cv T -> cv T, 
// T& -> T& 
// reference_wrapper<T> -> T&
// const reference_wrapper<T> -> T&
// array -> const ref array

// make bound arguments const, this is a deliberate design choice, the
// purpose is to prevent side effects to bound arguments that are stored
// as copies
template<class T>
struct bind_traits {
  typedef const T type; 
};

template<class T>
struct bind_traits<T&> {
  typedef T& type; 
};

// null_types are an exception, we always want to store them as non const
// so that other templates can assume that null_type is always without const
template<>
struct bind_traits<null_type> {
  typedef null_type type;
};

// the bind_tuple_mapper, bind_type_generators may 
// introduce const to null_type
template<>
struct bind_traits<const null_type> {
  typedef null_type type;
};

// Arrays can't be stored as plain types; convert them to references.
// All arrays are converted to const. This is because bind takes its
// parameters as const T& and thus the knowledge of the potential 
// non-constness of actual argument is lost.
template<class T, int n>  struct bind_traits <T[n]> {
  typedef const T (&type)[n];
};

template<class T, int n> 
struct bind_traits<const T[n]> {
  typedef const T (&type)[n];
};

template<class T, int n>  struct bind_traits<volatile T[n]> {
  typedef const volatile T (&type)[n];
};

template<class T, int n> 
struct bind_traits<const volatile T[n]> {
  typedef const volatile T (&type)[n];
};

template<class R>
struct bind_traits<R()> {
    typedef R(&type)();
};

template<class R, class Arg1>
struct bind_traits<R(Arg1)> {
    typedef R(&type)(Arg1);
};

template<class R, class Arg1, class Arg2>
struct bind_traits<R(Arg1, Arg2)> {
    typedef R(&type)(Arg1, Arg2);
};

template<class R, class Arg1, class Arg2, class Arg3>
struct bind_traits<R(Arg1, Arg2, Arg3)> {
    typedef R(&type)(Arg1, Arg2, Arg3);
};

template<class R, class Arg1, class Arg2, class Arg3, class Arg4>
struct bind_traits<R(Arg1, Arg2, Arg3, Arg4)> {
    typedef R(&type)(Arg1, Arg2, Arg3, Arg4);
};

template<class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct bind_traits<R(Arg1, Arg2, Arg3, Arg4, Arg5)> {
    typedef R(&type)(Arg1, Arg2, Arg3, Arg4, Arg5);
};

template<class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
struct bind_traits<R(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6)> {
    typedef R(&type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
};

template<class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
struct bind_traits<R(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7)> {
    typedef R(&type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
};

template<class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
struct bind_traits<R(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8)> {
    typedef R(&type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8);
};

template<class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
struct bind_traits<R(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9)> {
    typedef R(&type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9);
};

template<class T> 
struct bind_traits<reference_wrapper<T> >{
  typedef T& type;
};

template<class T> 
struct bind_traits<const reference_wrapper<T> >{
  typedef T& type;
};

template<>
struct bind_traits<void> {
  typedef void type;
};



template <
  class T0 = null_type, class T1 = null_type, class T2 = null_type, 
  class T3 = null_type, class T4 = null_type, class T5 = null_type, 
  class T6 = null_type, class T7 = null_type, class T8 = null_type, 
  class T9 = null_type
>
struct bind_tuple_mapper {
  typedef
    tuple<typename bind_traits<T0>::type, 
          typename bind_traits<T1>::type, 
          typename bind_traits<T2>::type, 
          typename bind_traits<T3>::type, 
          typename bind_traits<T4>::type, 
          typename bind_traits<T5>::type, 
          typename bind_traits<T6>::type, 
          typename bind_traits<T7>::type,
          typename bind_traits<T8>::type,
          typename bind_traits<T9>::type> type;
};

// bind_traits, except map const T& -> const T
  // this is needed e.g. in currying. Const reference arguments can
  // refer to temporaries, so it is not safe to store them as references.
  template <class T> struct remove_const_reference {
    typedef typename bind_traits<T>::type type;
  };

  template <class T> struct remove_const_reference<const T&> {
    typedef const T type;
  };


// maps the bind argument types to the resulting lambda functor type
template <
  class T0 = null_type, class T1 = null_type, class T2 = null_type, 
  class T3 = null_type, class T4 = null_type, class T5 = null_type, 
  class T6 = null_type, class T7 = null_type, class T8 = null_type, 
  class T9 = null_type
>
class bind_type_generator {

  typedef typename
  detail::bind_tuple_mapper<
    T0, T1, T2, T3, T4, T5, T6, T7, T8, T9
  >::type args_t;

  BOOST_STATIC_CONSTANT(int, nof_elems = boost::tuples::length<args_t>::value);

  typedef 
    action<
      nof_elems, 
      function_action<nof_elems>
    > action_type;

public:
  typedef
    lambda_functor<
      lambda_functor_base<
        action_type, 
        args_t
      >
    > type; 
    
};


   
} // detail
   
template <class T> inline const T&  make_const(const T& t) { return t; }


} // end of namespace lambda
} // end of namespace boost


   
#endif // BOOST_LAMBDA_TRAITS_HPP
