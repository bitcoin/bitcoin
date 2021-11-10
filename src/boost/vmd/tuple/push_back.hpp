
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_TUPLE_PUSH_BACK_HPP)
#define BOOST_VMD_TUPLE_PUSH_BACK_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_TUPLE_PUSH_BACK(tuple,elem)

    \brief appends an element to the end of a tuple. 

    tuple = tuple to to append an element to. <br/>
    elem  = element to append.

    If the tuple is an empty tuple the result is a tuple with the single element.
    Otherwise the result is a tuple after adding the element to the end.
*/

#define BOOST_VMD_TUPLE_PUSH_BACK(tuple,elem) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(tuple), \
            BOOST_VMD_IDENTITY((elem)), \
            BOOST_PP_TUPLE_PUSH_BACK \
            ) \
        (tuple,elem) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_TUPLE_PUSH_BACK_HPP */
