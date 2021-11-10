
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_TUPLE_TO_LIST_HPP)
#define BOOST_VMD_TUPLE_TO_LIST_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_TUPLE_TO_LIST(tuple)

    \brief converts a tuple to a list.

    tuple = tuple to be converted.
    
    If the tuple is an empty tuple it is converted to an empty list (BOOST_PP_NIL).
    Otherwise the tuple is converted to a list with the same number of elements as the tuple.
*/

#define BOOST_VMD_TUPLE_TO_LIST(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(tuple), \
            BOOST_VMD_IDENTITY(BOOST_PP_NIL), \
            BOOST_PP_TUPLE_TO_LIST \
            ) \
        (tuple) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_TUPLE_TO_LIST_HPP */
