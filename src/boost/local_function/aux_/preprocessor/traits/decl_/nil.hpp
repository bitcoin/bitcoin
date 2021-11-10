
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_NIL_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_NIL_HPP_

#include <boost/preprocessor/facilities/empty.hpp>

#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_NIL \
    ( \
        /* returns: list of `return result_type` */ \
        BOOST_PP_NIL /* nil list */ \
    , \
        /* params: list of 2-tuples (param_decl, param_default) */ \
        BOOST_PP_NIL /* nil list */ \
    , \
        /* const-binds: list of 2-tuple `(var_untyped, var_typed)` */ \
        BOOST_PP_NIL /* nil list */ \
    , \
        /* const-bind this: list of 1-tuple `(type)` */ \
        BOOST_PP_NIL \
      /* number of const-bind `this` */ \
    , \
        /* binds: list of 2-tuple `(var_untyped, var_typed)` */ \
        BOOST_PP_NIL /* nil list */ \
    , \
        /* bind this: list of 1-type `(type)` */ \
        BOOST_PP_NIL \
    , \
        /* error: `[ERROR_message_text] EMPTY` */ \
        BOOST_PP_EMPTY /* no error */ \
    )

#endif // #include guard

