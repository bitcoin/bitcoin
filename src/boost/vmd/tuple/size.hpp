
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_TUPLE_SIZE_HPP)
#define BOOST_VMD_TUPLE_SIZE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_TUPLE_SIZE(tuple)

    \brief  expands to the size of the tuple passed to it. 

    tuple = tuple whose size is to be extracted. 
    
    If the tuple is an empty tuple its size is 0.
    Otherwise the result is the number of elements in the tuple.
*/

#define BOOST_VMD_TUPLE_SIZE(tuple) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(tuple), \
            BOOST_VMD_IDENTITY(0), \
            BOOST_PP_TUPLE_SIZE \
            ) \
        (tuple) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_TUPLE_SIZE_HPP */
