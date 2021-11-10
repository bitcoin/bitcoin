// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_CONFIG_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_CONFIG_HPP_INCLUDED

#ifndef BOOST_TYPE_ERASURE_MAX_FUNCTIONS
/** The maximum number of functions that an @ref boost::type_erasure::any "any" can have. */
#define BOOST_TYPE_ERASURE_MAX_FUNCTIONS 50
#endif
#ifndef BOOST_TYPE_ERASURE_MAX_ARITY
/** The maximum number of arguments that functions in the library support. */
#define BOOST_TYPE_ERASURE_MAX_ARITY 5
#endif
#ifndef BOOST_TYPE_ERASURE_MAX_TUPLE_SIZE
/** The maximum number of elements in a @ref boost::type_erasure::tuple "tuple". */
#define BOOST_TYPE_ERASURE_MAX_TUPLE_SIZE 5
#endif

#endif
