
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_HPP_

#include <boost/preprocessor/tuple/elem.hpp>

// PRIVATE //

// Param 2-tuple `([auto | register] type name, default_value)`.
#define BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_INDEX_DECL_    0
#define BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_INDEX_DEFAULT_ 1
#define BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_INDEX_MAX_     2

// PUBLIC //

// Expand: `[auto | register] type_ name_` (parameter declaration).
#define BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_DECL(param_traits) \
    BOOST_PP_TUPLE_ELEM(BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_INDEX_MAX_, \
            BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_INDEX_DECL_, param_traits)

// Expand: `default ... EMPTY()` if default value, `EMPTY()` otherwise.
// Leading default is kept because default value might not be alphanumeric
// (e.g., -123) so failing `CAT` for `IS_EMPTY` check.
#define BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_DEFAULT(param_traits) \
    BOOST_PP_TUPLE_ELEM(BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_INDEX_MAX_, \
            BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_INDEX_DEFAULT_, \
            param_traits)(/* expand EMPTY */)

#endif // #include guard

