
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IS_ARRAY_COMMON_HPP)
#define BOOST_VMD_DETAIL_IS_ARRAY_COMMON_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_number.hpp>
#include <boost/vmd/is_tuple.hpp>
#include <boost/vmd/detail/is_empty_tuple.hpp>

#define BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL(2,BOOST_PP_TUPLE_SIZE(tuple)), \
            BOOST_VMD_DETAIL_IS_ARRAY_CHECK_ARRAY_FORM, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX_D(d,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL_D(d,2,BOOST_PP_TUPLE_SIZE(tuple)), \
            BOOST_VMD_DETAIL_IS_ARRAY_CHECK_ARRAY_FORM_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,tuple) \
        ) \
/**/

/*

  Check if the first element of the tuple is equal to the number of tuple elements of the second element

*/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH_SIZE(x) \
    BOOST_PP_EQUAL(BOOST_PP_TUPLE_ELEM(0,x),BOOST_PP_TUPLE_SIZE(BOOST_PP_TUPLE_ELEM(1,x))) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH_SIZE_D(d,x) \
    BOOST_PP_EQUAL_D(d,BOOST_PP_TUPLE_ELEM(0,x),BOOST_PP_TUPLE_SIZE(BOOST_PP_TUPLE_ELEM(1,x))) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_EMPTY(tuple) \
    BOOST_VMD_DETAIL_IS_EMPTY_TUPLE_SIZE(BOOST_PP_TUPLE_ELEM(1,tuple)) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_EMPTY_D(d,tuple) \
    BOOST_VMD_DETAIL_IS_EMPTY_TUPLE_SIZE(BOOST_PP_TUPLE_ELEM(1,tuple)) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH(tuple) \
    BOOST_PP_IF \
        ( \
        BOOST_PP_TUPLE_ELEM(0,tuple), \
        BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH_SIZE, \
        BOOST_VMD_DETAIL_IS_ARRAY_CHECK_EMPTY \
        ) \
    (tuple) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH_D(d,tuple) \
    BOOST_PP_IF \
        ( \
        BOOST_PP_TUPLE_ELEM(0,tuple), \
        BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH_SIZE_D, \
        BOOST_VMD_DETAIL_IS_ARRAY_CHECK_EMPTY_D \
        ) \
    (d,tuple) \
/**/

/*

  Check if the first element is numeric

*/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
          ( \
          BOOST_VMD_DETAIL_IS_ARRAY_NUM(BOOST_PP_TUPLE_ELEM(0,tuple)), \
          BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH, \
          BOOST_VMD_IDENTITY(0) \
          ) \
        (tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_D(d,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
          ( \
          BOOST_VMD_DETAIL_IS_ARRAY_NUM(BOOST_PP_TUPLE_ELEM(0,tuple)), \
          BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_MATCH_D, \
          BOOST_VMD_IDENTITY(0) \
          ) \
        (d,tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMBER(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_NUMBER(BOOST_PP_TUPLE_ELEM(0,tuple)), \
            BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMBER_D(d,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_NUMBER(BOOST_PP_TUPLE_ELEM(0,tuple)), \
            BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMERIC_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,tuple) \
        ) \
/**/

/*

  Check if the second element is a tuple

*/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_ARRAY_FORM(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
          ( \
          BOOST_VMD_IS_TUPLE(BOOST_PP_TUPLE_ELEM(1,tuple)), \
          BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMBER, \
          BOOST_VMD_IDENTITY(0) \
          ) \
        (tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_CHECK_ARRAY_FORM_D(d,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
          ( \
          BOOST_VMD_IS_TUPLE(BOOST_PP_TUPLE_ELEM(1,tuple)), \
          BOOST_VMD_DETAIL_IS_ARRAY_CHECK_NUMBER_D, \
          BOOST_VMD_IDENTITY(0) \
          ) \
        (d,tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_ARRAY_NUM(x) BOOST_VMD_IS_EMPTY(BOOST_PP_CAT(BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_, x) BOOST_PP_EMPTY())
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_0
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_1
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_2
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_3
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_4
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_5
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_6
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_7
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_8
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_9
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_10
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_11
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_12
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_13
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_14
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_15
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_16
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_17
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_18
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_19
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_20
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_21
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_22
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_23
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_24
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_25
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_26
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_27
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_28
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_29
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_30
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_31
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_32
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_33
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_34
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_35
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_36
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_37
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_38
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_39
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_40
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_41
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_42
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_43
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_44
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_45
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_46
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_47
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_48
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_49
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_50
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_51
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_52
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_53
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_54
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_55
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_56
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_57
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_58
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_59
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_60
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_61
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_62
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_63
#define BOOST_VMD_DETAIL_IS_ARRAY_NUM_HELPER_64

#endif /* BOOST_VMD_DETAIL_IS_ARRAY_COMMON_HPP */
