// -- Boost Lambda Library -------------------------------------------------

// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// --------------------------------------------------

#ifndef BOOST_LAMBDA_ARITY_CODE_HPP
#define BOOST_LAMBDA_ARITY_CODE_HPP

#include "boost/type_traits/cv_traits.hpp"
#include "boost/type_traits/transform_traits.hpp"

namespace boost { 
namespace lambda {

// These constants state, whether a lambda_functor instantiation results from 
// an expression which contains no placeholders (NONE), 
// only free1 placeholders (FIRST), 
// free2 placeholders and maybe free1 placeholders (SECOND),
// free3 and maybe free1 and free2 placeholders (THIRD),
// freeE placeholders and maybe free1 and free2  (EXCEPTION).
// RETHROW means, that a rethrow expression is used somewhere in the lambda_functor.

enum { NONE             = 0x00, // Notice we are using bits as flags here.
       FIRST            = 0x01, 
       SECOND           = 0x02, 
       THIRD            = 0x04, 
       EXCEPTION        = 0x08, 
       RETHROW          = 0x10};


template<class T>
struct get_tuple_arity;

namespace detail {

template <class T> struct get_arity_;

} // end detail;

template <class T> struct get_arity {

  BOOST_STATIC_CONSTANT(int, value = detail::get_arity_<typename boost::remove_cv<typename boost::remove_reference<T>::type>::type>::value);

};

namespace detail {

template<class T>
struct get_arity_ {
  BOOST_STATIC_CONSTANT(int, value = 0);
};

template<class T>
struct get_arity_<lambda_functor<T> > {
  BOOST_STATIC_CONSTANT(int, value = get_arity<T>::value);
};

template<class Action, class Args>
struct get_arity_<lambda_functor_base<Action, Args> > {
  BOOST_STATIC_CONSTANT(int, value = get_tuple_arity<Args>::value);
};

template<int I>
struct get_arity_<placeholder<I> > {
  BOOST_STATIC_CONSTANT(int, value = I);
};

} // detail 

template<class T>
struct get_tuple_arity {
  BOOST_STATIC_CONSTANT(int, value = get_arity<typename T::head_type>::value | get_tuple_arity<typename T::tail_type>::value);
};


template<>
struct get_tuple_arity<null_type> {
  BOOST_STATIC_CONSTANT(int, value = 0);
};


  // Does T have placeholder<I> as it's subexpression?

template<class T, int I>
struct has_placeholder {
  BOOST_STATIC_CONSTANT(bool, value = (get_arity<T>::value & I) != 0);
}; 

template<int I, int J>
struct includes_placeholder {
  BOOST_STATIC_CONSTANT(bool, value = (J & I) != 0);
};

template<int I, int J>
struct lacks_placeholder {
  BOOST_STATIC_CONSTANT(bool, value = ((J & I) == 0));
};


} // namespace lambda
} // namespace boost

#endif
