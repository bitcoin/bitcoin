
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_SEQ_POP_FRONT_HPP)
#define BOOST_VMD_SEQ_POP_FRONT_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq/pop_front.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/vmd/empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_SEQ_POP_FRONT(seq)

    \brief pops an element from the front of a seq. 

    seq = seq to pop an element from.

    If the seq is an empty seq the result is undefined.
    If the seq is a single element the result is an empty seq.
    Otherwise the result is a seq after removing the first element.
*/

#define BOOST_VMD_SEQ_POP_FRONT(seq) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE(seq),1), \
        BOOST_VMD_EMPTY, \
        BOOST_PP_SEQ_POP_FRONT \
        ) \
    (seq) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_SEQ_POP_FRONT_HPP */
