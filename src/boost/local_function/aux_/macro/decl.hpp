
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_DECL_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_DECL_HPP_

#include <boost/local_function/aux_/macro/code_/result.hpp>
#include <boost/local_function/aux_/macro/code_/bind.hpp>
#include <boost/local_function/aux_/macro/code_/functor.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_error.hpp>
#include <boost/scope_exit.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

// PRIVATE //

#define BOOST_LOCAL_FUNCTION_AUX_DECL_OK_(id, typename01, decl_traits) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT(id, typename01, decl_traits) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_BIND(id, typename01, decl_traits) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR(id, typename01, decl_traits) 

#define BOOST_LOCAL_FUNCTION_AUX_DECL_ERROR_(id, typename01, decl_traits) \
    BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS( \
            BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_RETURNS(decl_traits)), \
        /* return specified, so no result type before this macro expansion */ \
        BOOST_PP_TUPLE_EAT(1) \
    , \
        /* even if error, must declare result type to prevent additional */ \
        /* error due to result type appearing before this macro expansion */ \
        BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_DECL \
    )(id) \
    ; /* close eventual previous statements, otherwise it has no effect */ \
    BOOST_MPL_ASSERT_MSG(false, /* always fails (there's an error) */ \
            BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_ERROR_MSG(decl_traits), ())\
    ; /* must close ASSERT macro for eventual use within class scope */

// PUBLIC //

#define BOOST_LOCAL_FUNCTION_AUX_DECL_ARGS_VAR \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (args) )

// Undefine local function bound args global variable. Actual declaration of
// this variable is made using SFINAE mechanisms by each local function macro.
extern boost::scope_exit::detail::undeclared
        BOOST_LOCAL_FUNCTION_AUX_DECL_ARGS_VAR;

// sign_params: parsed parenthesized params.
#define BOOST_LOCAL_FUNCTION_AUX_DECL(id, typename01, decl_traits) \
    BOOST_PP_IIF(BOOST_PP_IS_EMPTY( \
            BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_ERROR_MSG(decl_traits)), \
        BOOST_LOCAL_FUNCTION_AUX_DECL_OK_ \
    , \
        BOOST_LOCAL_FUNCTION_AUX_DECL_ERROR_ \
    )(id, typename01, decl_traits)

#endif // #include guard

