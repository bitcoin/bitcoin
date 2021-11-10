
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_SEQUENCE_ELEM_HPP)
#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_HPP

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/logical/compl.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/preprocessor/tuple/replace.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_identifier.hpp>
#include <boost/vmd/detail/empty_result.hpp>
#include <boost/vmd/detail/equal_type.hpp>
#include <boost/vmd/detail/match_identifier.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/not_empty.hpp>
#include <boost/vmd/detail/only_after.hpp>
#include <boost/vmd/detail/sequence_common.hpp>

/*

    Given modifications and the requested type, 
    determine whether or not we should be checking for specific identifiers
    
    1 = check for specific identifiers
    0 = do no check for specific identifiers

*/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_CHECK_FOR_IDENTIFIERS(nm,type) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            type, \
            BOOST_VMD_TYPE_IDENTIFIER \
            ), \
        BOOST_VMD_DETAIL_NOT_EMPTY \
            ( \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_CHECK_FOR_IDENTIFIERS_D(d,nm,type) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            type, \
            BOOST_VMD_TYPE_IDENTIFIER \
            ), \
        BOOST_VMD_DETAIL_NOT_EMPTY \
            ( \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
            ) \
        ) \
/**/

/*

    Given modifications, determine whether or not an index should be part of the result
    
    1 = index should be part of the result
    0 = index should not be part of the result

*/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_IS_INDEX_RESULT(nm) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_INDEX(nm), \
        BOOST_PP_BITAND \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE \
                ( \
                BOOST_VMD_DETAIL_MODS_RESULT_TYPE(nm), \
                BOOST_VMD_TYPE_IDENTIFIER \
                ), \
            BOOST_VMD_DETAIL_NOT_EMPTY \
                ( \
                BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_IS_INDEX_RESULT_D(d,nm) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_INDEX(nm), \
        BOOST_PP_BITAND \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                ( \
                d, \
                BOOST_VMD_DETAIL_MODS_RESULT_TYPE(nm), \
                BOOST_VMD_TYPE_IDENTIFIER \
                ), \
            BOOST_VMD_DETAIL_NOT_EMPTY \
                ( \
                BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_INDEX(nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(nm), \
        (,,), \
        (,) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_NO_INDEX(nm) \
    BOOST_PP_EXPR_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(nm), \
        (,) \
        ) \
/**/

/*

    Returns a failure result given the modifications

*/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT(nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_IS_INDEX_RESULT(nm), \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_INDEX, \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_NO_INDEX \
            ) \
        (nm),nm, \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_D(d,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_IS_INDEX_RESULT_D(d,nm), \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_INDEX, \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_NO_INDEX \
            ) \
        (nm),nm, \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_TUPLE(res) \
    BOOST_PP_TUPLE_ELEM \
        ( \
        0, \
        BOOST_PP_TUPLE_ELEM(0,res) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_ONLY(res) \
    BOOST_PP_TUPLE_ELEM(0,res) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_TUPLE(res) \
    BOOST_PP_TUPLE_ELEM \
        ( \
        1, \
        BOOST_PP_TUPLE_ELEM(0,res) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_ONLY(res) \
    BOOST_PP_TUPLE_ELEM(1,res) \
/**/

/*

  Gets the 'data' of the result given the result and modifications

*/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA(res,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_TUPLE, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_ONLY \
        ) \
    (res) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_D(d,res,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D(d,nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_TUPLE, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_ONLY \
        ) \
    (res) \
/**/

/*

  Gets the 'type' of the result given the result and modifications

*/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE(res,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_TUPLE, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_ONLY \
        ) \
    (res) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_D(d,res,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D(d,nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_TUPLE, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_ONLY \
        ) \
    (res) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_HAS_FAILED_TUPLE(res) \
    BOOST_VMD_IS_EMPTY \
        ( \
        BOOST_PP_TUPLE_ELEM(0,res) \
        ) \
/**/

/*

    Determines whether the result from the element access has failed or not
    
    returns 1 if it has failed, otherwise 0.

*/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_HAS_FAILED(res,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_HAS_FAILED_TUPLE, \
        BOOST_VMD_IS_EMPTY \
        ) \
    (res) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_HAS_FAILED_D(d,res,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D(d,nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_HAS_FAILED_TUPLE, \
        BOOST_VMD_IS_EMPTY \
        ) \
    (res) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY_CHELM(seq,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_GREATER(BOOST_PP_SEQ_SIZE(seq),elem), \
        BOOST_PP_SEQ_ELEM, \
        BOOST_VMD_EMPTY \
        ) \
    (elem,seq) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY_CHELM_D(d,seq,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_GREATER_D(d,BOOST_PP_SEQ_SIZE(seq),elem), \
        BOOST_PP_SEQ_ELEM, \
        BOOST_VMD_EMPTY \
        ) \
    (elem,seq) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_CHELM_RES(data,elem) \
    ( \
    BOOST_PP_SEQ_ELEM(elem,BOOST_PP_TUPLE_ELEM(0,data)), \
    BOOST_PP_TUPLE_ELEM(1,data) \
    ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_CHELM(data,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_GREATER(BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(0,data)),elem), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_CHELM_RES, \
        BOOST_VMD_DETAIL_EMPTY_RESULT \
        ) \
    (data,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_CHELM_D(d,data,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_GREATER_D(d,BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(0,data)),elem), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_CHELM_RES, \
        BOOST_VMD_DETAIL_EMPTY_RESULT \
        ) \
    (data,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY(seq,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(seq), \
        BOOST_VMD_EMPTY, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY_CHELM \
        ) \
    (seq,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY_D(d,seq,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(seq), \
        BOOST_VMD_EMPTY, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY_CHELM_D \
        ) \
    (d,seq,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER(data,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(BOOST_PP_TUPLE_ELEM(0,data)), \
        BOOST_VMD_DETAIL_EMPTY_RESULT, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_CHELM \
        ) \
    (data,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_D(d,data,elem) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(BOOST_PP_TUPLE_ELEM(0,data)), \
        BOOST_VMD_DETAIL_EMPTY_RESULT, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_CHELM_D \
        ) \
    (d,data,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ(seq,elem,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY \
        ) \
    (seq,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_D(d,seq,elem,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D(d,nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_AFTER_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_ONLY_D \
        ) \
    (d,seq,elem) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_VSEQ(...) \
    BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PROCESS(elem,vseq,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE \
            ( \
            vseq, \
            elem, \
            BOOST_VMD_TYPE_SEQ, \
            nm \
            ), \
        elem, \
        nm \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PROCESS_D(d,elem,vseq,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FSEQ_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE_D \
            ( \
            d, \
            vseq, \
            elem, \
            BOOST_VMD_TYPE_SEQ, \
            nm \
            ), \
        elem, \
        nm \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_EMPTY(elem,vseq,nm) \
    BOOST_PP_EXPR_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER(nm), \
        (,) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_EMPTY_D(d,elem,vseq,nm) \
    BOOST_PP_EXPR_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D(d,nm), \
        (,) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_CE(elem,vseq,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_NOT_EMPTY(vseq), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PROCESS, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_EMPTY \
        ) \
    (elem,vseq,nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_CE_D(d,elem,vseq,nm) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_NOT_EMPTY(vseq), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PROCESS_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_EMPTY_D \
        ) \
    (d,elem,vseq,nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN_D(d,res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN(res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN(res,nm,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_MODS_IS_RESULT_ONLY_AFTER(nm), \
            BOOST_PP_TUPLE_ELEM, \
            BOOST_VMD_IDENTITY(res) \
            ) \
        (1,res) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_INDEX_JRES(data,index) \
    BOOST_PP_IF \
        ( \
        index, \
        (data,BOOST_PP_DEC(index)), \
        (,) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_JRES(data,index) \
    BOOST_PP_EXPR_IF \
        ( \
        index, \
        data \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_MATCH(data,nm,index) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_INDEX(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_INDEX_JRES, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_JRES \
        ) \
    (data,index) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID_TUP(data,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_MATCH \
        ( \
        data, \
        nm, \
        BOOST_VMD_DETAIL_MATCH_IDENTIFIER \
            ( \
            data, \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID_TUP_D(d,data,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_MATCH \
        ( \
        data, \
        nm, \
        BOOST_VMD_DETAIL_MATCH_IDENTIFIER_D \
            ( \
            d, \
            data, \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID(data,nm,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_CHECK_FOR_IDENTIFIERS(nm,type), \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID_TUP,    \
            BOOST_VMD_IDENTITY(data) \
            ) \
        (data,nm) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID_D(d,data,nm,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_CHECK_FOR_IDENTIFIERS_D(d,nm,type), \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID_TUP_D,    \
            BOOST_VMD_IDENTITY(data) \
            ) \
        (d,data,nm) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY(res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID \
        ( \
        BOOST_PP_TUPLE_ELEM(1,res), \
        nm, \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_D(d,res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_ID_D \
        ( \
        d, \
        BOOST_PP_TUPLE_ELEM(1,res), \
        nm, \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_CHANGE(res,nm,type) \
    BOOST_PP_TUPLE_REPLACE \
        ( \
        res, \
        0, \
        BOOST_PP_TUPLE_ELEM \
            ( \
            1, \
            BOOST_PP_TUPLE_ELEM(0,res) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_CHANGE_D(d,res,nm,type) \
    BOOST_PP_TUPLE_REPLACE_D \
        ( \
        d, \
        res, \
        0, \
        BOOST_PP_TUPLE_ELEM \
            ( \
            1, \
            BOOST_PP_TUPLE_ELEM(0,res) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER(res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_ONLY_AFTER(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_CHANGE \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER_D(d,res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_ONLY_AFTER(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_CHANGE_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_SUCCESS(res,nm,type,index) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER \
        ( \
        BOOST_PP_TUPLE_PUSH_BACK(res,BOOST_PP_DEC(index)), \
        nm, \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_SUCCESS_D(d,res,nm,type,index) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER_D \
        ( \
        d, \
        BOOST_PP_TUPLE_PUSH_BACK(res,BOOST_PP_DEC(index)), \
        nm, \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_FAILURE(res,nm,type,index) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT(nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_FAILURE_D(d,res,nm,type,index) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_D(d,nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_INDEX_JRES(res,nm,type,index) \
    BOOST_PP_IF \
        ( \
        index, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_SUCCESS, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_FAILURE \
        ) \
    (res,nm,type,index)    \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_INDEX_JRES_D(d,res,nm,type,index) \
    BOOST_PP_IF \
        ( \
        index, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_SUCCESS_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_FAILURE_D \
        ) \
    (d,res,nm,type,index)    \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES_FAILURE(res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT(nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES_FAILURE_D(d,res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_D(d,nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES(res,nm,type,index) \
    BOOST_PP_IF \
        ( \
        index, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES_FAILURE \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES_D(d,res,nm,type,index) \
    BOOST_PP_IF \
        ( \
        index, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES_FAILURE_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID_RES(res,nm,type,index) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_INDEX(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_INDEX_JRES, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES \
        ) \
    (res,nm,type,index)    \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID_RES_D(d,res,nm,type,index) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_INDEX(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_INDEX_JRES_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_JRES_D \
        ) \
    (d,res,nm,type,index)    \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID(res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID_RES \
        ( \
        res, \
        nm, \
        type, \
        BOOST_VMD_DETAIL_MATCH_IDENTIFIER \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA(res,nm), \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID_D(d,res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID_RES_D \
        ( \
        d, \
        res, \
        nm, \
        type, \
        BOOST_VMD_DETAIL_MATCH_IDENTIFIER_D \
            ( \
            d, \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_D(d,res,nm), \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(nm) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT(res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_CHECK_FOR_IDENTIFIERS(nm,type), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_D(d,res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_CHECK_FOR_IDENTIFIERS_D(d,nm,type), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ID_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_ONLY_CAFTER_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK(res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER(nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_D(d,res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_STATE_IS_AFTER_D(d,nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_SPLIT_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_ONLY_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_FAILED(res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT(nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_FAILED_D(d,res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_D(d,nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_ID(res,nm,type) \
    BOOST_VMD_IS_IDENTIFIER \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA(res,nm) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_ID_D(d,res,nm,type) \
    BOOST_VMD_IS_IDENTIFIER_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_DATA_D(d,res,nm) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_OTHER(res,nm,type) \
    BOOST_VMD_DETAIL_EQUAL_TYPE \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE(res,nm), \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_OTHER_D(d,res,nm,type) \
    BOOST_VMD_DETAIL_EQUAL_TYPE_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_RESULT_TYPE_D(d,res,nm), \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE(res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            BOOST_VMD_TYPE_IDENTIFIER, \
            type \
            ), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_ID, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_OTHER \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_D(d,res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            BOOST_VMD_TYPE_IDENTIFIER, \
            type \
            ), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_ID_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_OTHER_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT(res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE(res,nm,type), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_FAILED \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_D(d,res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_MATCHING_TYPE_D(d,res,nm,type), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_CHECK_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_FAILED_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_NF(res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(type), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_NF_D(d,res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(type), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_FIN_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_PT_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_FAILED(res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT(nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_FAILED_D(d,res,nm,type) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_FAILURE_RESULT_D(d,nm) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE(res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_HAS_FAILED(res,nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_FAILED, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_NF \
        ) \
    (res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_D(d,res,nm,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_HAS_FAILED_D(d,res,nm), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_FAILED_D, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_NF_D \
        ) \
    (d,res,nm,type) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_COA(res,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE \
        ( \
        res, \
        nm, \
        BOOST_VMD_DETAIL_MODS_RESULT_TYPE(nm) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_COA_D(d,res,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_TYPE_D \
        ( \
        d, \
        res, \
        nm, \
        BOOST_VMD_DETAIL_MODS_RESULT_TYPE(nm) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM(elem,vseq,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_COA \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_CE(elem,vseq,nm), \
        nm \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_D(d,elem,vseq,nm) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_COA_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_CE_D(d,elem,vseq,nm), \
        nm \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM(allow,elem,...) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM \
        ( \
        elem, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_VSEQ(__VA_ARGS__), \
        BOOST_VMD_DETAIL_NEW_MODS(allow,__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_ELEM_D(d,allow,elem,...) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_NM_D \
        ( \
        d, \
        elem, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_VSEQ(__VA_ARGS__), \
        BOOST_VMD_DETAIL_NEW_MODS_D(d,allow,__VA_ARGS__) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_SEQUENCE_ELEM_HPP */
