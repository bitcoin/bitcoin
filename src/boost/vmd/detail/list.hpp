
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_LIST_HPP)
#define BOOST_VMD_DETAIL_LIST_HPP

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/pop_back.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/empty_result.hpp>
#include <boost/vmd/detail/identifier.hpp>
#include <boost/vmd/detail/is_list.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/parens.hpp>

#define BOOST_VMD_DETAIL_LIST_CHECK_FOR_LIST(tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_LIST \
            ( \
            BOOST_PP_TUPLE_ELEM \
                ( \
                0, \
                tuple \
                ) \
            ), \
        tuple, \
        (,) \
        ) \
/**/

#define BOOST_VMD_DETAIL_LIST_CHECK_FOR_LIST_D(d,tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_LIST_D \
            ( \
            d, \
            BOOST_PP_TUPLE_ELEM \
                ( \
                0, \
                tuple \
                ) \
            ), \
        tuple, \
        (,) \
        ) \
/**/

#define BOOST_VMD_DETAIL_LIST_CHECK_RETURN(tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY \
            ( \
            BOOST_PP_TUPLE_ELEM \
                ( \
                0, \
                tuple \
                ) \
            ), \
        BOOST_VMD_DETAIL_EMPTY_RESULT, \
        BOOST_VMD_DETAIL_LIST_CHECK_FOR_LIST \
        ) \
    (tuple) \
/**/

#define BOOST_VMD_DETAIL_LIST_CHECK_RETURN_D(d,tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY \
            ( \
            BOOST_PP_TUPLE_ELEM \
                ( \
                0, \
                tuple \
                ) \
            ), \
        BOOST_VMD_DETAIL_EMPTY_RESULT, \
        BOOST_VMD_DETAIL_LIST_CHECK_FOR_LIST_D \
        ) \
    (d,tuple) \
/**/

#define BOOST_VMD_DETAIL_LIST_EMPTY_LIST(list) \
    BOOST_VMD_DETAIL_IDENTIFIER(list,BOOST_PP_NIL,BOOST_VMD_RETURN_AFTER,BOOST_VMD_RETURN_INDEX) \
/**/

#define BOOST_VMD_DETAIL_LIST_EMPTY_LIST_D(d,list) \
    BOOST_VMD_DETAIL_IDENTIFIER_D(d,list,BOOST_PP_NIL,BOOST_VMD_RETURN_AFTER,BOOST_VMD_RETURN_INDEX) \
/**/

#define BOOST_VMD_DETAIL_LIST_TUPLE(param) \
    BOOST_VMD_DETAIL_LIST_CHECK_RETURN \
        ( \
        BOOST_VMD_DETAIL_PARENS(param,BOOST_VMD_RETURN_AFTER) \
        ) \
/**/

#define BOOST_VMD_DETAIL_LIST_TUPLE_D(d,param) \
    BOOST_VMD_DETAIL_LIST_CHECK_RETURN_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_PARENS_D(d,param,BOOST_VMD_RETURN_AFTER) \
        ) \
/**/

#define BOOST_VMD_DETAIL_LIST_EMPTY_PROCESS(tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY \
            ( \
            BOOST_PP_TUPLE_ELEM(0,tuple) \
            ), \
        (,), \
        tuple \
        ) \
/**/

#define BOOST_VMD_DETAIL_LIST_EMPTY(list) \
    BOOST_VMD_DETAIL_LIST_EMPTY_PROCESS \
        ( \
        BOOST_VMD_DETAIL_LIST_EMPTY_LIST(list) \
        ) \
/**/

#define BOOST_VMD_DETAIL_LIST_EMPTY_D(d,list) \
    BOOST_VMD_DETAIL_LIST_EMPTY_PROCESS \
        ( \
        BOOST_VMD_DETAIL_LIST_EMPTY_LIST_D(d,list) \
        ) \
/**/

#define BOOST_VMD_DETAIL_LIST_PROCESS(list) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_IS_BEGIN_PARENS(list), \
        BOOST_VMD_DETAIL_LIST_TUPLE, \
        BOOST_VMD_DETAIL_LIST_EMPTY \
        ) \
    (list) \
/**/

#define BOOST_VMD_DETAIL_LIST_SPLIT(list) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(list), \
        BOOST_VMD_DETAIL_EMPTY_RESULT, \
        BOOST_VMD_DETAIL_LIST_PROCESS \
        ) \
    (list) \
/**/

#define BOOST_VMD_DETAIL_LIST_BEGIN(list) \
    BOOST_PP_TUPLE_ELEM(0,BOOST_VMD_DETAIL_LIST_SPLIT(list)) \
/**/

#define BOOST_VMD_DETAIL_LIST_PROCESS_D(d,list) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_IS_BEGIN_PARENS(list), \
        BOOST_VMD_DETAIL_LIST_TUPLE_D, \
        BOOST_VMD_DETAIL_LIST_EMPTY_D \
        ) \
    (d,list) \
/**/

#define BOOST_VMD_DETAIL_LIST_SPLIT_D(d,list) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(list), \
        BOOST_VMD_DETAIL_EMPTY_RESULT, \
        BOOST_VMD_DETAIL_LIST_PROCESS_D \
        ) \
    (d,list) \
/**/

#define BOOST_VMD_DETAIL_LIST_BEGIN_D(d,list) \
    BOOST_PP_TUPLE_ELEM(0,BOOST_VMD_DETAIL_LIST_SPLIT_D(d,list)) \
/**/

#define BOOST_VMD_DETAIL_LIST_D(d,...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS_D(d,BOOST_VMD_ALLOW_AFTER,__VA_ARGS__) \
            ), \
        BOOST_VMD_DETAIL_LIST_SPLIT_D, \
        BOOST_VMD_DETAIL_LIST_BEGIN_D \
        ) \
    (d,BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#define BOOST_VMD_DETAIL_LIST(...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS(BOOST_VMD_ALLOW_AFTER,__VA_ARGS__) \
            ), \
        BOOST_VMD_DETAIL_LIST_SPLIT, \
        BOOST_VMD_DETAIL_LIST_BEGIN \
        ) \
    (BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#endif /* BOOST_VMD_DETAIL_LIST_HPP */
