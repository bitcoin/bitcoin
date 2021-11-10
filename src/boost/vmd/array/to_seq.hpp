
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_ARRAY_TO_SEQ_HPP)
#define BOOST_VMD_ARRAY_TO_SEQ_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/array/to_seq.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/is_empty_array.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_ARRAY_TO_SEQ(array)

    \brief converts an array to a seq.

    array = array to be converted.
    
    If the array is an array of 0 elements it is converted to an empty seq.
    Otherwise the array is converted to a seq with the same number of elements as the array.
*/

#define BOOST_VMD_ARRAY_TO_SEQ(array) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY_ARRAY(array), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_ARRAY_TO_SEQ \
        ) \
    (array) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_ARRAY_TO_SEQ_HPP */
