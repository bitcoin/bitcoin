
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_SEQUENCE_TO_LIST_HPP)
#define BOOST_VMD_DETAIL_SEQUENCE_TO_LIST_HPP

#include <boost/preprocessor/control/iif.hpp>
#include <boost/vmd/detail/modifiers.hpp>
#include <boost/vmd/detail/mods.hpp>
#include <boost/vmd/detail/not_empty.hpp>
#include <boost/vmd/detail/sequence_elem.hpp>

#define BOOST_VMD_DETAIL_SEQUENCE_TO_LIST(...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_NOT_EMPTY(BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_VSEQ(__VA_ARGS__)), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE \
            ( \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_VSEQ(__VA_ARGS__), \
            , \
            BOOST_VMD_TYPE_LIST, \
            BOOST_VMD_DETAIL_NEW_MODS(BOOST_VMD_ALLOW_RETURN,__VA_ARGS__) \
            ), \
        BOOST_PP_NIL \
        ) \
/**/

#define BOOST_VMD_DETAIL_SEQUENCE_TO_LIST_D(d,...) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_DETAIL_NOT_EMPTY(BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_VSEQ(__VA_ARGS__)), \
        BOOST_VMD_DETAIL_SEQUENCE_ELEM_PROCESS_TUPLE_D \
            ( \
            d, \
            BOOST_VMD_DETAIL_SEQUENCE_ELEM_GET_VSEQ(__VA_ARGS__), \
            , \
            BOOST_VMD_TYPE_LIST, \
            BOOST_VMD_DETAIL_NEW_MODS_D(d,BOOST_VMD_ALLOW_RETURN,__VA_ARGS__) \
            ), \
        BOOST_PP_NIL \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_SEQUENCE_TO_LIST_HPP */
