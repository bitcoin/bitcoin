
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_SEQUENCE_COMMON_HPP)
#define BOOST_VMD_DETAIL_SEQUENCE_COMMON_HPP

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/array/push_back.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/less_equal.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/control/while.hpp>
#include <boost/preprocessor/list/append.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/seq/push_back.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/preprocessor/tuple/replace.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_empty_list.hpp>
#include <boost/vmd/detail/array.hpp>
#include <boost/vmd/detail/equal_type.hpp>
#include <boost/vmd/detail/identifier.hpp>
#include <boost/vmd/detail/identifier_type.hpp>
#include <boost/vmd/detail/list.hpp>
#include <boost/vmd/detail/modifiers.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/seq.hpp>
#include <boost/vmd/detail/tuple.hpp>

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT_ELEM 0
#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ELEM 1
#define BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM_ELEM 2
#define BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE_ELEM 3
#define BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM_ELEM 4
#define BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES_ELEM 5
#define BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX_ELEM 6

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT_ELEM,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state) \
    BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ELEM,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state) \
        BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM_ELEM,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE(state) \
        BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE_ELEM,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state) \
        BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM_ELEM,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_RETURN(d,from,number) \
    BOOST_PP_EQUAL_D \
        ( \
        d, \
        BOOST_PP_TUPLE_ELEM(0,from), \
        number \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_NO_RETURN(d,from) \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_RETURN(d,from,BOOST_VMD_DETAIL_MODS_NO_RETURN) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_EXACT_RETURN(d,from) \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_RETURN(d,from,BOOST_VMD_DETAIL_MODS_RETURN) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_GENERAL_RETURN(d,from) \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_RETURN(d,from,BOOST_VMD_DETAIL_MODS_RETURN_TUPLE) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_ARRAY_RETURN(d,from) \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_RETURN(d,from,BOOST_VMD_DETAIL_MODS_RETURN_ARRAY) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_LIST_RETURN(d,from) \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_RETURN(d,from,BOOST_VMD_DETAIL_MODS_RETURN_LIST) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER(from) \
    BOOST_PP_EQUAL \
        ( \
        BOOST_PP_TUPLE_ELEM(1,from), \
        BOOST_VMD_DETAIL_MODS_RETURN_AFTER \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D(d,from) \
    BOOST_PP_EQUAL_D \
        ( \
        d, \
        BOOST_PP_TUPLE_ELEM(1,from), \
        BOOST_VMD_DETAIL_MODS_RETURN_AFTER \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state) \
        BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES_ELEM,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX(state) \
        BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX_ELEM,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_IS_EMPTY(state) \
    BOOST_VMD_IS_EMPTY \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_FROM_EMPTY(state,data) \
    (data) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_TO_SEQ(state,data) \
    BOOST_PP_SEQ_PUSH_BACK(BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state),data) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_TO_TUPLE(state,data) \
    BOOST_PP_TUPLE_PUSH_BACK(BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state),data) \
/**/

// Array

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_BOOST_VMD_TYPE_ARRAY(d,state,data) \
    BOOST_PP_ARRAY_PUSH_BACK(BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state),data) \
/**/

// List

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_BOOST_VMD_TYPE_LIST(d,state,data) \
    BOOST_PP_LIST_APPEND_D(d,BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state),(data,BOOST_PP_NIL)) \
/**/

// Seq

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_BOOST_VMD_TYPE_SEQ(d,state,data) \
    BOOST_PP_IIF \
         ( \
         BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_IS_EMPTY(state), \
         BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_FROM_EMPTY, \
         BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_TO_SEQ \
         ) \
     (state,data) \
/**/

// Tuple

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_BOOST_VMD_TYPE_TUPLE(d,state,data) \
    BOOST_PP_IIF \
         ( \
         BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_IS_EMPTY(state), \
         BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_FROM_EMPTY, \
         BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_TO_TUPLE \
         ) \
     (state,data) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_GET_NAME(state) \
    BOOST_PP_CAT \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_, \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_PROCESS(d,name,state,data) \
    name(d,state,data) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_GET_DATA(d,state,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_NO_RETURN \
                ( \
                d, \
                BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state) \
                ), \
            BOOST_PP_TUPLE_ELEM, \
            BOOST_VMD_IDENTITY(tuple) \
            ) \
        (1,tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD(d,state,ttuple) \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_PROCESS \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_GET_NAME(state), \
        state, \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD_GET_DATA(d,state,ttuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_PROCESSING_ELEM_CHECK(d,state) \
    BOOST_PP_EQUAL_D \
        ( \
        d, \
        BOOST_PP_SEQ_SIZE(BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state)), \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_PROCESSING_ELEM(d,state) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state)), \
            BOOST_VMD_IDENTITY(0), \
            BOOST_VMD_DETAIL_SEQUENCE_PROCESSING_ELEM_CHECK \
            ) \
        (d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_GET_TYPE(state) \
        BOOST_PP_TUPLE_ELEM \
            ( \
            0, \
            BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX(state),BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state)) \
            ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_STATE_GET_TYPE_REENTRANT(state) \
        BOOST_PP_TUPLE_ELEM \
            ( \
            1, \
            BOOST_PP_TUPLE_ELEM(BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX(state),BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state)) \
            ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_UNKNOWN(d,state) \
    ( \
    , \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD(d,state,(BOOST_VMD_TYPE_UNKNOWN,BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state))), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state), \
    BOOST_PP_INC(BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX(state)) \
    ) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_GET_FULL_TYPE_CHECK_ID(d,type,id) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,type,BOOST_VMD_TYPE_IDENTIFIER), \
            BOOST_VMD_DETAIL_IDENTIFIER_TYPE_D, \
            BOOST_VMD_IDENTITY(type) \
            ) \
        (d,id) \
        ) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_GET_FULL_TYPE(d,state,tuple) \
    BOOST_VMD_DETAIL_SEQUENCE_GET_FULL_TYPE_CHECK_ID \
        ( \
        d, \
        BOOST_PP_CAT \
            ( \
            BOOST_VMD_TYPE_, \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_GET_TYPE(state) \
            ), \
        BOOST_PP_TUPLE_ELEM(0,tuple) \
        ) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND_PROCESS(d,state,tuple) \
    ( \
    BOOST_PP_TUPLE_ELEM(1,tuple), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD \
        ( \
        d, \
        state, \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_GET_FULL_TYPE(d,state,tuple), \
            BOOST_PP_TUPLE_ELEM(0,tuple) \
            ) \
        ), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state), \
    BOOST_PP_INC(BOOST_PP_TUPLE_SIZE(BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state))) \
    ) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND_SEQ_SINGLE(d,tuple) \
    BOOST_PP_EQUAL_D(d,BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(0,tuple)),1) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND_SEQ(d,state,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_SEQUENCE_GET_FULL_TYPE(d,state,tuple), \
                BOOST_VMD_TYPE_SEQ \
                ), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND_SEQ_SINGLE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,tuple) \
        ) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND(d,state,tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND_SEQ(d,state,tuple), \
        BOOST_VMD_DETAIL_SEQUENCE_INCREMENT_INDEX, \
        BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND_PROCESS \
        ) \
    (d,state,tuple) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_INCREMENT_INDEX(d,state,tuple) \
    BOOST_PP_TUPLE_REPLACE_D(d,state,BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX_ELEM,BOOST_PP_INC(BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX(state))) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_TEST_TYPE_TUPLE(d,state,tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY \
            ( \
            BOOST_PP_TUPLE_ELEM(0,tuple) \
            ), \
        BOOST_VMD_DETAIL_SEQUENCE_INCREMENT_INDEX, \
        BOOST_VMD_DETAIL_SEQUENCE_TYPE_FOUND \
        ) \
    (d,state,tuple) \
/**/

#define    BOOST_VMD_DETAIL_SEQUENCE_TEST_TYPE(d,call,state) \
    BOOST_VMD_DETAIL_SEQUENCE_TEST_TYPE_TUPLE \
        ( \
        d, \
        state, \
        call(BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state),BOOST_VMD_RETURN_AFTER) \
        ) \
/**/
    
#define    BOOST_VMD_DETAIL_SEQUENCE_TEST_TYPE_D(d,call,state) \
    BOOST_VMD_DETAIL_SEQUENCE_TEST_TYPE_TUPLE \
        ( \
        d, \
        state, \
        call(d,BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state),BOOST_VMD_RETURN_AFTER) \
        ) \
/**/
    
#define BOOST_VMD_DETAIL_SEQUENCE_GCLRT(state) \
    BOOST_PP_CAT \
        ( \
        BOOST_VMD_DETAIL_, \
        BOOST_PP_CAT(BOOST_VMD_DETAIL_SEQUENCE_STATE_GET_TYPE(state),_D) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_GCLPL(state) \
    BOOST_PP_CAT \
        ( \
        BOOST_VMD_DETAIL_, \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_GET_TYPE(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST_GCL(state,rflag) \
    BOOST_PP_IIF \
        ( \
        rflag, \
        BOOST_VMD_DETAIL_SEQUENCE_GCLRT, \
        BOOST_VMD_DETAIL_SEQUENCE_GCLPL \
        ) \
    (state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST_RT_CALL(d,call,state,rflag) \
    BOOST_PP_IIF \
        ( \
        rflag, \
        BOOST_VMD_DETAIL_SEQUENCE_TEST_TYPE_D, \
        BOOST_VMD_DETAIL_SEQUENCE_TEST_TYPE \
        ) \
    (d,call,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST_RT(d,state,rflag) \
    BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST_RT_CALL \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST_GCL(state,rflag), \
        state, \
        rflag \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST(d,state) \
    BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST_RT \
        ( \
        d, \
        state, \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_GET_TYPE_REENTRANT(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_INNER_OP(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL_D \
            ( \
            d, \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX(state), \
            BOOST_PP_TUPLE_SIZE(BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state)) \
            ), \
        BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_UNKNOWN, \
        BOOST_VMD_DETAIL_SEQUENCE_INNER_OP_TEST \
        ) \
    (d,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_INNER_PRED(d,state) \
    BOOST_PP_NOT_EQUAL_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_INDEX(state), \
        BOOST_PP_INC(BOOST_PP_TUPLE_SIZE(BOOST_VMD_DETAIL_SEQUENCE_STATE_TYPES(state))) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES_ELEM_FROM(d,from) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_GENERAL_RETURN(d,from), \
        ((SEQ,1),(TUPLE,1)), \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_EXACT_RETURN(d,from), \
            ((SEQ,1),(LIST,1),(ARRAY,1),(TUPLE,1)), \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_ARRAY_RETURN(d,from), \
                ((SEQ,1),(ARRAY,1),(TUPLE,1)), \
                BOOST_PP_IIF \
                    ( \
                    BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_LIST_RETURN(d,from), \
                    ((SEQ,1),(LIST,1),(TUPLE,1)), \
                    ((SEQ,1),(TUPLE,1)) \
                    ) \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES_ELEM(d,state) \
    BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES_ELEM_FROM \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES_ANY(d,state) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY \
                ( \
                BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state) \
                ), \
            BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES_ELEM, \
            BOOST_VMD_IDENTITY(((SEQ,1),(TUPLE,1))) \
            ) \
        (d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_PROCESSING_ELEM(d,state), \
        BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES_ELEM, \
        BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES_ANY \
        ) \
    (d,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN(d,state) \
    BOOST_PP_WHILE_ ## d \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_INNER_PRED, \
        BOOST_VMD_DETAIL_SEQUENCE_INNER_OP, \
        BOOST_PP_TUPLE_PUSH_BACK \
            ( \
            BOOST_PP_TUPLE_PUSH_BACK \
                ( \
                state, \
                BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN_TUPLE_TYPES(d,state) \
                ), \
            0 \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_ID_LOOP(d,state) \
    BOOST_PP_WHILE_ ## d \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_INNER_PRED, \
        BOOST_VMD_DETAIL_SEQUENCE_INNER_OP, \
        BOOST_PP_TUPLE_PUSH_BACK(BOOST_PP_TUPLE_PUSH_BACK(state,((IDENTIFIER,1))),0) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_ID_EL(d,state) \
    ( \
    BOOST_PP_TUPLE_ELEM(1,BOOST_VMD_DETAIL_LIST_D(d,BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state),BOOST_VMD_RETURN_AFTER)), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT_ADD(d,state,(BOOST_VMD_TYPE_LIST,BOOST_PP_NIL)), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state) \
    ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_ID(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY_LIST_D \
            ( \
            d, \
            BOOST_VMD_DETAIL_IDENTIFIER_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state) \
                ) \
            ), \
        BOOST_VMD_DETAIL_SEQUENCE_OP_ID_EL, \
        BOOST_VMD_DETAIL_SEQUENCE_OP_ID_LOOP \
        ) \
    (d,state) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP_REDUCE_STATE(state) \
    ( \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_OUTTYPE(state), \
    BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state) \
    ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_OP(d,state) \
    BOOST_VMD_DETAIL_SEQUENCE_OP_REDUCE_STATE \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_IS_BEGIN_PARENS \
                ( \
                BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state) \
                ), \
            BOOST_VMD_DETAIL_SEQUENCE_OP_PAREN, \
            BOOST_VMD_DETAIL_SEQUENCE_OP_ID \
            ) \
        (d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_PRED_CELEM_SZ(d,state) \
    BOOST_PP_LESS_EQUAL_D \
        ( \
        d, \
        BOOST_PP_SEQ_SIZE(BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state)), \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_PRED_CELEM(d,state) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITOR \
                ( \
                BOOST_VMD_IS_EMPTY \
                    ( \
                    BOOST_VMD_DETAIL_SEQUENCE_STATE_ELEM(state) \
                    ), \
                BOOST_VMD_IS_EMPTY \
                    ( \
                    BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state) \
                    ) \
                ), \
            BOOST_VMD_IDENTITY(1), \
            BOOST_VMD_DETAIL_SEQUENCE_PRED_CELEM_SZ \
            ) \
        (d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_PRED(d,state) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY \
                ( \
                BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state) \
                ), \
            BOOST_VMD_IDENTITY(0), \
            BOOST_VMD_DETAIL_SEQUENCE_PRED_CELEM \
            ) \
        (d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE_CHECK(output) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE(output,BOOST_VMD_TYPE_ARRAY), \
        (0,()), \
        BOOST_PP_NIL \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE_CHECK_D(d,output) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,output,BOOST_VMD_TYPE_ARRAY), \
        (0,()), \
        BOOST_PP_NIL \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE(output) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITOR \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE(output,BOOST_VMD_TYPE_SEQ), \
            BOOST_VMD_DETAIL_EQUAL_TYPE(output,BOOST_VMD_TYPE_TUPLE) \
            ), \
        BOOST_VMD_EMPTY, \
        BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE_CHECK \
        ) \
    (output) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE_D(d,output) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITOR \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,output,BOOST_VMD_TYPE_SEQ), \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,output,BOOST_VMD_TYPE_TUPLE) \
            ), \
        BOOST_VMD_EMPTY, \
        BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE_CHECK_D \
        ) \
    (d,output) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE_GET(state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state) \
            ), \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state), \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state) \
            ), \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state) \
        ) 
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE_GET_D(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D \
            ( \
            d, \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_FROM(state) \
            ), \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state), \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_INPUT(state) \
            ), \
            BOOST_VMD_DETAIL_SEQUENCE_STATE_RESULT(state) \
        ) 
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE(vseq,elem,output,from) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE_GET \
        ( \
        BOOST_PP_WHILE \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_PRED, \
            BOOST_VMD_DETAIL_SEQUENCE_OP, \
                ( \
                vseq, \
                BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE \
                    ( \
                    output \
                    ), \
                elem, \
                output, \
                from \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE_D(d,vseq,elem,output,from) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE_GET_D \
        ( \
        d, \
        BOOST_PP_WHILE_ ## d \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_PRED, \
            BOOST_VMD_DETAIL_SEQUENCE_OP, \
                ( \
                vseq, \
                BOOST_VMD_DETAIL_SEQUENCE_EMPTY_TYPE_D \
                    ( \
                    d, \
                    output \
                    ), \
                elem, \
                output, \
                from \
                ) \
            ) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_SEQUENCE_COMMON_HPP */
