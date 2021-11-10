// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_DISPATCH_TUPLE_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_DISPATCH_TUPLE_HPP

#include <boost/preprocessor/tuple/elem.hpp>

// Accessor macros for the input tuple to the dispatch macros.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_BASE_NAME(x)                       \
    BOOST_PP_TUPLE_ELEM(5, 0, x)
/**/

#define BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)                      \
    BOOST_PP_TUPLE_ELEM(5, 1, x)
/**/

#define BOOST_PARAMETER_FUNCTION_DISPATCH_IS_MEMBER(x)                       \
    BOOST_PP_TUPLE_ELEM(5, 2, x)
/**/

#define BOOST_PARAMETER_FUNCTION_DISPATCH_IS_CONST(x)                        \
    BOOST_PP_TUPLE_ELEM(5, 3, x)
/**/

#define BOOST_PARAMETER_FUNCTION_DISPATCH_TAG_NAMESPACE(x)                   \
    BOOST_PP_TUPLE_ELEM(5, 4, x)
/**/

#endif  // include guard

