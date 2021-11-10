
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_TUPLE_REMOVE_HPP)
#define BOOST_VMD_TUPLE_REMOVE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/logical/bitand.hpp>
#include <boost/preprocessor/tuple/remove.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_TUPLE_REMOVE(tuple,index)

    \brief removes an element from a tuple.

    tuple = tuple from which an element is to be removed. <br/>
    index = The zero-based position in tuple of the element to be removed.

    If index is greater or equal to the tuple size the result is undefined.
    If the tuple is a single element and the index is 0 the result is an empty tuple.
    Otherwise the result is a tuple after removing the index element.
*/

#define BOOST_VMD_TUPLE_REMOVE(tuple,index) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITAND \
            ( \
            BOOST_PP_EQUAL(index,0), \
            BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(tuple),1) \
            ), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_TUPLE_REMOVE \
        ) \
    (tuple,index) \
/**/

/** \def BOOST_VMD_TUPLE_REMOVE_D(d,tuple,index)

    \brief removes an element from a tuple. It reenters BOOST_PP_WHILE with maximum efficiency.

    d     = The next available BOOST_PP_WHILE iteration. <br/>
    tuple = tuple from which an element is to be removed. <br/>
    index = The zero-based position in tuple of the element to be removed.

    If index is greater or equal to the tuple size the result is undefined.
    If the tuple is a single element and the index is 0 the result is an empty tuple.
    Otherwise the result is a tuple after removing the index element.
*/

#define BOOST_VMD_TUPLE_REMOVE_D(d,tuple,index) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_BITAND \
            ( \
            BOOST_PP_EQUAL_D(d,index,0), \
            BOOST_PP_EQUAL_D(d,BOOST_PP_TUPLE_SIZE(tuple),1) \
            ), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_TUPLE_REMOVE_D \
        ) \
    (d,tuple,index) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_TUPLE_REMOVE_HPP */
