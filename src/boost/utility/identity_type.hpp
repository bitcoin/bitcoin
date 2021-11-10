
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/utility/identity_type

/** @file
Wrap type expressions with round parenthesis so they can be passed to macros
even if they contain commas.
*/

#ifndef BOOST_IDENTITY_TYPE_HPP_
#define BOOST_IDENTITY_TYPE_HPP_

#include <boost/type_traits/function_traits.hpp>

/**
@brief This macro allows to wrap the specified type expression within extra
round parenthesis so the type can be passed as a single macro parameter even if
it contains commas (not already wrapped within round parenthesis).

@Params
@Param{parenthesized_type,
The type expression to be passed as macro parameter wrapped by a single set
of round parenthesis <c>(...)</c>.
This type expression can contain an arbitrary number of commas.
}
@EndParams

This macro works on any C++03 compiler (it does not use variadic macros).

This macro must be prefixed by <c>typename</c> when used within templates.
Note that the compiler will not be able to automatically determine function
template parameters when they are wrapped with this macro (these parameters
need to be explicitly specified when calling the function template).

On some compilers (like GCC), using this macro on abstract types requires to
add and remove a reference to the specified type.
*/
#define BOOST_IDENTITY_TYPE(parenthesized_type) \
    /* must NOT prefix this with `::` to work with parenthesized syntax */ \
    boost::function_traits< void parenthesized_type >::arg1_type

#endif // #include guard

