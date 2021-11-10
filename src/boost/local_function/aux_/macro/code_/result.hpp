
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_HPP_

#include <boost/local_function/aux_/symbol.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_returns.hpp>
#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/cat.hpp>

// PRIVATE //

#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_FUNC_(id) \
    /* symbol (not internal) also gives error if missing result type */ \
    BOOST_PP_CAT( \
  ERROR_missing_result_type_before_the_local_function_parameter_macro_id, \
            id)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_PARAMS_(id) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (deduce_result_params)(id) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE_(id) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (result_type)(id) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_INDEX_ \
    /* this does not have to be an integral index because ScopeExit uses */ \
    /* just as a symbol to concatenate go generate unique symbols (but */ \
    /* if it'd ever needed to became integral, the number of function */ \
    /* params + 1 as in the macro CONFIG_ARITY_MAX could be used) */ \
    result

// User did not explicitly specified result type, deduce it (using Typeof).
#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_DEDUCE_( \
        id, typename01, decl_traits) \
    /* user specified result type here */ \
    BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_DECL(id) \
    /* tagging, wrapping, etc as from ScopeExit type deduction are */ \
    /* necessary within templates (at least on GCC) to work around an */ \
    /* compiler internal errors) */ \
    BOOST_SCOPE_EXIT_DETAIL_TAG_DECL(0, /* no recursive step r */ \
            id, BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_INDEX_, \
            BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_FUNC_(id)) \
    BOOST_SCOPE_EXIT_DETAIL_CAPTURE_DECL(0, /* no recursive step r */ \
            ( id, BOOST_PP_EXPR_IIF(typename01, typename) ), \
            BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_INDEX_, \
            BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_FUNC_(id)) \
    /* extra struct to workaround GCC and other compiler's issues */ \
    struct BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_PARAMS_(id) { \
        typedef \
            BOOST_PP_EXPR_IIF(typename01, typename) \
            ::boost::function_traits< \
                BOOST_PP_EXPR_IIF(typename01, typename) \
                ::boost::remove_pointer< \
                    BOOST_SCOPE_EXIT_DETAIL_CAPTURE_T(id, \
                            BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_INDEX_, \
                            BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_FUNC_(id)) \
                >::type \
            >::result_type \
            BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE_(id) \
        ; \
    };

// Use result type as explicitly specified by user (no type deduction needed).
// Precondition: RETURNS(decl_traits) != NIL
#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPED_( \
        id, typename01, decl_traits) \
    /* user specified result type here */ \
    struct BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_PARAMS_(id) { \
        typedef \
            BOOST_PP_LIST_FIRST( \
                    BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_RETURNS( \
                            decl_traits)) \
            BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE_(id) \
        ; \
    };

// PUBLIC //

#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE(id, typename01) \
    BOOST_PP_EXPR_IIF(typename01, typename) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_PARAMS_(id) :: \
    BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE_(id)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_DECL(id) \
    /* result type here */ (*BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_FUNC_(id))();

#define BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT(id, typename01, decl_traits) \
    BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS( \
            BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_RETURNS(decl_traits)), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPED_ \
    , \
        BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_DEDUCE_ \
    )(id, typename01, decl_traits)

#endif // #include guard

