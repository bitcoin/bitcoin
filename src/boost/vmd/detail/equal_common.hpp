
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_EQUAL_COMMON_HPP)
#define BOOST_VMD_DETAIL_EQUAL_COMMON_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/logical/bitxor.hpp>
#include <boost/preprocessor/logical/compl.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/get_type.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/match_single_identifier.hpp>
#include <boost/vmd/detail/equal_type.hpp>

#define BOOST_VMD_DETAIL_EQUAL_CONCAT_1 (1)

#define BOOST_VMD_DETAIL_EQUAL_IS_1(res) \
    BOOST_PP_IS_BEGIN_PARENS \
        ( \
        BOOST_PP_CAT \
            ( \
            BOOST_VMD_DETAIL_EQUAL_CONCAT_, \
            res \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_MATCH_SINGLE_IDENTIFIER(d,vseq1,vseq2) \
    BOOST_VMD_DETAIL_MATCH_SINGLE_IDENTIFIER(vseq1,vseq2) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_CNI_SMP(vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_NUMBER), \
        BOOST_PP_EQUAL, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE(vtype,BOOST_VMD_TYPE_TYPE), \
            BOOST_VMD_DETAIL_EQUAL_TYPE, \
            BOOST_VMD_DETAIL_MATCH_SINGLE_IDENTIFIER \
            ) \
        ) \
    (vseq1,vseq2) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_CNI_SMP_D(d,vseq1,vseq2,vtype) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_NUMBER), \
        BOOST_PP_EQUAL_D, \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype,BOOST_VMD_TYPE_TYPE), \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D, \
            BOOST_VMD_DETAIL_EQUAL_MATCH_SINGLE_IDENTIFIER \
            ) \
        ) \
    (d,vseq1,vseq2) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH_SIZE(vseq1,vseq2) \
    BOOST_PP_NOT_EQUAL \
        ( \
        BOOST_PP_TUPLE_SIZE(vseq1), \
        BOOST_PP_TUPLE_SIZE(vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH_SIZE_D(d,vseq1,vseq2) \
    BOOST_PP_NOT_EQUAL_D \
        ( \
        d, \
        BOOST_PP_TUPLE_SIZE(vseq1), \
        BOOST_PP_TUPLE_SIZE(vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH(vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE(vtype1,BOOST_VMD_TYPE_TUPLE), \
                BOOST_VMD_DETAIL_EQUAL_TYPE(vtype2,BOOST_VMD_TYPE_TUPLE) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH_SIZE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (vseq1,vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH_D(d,vseq1,vseq2,vtype1,vtype2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_BITAND \
                ( \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype1,BOOST_VMD_TYPE_TUPLE), \
                BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,vtype2,BOOST_VMD_TYPE_TUPLE) \
                ), \
            BOOST_VMD_DETAIL_EQUAL_IS_TUPLE_MISMATCH_SIZE_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,vseq1,vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_BOTH_EMPTY(...) 1

#define BOOST_VMD_DETAIL_EQUAL_CHK_MATCH(bp1,bp2) \
    BOOST_PP_COMPL \
        ( \
        BOOST_PP_BITXOR \
            ( \
            bp1, \
            bp2 \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_CHK_PARENS_MATCH(vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_CHK_MATCH \
        ( \
        BOOST_PP_IS_BEGIN_PARENS(vseq1), \
        BOOST_PP_IS_BEGIN_PARENS(vseq2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_EQUAL_CHK_EMPTY_MATCH(vseq1,vseq2) \
    BOOST_VMD_DETAIL_EQUAL_CHK_MATCH \
        ( \
        BOOST_VMD_IS_EMPTY(vseq1), \
        BOOST_VMD_IS_EMPTY(vseq2) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_EQUAL_COMMON_HPP */
