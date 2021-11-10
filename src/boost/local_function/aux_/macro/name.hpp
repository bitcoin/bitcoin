
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_NAME_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_NAME_HPP_

#include <boost/local_function/config.hpp>
#include <boost/local_function/aux_/macro/decl.hpp>
#include <boost/local_function/aux_/macro/code_/functor.hpp>
#include <boost/local_function/detail/preprocessor/keyword/recursive.hpp>
#include <boost/local_function/detail/preprocessor/keyword/inline.hpp>
#include <boost/local_function/aux_/function.hpp>
#include <boost/local_function/aux_/symbol.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

// PRIVATE //

#define BOOST_LOCAL_FUNCTION_AUX_NAME_LOCAL_TYPE_(local_function_name) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (local_type)(local_function_name) )

#define BOOST_LOCAL_FUNCTION_AUX_NAME_INIT_RECURSION_FUNC_ \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (init_recursion) )

#define BOOST_LOCAL_FUNCTION_AUX_NAME_RECURSIVE_FUNC_( \
        is_recursive, local_function_name) \
    BOOST_PP_IIF(is_recursive, \
        local_function_name \
    , \
        BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (nonrecursive_local_function_name) ) \
    )

#define BOOST_LOCAL_FUNCTION_AUX_NAME_END_LOCAL_FUNCTOR_(typename01, \
        local_function_name, is_recursive, \
        local_functor_name, nonlocal_functor_name) \
    /* FUNCTION macro expanded to: typedef class functor ## __LINE__ { ... */ \
    BOOST_PP_EXPR_IIF(is_recursive, \
        /* member var with function name for recursive calls; it cannot be */ \
        /* `const` because it is init after construction (because */ \
        /* constructor doesn't know local function name) */ \
        /* run-time: even when optimizing, recursive calls cannot be */ \
        /* optimized (i.e., they must be via the non-local functor) */ \
        /* because this cannot be a mem ref because its name is not known */ \
        /* by the constructor so it cannot be set by the mem init list */ \
    private: \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_TYPE \
                BOOST_LOCAL_FUNCTION_AUX_NAME_RECURSIVE_FUNC_(is_recursive, \
                        local_function_name); \
        /* run-time: the `init_recursion()` function cannot be called */ \
        /* by the constructor to allow for compiler optimization */ \
        /* (inlining) so it must be public to be called (see below) */ \
    public: \
        inline void BOOST_LOCAL_FUNCTION_AUX_NAME_INIT_RECURSION_FUNC_( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_TYPE& functor) { \
            local_function_name = functor; \
        } \
    ) \
    } BOOST_LOCAL_FUNCTION_AUX_NAME_LOCAL_TYPE_(local_function_name); \
    /* local functor can be passed as tparam only on C++11 (faster) */ \
    BOOST_LOCAL_FUNCTION_AUX_NAME_LOCAL_TYPE_(local_function_name) \
            local_functor_name(BOOST_LOCAL_FUNCTION_AUX_DECL_ARGS_VAR.value); \
    /* non-local functor can always be passed as tparam (but slower) */ \
    BOOST_PP_EXPR_IIF(typename01, typename) \
    BOOST_LOCAL_FUNCTION_AUX_NAME_LOCAL_TYPE_(local_function_name):: \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_TYPE \
            nonlocal_functor_name; /* functor variable */ \
    /* the order of the following 2 function calls cannot be changed */ \
    /* because init_recursion uses the local_functor so the local_functor */ \
    /* must be init first */ \
    local_functor_name.BOOST_LOCAL_FUNCTION_AUX_FUNCTION_INIT_CALL_FUNC( \
            &local_functor_name, nonlocal_functor_name); \
    BOOST_PP_EXPR_IIF(is_recursive, \
        /* init recursion causes MSVC to not optimize local function not */ \
        /* even when local functor is used as template parameter so no */ \
        /* recursion unless all inlining optimizations are specified off */ \
        local_functor_name.BOOST_LOCAL_FUNCTION_AUX_NAME_INIT_RECURSION_FUNC_( \
                nonlocal_functor_name); \
    )

#define BOOST_LOCAL_FUNCTION_AUX_NAME_FUNCTOR_(local_function_name) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (local_function_name) )

// This can always be passed as a template parameters (on all compilers).
// However, it is slower because it cannot be inlined.
// Passed at tparam: Yes (on all C++). Inlineable: No. Recursive: No.
#define BOOST_LOCAL_FUNCTION_AUX_NAME_(typename01, local_function_name) \
    BOOST_LOCAL_FUNCTION_AUX_NAME_END_LOCAL_FUNCTOR_(typename01, \
            local_function_name, \
            /* local function is not recursive (because recursion and its */ \
            /* initialization cannot be inlined even on C++11, */ \
            /* so this allows optimization at least on C++11) */ \
            0 /* not recursive */ , \
            /* local functor */ \
            BOOST_LOCAL_FUNCTION_AUX_NAME_FUNCTOR_(local_function_name), \
            /* local function declared as non-local functor -- but it can */ \
            /* be inlined only by C++11 and it cannot be recursive */ \
            local_function_name)

// This is faster on some compilers but not all (e.g., it is faster on GCC
// because its optimization inlines it but not on MSVC). However, it cannot be
// passed as a template parameter on non C++11 compilers.
// Passed at tparam: Only on C++11. Inlineable: Yes. Recursive: No.
#define BOOST_LOCAL_FUNCTION_AUX_NAME_INLINE_(typename01, local_function_name) \
    BOOST_LOCAL_FUNCTION_AUX_NAME_END_LOCAL_FUNCTOR_(typename01, \
            local_function_name, \
            /* inlined local function is never recursive (because recursion */ \
            /* and its initialization cannot be inlined)*/ \
            0 /* not recursive */ , \
            /* inlined local function declared as local functor (maybe */ \
            /* inlined even by non C++11 -- but it can be passed as */ \
            /* template parameter only on C++11 */ \
            local_function_name, \
            /* non-local functor */ \
            BOOST_LOCAL_FUNCTION_AUX_NAME_FUNCTOR_(local_function_name))

// This is slower on all compilers (C++11 and non) because recursion and its
// initialization can never be inlined.
// Passed at tparam: Yes. Inlineable: No. Recursive: Yes.
#define BOOST_LOCAL_FUNCTION_AUX_NAME_RECURSIVE_( \
        typename01, local_function_name) \
    BOOST_LOCAL_FUNCTION_AUX_NAME_END_LOCAL_FUNCTOR_(typename01, \
            local_function_name, \
            /* recursive local function -- but it cannot be inlined */ \
            1 /* recursive */ , \
            /* local functor */ \
            BOOST_LOCAL_FUNCTION_AUX_NAME_FUNCTOR_(local_function_name), \
            /* local function declared as non-local functor -- but it can */ \
            /* be inlined only by C++11 */ \
            local_function_name)

// Inlined local functions are specified by `..._NAME(inline name)`.
// They have more chances to be inlined for faster run-times by some compilers
// (for example by GCC but not by MSVC). C++11 compilers can always inline
// local functions even if they are not explicitly specified inline.
#define BOOST_LOCAL_FUNCTION_AUX_NAME_PARSE_INLINE_( \
        typename01, qualified_name) \
    BOOST_PP_IIF(BOOST_PP_BITOR( \
            BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS, \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_INLINE_FRONT( \
                    qualified_name)), \
        /* on C++11 always use inlining because compilers might optimize */ \
        /* it to be faster and it can also be passed as tparam */ \
        BOOST_LOCAL_FUNCTION_AUX_NAME_INLINE_ \
    , \
        /* on non C++11 don't use liniling unless explicitly specified by */ \
        /* programmers `inline name` the inlined local function cannot be */ \
        /* passed as tparam */ \
        BOOST_LOCAL_FUNCTION_AUX_NAME_ \
    )(typename01, BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_INLINE_REMOVE_FRONT( \
            qualified_name))

// Expand to 1 iff `recursive name` or `recursive inline name` or
// `inline recursive name`.
#define BOOST_LOCAL_FUNCTION_AUX_NAME_IS_RECURSIVE_(qualified_name) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_RECURSIVE_FRONT( \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_INLINE_REMOVE_FRONT( \
        qualified_name \
    ))

// Revmoes `recursive`, `inline recursive`, and `recursive inline` from front.
#define BOOST_LOCAL_FUNCTION_AUX_NAME_REMOVE_RECURSIVE_AND_INLINE_( \
        qualified_name) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_RECURSIVE_REMOVE_FRONT( \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_INLINE_REMOVE_FRONT( \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_RECURSIVE_REMOVE_FRONT( \
        qualified_name \
    )))

#define BOOST_LOCAL_FUNCTION_AUX_NAME_RECURSIVE_REMOVE_(qualified_name) \
    BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_AUX_NAME_IS_RECURSIVE_(qualified_name), \
        BOOST_LOCAL_FUNCTION_AUX_NAME_REMOVE_RECURSIVE_AND_INLINE_ \
    , \
        qualified_name /* might be `name` or `inline name` */ \
        BOOST_PP_TUPLE_EAT(1) \
    )(qualified_name)

// Recursive local function are specified by `..._NAME(recursive name)`. 
// They can never be inlined for faster run-time (not even by C++11 compilers).
#define BOOST_LOCAL_FUNCTION_AUX_NAME_PARSE_RECURSIVE_( \
        typename01, qualified_name) \
    BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_AUX_NAME_IS_RECURSIVE_(qualified_name), \
        /* recursion can never be inlined (not even on C++11) */ \
        BOOST_LOCAL_FUNCTION_AUX_NAME_RECURSIVE_ \
    , \
        BOOST_LOCAL_FUNCTION_AUX_NAME_PARSE_INLINE_ \
    )(typename01, \
            BOOST_LOCAL_FUNCTION_AUX_NAME_RECURSIVE_REMOVE_(qualified_name))

// PUBLIC //

#define BOOST_LOCAL_FUNCTION_AUX_NAME(typename01, qualified_name) \
    BOOST_LOCAL_FUNCTION_AUX_NAME_PARSE_RECURSIVE_(typename01, qualified_name)

#endif // #include guard

