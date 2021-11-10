
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_SEQ_TO_ARRAY_HPP)
#define BOOST_VMD_SEQ_TO_ARRAY_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq/to_array.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_SEQ_TO_ARRAY(seq)

    \brief converts a seq to an array.

    seq = seq to be converted.
    
    If the seq is an empty seq it is converted to an array with 0 elements.
    Otherwise the seq is converted to an array with the same number of elements as the seq.
*/

#define BOOST_VMD_SEQ_TO_ARRAY(seq) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(seq), \
            BOOST_VMD_IDENTITY((0,())), \
            BOOST_PP_SEQ_TO_ARRAY \
            ) \
        (seq) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_SEQ_TO_ARRAY_HPP */
