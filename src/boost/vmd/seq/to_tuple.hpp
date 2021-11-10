
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_SEQ_TO_TUPLE_HPP)
#define BOOST_VMD_SEQ_TO_TUPLE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>
#include <boost/vmd/empty.hpp>
#include <boost/vmd/is_empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_SEQ_TO_TUPLE(seq)

    \brief converts a seq to a tuple.

    seq = seq to be converted.
    
    If the seq is an empty seq it is converted to an empty tuple.
    Otherwise the seq is converted to a tuple with the same number of elements as the seq.
*/

#define BOOST_VMD_SEQ_TO_TUPLE(seq) \
    BOOST_PP_IIF \
        ( \
        BOOST_VMD_IS_EMPTY(seq), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_SEQ_TO_TUPLE \
        ) \
    (seq) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_SEQ_TO_TUPLE_HPP */
