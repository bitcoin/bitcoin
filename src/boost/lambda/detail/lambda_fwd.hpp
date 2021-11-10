//  lambda_fwd.hpp - Boost Lambda Library -------------------------------

// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// -------------------------------------------------------

#ifndef BOOST_LAMBDA_FWD_HPP
#define BOOST_LAMBDA_FWD_HPP

namespace boost { 
namespace lambda { 

namespace detail {

template<class T> struct generate_error;

}   
// -- placeholders --------------------------------------------

template <int I> struct placeholder;

// function_adaptors
template <class Func> 
struct function_adaptor;

template <int I, class Act> class action;

template <class Base> 
class lambda_functor;

template <class Act, class Args> 
class lambda_functor_base;

} // namespace lambda
} // namespace boost


//  #define CALL_TEMPLATE_ARGS class A, class Env
//  #define CALL_FORMAL_ARGS A& a, Env& env
//  #define CALL_ACTUAL_ARGS a, env
//  #define CALL_ACTUAL_ARGS_NO_ENV a
//  #define CALL_REFERENCE_TYPES A&, Env&
//  #define CALL_PLAIN_TYPES A, Env
#define CALL_TEMPLATE_ARGS class A, class B, class C, class Env
#define CALL_FORMAL_ARGS A& a, B& b, C& c, Env& env
#define CALL_ACTUAL_ARGS a, b, c, env
#define CALL_ACTUAL_ARGS_NO_ENV a, b, c
#define CALL_REFERENCE_TYPES A&, B&, C&, Env&
#define CALL_PLAIN_TYPES A, B, C, Env

namespace boost {
namespace lambda {
namespace detail {

template<class A1, class A2, class A3, class A4>
void do_nothing(A1&, A2&, A3&, A4&) {}

} // detail
} // lambda
} // boost

// prevent the warnings from unused arguments
#define CALL_USE_ARGS \
::boost::lambda::detail::do_nothing(a, b, c, env)



#endif
