
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IDENTIFIER_HPP)
#define BOOST_VMD_DETAIL_IDENTIFIER_HPP

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/logical/bitor.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/detail/identifier_concat.hpp>
#include <boost/vmd/detail/is_entire.hpp>
#include <boost/vmd/detail/match_identifier.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/not_empty.hpp>
#include <boost/vmd/detail/parens.hpp>

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_SUCCESS(id,rest,keymatch,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_SUCCESS_MODS(id,rest,BOOST_PP_DEC(keymatch),mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_FAILURE(id,rest,keymatch,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_MODS(mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE_RESULT(id,rest,keymatch,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL(keymatch,0), \
        BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_FAILURE, \
        BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_SUCCESS \
        ) \
    (id,rest,keymatch,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE_RESULT_D(d,id,rest,keymatch,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL_D(d,keymatch,0), \
        BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_FAILURE, \
        BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_SUCCESS \
        ) \
    (id,rest,keymatch,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE(id,rest,keytuple,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE_RESULT \
        ( \
        id, \
        rest, \
        BOOST_VMD_DETAIL_MATCH_IDENTIFIER(id,keytuple), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE_D(d,id,rest,keytuple,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE_RESULT_D \
        ( \
        d, \
        id, \
        rest, \
        BOOST_VMD_DETAIL_MATCH_IDENTIFIER_D(d,id,keytuple), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_MAKE_SPLIT(tuple) \
    ( \
    BOOST_PP_TUPLE_ELEM \
        ( \
        0, \
        BOOST_PP_TUPLE_ELEM(0,tuple) \
        ), \
    BOOST_PP_TUPLE_ELEM(1,tuple) \
    ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SPLIT_SEQUENCE_CONCAT_DATA(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY \
                ( \
                BOOST_PP_TUPLE_ELEM(0,tuple) \
                ), \
            BOOST_VMD_IDENTITY(tuple), \
            BOOST_VMD_DETAIL_IDENTIFIER_MAKE_SPLIT \
            ) \
        (tuple) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SPLIT_SEQUENCE_CONCAT(vcseq) \
    BOOST_VMD_DETAIL_IDENTIFIER_SPLIT_SEQUENCE_CONCAT_DATA \
        ( \
        BOOST_VMD_DETAIL_PARENS(vcseq,BOOST_VMD_RETURN_AFTER) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_GETID_TID(tid) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(tid), \
            BOOST_VMD_IDENTITY(tid), \
            BOOST_PP_TUPLE_ELEM \
            ) \
        (0,tid) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_GETID_SEQUENCE(vseq) \
    BOOST_VMD_DETAIL_IDENTIFIER_GETID_TID \
        ( \
        BOOST_VMD_DETAIL_PARENS(BOOST_VMD_DETAIL_IDENTIFIER_CONCATENATE(vseq)) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SPLIT_SEQUENCE(vseq) \
     BOOST_VMD_DETAIL_IDENTIFIER_SPLIT_SEQUENCE_CONCAT \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_CONCATENATE(vseq) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS(id,rest,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE \
        ( \
        id, \
        rest, \
        BOOST_VMD_DETAIL_MODS_RESULT_OTHER(mods), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_D(d,id,rest,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_TUPLE_D \
        ( \
        d, \
        id, \
        rest, \
        BOOST_VMD_DETAIL_MODS_RESULT_OTHER(mods), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_JUST(id,rest,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_SUCCESS_MODS(id,rest,0,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_JUST_D(d,id,rest,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_SUCCESS_MODS(id,rest,0,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_CHECK_KEYS(id,rest,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY \
            ( \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(mods) \
            ), \
        BOOST_VMD_DETAIL_IDENTIFIER_JUST, \
        BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS \
        ) \
    (id,rest,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_CHECK_KEYS_D(d,id,rest,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY \
            ( \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(mods) \
            ), \
        BOOST_VMD_DETAIL_IDENTIFIER_JUST_D, \
        BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_KEYS_D \
        ) \
    (d,id,rest,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_MKEYS(mods) \
    BOOST_PP_BITAND \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_INDEX(mods), \
        BOOST_VMD_DETAIL_NOT_EMPTY \
            ( \
            BOOST_VMD_DETAIL_MODS_RESULT_OTHER(mods) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SUCCESS_MODS(id,rest,keymatch,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_MKEYS(mods), \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(mods), \
            (id,rest,keymatch), \
            (id,keymatch) \
            ), \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(mods), \
            (id,rest), \
            id \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_MODS(mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_MKEYS(mods), \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(mods), \
            (,,), \
            (,) \
            ), \
        BOOST_PP_EXPR_IIF \
            ( \
            BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(mods), \
            (,) \
            ) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_REST(id,rest,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_MODS(mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_REST_D(d,id,rest,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_MODS(mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_ID_REST(id,rest,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(id), \
        BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_REST, \
        BOOST_VMD_DETAIL_IDENTIFIER_CHECK_KEYS \
        ) \
    (id,rest,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_ID_REST_D(d,id,rest,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(id), \
        BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_REST_D, \
        BOOST_VMD_DETAIL_IDENTIFIER_CHECK_KEYS_D \
        ) \
    (d,id,rest,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_DATA(tuple,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_ID_REST \
        ( \
        BOOST_PP_TUPLE_ELEM(0,tuple), \
        BOOST_PP_TUPLE_ELEM(1,tuple), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_ID(id,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(id), \
        BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_REST, \
        BOOST_VMD_DETAIL_IDENTIFIER_CHECK_KEYS \
        ) \
    (id,,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_ID_D(d,id,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(id), \
        BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_REST_D, \
        BOOST_VMD_DETAIL_IDENTIFIER_CHECK_KEYS_D \
        ) \
    (d,id,,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_DATA_D(d,tuple,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_ID_REST_D \
        ( \
        d, \
        BOOST_PP_TUPLE_ELEM(0,tuple), \
        BOOST_PP_TUPLE_ELEM(1,tuple), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_AFTER(vseq,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_DATA \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_SPLIT_SEQUENCE(vseq), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_AFTER_D(d,vseq,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_DATA_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_IDENTIFIER_SPLIT_SEQUENCE(vseq), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_ID(vseq,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_ID \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_GETID_SEQUENCE(vseq), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_ID_D(d,vseq,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_PROCESS_ID_D \
        ( \
        d, \
        BOOST_VMD_DETAIL_IDENTIFIER_GETID_SEQUENCE(vseq), \
        mods \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE(vseq,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(mods), \
        BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_AFTER, \
        BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_ID \
        ) \
    (vseq,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_D(d,vseq,mods) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_MODS_IS_RESULT_AFTER(mods), \
        BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_AFTER_D, \
        BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_ID_D \
        ) \
    (d,vseq,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_EX_FAILURE(vseq,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_MODS(mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_EX_FAILURE_D(d,vseq,mods) \
    BOOST_VMD_DETAIL_IDENTIFIER_FAILURE_MODS(mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_EX(vseq,mods) \
    BOOST_PP_IIF \
      ( \
      BOOST_PP_BITOR \
          ( \
          BOOST_VMD_IS_EMPTY(vseq), \
          BOOST_PP_IS_BEGIN_PARENS(vseq) \
          ), \
      BOOST_VMD_DETAIL_IDENTIFIER_EX_FAILURE, \
      BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE \
      ) \
    (vseq,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_EX_D(d,vseq,mods) \
    BOOST_PP_IIF \
      ( \
      BOOST_PP_BITOR \
          ( \
          BOOST_VMD_IS_EMPTY(vseq), \
          BOOST_PP_IS_BEGIN_PARENS(vseq) \
          ), \
      BOOST_VMD_DETAIL_IDENTIFIER_EX_FAILURE_D, \
      BOOST_VMD_DETAIL_IDENTIFIER_SEQUENCE_D \
      ) \
    (d,vseq,mods) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER(...) \
    BOOST_VMD_DETAIL_IDENTIFIER_EX \
        ( \
        BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__), \
        BOOST_VMD_DETAIL_NEW_MODS(BOOST_VMD_ALLOW_INDEX,__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IDENTIFIER_D(d,...) \
    BOOST_VMD_DETAIL_IDENTIFIER_EX_D \
        ( \
        d, \
        BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__), \
        BOOST_VMD_DETAIL_NEW_MODS_D(d,BOOST_VMD_ALLOW_INDEX,__VA_ARGS__) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_IDENTIFIER_MULTIPLE(...) \
    BOOST_VMD_DETAIL_IS_ENTIRE \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER(__VA_ARGS__,BOOST_VMD_RETURN_AFTER) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_IDENTIFIER_MULTIPLE_D(d,...) \
    BOOST_VMD_DETAIL_IS_ENTIRE \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_D(d,__VA_ARGS__,BOOST_VMD_RETURN_AFTER) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IDENTIFIER_HPP */
