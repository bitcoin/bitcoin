
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_CONFIG_HPP_
#define BOOST_LOCAL_FUNCTION_CONFIG_HPP_

#ifndef DOXYGEN

#include <boost/config.hpp>

#ifndef BOOST_LOCAL_FUNCTION_CONFIG_FUNCTION_ARITY_MAX
#   define BOOST_LOCAL_FUNCTION_CONFIG_FUNCTION_ARITY_MAX 5
#endif

#ifndef BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX
#   define BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX 10
#endif

#ifndef BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
#   ifdef BOOST_NO_CXX11_LOCAL_CLASS_TEMPLATE_PARAMETERS
#       define BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS 0
#   else
#       define BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS 1
#   endif
#elif BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS // If true, force it to 1.
#   undef BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS
#   define BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS 1
#endif

#else // DOXYGEN

/** @file
@brief Configuration macros allow to change the behaviour of this library at
compile-time.
*/

/**
@brief Maximum number of parameters supported by local functions.

If programmers leave this configuration macro undefined, its default
value is <c>5</c> (increasing this number might increase compilation time).
When defined by programmers, this macro must be a non-negative integer number.

@Note This macro specifies the maximum number of local function parameters
excluding bound variables (which are instead specified by
@RefMacro{BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX}).

@See @RefSect{tutorial, Tutorial} section,
@RefSect{getting_started, Getting Started} section,
@RefMacro{BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX}.
*/
#define BOOST_LOCAL_FUNCTION_CONFIG_ARITY_MAX

/**
@brief Maximum number of bound variables supported by local functions.

If programmers leave this configuration macro undefined, its default
value is <c>10</c> (increasing this number might increase compilation time).
When defined by programmers, this macro must be a non-negative integer number.

@Note This macro specifies the maximum number of bound variables excluding
local function parameters (which are instead specified by
@RefMacro{BOOST_LOCAL_FUNCTION_CONFIG_ARITY_MAX}).

@See @RefSect{tutorial, Tutorial} section,
@RefSect{getting_started, Getting Started} section,
@RefMacro{BOOST_LOCAL_FUNCTION_CONFIG_ARITY_MAX}.
*/
#define BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX

/**
@brief Specify when local functions can be passed as template parameters
without introducing any run-time overhead.

If this macro is defined to <c>1</c>, this library will assume that the
compiler allows to pass local classes as template parameters:
@code
    template<typename T> void f(void) {}

    int main(void) {
        struct local_class {};
        f<local_class>();
        return 0;
    }
@endcode
This is the case for C++11 compilers and some C++03 compilers (e.g., MSVC), but
it is not the case in general for most C++03 compilers (including GCC).
This will allow the library to pass local functions as template parameters
without introducing any run-time overhead (specifically without preventing the
compiler from optimizing local function calls by inlining their assembly code).

If this macro is defined to <c>0</c> instead, this library will introduce
a run-time overhead associated to resolving a function pointer call in order to
still allow to pass the local functions as template parameters.

It is recommended to leave this macro undefined.
In this case, the library will automatically define this macro to <c>0</c> if
the Boost.Config macro <c>BOOST_NO_CXX11_LOCAL_CLASS_TEMPLATE_PARAMETERS</c> is
defined for the specific compiler, and to <c>1</c> otherwise.

@See @RefSect{getting_started, Getting Started} section,
@RefSect{advanced_topics, Advanced Topics} section,
@RefMacro{BOOST_LOCAL_FUNCTION_NAME}.
*/
#define BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS

#endif // DOXYGEN

#endif // #include guard

