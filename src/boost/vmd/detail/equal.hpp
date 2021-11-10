
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_EQUAL_HPP)
#define BOOST_VMD_DETAIL_EQUAL_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/detail/auto_rec.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/overload.hpp>
#include <boost/preprocessor/logical/compl.hpp>
#include <boost/vmd/detail/recurse/equal/equal_headers.hpp>
#include <boost/vmd/is_identifier.hpp>
#include <boost/vmd/is_empty_list.hpp>
#include <boost/vmd/detail/not_empty.hpp>

#include <boost/vmd/detail/recurse/equal/equal_1.hpp>
#include <boost/vmd/detail/recurse/equal/equal_2.hpp>
#include <boost/vmd/detail/recurse/equal/equal_3.hpp>
#include <boost/vmd/detail/recurse/equal/equal_4.hpp>
#include <boost/vmd/detail/recurse/equal/equal_5.hpp>
#include <boost/vmd/detail/recurse/equal/equal_6.hpp>
#include <boost/vmd/detail/recurse/equal/equal_7.hpp>
#include <boost/vmd/detail/recurse/equal/equal_8.hpp>
#include <boost/vmd/detail/recurse/equal/equal_9.hpp>
#include <boost/vmd/detail/recurse/equal/equal_10.hpp>
#include <boost/vmd/detail/recurse/equal/equal_11.hpp>
#include <boost/vmd/detail/recurse/equal/equal_12.hpp>
#include <boost/vmd/detail/recurse/equal/equal_13.hpp>
#include <boost/vmd/detail/recurse/equal/equal_14.hpp>
#include <boost/vmd/detail/recurse/equal/equal_15.hpp>
#include <boost/vmd/detail/recurse/equal/equal_16.hpp>

#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_1(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_1_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_2(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_2_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_3(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_3_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_4(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_4_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_5(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_5_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_6(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_6_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_7(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_7_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_8(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_8_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_9(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_9_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_10(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_10_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_11(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_11_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_12(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_12_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_13(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_13_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_14(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_14_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_15(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_15_D(d,vseq1,vseq2)
#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_16(d,vseq1,vseq2) BOOST_VMD_DETAIL_EQUAL_16_D(d,vseq1,vseq2)

#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_AUTO_REC_D(n) \
    BOOST_VMD_DETAIL_EQUAL_IS_1 \
        ( \
        BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_ ## n(1,,) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_SIMPLE_D \
    BOOST_PP_CAT \
        ( \
        BOOST_VMD_DETAIL_EQUAL_SIMPLE_D_, \
        BOOST_PP_AUTO_REC(BOOST_VMD_DETAIL_EQUAL_SIMPLE_AUTO_REC_D,16) \
        ) \
/**/


#define BOOST_VMD_DETAIL_EQUAL_CNI_CHK(vseq1,vseq2,vtype) \
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
        BOOST_VMD_DETAIL_DATA_EQUAL, \
        BOOST_VMD_DETAIL_EQUAL_CNI_SMP \
        ) \
    (vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_CNI_CHK_D(d,vseq1,vseq2,vtype) \
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
        BOOST_VMD_DETAIL_DATA_EQUAL_D, \
        BOOST_VMD_DETAIL_EQUAL_CNI_SMP_D \
        ) \
    (d,vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_VSEQ(vseq1,vseq2,vtype) \
    BOOST_VMD_DETAIL_DATA_EQUAL \
        ( \
        BOOST_VMD_TO_SEQ(vseq1), \
        BOOST_VMD_TO_SEQ(vseq2), \
        BOOST_VMD_TYPE_SEQ \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_VSEQ_D(d,vseq1,vseq2,vtype) \
    BOOST_VMD_DETAIL_DATA_EQUAL_D \
        ( \
        d, \
        BOOST_VMD_TO_SEQ_D(d,vseq1), \
        BOOST_VMD_TO_SEQ_D(d,vseq2), \
        BOOST_VMD_TYPE_SEQ \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_CNI(vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            vtype, \
            BOOST_VMD_TYPE_SEQUENCE \
            ), \
        BOOST_VMD_DETAIL_EQUAL_VSEQ, \
        BOOST_VMD_DETAIL_EQUAL_CNI_CHK \
        ) \
    (vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_CNI_D(d,vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            vtype, \
            BOOST_VMD_TYPE_SEQUENCE \
            ), \
        BOOST_VMD_DETAIL_EQUAL_VSEQ_D, \
        BOOST_VMD_DETAIL_EQUAL_CNI_CHK_D \
        ) \
    (d,vseq1,vseq2,vtype) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_TUPLE(vseq1,vtype1,type) \
    BOOST_PP_BITOR \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            BOOST_VMD_TYPE_ARRAY, \
            vtype1 \
            ), \
        BOOST_PP_BITAND \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE \
                ( \
                BOOST_VMD_TYPE_LIST, \
                vtype1 \
                ), \
            BOOST_PP_COMPL \
                ( \
                BOOST_VMD_IS_EMPTY_LIST(vseq1) \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_TUPLE_D(d,vseq1,vtype1,type) \
    BOOST_PP_BITOR \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            BOOST_VMD_TYPE_ARRAY, \
            vtype1 \
            ), \
        BOOST_PP_BITAND \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                ( \
                d, \
                BOOST_VMD_TYPE_LIST, \
                vtype1 \
                ), \
            BOOST_PP_COMPL \
                ( \
                BOOST_VMD_IS_EMPTY_LIST_D(d,vseq1) \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_IDENTIFIER(vseq1,vtype1,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE \
                ( \
                BOOST_VMD_TYPE_IDENTIFIER, \
                type \
                ), \
            BOOST_VMD_IS_IDENTIFIER, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (vseq1) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_IDENTIFIER_D(d,vseq1,vtype1,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                ( \
                d, \
                BOOST_VMD_TYPE_IDENTIFIER, \
                type \
                ), \
            BOOST_VMD_IS_IDENTIFIER_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,vseq1) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK(vseq1,vtype1,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE \
            ( \
            BOOST_VMD_TYPE_TUPLE, \
            type \
            ), \
        BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_TUPLE, \
        BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_IDENTIFIER \
        ) \
    (vseq1,vtype1,type) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_D(d,vseq1,vtype1,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D \
            ( \
            d, \
            BOOST_VMD_TYPE_TUPLE, \
            type \
            ), \
        BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_TUPLE_D, \
        BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_IDENTIFIER_D \
        ) \
    (d,vseq1,vtype1,type) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE(vseq1,vtype1,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE \
                ( \
                vtype1, \
                type \
                ), \
            BOOST_VMD_IDENTITY(1), \
            BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK \
            ) \
        (vseq1,vtype1,type)    \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_D(d,vseq1,vtype1,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                ( \
                d, \
                vtype1, \
                type \
                ), \
            BOOST_VMD_IDENTITY(1), \
            BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_CHECK_D \
            ) \
        (d,vseq1,vtype1,type)    \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_WT_CHECK(vseq1,vseq2,vtype1,vtype2,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE \
                    ( \
                    vtype1, \
                    vtype2 \
                    ), \
                BOOST_VMD_IDENTITY_RESULT \
                    ( \
                    BOOST_PP_IIF \
                        ( \
                        BOOST_VMD_DETAIL_NOT_EMPTY(type), \
                        BOOST_VMD_DETAIL_EQUAL_TEST_TYPE, \
                        BOOST_VMD_IDENTITY(1) \
                        ) \
                    (vseq1,vtype1,type) \
                    ) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_CNI, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (vseq1,vseq2,vtype1) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_WT_CHECK_D(d,vseq1,vseq2,vtype1,vtype2,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                    ( \
                    d, \
                    vtype1, \
                    vtype2 \
                    ), \
                BOOST_VMD_IDENTITY_RESULT \
                    ( \
                    BOOST_PP_IIF \
                        ( \
                        BOOST_VMD_DETAIL_NOT_EMPTY(type), \
                        BOOST_VMD_DETAIL_EQUAL_TEST_TYPE_D, \
                        BOOST_VMD_IDENTITY(1) \
                        ) \
                    (d,vseq1,vtype1,type) \
                    ) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_CNI_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,vseq1,vseq2,vtype1) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_WT_CONVERT(vseq1,vseq2,vtype1,vtype2,type) \
    BOOST_VMD_DETAIL_EQUAL_WT_CHECK \
        ( \
        vseq1, \
        vseq2, \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE(vseq1,vtype1), \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE(vseq2,vtype2), \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_WT_CONVERT_D(d,vseq1,vseq2,vtype1,vtype2,type) \
    BOOST_VMD_DETAIL_EQUAL_WT_CHECK_D \
        ( \
        d, \
        vseq1, \
        vseq2, \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_D(d,vseq1,vtype1), \
        BOOST_VMD_DETAIL_ADJUST_TUPLE_TYPE_D(d,vseq2,vtype2), \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_WT(vseq1,vseq2,vtype1,vtype2,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH(vseq1,vseq2,vtype1,vtype2), \
            BOOST_VMD_IDENTITY(0), \
            BOOST_VMD_DETAIL_EQUAL_WT_CONVERT \
            ) \
        (vseq1,vseq2,vtype1,vtype2,type) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_WT_D(d,vseq1,vseq2,vtype1,vtype2,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH_D(d,vseq1,vseq2,vtype1,vtype2), \
            BOOST_VMD_IDENTITY(0), \
            BOOST_VMD_DETAIL_EQUAL_WT_CONVERT_D \
            ) \
        (d,vseq1,vseq2,vtype1,vtype2,type) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_GTYPE(vseq1,vseq2,type) \
    BOOST_VMD_DETAIL_EQUAL_WT \
        ( \
        vseq1, \
        vseq2, \
        BOOST_VMD_GET_TYPE(vseq1,BOOST_VMD_RETURN_TYPE_TUPLE), \
        BOOST_VMD_GET_TYPE(vseq2,BOOST_VMD_RETURN_TYPE_TUPLE), \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_GTYPE_D(d,vseq1,vseq2,type) \
    BOOST_VMD_DETAIL_EQUAL_WT_D \
        ( \
        d, \
        vseq1, \
        vseq2, \
        BOOST_VMD_GET_TYPE_D(d,vseq1,BOOST_VMD_RETURN_TYPE_TUPLE), \
        BOOST_VMD_GET_TYPE_D(d,vseq2,BOOST_VMD_RETURN_TYPE_TUPLE), \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_EBP(vseq1,vseq2,be1,be2,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_CHK_MATCH(be1,be2), \
                BOOST_VMD_DETAIL_EQUAL_CHK_PARENS_MATCH(vseq1,vseq2) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_GTYPE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (vseq1,vseq2,type) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_EBP_D(d,vseq1,vseq2,be1,be2,type) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_CHK_MATCH(be1,be2), \
                BOOST_VMD_DETAIL_EQUAL_CHK_PARENS_MATCH(vseq1,vseq2) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_GTYPE_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,vseq1,vseq2,type) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_EMPTY(vseq1,vseq2,be1,be2,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITAND(be1,be2), \
        BOOST_VMD_DETAIL_EQUAL_BOTH_EMPTY, \
        BOOST_VMD_DETAIL_EQUAL_EBP \
        ) \
    (vseq1,vseq2,be1,be2,type) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_EMPTY_D(d,vseq1,vseq2,be1,be2,type) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITAND(be1,be2), \
        BOOST_VMD_DETAIL_EQUAL_BOTH_EMPTY, \
        BOOST_VMD_DETAIL_EQUAL_EBP_D \
        ) \
    (d,vseq1,vseq2,be1,be2,type) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_OV0(vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_EMPTY \
        ( \
        vseq1, \
        , \
        BOOST_VMD_IS_EMPTY(vseq1), \
        1, \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_OV1(vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_EMPTY \
        ( \
        vseq1, \
        vseq2, \
        BOOST_VMD_IS_EMPTY(vseq1), \
        BOOST_VMD_IS_EMPTY(vseq2), \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_OV2(vseq1,vseq2,type) \
    BOOST_VMD_DETAIL_EQUAL_EMPTY \
        ( \
        vseq1, \
        vseq2, \
        BOOST_VMD_IS_EMPTY(vseq1), \
        BOOST_VMD_IS_EMPTY(vseq2), \
        type \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_D_OV0(d,vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_EMPTY_D \
        ( \
        d, \
        vseq1, \
        , \
        BOOST_VMD_IS_EMPTY(vseq1), \
        1, \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_D_OV1(d,vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_EMPTY_D \
        ( \
        d, \
        vseq1, \
        vseq2, \
        BOOST_VMD_IS_EMPTY(vseq1), \
        BOOST_VMD_IS_EMPTY(vseq2), \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_D_OV2(d,vseq1,vseq2,type) \
    BOOST_VMD_DETAIL_EQUAL_EMPTY_D \
        ( \
        d, \
        vseq1, \
        vseq2, \
        BOOST_VMD_IS_EMPTY(vseq1), \
        BOOST_VMD_IS_EMPTY(vseq2), \
        type \
        ) \
/**/

#if BOOST_VMD_MSVC

#define BOOST_VMD_DETAIL_EQUAL(vseq1,...) \
    BOOST_PP_CAT(BOOST_PP_OVERLOAD(BOOST_VMD_DETAIL_EQUAL_OV,__VA_ARGS__)(vseq1,__VA_ARGS__),BOOST_PP_EMPTY()) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_D(d,vseq1,...) \
    BOOST_PP_CAT(BOOST_PP_OVERLOAD(BOOST_VMD_DETAIL_EQUAL_D_OV,__VA_ARGS__)(d,vseq1,__VA_ARGS__),BOOST_PP_EMPTY()) \
/**/

#else

#define BOOST_VMD_DETAIL_EQUAL(vseq1,...) \
    BOOST_PP_OVERLOAD(BOOST_VMD_DETAIL_EQUAL_OV,__VA_ARGS__)(vseq1,__VA_ARGS__) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_D(d,vseq1,...) \
    BOOST_PP_OVERLOAD(BOOST_VMD_DETAIL_EQUAL_D_OV,__VA_ARGS__)(d,vseq1,__VA_ARGS__) \
/**/

#endif

#endif /* BOOST_VMD_DETAIL_EQUAL_HPP */
