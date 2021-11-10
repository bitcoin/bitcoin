
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_HPP_

#include <boost/local_function/config.hpp>
#include <boost/local_function/aux_/symbol.hpp>
#include <boost/local_function/aux_/function.hpp>
#include <boost/local_function/aux_/add_pointed_const.hpp>
#include <boost/local_function/aux_/member.hpp>
#include <boost/local_function/aux_/nobind.hpp>
#include <boost/local_function/aux_/macro/decl.hpp>
#include <boost/local_function/aux_/macro/typeof.hpp>
#include <boost/local_function/aux_/macro/code_/result.hpp>
#include <boost/local_function/aux_/macro/code_/bind.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_params.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_binds.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_const_binds.hpp>
#include <boost/local_function/detail/preprocessor/keyword/auto.hpp>
#include <boost/local_function/detail/preprocessor/keyword/register.hpp>
#include <boost/local_function/detail/preprocessor/keyword/thisunderscore.hpp>
#include <boost/utility/identity_type.hpp>
#include <boost/scope_exit.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/logical/compl.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/tuple/rem.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/list/size.hpp>
#include <boost/preprocessor/list/for_each_i.hpp>
#include <boost/preprocessor/list/first_n.hpp>

// PRIVATE //

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CLASS_TYPE_(id) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (functor)(id) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_F_TYPE_ \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (function_type) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BODY_FUNC_ \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (body) )

// Unbind parameters.

// i: 1 for 1st param, 2 for 2nd, ... (start from 1 not 0).
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_NAME_(i) \
    /* this must be a generic parameter name because type and name */ \
    /* are not separate tokens in the macro syntax so name is not available */ \
    /* separately from its type */ \
    BOOST_PP_CAT(arg, i)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_NAME_ENUM_( \
        r, unused, i, param_traits) \
    BOOST_PP_COMMA_IF(i) /* enumeration commas */ \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_NAME_(BOOST_PP_INC(i))

// i: 1 for 1st param, 2 for 2nd, ... (start from 1 not 0).
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_TYPE_(typename01, i) \
    /* the parameter type must be accessed using function traits from */ \
    /* function type because it is not available to the macro syntax */ \
    /* separately from the parameter name */ \
    BOOST_PP_EXPR_IIF(typename01, typename) \
    ::boost::function_traits< \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_F_TYPE_ \
    >::BOOST_PP_CAT(BOOST_PP_CAT(arg, i), _type) \

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_ARG_TYPEDEF_( \
        r, typename01, i, param_traits) \
    typedef \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_TYPE_(typename01, \
                BOOST_PP_INC(i)) \
        /* name must follow Boost.FunctionTraits arg1_type, arg2_type, ... */ \
        BOOST_PP_CAT(BOOST_PP_CAT(arg, BOOST_PP_INC(i)), _type) \
    ;

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_DECL_( \
        r, typename01, i, param_traits) \
    BOOST_PP_EXPR_IIF(typename01, typename) \
    ::boost::call_traits< \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_TYPE_(typename01, \
                BOOST_PP_INC(i)) \
    >::param_type \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_NAME_(BOOST_PP_INC(i))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_DECL_ENUM_( \
        r, typename01, i, param_traits) \
    BOOST_PP_COMMA_IF(i) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_DECL_(r, typename01, i, \
            param_traits)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_PARAM_ARG_DECL_( \
        r, typename01, i, param_traits) \
    , BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_DECL_(r, typename01, i, \
            param_traits)

// Precondition: !EMPTY(DEFAULT(param_traits))
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_DEFAULT_ASSIGNMENT_( \
        param_traits) \
    = BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_DEFAULT_REMOVE_FRONT( \
            BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_DEFAULT(param_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_DECL_ENUM_( \
        r, default01, i, param_traits) \
    BOOST_PP_COMMA_IF(i) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_AUTO_REMOVE_BACK( \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_REGISTER_REMOVE_BACK( \
        BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_DECL(param_traits) \
    )) \
    BOOST_PP_IIF(BOOST_PP_COMPL(default01), \
        BOOST_PP_TUPLE_EAT(1) /* without default */ \
    , BOOST_PP_IIF(BOOST_PP_IS_EMPTY( \
            BOOST_LOCAL_FUNCTION_AUX_PP_PARAM_TRAITS_DEFAULT(param_traits)), \
        BOOST_PP_TUPLE_EAT(1) /* has no default */ \
    , /* else, with default and has default */ \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_DEFAULT_ASSIGNMENT_ \
    ))(param_traits)

// Bound parameters.

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAMS_ \
    bind_params /* constructor void* param */

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_VAR_(i) \
    /* named `bind0`, `bind1`, ... */ \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (bind)(i) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_THIS_ \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (bind_this) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_STATIC_BIND_MEMBER_THIS_( \
        id) \
    , static_cast< BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CLASS_TYPE_(id)* >( \
            object)->BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_THIS_

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_TYPE_( \
        id, typename01, offset, i, bind_var_without_type) \
    BOOST_PP_EXPR_IIF(typename01, typename) \
    BOOST_SCOPE_EXIT_DETAIL_PARAMS_T(id):: \
    BOOST_SCOPE_EXIT_DETAIL_PARAM_T(id, BOOST_PP_ADD(i, offset), \
            bind_var_without_type) \

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_MEMBER_BIND_ENUM_( \
        r, offset, i, bind_traits) \
    BOOST_PP_COMMA_IF(i) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_VAR_( \
            BOOST_PP_ADD(offset, i))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_TYPE_( \
        r, id_typename_offset_const, i, bind_var_without_type) \
    /* IMPORTANT: here can't use `PP_KEYWORD_IS_THISUNDERSCORE_FRONT()` */ \
    /* because some `param_name` might start with non-alphanumeric symbol */ \
    /* `&` (but that is never the case for `this`) */ \
    BOOST_PP_IIF(BOOST_PP_COMPL(BOOST_PP_TUPLE_ELEM(4, 3, \
            id_typename_offset_const)), \
        BOOST_PP_EMPTY \
    , BOOST_PP_IIF( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_THISUNDERSCORE_BACK( \
                    bind_var_without_type), \
        /* pointed obj const */ \
        BOOST_PP_EXPR_IIF(BOOST_PP_TUPLE_ELEM(4, 1, id_typename_offset_const), \
            typename \
        ) \
        BOOST_PP_IDENTITY( ::boost::local_function::aux::add_pointed_const< ) \
    , \
        BOOST_PP_EXPR_IIF(BOOST_PP_TUPLE_ELEM(4, 1, id_typename_offset_const), \
            typename \
        ) \
        BOOST_PP_IDENTITY( ::boost::add_const< ) /* outer type const */ \
    ))() \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_TYPE_( \
            BOOST_PP_TUPLE_ELEM(4, 0, id_typename_offset_const), \
            BOOST_PP_TUPLE_ELEM(4, 1, id_typename_offset_const), \
            BOOST_PP_TUPLE_ELEM(4, 2, id_typename_offset_const), \
            i, bind_var_without_type) \
    BOOST_PP_EXPR_IIF(BOOST_PP_TUPLE_ELEM(4, 3, id_typename_offset_const), \
        >::type \
    )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_( \
        r, id_typename_offset_const, i, bind_var_without_type) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_TYPE_( \
            r, id_typename_offset_const, i, bind_var_without_type) \
    BOOST_PP_IIF( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_THISUNDERSCORE_BACK( \
                    BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                            bind_var_without_type)), \
        this_ BOOST_PP_TUPLE_EAT(1) \
    , \
        BOOST_PP_TUPLE_REM(1) \
    )(bind_var_without_type)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_BIND_TYPE_( \
        r, id_typename_offset_const, i, bind_traits) \
    , BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_TYPE_( \
            r, id_typename_offset_const, i, \
            BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_PARAM_( \
        offset, i) \
    BOOST_PP_CAT(bind, BOOST_PP_ADD(offset, i))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_PARAM_ENUM_( \
        r, offset, i, bind_traits) \
    BOOST_PP_COMMA_IF(i) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_PARAM_(offset, i)

#define \
BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_BIND_PARAM_DECL_( \
        r, id_typename_offset_const, i, bind_traits) \
    , BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_TYPE_( \
            r, id_typename_offset_const, i, \
            BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits)) & \
      BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_PARAM_( \
            BOOST_PP_TUPLE_ELEM(4, 2, id_typename_offset_const), i)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_BIND_THIS_TYPE_( \
        id, typename01) \
    , BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, typename01)
      
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_THIS_PARAM_ \
    bind_this

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_BIND_THIS_PARAM_DECL_( \
        id, typename01) \
    , BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, typename01) & \
      BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_THIS_PARAM_

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_NOBIND_(z, n, unused) \
    , ::boost::local_function::aux::nobind

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_NOBIND_TYPE_(z, n, unused) \
    , ::boost::local_function::aux::nobind_t

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_NOBIND_PARAM_DECL_( \
        z, n, unused) \
    , ::boost::local_function::aux::nobind_t & \
    /* param name not needed here because no bind param not used */

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_ENUM_( \
        r, id_typename_offset_const, i, bind_traits) \
    BOOST_PP_COMMA_IF(i) /* enumeration commas */ \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_( \
            r, id_typename_offset_const, i, \
            BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits))

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_DECL_( \
        r, id_typename_offset_const, i, bind_traits) \
    BOOST_PP_EXPR_IIF(BOOST_PP_TUPLE_ELEM(4, 1, id_typename_offset_const), \
        typename \
    ) \
    ::boost::local_function::aux::member_type< \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_TYPE_( \
                r, id_typename_offset_const, i, bind_var_without_type) \
    >::reference \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_VAR_( \
            BOOST_PP_ADD(i, BOOST_PP_TUPLE_ELEM(4, 2, \
                    id_typename_offset_const))) \
    ; /* end member variable declaration */

#define \
BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_STATIC_BIND_MEMBER_( \
        r, id_typename_offset_const, i, bind_traits) \
    , static_cast< BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CLASS_TYPE_( \
            BOOST_PP_TUPLE_ELEM(4, 0, id_typename_offset_const))* >(object)-> \
      BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_VAR_( \
            BOOST_PP_ADD(i, BOOST_PP_TUPLE_ELEM(4, 2, \
            id_typename_offset_const)))

#define \
BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_INIT_ENUM_( \
        r, id_offset, i, bind_traits) \
    BOOST_PP_COMMA_IF(i) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_VAR_( \
            BOOST_PP_ADD(i, BOOST_PP_TUPLE_ELEM(2, 1, id_offset))) \
    ( /* member variable initialization */ \
        static_cast< \
            BOOST_SCOPE_EXIT_DETAIL_PARAMS_T( \
                    BOOST_PP_TUPLE_ELEM(2, 0, id_offset))* \
        >(BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAMS_)-> \
        BOOST_SCOPE_EXIT_DETAIL_PARAM( \
              BOOST_PP_TUPLE_ELEM(2, 0, id_offset) \
            , BOOST_PP_ADD(i, BOOST_PP_TUPLE_ELEM(2, 1, id_offset)) \
            , BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                    bind_traits) \
        ).value \
    )

// Typeof type-definitions.

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_TYPEDEF_( \
        r, id_typename_offset_const, i, bind_traits) \
    typedef /* the type with the special typeof name */ \
        BOOST_LOCAL_FUNCTION_AUX_TYPEOF_TYPE( \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_( \
                    r, id_typename_offset_const, i, \
                    BOOST_LOCAL_FUNCTION_AUX_PP_BIND_TRAITS_VAR_WITHOUT_TYPE( \
                            bind_traits)) \
        ) \
    ; /* end typedef */

// Expand to the function type `R (A1, ...)`.
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_F_( \
        id, typename01, decl_traits, has_type, function_type) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE(id, typename01) \
    BOOST_PP_EXPR_IIF(has_type, (function_type) ) \
    ( \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_DECL_ENUM_, \
                0, /* without defaults */ \
                BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_PARAMS(decl_traits)) \
    )

// Functor call operations.

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_BODY_(id, typename01, \
        const_bind_macro, bind_macro, const_bind_this_macro, bind_this_macro, \
        param_macro, params, \
        const_binds, has_const_bind_this, binds, has_bind_this) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BODY_FUNC_( \
        BOOST_PP_LIST_FOR_EACH_I(const_bind_macro, \
                0 /* no offset */, const_binds) \
        /* pass plain binds */ \
        BOOST_PP_COMMA_IF( \
            BOOST_PP_BITAND( \
                  BOOST_PP_LIST_IS_CONS(const_binds) \
                , BOOST_PP_LIST_IS_CONS(binds) \
            ) \
        ) \
        BOOST_PP_LIST_FOR_EACH_I(bind_macro, \
                /* offset index of # const-binds (could be 0) */ \
                BOOST_PP_LIST_SIZE(const_binds), binds) \
        /* pass bind `this` */ \
        BOOST_PP_COMMA_IF( \
            BOOST_PP_BITAND( \
                  BOOST_PP_BITOR( \
                      BOOST_PP_LIST_IS_CONS(const_binds) \
                    , BOOST_PP_LIST_IS_CONS(binds) \
                  ) \
                , BOOST_PP_BITOR(has_const_bind_this, has_bind_this) \
            ) \
        ) \
        BOOST_PP_EXPR_IIF(has_const_bind_this, const_bind_this_macro) \
        BOOST_PP_EXPR_IIF(has_bind_this, bind_this_macro) \
        /* pass params */ \
        BOOST_PP_COMMA_IF( \
            BOOST_PP_BITAND( \
                  BOOST_PP_BITOR( \
                      BOOST_PP_BITOR( \
                          BOOST_PP_LIST_IS_CONS(const_binds) \
                        , BOOST_PP_LIST_IS_CONS(binds) \
                      ) \
                    , BOOST_PP_BITOR(has_const_bind_this, has_bind_this) \
                  ) \
                , BOOST_PP_LIST_IS_CONS(params) \
            ) \
        ) \
        BOOST_PP_LIST_FOR_EACH_I(param_macro, ~, params) \
    )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_(z, defaults_n, \
        id, typename01, decl_traits, params, \
        const_binds, has_const_bind_this, binds, has_bind_this) \
    inline BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE(id, typename01) \
    operator()( \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_DECL_ENUM_, \
                typename01, params) \
    ) /* cannot be const because of binds (same as for global fctor) */ { \
        /* just forward call to member function with local func name */ \
        return BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_BODY_(id, typename01,\
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_MEMBER_BIND_ENUM_, \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_MEMBER_BIND_ENUM_, \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_THIS_, \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_THIS_, \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_NAME_ENUM_, \
                params, const_binds, has_const_bind_this, binds, \
                has_bind_this); \
    }

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_FUNC_( \
        z, defaults_n, unused) \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (call)(defaults_n) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_COMMA_FUNC_ADDR_( \
        z, defaults_n, unused) \
    , &BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_FUNC_(z, defaults_n, ~)

// Precondition: NO_LOCAL_TYPES_AS_TEMPLATE_PARAMS.
#define \
BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_COMMA_BIND_PARAM_DECLS_( \
        id, typename01, const_binds, has_const_bind_this, binds, has_bind_this)\
    BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_BIND_PARAM_DECL_, \
            ( id, typename01, 0 /* no offset */, 1 /* const */ ), const_binds) \
    BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_BIND_PARAM_DECL_, \
            /* offset of # of const-binds */ \
            ( id, typename01, BOOST_PP_LIST_SIZE(const_binds), 0 /* const */ ),\
            binds) \
    BOOST_PP_IIF(BOOST_PP_BITOR(has_bind_this, \
            has_const_bind_this), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_BIND_THIS_PARAM_DECL_ \
    , \
        BOOST_PP_TUPLE_EAT(2) \
    )(id, typename01) \
    /* fill with nobind_t (if no local-types as tparams) */ \
    BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX, \
            BOOST_PP_IIF(BOOST_PP_BITOR(has_bind_this, \
                    has_const_bind_this), \
                BOOST_PP_INC \
            , \
                BOOST_PP_TUPLE_REM(1) \
            )(BOOST_PP_LIST_SIZE(BOOST_PP_LIST_APPEND(const_binds, \
                    binds)))), \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_NOBIND_PARAM_DECL_, ~)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_OPERATOR_( \
        id, typename01, \
        params, const_binds, has_const_bind_this, binds, has_bind_this) \
    operator()( \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_NAME_ENUM_, ~, \
                params) \
    )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_BODY_( \
        id, typename01, \
        params, const_binds, has_const_bind_this, binds, has_bind_this) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_BODY_(id, typename01, \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_PARAM_ENUM_, \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_PARAM_ENUM_, \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_THIS_PARAM_, \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_THIS_PARAM_, \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_ARG_NAME_ENUM_, \
            params, const_binds, has_const_bind_this, binds, has_bind_this)

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_(z, defaults_n, \
        id, typename01, decl_traits, params, \
        const_binds, has_const_bind_this, binds, has_bind_this) \
    inline static BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE(id, typename01) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_FUNC_(z, defaults_n, ~)( \
        void* object \
        BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS, \
            BOOST_PP_TUPLE_EAT(6) \
        , \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_COMMA_BIND_PARAM_DECLS_ \
        )(id, typename01, \
                const_binds, has_const_bind_this, binds, has_bind_this) \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_PARAM_ARG_DECL_, \
                typename01, params) \
    ) { \
        /* run-time: casting object to this class type and forward call to */ \
        /* `operator()` (this performs better than doing multiple casting */ \
        /* or using a casted object local variable here to call body */ \
        /* directly from here without passing via `operator()`) */ \
        /* compliance: passing local class type to `static_cast` is fully */ \
        /* C++03 compliant because `static_cast` is not a template (even */ \
        /* if its syntax resembles a function template call) in fact even */ \
        /* in C is legal to cast to a local struct (using C-style casting) */ \
        return \
            static_cast< \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CLASS_TYPE_(id)* \
            >(object)-> \
            BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS,\
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_OPERATOR_ \
            , \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_BODY_ \
            )(id, typename01, params, \
                    const_binds, has_const_bind_this, binds, has_bind_this) \
        ; \
    }
        
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_FOR_DEFAULTS_(z, defaults_n, \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    BOOST_PP_EXPAND( \
    BOOST_PP_TUPLE_ELEM(9, 0, \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    ( z, defaults_n \
    , BOOST_PP_TUPLE_ELEM(9, 1, /* id */\
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    , BOOST_PP_TUPLE_ELEM(9, 2, /* typename01 */ \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    , BOOST_PP_TUPLE_ELEM(9, 3, /* decl_traits */ \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    , BOOST_PP_LIST_FIRST_N( /* remove last n default params */ \
          BOOST_PP_SUB(BOOST_PP_LIST_SIZE(BOOST_PP_TUPLE_ELEM(9, 4, \
            op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis)),\
            defaults_n) \
        , BOOST_PP_TUPLE_ELEM(9, 4, \
            op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
      ) \
    , BOOST_PP_TUPLE_ELEM(9, 5, /* const_binds */ \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    , BOOST_PP_TUPLE_ELEM(9, 6, /* has_const_bind_this */ \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    , BOOST_PP_TUPLE_ELEM(9, 7, /* binds */ \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    , BOOST_PP_TUPLE_ELEM(9, 8, /* has_bind_this */ \
        op_id_typename_decl_params_constbinds_hasconstthis_binds_hasthis) \
    ) /* end `op_macro(...)` */ \
    ) /* end expand */

// Functor binds.
            
// Precondition: NO_LOCAL_TYPES_AS_TEMPLATE_PARAMS.
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_BIND_TYPES_( \
        id, typename01, const_binds, has_const_bind_this, binds, has_bind_this)\
    BOOST_PP_LIST_FOR_EACH_I( \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_BIND_TYPE_, \
            ( id, typename01, 0 /* no offset */, 1 /* const */ ), \
            const_binds) \
    BOOST_PP_LIST_FOR_EACH_I( \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_BIND_TYPE_, \
            /* offset of # of const-binds */ \
            ( id, typename01, BOOST_PP_LIST_SIZE(const_binds), 0 /* const */ ),\
            binds) \
    BOOST_PP_IIF(BOOST_PP_BITOR(has_bind_this, \
            has_const_bind_this), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_BIND_THIS_TYPE_ \
    , \
        BOOST_PP_TUPLE_EAT(2) \
    )(id, typename01) \
    /* fill with nobind_t (if no local-types as tparams) */ \
    BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX, \
            BOOST_PP_IIF(BOOST_PP_BITOR(has_bind_this, has_const_bind_this), \
                BOOST_PP_INC \
            , \
                BOOST_PP_TUPLE_REM(1) \
            )(BOOST_PP_LIST_SIZE(BOOST_PP_LIST_APPEND(const_binds, binds)))), \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_NOBIND_TYPE_, ~)
        
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_TYPEOF_TYPEDEFS_( \
        id, typename01, const_binds, has_const_bind_this, binds, has_bind_this)\
    /* typeof types -- these types are qualified with extra eventual */ \
    /* const and/or & if their variables are bound by const and/or & */ \
    /* (this is because it is not possible to strip the eventual & */ \
    /* given that the var name is always attached to the & symbol plus */ \
    /* programmers can always remove const& using type traits) */ \
    /* const bind typeof types */ \
    BOOST_PP_LIST_FOR_EACH_I( \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_TYPEDEF_,\
            (id, typename01, 0 /* no offset */, 1 /* const-bind */ ), \
            const_binds) \
    /* bind typeof types */ \
    BOOST_PP_LIST_FOR_EACH_I( \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_TYPEDEF_, \
            /* offset index with # of preceding const-binds (if any) */ \
            ( id, typename01, BOOST_PP_LIST_SIZE(const_binds), \
              0 /* not const-bind */ ), binds) \
    /* const this... */ \
    BOOST_PP_EXPR_IIF(has_const_bind_this, \
        typedef BOOST_LOCAL_FUNCTION_AUX_TYPEOF_TYPE( \
            BOOST_PP_EXPR_IIF(typename01, typename) \
            ::boost::local_function::aux::add_pointed_const< \
                BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, typename01) \
            >::type \
            this_ \
        ) ; /* close typedef */ \
    ) \
    /* ... or, non-const this */ \
    BOOST_PP_EXPR_IIF(has_bind_this, \
        typedef BOOST_LOCAL_FUNCTION_AUX_TYPEOF_TYPE( \
            BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, typename01) \
            this_ \
        ) ; /* close typedef */ \
    )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_DECLS_( \
        id, typename01, const_binds, has_const_bind_this, binds, has_bind_this)\
    /* run-time: it is faster if call `operator()` just accesses member */ \
    /* references to the ScopeExit struct instead of accessing the bind */ \
    /* struct at each call (these mem refs are init by the constructor) */ \
    BOOST_PP_LIST_FOR_EACH_I( /* const bind member references */ \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_DECL_,\
            ( id, typename01, 0 /* no offset */, 1 /* const */ ), \
            const_binds) \
    BOOST_PP_LIST_FOR_EACH_I( /* bind member references */ \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_DECL_,\
            /* offset index of # of const-binds (could be 0) */ \
            ( id, typename01, BOOST_PP_LIST_SIZE(const_binds), \
              0 /* no const */ ), binds) \
    /* bind this const or not (pointed-const is not added here because */ \
    /* this is a reference, it is added to the this_ body param instead */ \
    BOOST_PP_EXPR_IIF(BOOST_PP_BITOR(has_bind_this, has_const_bind_this), \
        /* this is * so no & */ \
        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, typename01) \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_THIS_ \
        ; /* end member variable declaration */ \
    )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_STATIC_BINDS_( \
        id, typename01, \
        const_binds, has_const_bind_this, binds, has_bind_this) \
    BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_STATIC_BIND_MEMBER_, \
        ( id, typename01, 0 /* no offset */, 1 /* const */ ), const_binds) \
    BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_MAYBECONST_STATIC_BIND_MEMBER_, \
        /* offset of # of const-binds */ \
        ( id, typename01, BOOST_PP_LIST_SIZE(const_binds), 0 /* const */ ), \
        binds) \
    BOOST_PP_IIF(BOOST_PP_BITOR(has_bind_this, \
            has_const_bind_this), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_STATIC_BIND_MEMBER_THIS_ \
    , \
        BOOST_PP_TUPLE_EAT(1) \
    )(id) \
    /* fill with nobind_t (if no local-types as tparams) */ \
    BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_LOCAL_FUNCTION_CONFIG_BIND_MAX, \
            BOOST_PP_IIF(BOOST_PP_BITOR(has_bind_this, has_const_bind_this), \
                BOOST_PP_INC \
            , \
                BOOST_PP_TUPLE_REM(1) \
            )(BOOST_PP_LIST_SIZE(BOOST_PP_LIST_APPEND(const_binds, binds)))), \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_NOBIND_, ~)

// Functor inits.
        
#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MEMBER_INITS_(id, typename01, \
        const_binds, has_const_bind_this, binds, has_bind_this) \
    BOOST_PP_EXPR_IIF(BOOST_PP_BITOR(BOOST_PP_BITOR(BOOST_PP_BITOR( \
            BOOST_PP_LIST_IS_CONS(const_binds), BOOST_PP_LIST_IS_CONS(binds)), \
            has_bind_this), has_const_bind_this), \
        : \
    ) \
    /* init const binds */ \
    BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_INIT_ENUM_, \
            ( id, 0 /* no offset */ ), const_binds) \
    /* init plain binds */ \
    BOOST_PP_COMMA_IF( \
        BOOST_PP_BITAND( \
              BOOST_PP_LIST_IS_CONS(const_binds) \
            , BOOST_PP_LIST_IS_CONS(binds) \
        ) \
    ) \
    BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_MEMBER_INIT_ENUM_, \
            /* offset index of # of const-binds (could be 0) */ \
            ( id, BOOST_PP_LIST_SIZE(const_binds) ), binds) \
    /* init `this` bind (const or not) */ \
    BOOST_PP_COMMA_IF( \
        BOOST_PP_BITAND( \
              BOOST_PP_BITOR( \
                  BOOST_PP_LIST_IS_CONS(const_binds) \
                , BOOST_PP_LIST_IS_CONS(binds) \
              ) \
            , BOOST_PP_BITOR(has_const_bind_this, has_bind_this) \
        ) \
    ) \
    BOOST_PP_EXPR_IIF(BOOST_PP_BITOR(has_const_bind_this, has_bind_this), \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_THIS_( \
            static_cast< BOOST_SCOPE_EXIT_DETAIL_PARAMS_T(id)* >( \
                    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAMS_)-> \
                    BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_VAR \
        ) \
    )

// Functor class.

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_( \
        id, typename01, decl_traits, params, \
        default_count, const_binds, has_const_bind_this, binds, has_bind_this) \
    typedef class BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CLASS_TYPE_(id) \
    /* run-time: do not use base class to allow for compiler optimizations */ \
    { \
        /* function type */ \
    private: \
        typedef \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_F_(id, typename01, \
                    decl_traits, 1 /* has type */, \
                    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_F_TYPE_) \
        ; \
        /* functor type -- this type cannot have ID postfix because it is */ \
        /* used the `NAME` macro (this symbol is within functor class so */ \
        /* it does not have to have ID postfix), must be public so it */ \
        /* can be accessed by `NAME` macro from outside this class */ \
    public: \
        typedef BOOST_PP_EXPR_IIF(typename01, typename) \
            BOOST_IDENTITY_TYPE(( /* IDENTITY for template param comma */ \
                ::boost::local_function::aux::function< \
                      BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_F_TYPE_ \
                    , default_count \
                    BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS,\
                        BOOST_PP_TUPLE_EAT(6) \
                    , \
                        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_BIND_TYPES_\
                    )(id, typename01, const_binds, has_const_bind_this, \
                            binds, has_bind_this) \
                > \
            )) \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_TYPE \
        ; \
    private: \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_TYPEOF_TYPEDEFS_( \
                id, typename01, \
                const_binds, has_const_bind_this, binds, has_bind_this) \
    public: \
        /* public trait interface following Boost.FunctionTraits names */ \
        /* (traits must be defined in both this and the global functor) */ \
        enum { arity = ::boost::function_traits< /* can't use static data */ \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_F_TYPE_ >::arity }; \
        typedef BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE(id, typename01) \
                result_type; \
        BOOST_PP_LIST_FOR_EACH_I( \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_ARG_TYPEDEF_, \
                typename01, params) \
        /* constructor */ \
        inline explicit BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CLASS_TYPE_(id)( \
                void* BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAMS_) \
            /* NOTE: there is no way to wrap member initializer commas */ \
            /* within paren so you must handle these commas manually if */ \
            /* expanding this macro within another macro */ \
            BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MEMBER_INITS_(id, typename01,\
                    const_binds, has_const_bind_this, binds, has_bind_this) \
        { /* do nothing */ } \
        /* run-time: implement `operator()` (and for all default params) so */ \
        /* this obj can be used directly as a functor for C++03 extensions */ \
        /* and optimized macros */ \
        BOOST_PP_REPEAT( \
                /* PP_INC to handle no dflt (EXPAND for MVSC) */ \
                BOOST_PP_EXPAND(BOOST_PP_INC(default_count)), \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_FOR_DEFAULTS_,\
                ( BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_, id, typename01 \
                , decl_traits, params, const_binds, has_const_bind_this, binds \
                , has_bind_this ) ) \
        /* compliance: trick to pass this local class as a template param */ \
        /* on pure C++03 without non C++03 extension */ \
        /* performance: this trick introduced _one_ indirect function call */ \
        /* via a function pointer that is usually not inlined by compliers */ \
        /* thus increasing run-time (also another trick using a base */ \
        /* interface class was investigated but virtual calls also cannot */ \
        /* inlined plus they require virtual table lookups to the "virtual */ \
        /* call trick" measured longer run-times than this "static call */ \
        /* trick") */ \
        BOOST_PP_REPEAT( \
                /* PP_INC to handle no dflt (EXPAND for MVSC) */ \
                BOOST_PP_EXPAND(BOOST_PP_INC(default_count)), \
                BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_CALL_FOR_DEFAULTS_,\
                ( BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_, id \
                , typename01, decl_traits, params, const_binds \
                , has_const_bind_this, binds, has_bind_this ) ) \
        inline static void BOOST_LOCAL_FUNCTION_AUX_FUNCTION_INIT_CALL_FUNC( \
              void* object \
            , BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_TYPE& functor \
        ) { \
            functor.BOOST_LOCAL_FUNCTION_AUX_FUNCTION_INIT_CALL_FUNC( \
                    object \
                    BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_CONFIG_LOCALS_AS_TPARAMS,\
                        BOOST_PP_TUPLE_EAT(6) \
                    , \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_COMMA_STATIC_BINDS_ \
                    )(id, typename01, const_binds, has_const_bind_this, \
                            binds, has_bind_this) \
                BOOST_PP_REPEAT( /* INC to handle no dflt (EXPAND for MVSC) */ \
                        BOOST_PP_EXPAND(BOOST_PP_INC(default_count)), \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_STATIC_CALL_COMMA_FUNC_ADDR_, \
                        ~) \
            ); \
        } \
    private: \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BIND_MEMBER_DECLS_(id, \
                typename01, const_binds, has_const_bind_this, binds, \
                has_bind_this) \
        /* this decl allows for nesting (local functions, etc) as */ \
        /* it makes the args variable visible within the body code (which */ \
        /* cannot be static); this is for compilation only as the args */ \
        /* variable is actually declared by the 1st enclosing local func */ \
        boost::scope_exit::detail::undeclared \
                BOOST_LOCAL_FUNCTION_AUX_DECL_ARGS_VAR; \
        /* body function (unfortunately, cannot be static to allow access */ \
        /* to member var with local function name for recursion but doing */ \
        /* so also allows the body to misuse `this` instead of `this_`) */ \
        inline BOOST_LOCAL_FUNCTION_AUX_CODE_RESULT_TYPE(id, typename01) \
        BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_BODY_FUNC_( \
                /* const binds */ \
                BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_ENUM_, \
                        ( id, typename01, 0 /* no offset */, 1 /* const */ ), \
                        const_binds) \
                /* plain binds */ \
                BOOST_PP_COMMA_IF( \
                    BOOST_PP_BITAND( \
                          BOOST_PP_LIST_IS_CONS(const_binds) \
                        , BOOST_PP_LIST_IS_CONS(binds) \
                    ) \
                ) \
                BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_MAYBECONST_BIND_DECL_ENUM_, \
                        /* offset index of # of const-binds (could be 0) */ \
                        ( id, typename01, BOOST_PP_LIST_SIZE(const_binds), \
                          0 /* not const-bind */ ), binds) \
                /* `this` bind */ \
                BOOST_PP_COMMA_IF( \
                    BOOST_PP_BITAND( \
                          BOOST_PP_BITOR( \
                              BOOST_PP_LIST_IS_CONS(const_binds) \
                            , BOOST_PP_LIST_IS_CONS(binds) \
                          ) \
                        , BOOST_PP_BITOR(has_const_bind_this, has_bind_this) \
                    ) \
                ) \
                /* const pointer to const object */ \
                BOOST_PP_EXPR_IIF(has_const_bind_this, \
                    BOOST_PP_EXPR_IIF(typename01, typename) \
                    ::boost::local_function::aux::add_pointed_const< \
                        BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, \
                                typename01) \
                    >::type \
                    const this_ /* special name to access object this */ \
                ) \
                /* const pointer to non-const object */ \
                BOOST_PP_EXPR_IIF(has_bind_this, \
                    BOOST_LOCAL_FUNCTION_AUX_CODE_BIND_THIS_TYPE(id, \
                            typename01) \
                    const this_ /* special name to access object this */ \
                ) \
                /* params (last because they can have defaults) */ \
                BOOST_PP_COMMA_IF( \
                    BOOST_PP_BITAND( \
                          BOOST_PP_BITOR( \
                              BOOST_PP_BITOR( \
                                  BOOST_PP_LIST_IS_CONS(const_binds) \
                                , BOOST_PP_LIST_IS_CONS(binds) \
                              ) \
                            , BOOST_PP_BITOR(has_const_bind_this, \
                                    has_bind_this) \
                          ) \
                        , BOOST_PP_LIST_IS_CONS(params) \
                    ) \
                ) \
                BOOST_PP_LIST_FOR_EACH_I( \
  BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_PARAM_DECL_ENUM_, \
                        1 /* with defaults */, params) \
            ) /* end body function params */ \
            /* cannot be const because recursive functor is non const member */\
    /* user local function definition `{ ... }` will follow here */ \
    /* `END` macro will close function class decl `};` here */ 

// PUBLIC //

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_TYPE \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL( (functor_type) )

#define BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR(id, typename01, decl_traits) \
    BOOST_LOCAL_FUNCTION_AUX_CODE_FUNCTOR_(id, typename01, decl_traits \
        /* params (might have defaults) */ \
        , BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_PARAMS(decl_traits) \
        , BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_PARAMS_DEFAULT_COUNT( \
                decl_traits) \
        /* const bind vars (`this` excluded) */ \
        , BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_CONST_BINDS(decl_traits) \
        /* if const bind `this` is present */ \
        , BOOST_PP_LIST_IS_CONS( \
                BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_CONST_BIND_THIS_TYPES( \
                decl_traits)) \
        /* bind (non-const) vars (`this` excluded) */ \
        , BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_BINDS(decl_traits) \
        /* if (non-const) bind `this` is present */ \
        , BOOST_PP_LIST_IS_CONS( \
                BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_BIND_THIS_TYPES( \
                decl_traits)) \
    )

#endif // #include guard

