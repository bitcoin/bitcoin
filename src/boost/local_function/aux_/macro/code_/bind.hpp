
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_HPP_

#include <boost/local_function/aux_/symbol.hpp>
#include <boost/local_function/aux_/macro/decl.hpp>
#include <boost/local_function/aux_/preprocessor/traits/bind.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_binds.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_const_binds.hpp>
#include <boost/utility/identity_type.hpp>
#include <boost/scope_exit.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/list/for_each_i.hpp>
#include <boost/preprocessor/list/append.hpp>

// PRIVATE //

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_PARAMS_VAR_(id) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (params)(id) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_TAG_DECL_(r, id, i, bind_traits) \
    BOOST_SCOPE_EXIT_DETAIL_TAG_DECL(r, id, i, \
            BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_CAPTURE_DECL_TYPED_( \
        r, id, typename01, i, bind_traits) \
    typedef BOOST_PP_EXPR_IIF(typename01, typename) \
        /* remove ref because typed var can have & prefix */ \
        ::boost::remove_reference< BOOST_PP_EXPR_IIF(typename01, typename) \
            ::boost::function_traits< \
                /* instead of using Boost.Typeof, get bind type as 1st */ \
                /* argument type of func type `void (type_ [&] var_)` */ \
                void ( BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITH_TYPE( \
                        bind_traits) ) \
            >::arg1_type \
        >::type \
        BOOST_SCOPE_EXIT_DETAIL_CAPTURE_T(id, i, \
                BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                        bind_traits)) \
    ; /* close typedef */

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_CAPTURE_DECL_DEDUCED_( \
        r, id, typename01, i, bind_traits) \
    BOOST_SCOPE_EXIT_DETAIL_CAPTURE_DECL(r, \
            ( \
                id \
            , \
                /* ScopeExit expects typename or EMPTY() here */ \
                BOOST_PP_EXPR_IIF(typename01, typename) \
            ), \
            i, BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_CAPTURE_DECL_( \
        r, id_typename, i, bind_traits) \
    BOOST_PP_IIF(BOOST_PP_IS_EMPTY( \
  BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITH_TYPE(bind_traits)), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_CAPTURE_DECL_DEDUCED_ \
    , \
        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_CAPTURE_DECL_TYPED_ \
    )(r, BOOST_PP_TUPLE_ELEM(2, 0, id_typename), \
            BOOST_PP_TUPLE_ELEM(2, 1, id_typename), i, bind_traits)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_PARAM_DECL_( \
        r, id_typename, i, bind_traits) \
    BOOST_SCOPE_EXIT_DETAIL_PARAM_DECL(r, \
            ( \
                BOOST_PP_TUPLE_ELEM(2, 0, id_typename) \
            , \
                /* ScopeExit expects typename or EMPTY() here */ \
                BOOST_PP_EXPR_IIF(BOOST_PP_TUPLE_ELEM(2, 1, id_typename), \
                    typename \
                ) \
            ), \
            i, BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_MEMBER_DECL_VAR_( \
        r, id, typename01, i, var) \
    BOOST_PP_EXPR_IIF(typename01, typename) \
    BOOST_IDENTITY_TYPE(( /* must use IDENTITY because of tparam comma */ \
        ::boost::scope_exit::detail::member< \
              BOOST_SCOPE_EXIT_DETAIL_PARAM_T(id, i, var) \
            , BOOST_SCOPE_EXIT_DETAIL_TAG(id, i) \
        > \
    )) \
    BOOST_SCOPE_EXIT_DETAIL_PARAM(id, i, var);

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_MEMBER_DECL_( \
        r, id_typename, i, bind_traits) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_MEMBER_DECL_VAR_(r, \
            BOOST_PP_TUPLE_ELEM(2, 0, id_typename), \
            BOOST_PP_TUPLE_ELEM(2, 1, id_typename), \
            i, BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_PARAM_INIT_(r, id, i, bind_traits) \
    BOOST_SCOPE_EXIT_DETAIL_PARAM_INIT(r, id, i, \
            BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_CAPTURE_TYPE_(id) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (this_capture_type)(id) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE_(id) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (this_type)(id) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPEDEF_DEDUCED_( \
        id, typename01, all_bind_this_types) \
    BOOST_SCOPE_EXIT_DETAIL_TYPEDEF_TYPEOF_THIS(id, \
            BOOST_PP_EXPR_IIF(typename01, typename), /* otherwise EMPTY() */ \
            BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_CAPTURE_TYPE_(id))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPEDEF_TYPED_( \
        id, typename01, all_bind_this_types) \
    typedef \
        BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_THIS_TYPE(BOOST_PP_LIST_FIRST( \
                all_bind_this_types)) \
        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_CAPTURE_TYPE_(id) \
    ;
    
#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPEDEF_( \
        id, typename01, all_bind_this_types) \
    /* typedef type_ */ \
    BOOST_PP_IIF(BOOST_PP_IS_EMPTY( \
            /* all_bind_this_type is list with 1 elem (possibly PP_EMPTY), */ \
            /* otherwise got a pp-parsing error before getting here */ \
            BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_THIS_TYPE( \
                    BOOST_PP_LIST_FIRST(all_bind_this_types))), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPEDEF_DEDUCED_ \
    , \
        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPEDEF_TYPED_ \
    )(id, typename01, all_bind_this_types)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_ALL_( \
        all_binds, all_bind_this_types, id, typename01) \
    /* binding tags */ \
    BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS(all_bind_this_types), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPEDEF_ \
    , \
        BOOST_PP_TUPLE_EAT(3) \
    )(id, typename01, all_bind_this_types) \
    BOOST_PP_LIST_FOR_EACH_I(BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_TAG_DECL_, id, \
            all_binds) \
    BOOST_PP_LIST_FOR_EACH_I(BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_CAPTURE_DECL_, \
            (id, typename01), all_binds) \
    /* binding class */ \
    struct BOOST_SCOPE_EXIT_DETAIL_PARAMS_T(id) { \
        /* interim capture types to workaround internal error on old GCC */ \
        BOOST_PP_EXPR_IIF(BOOST_PP_LIST_IS_CONS(all_bind_this_types), \
            typedef BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_CAPTURE_TYPE_(id) \
                    BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE_(id) ; \
        ) \
        BOOST_PP_EXPR_IIF(BOOST_PP_LIST_IS_CONS(all_bind_this_types), \
            BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE_(id) \
            BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_VAR; \
        ) \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_PARAM_DECL_, \
                (id, typename01), all_binds) \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_MEMBER_DECL_, \
                (id, typename01), all_binds) \
    } BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_PARAMS_VAR_(id) = \
    /* NOTE: there is no way to wrap member initializer commas within */ \
    /* parenthesis so you must handle these commas manually if expanding */ \
    /* this macro within another macro */ \
    { \
        /* initialize the struct with param values to bind */ \
        BOOST_PP_EXPR_IIF(BOOST_PP_LIST_IS_CONS(all_bind_this_types), \
                this) /* here name `this` access object at enclosing scope */ \
        BOOST_PP_COMMA_IF(BOOST_PP_BITAND( \
              BOOST_PP_LIST_IS_CONS(all_bind_this_types) \
            , BOOST_PP_LIST_IS_CONS(all_binds) \
        )) \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_PARAM_INIT_, id, all_binds) \
    };

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_(id, typename01, decl_traits) \
    /* IMPORTANT: the order of these appends is important, it must follow */ \
    /* the indexing order used by the functor code which starts */ \
    /* enumerating const binds and then non-const binds */ \
    BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_ALL_( \
            BOOST_PP_LIST_APPEND( \
                    BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_CONST_BINDS( \
                            decl_traits),\
                    BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_BINDS( \
                            decl_traits)), \
            BOOST_PP_LIST_APPEND( \
  BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_CONST_BIND_THIS_TYPES(decl_traits), \
                    BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_BIND_THIS_TYPES( \
                            decl_traits)), \
            id, typename01)

// PUBLIC //

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, typename01) \
    BOOST_PP_EXPR_IIF(typename01, typename) \
    BOOST_SCOPE_EXIT_DETAIL_PARAMS_T(id):: \
            BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE_(id)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_VAR \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (this_var) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_BIND(id, typename01, decl_traits) \
    /* the binding data structures must be declared and initialized (to */ \
    /* empty structs, so hopefully the compiler will optimize away the */ \
    /* no-op code) even when there is no bound param because these structs */ \
    /* are used to init `...args.value` which is always used by the `END` */ \
    /* macro later because this macro does not know if there are bound */ \
    /* params or not */ \
    BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_(id, typename01, decl_traits) \
    /* this code takes advantage of the template argument list/comparison */ \
    /* operator ambiguity to declare a variable iff it hasn't already been */ \
    /* declared in that scope; the second occurrence is parsed as: */ \
    /*  (declared<(resolve<sizeof(boost_local_auxXargs)>::cmp1<0)>::cmp2> */ \
    /*      ...Xargs); */ \
    /* which is a no-op */ \
    ::boost::scope_exit::detail::declared< boost::scope_exit::detail::resolve< \
        /* cannot prefix with `::` as in `sizeof(:: ...` because the name */ \
        /* must refer to the local variable name to allow multiple local */ \
        /* functions (and exits) within the same scope (however this */ \
        /* does not allow for nesting because local variables cannot be */ \
        /* used in nested code blocks) */ \
        sizeof(BOOST_LOCAL_FUNCTION_AUX_DECL_ARGS_VAR) \
    >::cmp1<0>::cmp2 > BOOST_LOCAL_FUNCTION_AUX_DECL_ARGS_VAR; \
    /* stores bound types/values into `...args` variable (args variable */ \
    /* can be accessed by `NAME` macro because doesn't use __LINE__ id) */ \
    BOOST_LOCAL_FUNCTION_AUX_DECL_ARGS_VAR.value = \
            &BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_PARAMS_VAR_(id);

#endif // #include guard

