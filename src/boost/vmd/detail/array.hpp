
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_ARRAY_HPP)
#define BOOST_VMD_DETAIL_ARRAY_HPP

#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/empty_result.hpp>
#include <boost/vmd/detail/is_array_common.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/tuple.hpp>

#define BOOST_VMD_DETAIL_ARRAY_CHECK_FOR_ARRAY(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX \
                ( \
                BOOST_PP_TUPLE_ELEM \
                    ( \
                    0, \
                    tuple \
                    ) \
                ), \
            BOOST_VMD_IDENTITY(tuple), \
            BOOST_VMD_DETAIL_EMPTY_RESULT \
            ) \
        (tuple)    \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_CHECK_FOR_ARRAY_D(d,tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX_D \
                ( \
                d, \
                BOOST_PP_TUPLE_ELEM \
                    ( \
                    0, \
                    tuple \
                    ) \
                ), \
            BOOST_VMD_IDENTITY(tuple), \
            BOOST_VMD_DETAIL_EMPTY_RESULT \
            ) \
        (tuple)    \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_PROCESS(tuple) \
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
        BOOST_VMD_DETAIL_ARRAY_CHECK_FOR_ARRAY \
        ) \
    (tuple) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_PROCESS_D(d,tuple) \
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
        BOOST_VMD_DETAIL_ARRAY_CHECK_FOR_ARRAY_D \
        ) \
    (d,tuple) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_SPLIT(param) \
    BOOST_VMD_DETAIL_ARRAY_PROCESS \
        ( \
        BOOST_VMD_DETAIL_TUPLE(param,BOOST_VMD_RETURN_AFTER) \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_SPLIT_D(d,param) \
    BOOST_VMD_DETAIL_ARRAY_PROCESS_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_TUPLE_D(d,param,BOOST_VMD_RETURN_AFTER) \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_BEGIN_CHECK_FOR_ARRAY(tuple) \
    BOOST_PP_EXPR_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX(tuple), \
        tuple \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_BEGIN_CHECK_FOR_ARRAY_D(d,tuple) \
    BOOST_PP_EXPR_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX_D(d,tuple), \
        tuple \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_BEGIN_TUPLE(tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(tuple), \
        BOOST_VMD_EMPTY, \
        BOOST_VMD_DETAIL_ARRAY_BEGIN_CHECK_FOR_ARRAY \
        ) \
    (tuple) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_BEGIN_TUPLE_D(d,tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(tuple), \
        BOOST_VMD_EMPTY, \
        BOOST_VMD_DETAIL_ARRAY_BEGIN_CHECK_FOR_ARRAY_D \
        ) \
    (d,tuple) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_BEGIN(param) \
    BOOST_VMD_DETAIL_ARRAY_BEGIN_TUPLE \
        ( \
        BOOST_VMD_DETAIL_TUPLE(param) \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_BEGIN_D(d,param) \
    BOOST_VMD_DETAIL_ARRAY_BEGIN_TUPLE_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_TUPLE_D(d,param) \
        ) \
/**/

#define BOOST_VMD_DETAIL_ARRAY(...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS(BOOST_VMD_ALLOW_AFTER,__VA_ARGS__) \
            ), \
        BOOST_VMD_DETAIL_ARRAY_SPLIT, \
        BOOST_VMD_DETAIL_ARRAY_BEGIN \
        ) \
    (BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#define BOOST_VMD_DETAIL_ARRAY_D(d,...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS(BOOST_VMD_ALLOW_AFTER,__VA_ARGS__) \
            ), \
        BOOST_VMD_DETAIL_ARRAY_SPLIT_D, \
        BOOST_VMD_DETAIL_ARRAY_BEGIN_D \
        ) \
    (d,BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__)) \
/**/

#endif /* BOOST_VMD_DETAIL_ARRAY_HPP */
