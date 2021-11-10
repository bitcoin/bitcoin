
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_EQUAL_12_HPP)
#define BOOST_VMD_DETAIL_EQUAL_12_HPP

#include <boost/vmd/detail/recurse/equal/equal_headers.hpp>

#define BOOST_VMD_DETAIL_EQUAL_12_CNI_CHK(vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITOR \
            ( \
            BOOST_PP_BITOR \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_ARRAY), \
                BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_LIST) \
                ), \
            BOOST_PP_BITOR \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_SEQ), \
                BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_TUPLE) \
                ) \
            ), \
        BOOST_VMD_DETAIL_DATA_EQUAL_12, \
        BOOST_VMD_DETAIL_EQUAL_CNI_SMP \
        ) \
    (vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_CNI_CHK_D(d,vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITOR \
            ( \
            BOOST_PP_BITOR \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_ARRAY), \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_LIST) \
                ), \
            BOOST_PP_BITOR \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_SEQ), \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_TUPLE) \
                ) \
            ), \
        BOOST_VMD_DETAIL_DATA_EQUAL_12_D, \
        BOOST_VMD_DETAIL_EQUAL_CNI_SMP_D \
        ) \
    (d,vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_VSEQ(vseq1,vseq2,vtype) \
    BOOST_VMD_DETAIL_DATA_EQUAL_12 \
        ( \
        BOOST_VMD_TO_SEQ(vseq1), \
        BOOST_VMD_TO_SEQ(vseq2), \
        BOOST_VMD_TYPE_SEQ \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_VSEQ_D(d,vseq1,vseq2,vtype) \
    BOOST_VMD_DETAIL_DATA_EQUAL_12_D \
        ( \
        d, \
        BOOST_VMD_TO_SEQ_D(d,vseq1), \
        BOOST_VMD_TO_SEQ_D(d,vseq2), \
        BOOST_VMD_TYPE_SEQ \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_CNI(vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            vtype, \
            BOOST_VMD_TYPE_SEQUENCE \
            ), \
        BOOST_VMD_DETAIL_EQUAL_12_VSEQ, \
        BOOST_VMD_DETAIL_EQUAL_12_CNI_CHK \
        ) \
    (vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_CNI_D(d,vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            vtype, \
            BOOST_VMD_TYPE_SEQUENCE \
            ), \
        BOOST_VMD_DETAIL_EQUAL_12_VSEQ_D, \
        BOOST_VMD_DETAIL_EQUAL_12_CNI_CHK_D \
        ) \
    (d,vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_WT_CHECK(vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE \
                ( \
                vtype1, \
                vtype2 \
                ), \
            BOOST_VMD_DETAIL_EQUAL_12_CNI, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (vseq1,vseq2,vtype1) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_WT_CHECK_D(d,vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                ( \
                d, \
                vtype1, \
                vtype2 \
                ), \
            BOOST_VMD_DETAIL_EQUAL_12_CNI_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,vseq1,vseq2,vtype1) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_WT_CONVERT(vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_DETAIL_EQUAL_12_WT_CHECK \
        ( \
        vseq1, \
        vseq2, \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE(vseq1,vtype1), \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE(vseq2,vtype2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_WT_CONVERT_D(d,vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_DETAIL_EQUAL_12_WT_CHECK_D \
        ( \
        d, \
        vseq1, \
        vseq2, \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_D(d,vseq1,vtype1), \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_D(d,vseq2,vtype2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_WT(vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH(vseq1,vseq2,vtype1,vtype2), \
            BOOST_VMD_IDENTITY(0), \
            BOOST_VMD_DETAIL_EQUAL_12_WT_CONVERT \
            ) \
        (vseq1,vseq2,vtype1,vtype2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_WT_D(d,vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH_D(d,vseq1,vseq2,vtype1,vtype2), \
            BOOST_VMD_IDENTITY(0), \
            BOOST_VMD_DETAIL_EQUAL_12_WT_CONVERT_D \
            ) \
        (d,vseq1,vseq2,vtype1,vtype2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_GTYPE(vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_12_WT \
        ( \
        vseq1, \
        vseq2, \
        BOOST_VMD_GET_TYPE(vseq1,BOOST_VMD_RETURN_TYPE_TUPLE), \
        BOOST_VMD_GET_TYPE(vseq2,BOOST_VMD_RETURN_TYPE_TUPLE) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_GTYPE_D(d,vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_12_WT_D \
        ( \
        d, \
        vseq1, \
        vseq2, \
        BOOST_VMD_GET_TYPE_D(d,vseq1,BOOST_VMD_RETURN_TYPE_TUPLE), \
        BOOST_VMD_GET_TYPE_D(d,vseq2,BOOST_VMD_RETURN_TYPE_TUPLE) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_EBP(vseq1,vseq2,be1,be2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_CHK_MATCH(be1,be2), \
                BOOST_VMD_DETAIL_EQUAL_CHK_PARENS_MATCH(vseq1,vseq2) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_12_GTYPE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (vseq1,vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_EBP_D(d,vseq1,vseq2,be1,be2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_CHK_MATCH(be1,be2), \
                BOOST_VMD_DETAIL_EQUAL_CHK_PARENS_MATCH(vseq1,vseq2) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_12_GTYPE_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,vseq1,vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_EMPTY(vseq1,vseq2,be1,be2) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITAND(be1,be2), \
        BOOST_VMD_DETAIL_EQUAL_BOTH_EMPTY, \
        BOOST_VMD_DETAIL_EQUAL_12_EBP \
        ) \
    (vseq1,vseq2,be1,be2) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_EMPTY_D(d,vseq1,vseq2,be1,be2) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITAND(be1,be2), \
        BOOST_VMD_DETAIL_EQUAL_BOTH_EMPTY, \
        BOOST_VMD_DETAIL_EQUAL_12_EBP_D \
        ) \
    (d,vseq1,vseq2,be1,be2) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12(vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_12_EMPTY \
        ( \
        vseq1, \
        vseq2, \
        BOOST_VMD_IS_EMPTY(vseq1), \
        BOOST_VMD_IS_EMPTY(vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_12_D(d,vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_12_EMPTY_D \
        ( \
        d, \
        vseq1, \
        vseq2, \
        BOOST_VMD_IS_EMPTY(vseq1), \
        BOOST_VMD_IS_EMPTY(vseq2) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_EQUAL_12_HPP */
