
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_DATA_EQUAL_HPP)
#define BOOST_VMD_DETAIL_DATA_EQUAL_HPP

#include <boost/vmd/detail/recurse/data_equal/data_equal_1.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_2.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_3.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_4.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_5.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_6.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_7.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_8.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_9.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_10.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_11.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_12.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_13.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_14.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_15.hpp>
#include <boost/vmd/detail/recurse/data_equal/data_equal_16.hpp>

#include <boost/vmd/detail/recurse/data_equal/data_equal_headers.hpp>

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP_PARENS(d,em1,em2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_DATA_EQUAL_IS_BOTH_COMPOSITE(em1,em2), \
            BOOST_VMD_IDENTITY(2), \
            BOOST_VMD_DETAIL_EQUAL_SIMPLE_D \
            ) \
        (d,em1,em2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP_PARENS_D(d,em1,em2) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_DATA_EQUAL_IS_BOTH_COMPOSITE(em1,em2), \
            BOOST_VMD_IDENTITY(2), \
            BOOST_VMD_DETAIL_EQUAL_SIMPLE_D \
            ) \
        (d,em1,em2) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP(d,state,em1,em2) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PROCESSING(d,state), \
        BOOST_VMD_DETAIL_EQUAL_SIMPLE_D, \
        BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP_PARENS \
        ) \
    (d,em1,em2) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP_D(d,state,em1,em2) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_COMP_PROCESSING(d,state), \
        BOOST_VMD_DETAIL_EQUAL_SIMPLE_D, \
        BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP_PARENS_D \
        ) \
    (d,em1,em2) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP \
        ( \
        d, \
        state, \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_FIRST_ELEMENT(d,state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_SECOND_ELEMENT(d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_D(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_CMP_D \
        ( \
        d, \
        state, \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_FIRST_ELEMENT(d,state), \
        BOOST_VMD_DETAIL_DATA_EQUAL_STATE_GET_SECOND_ELEMENT(d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_OP_RESULT \
        ( \
        d, \
        state, \
        BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ(d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_OP_D(d,state) \
    BOOST_VMD_DETAIL_DATA_EQUAL_OP_RESULT \
        ( \
        d, \
        state, \
        BOOST_VMD_DETAIL_DATA_EQUAL_OP_TEQ_D(d,state) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_LOOP(dataf,datas,sz,vtype) \
    BOOST_PP_TUPLE_ELEM \
        ( \
        0, \
        BOOST_PP_WHILE \
            ( \
            BOOST_VMD_DETAIL_DATA_EQUAL_PRED, \
            BOOST_VMD_DETAIL_DATA_EQUAL_OP, \
                ( \
                1, \
                dataf, \
                datas, \
                sz, \
                vtype, \
                0, \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_LOOP_D(d,dataf,datas,sz,vtype) \
    BOOST_PP_TUPLE_ELEM \
        ( \
        0, \
        BOOST_PP_WHILE_ ## d \
            ( \
            BOOST_VMD_DETAIL_DATA_EQUAL_PRED, \
            BOOST_VMD_DETAIL_DATA_EQUAL_OP_D, \
                ( \
                1, \
                dataf, \
                datas, \
                sz, \
                vtype, \
                0, \
                ) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_SZ(dataf,datas,szf,szs,vtype) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL(szf,szs), \
            BOOST_VMD_DETAIL_DATA_EQUAL_LOOP, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (dataf,datas,szf,vtype) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_SZ_D(d,dataf,datas,szf,szs,vtype) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL_D(d,szf,szs), \
            BOOST_VMD_DETAIL_DATA_EQUAL_LOOP_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,dataf,datas,szf,vtype) \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL(dataf,datas,vtype) \
    BOOST_VMD_DETAIL_DATA_EQUAL_SZ \
        ( \
        dataf, \
        datas, \
        BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE(dataf,vtype), \
        BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE(datas,vtype), \
        vtype \
        ) \
/**/

#define BOOST_VMD_DETAIL_DATA_EQUAL_D(d,dataf,datas,vtype) \
    BOOST_VMD_DETAIL_DATA_EQUAL_SZ_D \
        ( \
        d, \
        dataf, \
        datas, \
        BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_D(d,dataf,vtype), \
        BOOST_VMD_DETAIL_DATA_EQUAL_GET_SIZE_D(d,datas,vtype), \
        vtype \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_DATA_EQUAL_HPP */
