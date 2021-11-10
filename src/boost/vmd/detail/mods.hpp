
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_MODS_HPP)
#define BOOST_VMD_DETAIL_MODS_HPP

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/control/while.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/preprocessor/tuple/replace.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_type.hpp>
#include <boost/vmd/detail/modifiers.hpp>

#define BOOST_VMD_DETAIL_MODS_NO_RETURN 0
#define BOOST_VMD_DETAIL_MODS_RETURN 1
#define BOOST_VMD_DETAIL_MODS_RETURN_TUPLE 2
#define BOOST_VMD_DETAIL_MODS_RETURN_ARRAY 3
#define BOOST_VMD_DETAIL_MODS_RETURN_LIST 4
#define BOOST_VMD_DETAIL_MODS_NO_AFTER 0
#define BOOST_VMD_DETAIL_MODS_RETURN_AFTER 1
#define BOOST_VMD_DETAIL_MODS_NO_INDEX 0
#define BOOST_VMD_DETAIL_MODS_RETURN_INDEX 1
#define BOOST_VMD_DETAIL_MODS_NO_ONLY_AFTER 0
#define BOOST_VMD_DETAIL_MODS_RETURN_ONLY_AFTER 1

#define BOOST_VMD_DETAIL_MODS_TUPLE_RETURN 0
#define BOOST_VMD_DETAIL_MODS_TUPLE_AFTER 1
#define BOOST_VMD_DETAIL_MODS_TUPLE_INDEX 2
#define BOOST_VMD_DETAIL_MODS_TUPLE_OTHER 3
#define BOOST_VMD_DETAIL_MODS_TUPLE_ONLY_AFTER 4
#define BOOST_VMD_DETAIL_MODS_TUPLE_TYPE 5

#define BOOST_VMD_DETAIL_MODS_DATA_INPUT 0
#define BOOST_VMD_DETAIL_MODS_DATA_INDEX 1
#define BOOST_VMD_DETAIL_MODS_DATA_SIZE 2
#define BOOST_VMD_DETAIL_MODS_DATA_RESULT 3
#define BOOST_VMD_DETAIL_MODS_DATA_ALLOW 4

#define BOOST_VMD_DETAIL_MODS_STATE_INPUT(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_DATA_INPUT,state) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_INDEX(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_DATA_INDEX,state) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_SIZE(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_DATA_SIZE,state) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_RESULT(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_DATA_RESULT,state) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_ALLOW(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_DATA_ALLOW,state) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_ALL(state) \
    BOOST_VMD_DETAIL_IS_ALLOW_ALL(BOOST_VMD_DETAIL_MODS_STATE_ALLOW(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_RETURN(state) \
    BOOST_VMD_DETAIL_IS_ALLOW_RETURN(BOOST_VMD_DETAIL_MODS_STATE_ALLOW(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_AFTER(state) \
    BOOST_VMD_DETAIL_IS_ALLOW_AFTER(BOOST_VMD_DETAIL_MODS_STATE_ALLOW(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_INDEX(state) \
    BOOST_VMD_DETAIL_IS_ALLOW_INDEX(BOOST_VMD_DETAIL_MODS_STATE_ALLOW(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_CURRENT(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_STATE_INDEX(state),BOOST_VMD_DETAIL_MODS_STATE_INPUT(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_TYPE(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_RETURN,BOOST_VMD_DETAIL_MODS_STATE_RESULT(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_AFTER(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_AFTER,BOOST_VMD_DETAIL_MODS_STATE_RESULT(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_ONLY_AFTER(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_ONLY_AFTER,BOOST_VMD_DETAIL_MODS_STATE_RESULT(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_STATE_TINDEX(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_INDEX,BOOST_VMD_DETAIL_MODS_STATE_RESULT(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_RESULT_RETURN_TYPE(result) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_RETURN,result) \
/**/

#define BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(result) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_AFTER,result) \
/**/

#define BOOST_VMD_DETAIL_MODS_IS_RESULT_ONLY_AFTER(result) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_ONLY_AFTER,result) \
/**/

#define BOOST_VMD_DETAIL_MODS_IS_RESULT_INDEX(result) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_INDEX,result) \
/**/

#define BOOST_VMD_DETAIL_MODS_RESULT_OTHER(result) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_OTHER,result) \
/**/

#define BOOST_VMD_DETAIL_MODS_RESULT_TYPE(result) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_MODS_TUPLE_TYPE,result) \
/**/

#define BOOST_VMD_DETAIL_MODS_PRED(d,state) \
    BOOST_PP_GREATER_D(d,BOOST_VMD_DETAIL_MODS_STATE_SIZE(state),BOOST_VMD_DETAIL_MODS_STATE_INDEX(state))
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_RETURN_TYPE(d,state,number) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            BOOST_VMD_DETAIL_MODS_DATA_RESULT, \
            BOOST_PP_TUPLE_REPLACE_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
                BOOST_VMD_DETAIL_MODS_TUPLE_RETURN, \
                number \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ONLY_AFTER(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            BOOST_VMD_DETAIL_MODS_DATA_RESULT, \
            BOOST_PP_TUPLE_REPLACE_D \
                ( \
                d, \
                BOOST_PP_TUPLE_REPLACE_D \
                    ( \
                    d, \
                    BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
                    BOOST_VMD_DETAIL_MODS_TUPLE_ONLY_AFTER, \
                    1 \
                    ), \
                BOOST_VMD_DETAIL_MODS_TUPLE_AFTER, \
                1 \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_INDEX(d,state,number) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            BOOST_VMD_DETAIL_MODS_DATA_RESULT, \
            BOOST_PP_TUPLE_REPLACE_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
                BOOST_VMD_DETAIL_MODS_TUPLE_INDEX, \
                number \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_GTT(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_RETURN_TYPE(d,state,BOOST_VMD_DETAIL_MODS_RETURN_TUPLE) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ET(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_RETURN_TYPE(d,state,BOOST_VMD_DETAIL_MODS_RETURN) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_SA(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_RETURN_TYPE(d,state,BOOST_VMD_DETAIL_MODS_RETURN_ARRAY) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_SL(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_RETURN_TYPE(d,state,BOOST_VMD_DETAIL_MODS_RETURN_LIST) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_NT(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_RETURN_TYPE(d,state,BOOST_VMD_DETAIL_MODS_NO_RETURN) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_AFT(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            BOOST_VMD_DETAIL_MODS_DATA_RESULT, \
            BOOST_PP_TUPLE_REPLACE_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
                BOOST_VMD_DETAIL_MODS_TUPLE_AFTER, \
                BOOST_VMD_DETAIL_MODS_RETURN_AFTER \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_NOAFT(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            BOOST_VMD_DETAIL_MODS_DATA_RESULT, \
            BOOST_PP_TUPLE_REPLACE_D \
                ( \
                d, \
                BOOST_PP_TUPLE_REPLACE_D \
                    ( \
                    d, \
                    BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
                    BOOST_VMD_DETAIL_MODS_TUPLE_ONLY_AFTER, \
                    0 \
                    ), \
                BOOST_VMD_DETAIL_MODS_TUPLE_AFTER, \
                BOOST_VMD_DETAIL_MODS_NO_AFTER \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_IND(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_INDEX(d,state,BOOST_VMD_DETAIL_MODS_RETURN_INDEX) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_NO_IND(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_INDEX(d,state,BOOST_VMD_DETAIL_MODS_NO_INDEX) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_CTUPLE_REPLACE(d,state,id) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
        BOOST_VMD_DETAIL_MODS_TUPLE_OTHER, \
        BOOST_PP_VARIADIC_TO_TUPLE(id) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_CTUPLE_ADD(d,state,id) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
        BOOST_VMD_DETAIL_MODS_TUPLE_OTHER, \
        BOOST_PP_TUPLE_PUSH_BACK \
            ( \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(BOOST_VMD_DETAIL_MODS_STATE_RESULT(state)), \
            id \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_CTUPLE(d,state,id) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY \
            ( \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(BOOST_VMD_DETAIL_MODS_STATE_RESULT(state)) \
            ), \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_CTUPLE_REPLACE, \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_CTUPLE_ADD \
        ) \
    (d,state,id) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_TYPE_RETURN(d,state,id) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
        BOOST_VMD_DETAIL_MODS_TUPLE_RETURN, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,id,BOOST_VMD_TYPE_ARRAY), \
            BOOST_VMD_DETAIL_MODS_RETURN_ARRAY, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,id,BOOST_VMD_TYPE_LIST), \
                BOOST_VMD_DETAIL_MODS_RETURN_LIST, \
                BOOST_VMD_DETAIL_MODS_RETURN_TUPLE \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_TYPE(d,state,id) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_TYPE_RETURN(d,state,id), \
        BOOST_VMD_DETAIL_MODS_TUPLE_TYPE, \
        id \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            BOOST_VMD_DETAIL_MODS_DATA_RESULT, \
            BOOST_PP_IIF \
                ( \
                BOOST_PP_BITAND \
                    ( \
                    BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_ALL(state), \
                    BOOST_VMD_IS_TYPE_D(d,id) \
                    ), \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_TYPE, \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN_CTUPLE \
                ) \
            (d,state,id) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE(d,state) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        state, \
        BOOST_VMD_DETAIL_MODS_DATA_INDEX, \
        BOOST_PP_INC(BOOST_VMD_DETAIL_MODS_STATE_INDEX(state)) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_ALL(d,state,id) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_RETURN_TYPE_TUPLE(id), \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_GTT, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_RETURN_TYPE(id), \
            BOOST_VMD_DETAIL_MODS_OP_CURRENT_ET, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_IS_RETURN_TYPE_ARRAY(id), \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_SA, \
                BOOST_PP_IIF \
                    ( \
                    BOOST_VMD_DETAIL_IS_RETURN_TYPE_LIST(id), \
                    BOOST_VMD_DETAIL_MODS_OP_CURRENT_SL, \
                    BOOST_PP_IIF \
                        ( \
                        BOOST_VMD_DETAIL_IS_RETURN_NO_TYPE(id), \
                        BOOST_VMD_DETAIL_MODS_OP_CURRENT_NT, \
                        BOOST_PP_IIF \
                            ( \
                            BOOST_VMD_DETAIL_IS_RETURN_AFTER(id), \
                            BOOST_VMD_DETAIL_MODS_OP_CURRENT_AFT, \
                            BOOST_PP_IIF \
                                ( \
                                BOOST_VMD_DETAIL_IS_RETURN_NO_AFTER(id), \
                                BOOST_VMD_DETAIL_MODS_OP_CURRENT_NOAFT, \
                                BOOST_PP_IIF \
                                    ( \
                                    BOOST_VMD_DETAIL_IS_RETURN_ONLY_AFTER(id), \
                                    BOOST_VMD_DETAIL_MODS_OP_CURRENT_ONLY_AFTER, \
                                    BOOST_PP_IIF \
                                        ( \
                                        BOOST_VMD_DETAIL_IS_RETURN_INDEX(id), \
                                        BOOST_VMD_DETAIL_MODS_OP_CURRENT_IND, \
                                        BOOST_PP_IIF \
                                            ( \
                                            BOOST_VMD_DETAIL_IS_RETURN_NO_INDEX(id), \
                                            BOOST_VMD_DETAIL_MODS_OP_CURRENT_NO_IND, \
                                            BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN \
                                            ) \
                                        ) \
                                    ) \
                                ) \
                            ) \
                        ) \
                    ) \
                ) \
            ) \
        ) \
    (d,state,id) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_RETURN(d,state,id) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_RETURN_TYPE_TUPLE(id), \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_GTT, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_RETURN_TYPE(id), \
            BOOST_VMD_DETAIL_MODS_OP_CURRENT_ET, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_IS_RETURN_TYPE_ARRAY(id), \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_SA, \
                BOOST_PP_IIF \
                    ( \
                    BOOST_VMD_DETAIL_IS_RETURN_TYPE_LIST(id), \
                    BOOST_VMD_DETAIL_MODS_OP_CURRENT_SL, \
                    BOOST_PP_IIF \
                        ( \
                        BOOST_VMD_DETAIL_IS_RETURN_NO_TYPE(id), \
                        BOOST_VMD_DETAIL_MODS_OP_CURRENT_NT, \
                        BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN \
                        ) \
                    ) \
                ) \
            ) \
        ) \
    (d,state,id) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_AFTER(d,state,id) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_RETURN_AFTER(id), \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_AFT, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_RETURN_NO_AFTER(id), \
            BOOST_VMD_DETAIL_MODS_OP_CURRENT_NOAFT, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_IS_RETURN_ONLY_AFTER(id), \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_ONLY_AFTER, \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN \
                ) \
            ) \
        ) \
    (d,state,id) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_INDEX(d,state,id) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_RETURN_AFTER(id), \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_AFT, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_RETURN_NO_AFTER(id), \
            BOOST_VMD_DETAIL_MODS_OP_CURRENT_NOAFT, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_IS_RETURN_ONLY_AFTER(id), \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_ONLY_AFTER, \
                BOOST_PP_IIF \
                    ( \
                    BOOST_VMD_DETAIL_IS_RETURN_INDEX(id), \
                    BOOST_VMD_DETAIL_MODS_OP_CURRENT_IND, \
                    BOOST_PP_IIF \
                        ( \
                        BOOST_VMD_DETAIL_IS_RETURN_NO_INDEX(id), \
                        BOOST_VMD_DETAIL_MODS_OP_CURRENT_NO_IND, \
                        BOOST_VMD_DETAIL_MODS_OP_CURRENT_UNKNOWN \
                        ) \
                    ) \
                ) \
            ) \
        ) \
    (d,state,id) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_UPDATE(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE(d,state) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_ID(d,state,id) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_ALL(state), \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_ALL, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_RETURN(state), \
            BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_RETURN, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_AFTER(state), \
                BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_AFTER, \
                BOOST_PP_IIF \
                    ( \
                    BOOST_VMD_DETAIL_MODS_STATE_IS_ALLOW_INDEX(state), \
                    BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_INDEX, \
                    BOOST_VMD_DETAIL_MODS_OP_CURRENT_ALLOW_UPDATE \
                    ) \
                ) \
            ) \
        ) \
    (d,state,id) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT_TUPLE(d,state,id) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT_UPDATE \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            BOOST_VMD_DETAIL_MODS_DATA_RESULT, \
            BOOST_PP_TUPLE_REPLACE_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_MODS_STATE_RESULT(state), \
                BOOST_VMD_DETAIL_MODS_TUPLE_OTHER, \
                id \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP_CURRENT(d,state,id) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_IS_BEGIN_PARENS(id), \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_TUPLE, \
        BOOST_VMD_DETAIL_MODS_OP_CURRENT_ID \
        ) \
    (d,state,id) \
/**/

#define BOOST_VMD_DETAIL_MODS_OP(d,state) \
    BOOST_VMD_DETAIL_MODS_OP_CURRENT(d,state,BOOST_VMD_DETAIL_MODS_STATE_CURRENT(state)) \
/**/

#define BOOST_VMD_DETAIL_MODS_LOOP(allow,tuple) \
    BOOST_PP_TUPLE_ELEM \
        ( \
        3, \
        BOOST_PP_WHILE \
            ( \
            BOOST_VMD_DETAIL_MODS_PRED, \
            BOOST_VMD_DETAIL_MODS_OP, \
                ( \
                tuple, \
                0, \
                BOOST_PP_TUPLE_SIZE(tuple), \
                (0,0,0,,0,), \
                allow \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_MODS_LOOP_D(d,allow,tuple) \
    BOOST_PP_TUPLE_ELEM \
        ( \
        3, \
        BOOST_PP_WHILE_ ## d \
            ( \
            BOOST_VMD_DETAIL_MODS_PRED, \
            BOOST_VMD_DETAIL_MODS_OP, \
                ( \
                tuple, \
                0, \
                BOOST_PP_TUPLE_SIZE(tuple), \
                (0,0,0,,0,), \
                allow \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_NEW_MODS_VAR(allow,tuple) \
    BOOST_VMD_DETAIL_MODS_LOOP \
        ( \
        allow, \
        BOOST_PP_TUPLE_POP_FRONT(tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_NEW_MODS_VAR_D(d,allow,tuple) \
    BOOST_VMD_DETAIL_MODS_LOOP_D \
        ( \
        d, \
        allow, \
        BOOST_PP_TUPLE_POP_FRONT(tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_NEW_MODS_IR(allow,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(tuple),1), \
            BOOST_VMD_IDENTITY((0,0,0,,0,)), \
            BOOST_VMD_DETAIL_NEW_MODS_VAR \
            ) \
        (allow,tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_NEW_MODS_IR_D(d,allow,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL_D(d,BOOST_PP_TUPLE_SIZE(tuple),1), \
            BOOST_VMD_IDENTITY((0,0,0,,0,)), \
            BOOST_VMD_DETAIL_NEW_MODS_VAR_D \
            ) \
        (d,allow,tuple) \
        ) \
/**/

/*

  Returns a six-element tuple:
  
  First tuple element  = 0 No type return
                         1 Exact type return
                         2 General tuple type return
                         3 Array return
                         4 List return
                         
  Second tuple element = 0 No after return
                         1 After return
                         
  Third tuple element  = 0 No identifier index
                         1 Identifier Index
                         
  Fourth tuple element = Tuple of other identifiers
  
  Fifth tuple element  = 0 No after only return
                         1 After only return
                         
  Sixth tuple element  = Type identifier
                         
  Input                = allow, either
                         BOOST_VMD_ALLOW_ALL
                         BOOST_VMD_ALLOW_RETURN
                         BOOST_VMD_ALLOW_AFTER
                         BOOST_VMD_ALLOW_INDEX
                         
                           ..., modifiers, first variadic is discarded
                         Possible modifiers are:
                         
                         BOOST_VMD_RETURN_NO_TYPE = (0,0)
                         BOOST_VMD_RETURN_TYPE = (1,0)
                         BOOST_VMD_RETURN_TYPE_TUPLE = (2,0)
                         BOOST_VMD_RETURN_TYPE_ARRAY = (3,0)
                         BOOST_VMD_RETURN_TYPE_LIST = (4,0)
                         
                         BOOST_VMD_RETURN_NO_AFTER = (0,0)
                         BOOST_VMD_RETURN_AFTER = (0,1)
  
*/

#define BOOST_VMD_DETAIL_NEW_MODS(allow,...) \
    BOOST_VMD_DETAIL_NEW_MODS_IR(allow,BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) \
/**/

#define BOOST_VMD_DETAIL_NEW_MODS_D(d,allow,...) \
    BOOST_VMD_DETAIL_NEW_MODS_IR_D(d,allow,BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) \
/**/

#endif /* BOOST_VMD_DETAIL_MODS_HPP */
