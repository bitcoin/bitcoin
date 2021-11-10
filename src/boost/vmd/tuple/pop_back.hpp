
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_TUPLE_POP_BACK_HPP)
#define BOOST_VMD_TUPLE_POP_BACK_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/pop_back.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_TUPLE_POP_BACK(tuple)

    \brief pops an element from the end of a tuple. 

    tuple = tuple to pop an element from.

    If the tuple is an empty tuple the result is undefined.
    If the tuple is a single element the result is an empty tuple.
    Otherwise the result is a tuple after removing the last element.
*/

#define BOOST_VMD_TUPLE_POP_BACK(tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(tuple),1), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_TUPLE_POP_BACK \
        ) \
    (tuple) \
/**/

/** \def BOOST_VMD_TUPLE_POP_BACK_Z(z,tuple)

    \brief pops an element from the end of a tuple. It reenters BOOST_PP_REPEAT with maximum efficiency.

    z     = the next available BOOST_PP_REPEAT dimension. <br/>
    tuple = tuple to pop an element from.

    If the tuple is an empty tuple the result is undefined.
    If the tuple is a single element the result is an empty tuple.
    Otherwise the result is a tuple after removing the last element.
*/

#define BOOST_VMD_TUPLE_POP_BACK_Z(z,tuple) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(tuple),1), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_TUPLE_POP_BACK_Z \
        ) \
    (z,tuple) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_TUPLE_POP_BACK_HPP */
