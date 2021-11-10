
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_SEQUENCE_TYPE_HPP)
#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_HPP

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/equal_type.hpp>
#include <boost/vmd/detail/is_array_common.hpp>
#include <boost/vmd/detail/is_list.hpp>
#include <boost/vmd/detail/modifiers.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/sequence_elem.hpp>

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_ARRAY(dtuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX(BOOST_PP_TUPLE_ELEM(1,dtuple)), \
        BOOST_VMD_TYPE_ARRAY, \
        BOOST_VMD_TYPE_TUPLE \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_ARRAY_D(d,dtuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_ARRAY_SYNTAX_D(d,BOOST_PP_TUPLE_ELEM(1,dtuple)), \
        BOOST_VMD_TYPE_ARRAY, \
        BOOST_VMD_TYPE_TUPLE \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_LIST(dtuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_LIST(BOOST_PP_TUPLE_ELEM(1,dtuple)), \
        BOOST_VMD_TYPE_LIST, \
        BOOST_VMD_TYPE_TUPLE \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_LIST_D(d,dtuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IS_LIST_D(d,BOOST_PP_TUPLE_ELEM(1,dtuple)), \
        BOOST_VMD_TYPE_LIST, \
        BOOST_VMD_TYPE_TUPLE \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_BOTH(dtuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE \
                ( \
                BOOST_VMD_TYPE_TUPLE, \
                BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_LIST(dtuple) \
                ), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_ARRAY, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_LIST) \
            ) \
        (dtuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_BOTH_D(d,dtuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D \
                ( \
                d, \
                BOOST_VMD_TYPE_TUPLE, \
                BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_LIST_D(d,dtuple) \
                ), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_ARRAY_D, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_LIST) \
            ) \
        (d,dtuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MODS(dtuple,rtype) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL(rtype,BOOST_VMD_DETAIL_MODS_RETURN_ARRAY), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_ARRAY, \
            BOOST_PP_IIF \
                ( \
                BOOST_PP_EQUAL(rtype,BOOST_VMD_DETAIL_MODS_RETURN_LIST), \
                BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_LIST, \
                BOOST_PP_IIF \
                    ( \
                    BOOST_PP_EQUAL(rtype,BOOST_VMD_DETAIL_MODS_RETURN_TUPLE), \
                    BOOST_VMD_IDENTITY(BOOST_PP_TUPLE_ELEM(0,dtuple)), \
                    BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_BOTH \
                    ) \
                ) \
            ) \
        ) \
    (dtuple) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MODS_D(d,dtuple,rtype) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_EQUAL_D(d,rtype,BOOST_VMD_DETAIL_MODS_RETURN_ARRAY), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_ARRAY_D, \
            BOOST_PP_IIF \
                ( \
                BOOST_PP_EQUAL_D(d,rtype,BOOST_VMD_DETAIL_MODS_RETURN_LIST), \
                BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_LIST_D, \
                BOOST_PP_IIF \
                    ( \
                    BOOST_PP_EQUAL_D(d,rtype,BOOST_VMD_DETAIL_MODS_RETURN_TUPLE), \
                    BOOST_VMD_IDENTITY(BOOST_PP_TUPLE_ELEM(0,dtuple)), \
                    BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_BOTH_D \
                    ) \
                ) \
            ) \
        ) \
    (d,dtuple) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MORE(dtuple,...) \
    BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MODS \
        ( \
        dtuple, \
        BOOST_VMD_DETAIL_MODS_RESULT_RETURN_TYPE \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS(BOOST_VMD_ALLOW_ALL,__VA_ARGS__) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MORE_D(d,dtuple,...) \
    BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MODS_D \
        ( \
        d, \
        dtuple, \
        BOOST_VMD_DETAIL_MODS_RESULT_RETURN_TYPE \
            ( \
            BOOST_VMD_DETAIL_NEW_MODS_D(d,BOOST_VMD_ALLOW_ALL,__VA_ARGS__) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_UNARY(dtuple,...) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE(BOOST_VMD_TYPE_TUPLE,BOOST_PP_TUPLE_ELEM(0,dtuple)), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MORE, \
            BOOST_VMD_IDENTITY(BOOST_PP_TUPLE_ELEM(0,dtuple)) \
            ) \
        (dtuple,__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_UNARY_D(d,dtuple,...) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_EQUAL_TYPE_D(d,BOOST_VMD_TYPE_TUPLE,BOOST_PP_TUPLE_ELEM(0,dtuple)), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_MORE_D, \
            BOOST_VMD_IDENTITY(BOOST_PP_TUPLE_ELEM(0,dtuple)) \
            ) \
        (d,dtuple,__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_SEQUENCE(tuple,...) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(BOOST_PP_TUPLE_ELEM(1,tuple)), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_UNARY, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_SEQUENCE) \
            ) \
        (BOOST_PP_TUPLE_ELEM(0,tuple),__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_SEQUENCE_D(d,tuple,...) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(BOOST_PP_TUPLE_ELEM(1,tuple)), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_UNARY_D, \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_SEQUENCE) \
            ) \
        (d,BOOST_PP_TUPLE_ELEM(0,tuple),__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE(tuple,...) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(BOOST_PP_TUPLE_ELEM(0,tuple)), \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_EMPTY), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_SEQUENCE \
            ) \
        (tuple,__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_D(d,tuple,...) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(BOOST_PP_TUPLE_ELEM(0,tuple)), \
            BOOST_VMD_IDENTITY(BOOST_VMD_TYPE_EMPTY), \
            BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_SEQUENCE_D \
            ) \
        (d,tuple,__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE(...) \
    BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE \
        ( \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM \
            ( \
            BOOST_VMD_ALLOW_ALL, \
            0, \
            BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__), \
            BOOST_VMD_RETURN_AFTER, \
            BOOST_VMD_RETURN_TYPE_TUPLE \
            ), \
        __VA_ARGS__ \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TYPE_D(d,...) \
    BOOST_VMD_DETAIL_SEQUENCE_TYPE_TUPLE_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_D \
            ( \
            d, \
            BOOST_VMD_ALLOW_ALL, \
            0, \
            BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__), \
            BOOST_VMD_RETURN_AFTER, \
            BOOST_VMD_RETURN_TYPE_TUPLE \
            ), \
        __VA_ARGS__ \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_SEQUENCE_TYPE_HPP */
