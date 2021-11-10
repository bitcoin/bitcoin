
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_HPP_

#include <boost/local_function/detail/preprocessor/keyword/const_bind.hpp>
#include <boost/local_function/detail/preprocessor/keyword/bind.hpp>
#include <boost/local_function/detail/preprocessor/keyword/default.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/while.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/list/size.hpp>
#include <boost/preprocessor/list/at.hpp>

// PRIVATE //

#define \
BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_IS_UNBIND_( \
        sign) \
    /* PP_OR/PP_BITOR (instead of IIF) don't expand on MSVC */ \
    BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_DEFAULT_FRONT(sign),\
        0 \
    , BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_CONST_BIND_FRONT( \
            sign), \
        0 \
    , BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_BIND_FRONT(sign), \
        0 \
    , \
        1 \
    )))

#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_PREV_( \
        sign, index, error) \
    BOOST_PP_IIF( \
  BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_IS_UNBIND_( \
                    BOOST_PP_LIST_AT(sign, BOOST_PP_DEC(index))), \
        error /* no err, fwd existing one if any */ \
    , \
        BOOST_PP_CAT(BOOST_PP_CAT(ERROR_default_value_at_element_, \
                BOOST_PP_INC(index)), _must_follow_an_unbound_parameter) \
        BOOST_PP_EMPTY /* because error might not be present */ \
    )

#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_INDEX_( \
        sign, index, error) \
    BOOST_PP_IF(index, /* can't use IIF because index can be any number */ \
        BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_PREV_ \
    , \
        ERROR_default_value_cannot_be_specified_as_the_first_element \
        BOOST_PP_EMPTY /* because error might not be present */ \
        BOOST_PP_TUPLE_EAT(3) \
    )(sign, index, error)

// While's operation.

#define \
BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_OP_DATA_( \
        sign, index, error) \
    ( \
        sign \
    , \
        BOOST_PP_INC(index) \
    , \
        BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_DEFAULT_FRONT( \
                BOOST_PP_LIST_AT(sign, index)), \
  BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_INDEX_ \
        , \
            error BOOST_PP_TUPLE_EAT(3) /* no err, fwd existing one if any */\
        )(sign, index, error) \
    )

#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_OP_(d, \
        sign_index_error) \
    BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_OP_DATA_( \
            BOOST_PP_TUPLE_ELEM(3, 0, sign_index_error), \
            BOOST_PP_TUPLE_ELEM(3, 1, sign_index_error), \
            BOOST_PP_TUPLE_ELEM(3, 2, sign_index_error))

// While predicate.

#define \
BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_PRED_DATA_( \
        sign, index, error) \
    BOOST_PP_BITAND( \
          BOOST_PP_IS_EMPTY(error (/* expand empty */) ) \
        , BOOST_PP_LESS(index, BOOST_PP_LIST_SIZE(sign)) \
    )

#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_PRED_( \
        d, sign_index_error) \
    BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_PRED_DATA_( \
            BOOST_PP_TUPLE_ELEM(3, 0, sign_index_error), \
            BOOST_PP_TUPLE_ELEM(3, 1, sign_index_error), \
            BOOST_PP_TUPLE_ELEM(3, 2, sign_index_error))

// PUBLIC //

// Validate parameters default values: `default ...` cannot be 1st element and
// it must follow an unbind param. Expand to `EMPTY` if no error, or
// `ERROR_message EMPTY` if error.
#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS(sign) \
    BOOST_PP_TUPLE_ELEM(3, 2, BOOST_PP_WHILE( \
  BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_PRED_, \
            BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN_VALIDATE_DEFAULTS_OP_,\
            (sign, 0, BOOST_PP_EMPTY)))

#endif // #include guard

