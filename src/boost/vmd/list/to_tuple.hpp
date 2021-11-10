
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_LIST_TO_TUPLE_HPP)
#define BOOST_VMD_LIST_TO_TUPLE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/list/to_tuple.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/is_empty_list.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_LIST_TO_TUPLE(list)

    \brief converts a list to a tuple.

    list = list to be converted.
    
    If the list is an empty list (BOOST_PP_NIL) it is converted to an empty tuple.
    Otherwise the list is converted to a tuple with the same number of elements as the list.
*/

#define BOOST_VMD_LIST_TO_TUPLE(list) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY_LIST(list), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_LIST_TO_TUPLE \
        ) \
    (list) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_LIST_TO_TUPLE_HPP */
