
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_DATA_EQUAL_COMMON_HPP)
#define BOOST_VMD_DETAIL_DATA_EQUAL_COMMON_HPP

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/array/size.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/list/at.hpp>
#include <boost/preprocessor/list/size.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/logical/compl.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/preprocessor/tuple/replace.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/get_type.hpp>
#include <boost/vmd/detail/equal_type.hpp>

#define BOOST_VMD_DETAIL_DATA_EQUAL_IS_BOTH_COMPOSITE(vseq1,vseq2) \
    BOOST_PP_BITAND \
        ( \
        BOOST_PP_IS_BEGIN_PARENS(vseq1), \
        BOOST_PP_IS_BEGIN_PARENS(vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_ARRAY(d,index,data) \
    BOOST_PP_ARRAY_ELEM(index,data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_LIST(d,index,data) \
    BOOST_PP_LIST_AT_D(d,data,index) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_SEQ(d,index,data) \
    BOOST_PP_SEQ_ELEM(index,data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_TUPLE(d,index,data) \
    BOOST_PP_TUPLE_ELEM(index,data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_ARRAY(data) \
    BOOST_PP_ARRAY_SIZE(data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_LIST(data) \
    BOOST_PP_LIST_SIZE(data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_SEQ(data) \
    BOOST_PP_SEQ_SIZE(data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_TUPLE(data) \
    BOOST_PP_TUPLE_SIZE(data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_ARRAY_D(d,data) \
    BOOST_PP_ARRAY_SIZE(data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_LIST_D(d,data) \
    BOOST_PP_LIST_SIZE_D(d,data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_SEQ_D(d,data) \
    BOOST_PP_SEQ_SIZE(data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_TUPLE_D(d,data) \
    BOOST_PP_TUPLE_SIZE(data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM(d,index,data,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_ARRAY), \
        BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_ARRAY, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_LIST), \
            BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_LIST, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_SEQ), \
                BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_SEQ, \
                BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM_TUPLE \
                ) \
            ) \
        ) \
    (d,index,data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE(data,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_ARRAY), \
        BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_ARRAY, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_LIST), \
            BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_LIST, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_SEQ), \
                BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_SEQ, \
                BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_TUPLE \
                ) \
            ) \
        ) \
    (data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_D(d,data,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_ARRAY), \
        BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_ARRAY_D, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_LIST), \
            BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_LIST_D, \
            BOOST_PP_IIF \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_SEQ), \
                BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_SEQ_D, \
                BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_TUPLE_D \
                ) \
            ) \
        ) \
    (d,data) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_RESULT(state) \
    BOOST_PP_TUPLE_ELEM(0,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_FIRST(state) \
    BOOST_PP_TUPLE_ELEM(1,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_SECOND(state) \
    BOOST_PP_TUPLE_ELEM(2,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_SIZE(state) \
    BOOST_PP_TUPLE_ELEM(3,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_TYPE(state) \
    BOOST_PP_TUPLE_ELEM(4,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_INDEX(state) \
    BOOST_PP_TUPLE_ELEM(5,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP(state) \
    BOOST_PP_TUPLE_ELEM(6,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PROCESSING(d,state) \
    BOOST_PP_BITAND \
        ( \
        BOOST_PP_EQUAL_D \
            ( \
            d, \
            BOOST_VMD_DETAIL_DATA_EQUAL_STATE_INDEX(state), \
            BOOST_VMD_DETAIL_DATA_EQUAL_STATE_SIZE(state) \
            ), \
        BOOST_PP_COMPL(BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_EMPTY(state)) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_EMPTY(state) \
    BOOST_VMD_IS_EMPTY(BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP(state)) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_INDEX(state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_EMPTY(state), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_TUPLE_ELEM \
        ) \
    (0,BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP(state)) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_POP_NE_EMPTY(d,state) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        state, \
        6, \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_POP_NE_REMOVE(d,state) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        state, \
        6, \
        BOOST_PP_TUPLE_POP_FRONT(BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP(state)) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_POP(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL_D(d,BOOST_PP_TUPLE_SIZE(BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP(state)),1), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_POP_NE_EMPTY, \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_POP_NE_REMOVE \
        ) \
    (d,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PUSH_CREATE(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_OP_SUCCESS \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            6, \
            (BOOST_VMD_DETAIL_DATA_EQUAL_STATE_INDEX(state)) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PUSH_ADD(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_OP_SUCCESS \
        ( \
        d, \
        BOOST_PP_TUPLE_REPLACE_D \
            ( \
            d, \
            state, \
            6, \
            BOOST_PP_TUPLE_PUSH_BACK \
                ( \
                BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP(state), \
                BOOST_VMD_DETAIL_DATA_EQUAL_STATE_INDEX(state) \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PUSH(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_EMPTY(state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PUSH_CREATE, \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PUSH_ADD \
        ) \
    (d,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_INDEX(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PROCESSING(d,state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_INDEX, \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_INDEX \
        ) \
    (state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_FIRST_ELEMENT(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM \
        ( \
        d, \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_INDEX(d,state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_FIRST(state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_TYPE(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_SECOND_ELEMENT(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_GET_ELEM \
        ( \
        d, \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_INDEX(d,state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_SECOND(state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_TYPE(state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_PRED(d,state) \
    BOOST_PP_BITAND \
        ( \
        BOOST_PP_EQUAL_D \
            ( \
            d, \
            BOOST_VMD_DETAIL_DATA_EQUAL_STATE_RESULT(state), \
            1 \
            ), \
        BOOST_PP_BITOR \
            ( \
            BOOST_PP_NOT_EQUAL_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_DATA_EQUAL_STATE_INDEX(state), \
                BOOST_VMD_DETAIL_DATA_EQUAL_STATE_SIZE(state) \
                ), \
            BOOST_PP_COMPL \
                ( \
                BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_EMPTY(state) \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_SUCCESS_NCP(d,state) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        state, \
        5, \
        BOOST_PP_INC(BOOST_VMD_DETAIL_DATA_EQUAL_STATE_INDEX(state)) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_SUCCESS(d,state) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PROCESSING(d,state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_POP, \
        BOOST_VMD_DETAIL_DATA_EQUAL_OP_SUCCESS_NCP \
        ) \
    (d,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_FAILURE(d,state) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        state, \
        0, \
        0 \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_RESULT(d,state,result) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL_D(d,result,0), \
        BOOST_VMD_DETAIL_DATA_EQUAL_OP_FAILURE, \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL_D(d,result,1), \
            BOOST_VMD_DETAIL_DATA_EQUAL_OP_SUCCESS, \
            BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PUSH \
            ) \
        ) \
    (d,state) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_TYPE(emf,ems,vtype) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            BOOST_VMD_GET_TYPE(emf), \
            vtype \
            ), \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            BOOST_VMD_GET_TYPE(ems), \
            vtype \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_TYPE_D(d,emf,ems,vtype) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            BOOST_VMD_GET_TYPE(emf), \
            vtype \
            ), \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            BOOST_VMD_GET_TYPE(ems), \
            vtype \
            ) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_DATA_EQUAL_COMMON_HPP */
