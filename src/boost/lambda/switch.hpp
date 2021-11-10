// Boost Lambda Library -- switch.hpp -----------------------------------
//
// Copyright (C) 2000 Gary Powell (powellg@amazon.com)
// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// --------------------------------------------------------------------------

#if !defined(BOOST_LAMBDA_SWITCH_HPP)
#define BOOST_LAMBDA_SWITCH_HPP

#include "boost/lambda/core.hpp"
#include "boost/lambda/detail/control_constructs_common.hpp"

#include "boost/preprocessor/enum_shifted_params.hpp"
#include "boost/preprocessor/repeat_2nd.hpp"
#include "boost/preprocessor/tuple.hpp"

namespace boost { 
namespace lambda {

// Switch actions
template <int N, class Switch1 = null_type, class Switch2 = null_type, 
          class Switch3 = null_type, class Switch4 = null_type,
          class Switch5 = null_type, class Switch6 = null_type, 
          class Switch7 = null_type, class Switch8 = null_type, 
          class Switch9 = null_type>
struct switch_action {};


namespace detail {

  // templates to represent special lambda functors for the cases in 
  // switch statements
  
template <int Value> struct case_label {};
struct default_label {};

template<class Type> struct switch_case_tag {};

  // a normal case is represented as:
  // tagged_lambda_functor<switch_case_tag<case_label<N> > >, LambdaFunctor>
  
  // the default case as:
  // tagged_lambda_functor<switch_case_tag<default_label> >, LambdaFunctor>


} // end detail


/// create switch_case_tag tagged_lambda_functors
template <int CaseValue, class Arg>
inline const 
tagged_lambda_functor<
  detail::switch_case_tag<detail::case_label<CaseValue> >, 
  lambda_functor<Arg> 
> 
case_statement(const lambda_functor<Arg>& a) { 
  return 
    tagged_lambda_functor<
      detail::switch_case_tag<detail::case_label<CaseValue> >, 
      lambda_functor<Arg> 
    >(a); 
}

// No case body case.
template <int CaseValue>
inline const 
tagged_lambda_functor<
  detail::switch_case_tag<detail::case_label<CaseValue> >,
  lambda_functor< 
    lambda_functor_base< 
      do_nothing_action, 
      null_type
    > 
  > 
> 
case_statement() { 
return 
  tagged_lambda_functor<
    detail::switch_case_tag<detail::case_label<CaseValue> >,
    lambda_functor< 
      lambda_functor_base< 
        do_nothing_action, 
        null_type
      > 
    > 
  > () ;
}

// default label
template <class Arg>
inline const 
tagged_lambda_functor<
  detail::switch_case_tag<detail::default_label>, 
  lambda_functor<Arg> 
> 
default_statement(const lambda_functor<Arg>& a) { 
  return 
    tagged_lambda_functor<
      detail::switch_case_tag<detail::default_label>, 
      lambda_functor<Arg> 
    >(a); 
}

// default lable, no case body case.
inline const 
tagged_lambda_functor<
  detail::switch_case_tag<detail::default_label>,
  lambda_functor< 
    lambda_functor_base< 
      do_nothing_action, 
      null_type
    > 
  > 
> 
default_statement() { 
return 
      lambda_functor_base< 
        do_nothing_action, 
        null_type 
      > () ;
}


// Specializations for lambda_functor_base of case_statement -----------------

// 0 case type:
// useless (just the condition part) but provided for completeness.
template<class Args>
class 
lambda_functor_base<
  switch_action<1>, 
  Args
> 
{
public:
  Args args;
  template <class SigArgs> struct sig { typedef void type; };
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    detail::select(::boost::tuples::get<1>(args), CALL_ACTUAL_ARGS);  
  }
};

// 1 case type:
// template<class Args, int Case1>
// class 
// lambda_functor_base<
//   action<
//     2, 
//     return_void_action<switch_action<detail::case_label<Case1> > > 
//   >, 
//   Args
// > 
// {
//   Args args;
// public:
//   explicit lambda_functor_base(const Args& a) : args(a) {}

//   template<class RET, class A, class B, class C>
//   RET call(A& a, B& b, C& c) const {
//     switch( detail::select(::boost::tuples::get<0>(args), a, b, c) )  
//     {
//       case Case1:                
//         detail::select(::boost::tuples::get<1>(args), a, b, c);
//         break;
//     }
//   }
// };

// switch with default being the sole label - doesn't make much sense but
// it is there for completeness
// template<class Args>
// class
// lambda_functor_base<
//   action<
//     2,
//     return_void_action<switch_action<detail::default_label> >
//   >,
//   Args
// >
// {
//   Args args;
// public:
//   explicit lambda_functor_base(const Args& a) : args(a) {}
// 
//   template<class RET, class A, class B, class C>
//   RET call(A& a, B& b, C& c) const {
//     switch( detail::select(::boost::tuples::get<0>(args), a, b, c) )
//     {
//       default:
//         detail::select(::boost::tuples::get<1>(args), a, b, c);
//         break;
//     }
//   }
// };



// // 2 case type:
// The different specializations are generated with Vesa Karvonen's 
// preprocessor library.

// This is just a comment to show what the generated classes look like

// template<class Args, int Case1, int Case2>
// class 
// lambda_functor_base<
//   action<3, 
//     return_void_action< 
//       switch_action< 
//         detail::case_label<Case1>,
//         detail::case_label<Case2>
//       > 
//     > 
//   >, 
//   Args
// > 
// {
//   Args args;
// public:
//   explicit lambda_functor_base(const Args& a) : args(a) {}

//   template<class RET, class A, class B, class C>
//   RET call(A& a, B& b, C& c) const {
//     switch( detail::select(::boost::tuples::get<0>(args), a, b, c) )  
//     {
//       case Case1:                
//         detail::select(::boost::tuples::get<1>(args), a, b, c);
//         break;
//       case Case2:                
//         detail::select(::boost::tuples::get<2>(args), a, b, c);
//         break;
//     }
//   }
// };

// template<class Args, int Case1>
// class 
// lambda_functor_base<
//   action<3, 
//     return_void_action< 
//       switch_action< 
//         detail::case_label<Case1>,
//         detail::default_label 
//       > 
//     > 
//   >, 
//   Args
// > 
// {
//   Args args;
// public:
//   explicit lambda_functor_base(const Args& a) : args(a) {}

//   template<class RET, class A, class B, class C>
//   RET call(A& a, B& b, C& c) const {
//     switch( detail::select(::boost::tuples::get<0>(args), a, b, c) )  
//     {
//       case Case1:                
//         detail::select(::boost::tuples::get<1>(args), a, b, c);
//         break;
//       default:                
//         detail::select(::boost::tuples::get<2>(args), a, b, c);
//         break;
//     }
//   }
// };
// -------------------------

// Some helper preprocessor macros ---------------------------------

// BOOST_LAMBDA_A_I_LIST(N, X) is a list of form X0, X1, ..., XN
// BOOST_LAMBDA_A_I_B_LIST(N, X, Y) is a list of form X0 Y, X1 Y, ..., XN Y

#define BOOST_LAMBDA_A_I(z, i, A) \
BOOST_PP_COMMA_IF(i) BOOST_PP_CAT(A,i)

#define BOOST_LAMBDA_A_I_B(z, i, T) \
BOOST_PP_COMMA_IF(i) BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2,0,T),i) BOOST_PP_TUPLE_ELEM(2,1,T)

#define BOOST_LAMBDA_A_I_LIST(i, A) \
BOOST_PP_REPEAT(i,BOOST_LAMBDA_A_I, A) 

#define BOOST_LAMBDA_A_I_B_LIST(i, A, B) \
BOOST_PP_REPEAT(i,BOOST_LAMBDA_A_I_B, (A,B)) 


// Switch related macros -------------------------------------------
#define BOOST_LAMBDA_SWITCH_CASE_BLOCK(z, N, A) \
  case Case##N: \
  detail::select(::boost::tuples::get<BOOST_PP_INC(N)>(args), CALL_ACTUAL_ARGS); \
  break;

#define BOOST_LAMBDA_SWITCH_CASE_BLOCK_LIST(N) \
BOOST_PP_REPEAT(N, BOOST_LAMBDA_SWITCH_CASE_BLOCK, FOO)
// 2 case type:

#define BOOST_LAMBDA_SWITCH_NO_DEFAULT_CASE(N)                                \
template<class Args, BOOST_LAMBDA_A_I_LIST(N, int Case)>                      \
class                                                                         \
lambda_functor_base<                                                          \
      switch_action<BOOST_PP_INC(N),                                          \
        BOOST_LAMBDA_A_I_B_LIST(N, detail::case_label<Case,>)                 \
      >,                                                                      \
  Args                                                                        \
>                                                                             \
{                                                                             \
public:                                                                       \
  Args args;                                                                  \
  template <class SigArgs> struct sig { typedef void type; };                 \
public:                                                                       \
  explicit lambda_functor_base(const Args& a) : args(a) {}                    \
                                                                              \
  template<class RET, CALL_TEMPLATE_ARGS>                                     \
  RET call(CALL_FORMAL_ARGS) const {                                          \
    switch( detail::select(::boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) ) \
    {                                                                         \
      BOOST_LAMBDA_SWITCH_CASE_BLOCK_LIST(N)                                  \
    }                                                                         \
  }                                                                           \
};

        

#define BOOST_LAMBDA_SWITCH_WITH_DEFAULT_CASE(N)                              \
template<                                                                     \
  class Args BOOST_PP_COMMA_IF(BOOST_PP_DEC(N))                               \
  BOOST_LAMBDA_A_I_LIST(BOOST_PP_DEC(N), int Case)                            \
>                                                                             \
class                                                                         \
lambda_functor_base<                                                          \
      switch_action<BOOST_PP_INC(N),                                          \
        BOOST_LAMBDA_A_I_B_LIST(BOOST_PP_DEC(N),                              \
                                detail::case_label<Case, >)                   \
        BOOST_PP_COMMA_IF(BOOST_PP_DEC(N))                                    \
        detail::default_label                                                 \
      >,                                                                      \
  Args                                                                        \
>                                                                             \
{                                                                             \
public:                                                                       \
  Args args;                                                                  \
  template <class SigArgs> struct sig { typedef void type; };                 \
public:                                                                       \
  explicit lambda_functor_base(const Args& a) : args(a) {}                    \
                                                                              \
  template<class RET, CALL_TEMPLATE_ARGS>                                     \
  RET call(CALL_FORMAL_ARGS) const {                                          \
    switch( detail::select(::boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) ) \
    {                                                                         \
        BOOST_LAMBDA_SWITCH_CASE_BLOCK_LIST(BOOST_PP_DEC(N))                  \
      default:                                                                \
        detail::select(::boost::tuples::get<N>(args), CALL_ACTUAL_ARGS);      \
        break;                                                                \
    }                                                                         \
  }                                                                           \
};






// switch_statement bind functions -------------------------------------

// The zero argument case, for completeness sake
inline const 
lambda_functor< 
  lambda_functor_base< 
    do_nothing_action, 
    null_type
  > 
>
switch_statement() { 
  return 
      lambda_functor_base< 
        do_nothing_action, 
        null_type
      > 
  ();
}

// 1 argument case, this is useless as well, just the condition part
template <class TestArg>
inline const 
lambda_functor< 
  lambda_functor_base< 
    switch_action<1>, 
    tuple<lambda_functor<TestArg> >
  > 
>
switch_statement(const lambda_functor<TestArg>& a1) { 
  return 
      lambda_functor_base< 
         switch_action<1>, 
         tuple< lambda_functor<TestArg> > 
      > 
    ( tuple<lambda_functor<TestArg> >(a1));
}


#define HELPER(z, N, FOO)                                      \
BOOST_PP_COMMA_IF(N)                                           \
BOOST_PP_CAT(                                                  \
  const tagged_lambda_functor<detail::switch_case_tag<TagData, \
  N>)                                                          \
BOOST_PP_COMMA() Arg##N>& a##N

#define HELPER_LIST(N) BOOST_PP_REPEAT(N, HELPER, FOO)


#define BOOST_LAMBDA_SWITCH_STATEMENT(N)                              \
template <class TestArg,                                              \
          BOOST_LAMBDA_A_I_LIST(N, class TagData),                    \
          BOOST_LAMBDA_A_I_LIST(N, class Arg)>                        \
inline const                                                          \
lambda_functor<                                                       \
  lambda_functor_base<                                                \
        switch_action<BOOST_PP_INC(N),                                \
          BOOST_LAMBDA_A_I_LIST(N, TagData)                           \
        >,                                                            \
    tuple<lambda_functor<TestArg>, BOOST_LAMBDA_A_I_LIST(N, Arg)>     \
  >                                                                   \
>                                                                     \
switch_statement(                                                     \
  const lambda_functor<TestArg>& ta,                                  \
  HELPER_LIST(N)                                                      \
)                                                                     \
{                                                                     \
  return                                                              \
      lambda_functor_base<                                            \
            switch_action<BOOST_PP_INC(N),                            \
              BOOST_LAMBDA_A_I_LIST(N, TagData)                       \
            >,                                                        \
        tuple<lambda_functor<TestArg>, BOOST_LAMBDA_A_I_LIST(N, Arg)> \
      >                                                               \
    ( tuple<lambda_functor<TestArg>, BOOST_LAMBDA_A_I_LIST(N, Arg)>   \
        (ta, BOOST_LAMBDA_A_I_LIST(N, a) ));                          \
}




// Here's the actual generation

#define BOOST_LAMBDA_SWITCH(N)           \
BOOST_LAMBDA_SWITCH_NO_DEFAULT_CASE(N)   \
BOOST_LAMBDA_SWITCH_WITH_DEFAULT_CASE(N)        

// Use this to avoid case 0, these macros work only from case 1 upwards
#define BOOST_LAMBDA_SWITCH_HELPER(z, N, A) \
BOOST_LAMBDA_SWITCH( BOOST_PP_INC(N) )

// Use this to avoid cases 0 and 1, these macros work only from case 2 upwards
#define BOOST_LAMBDA_SWITCH_STATEMENT_HELPER(z, N, A) \
BOOST_LAMBDA_SWITCH_STATEMENT(BOOST_PP_INC(N))

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4065)
#endif

  // up to 9 cases supported (counting default:)
BOOST_PP_REPEAT_2ND(9,BOOST_LAMBDA_SWITCH_HELPER,FOO)
BOOST_PP_REPEAT_2ND(9,BOOST_LAMBDA_SWITCH_STATEMENT_HELPER,FOO)

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

} // namespace lambda 
} // namespace boost


#undef HELPER
#undef HELPER_LIST

#undef BOOST_LAMBDA_SWITCH_HELPER
#undef BOOST_LAMBDA_SWITCH
#undef BOOST_LAMBDA_SWITCH_NO_DEFAULT_CASE
#undef BOOST_LAMBDA_SWITCH_WITH_DEFAULT_CASE

#undef BOOST_LAMBDA_SWITCH_CASE_BLOCK
#undef BOOST_LAMBDA_SWITCH_CASE_BLOCK_LIST

#undef BOOST_LAMBDA_SWITCH_STATEMENT
#undef BOOST_LAMBDA_SWITCH_STATEMENT_HELPER



#endif







