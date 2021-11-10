
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_SEQ_PUSH_FRONT_HPP)
#define BOOST_VMD_SEQ_PUSH_FRONT_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq/push_front.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_SEQ_PUSH_FRONT(seq,elem)

    \brief inserts an element at the beginning of a seq. 

    seq = seq to insert an element at. <br/>
    elem  = element to insert.

    If the seq is an empty seq the result is a seq with the single element.
    Otherwise the result is a seq after inserting the element at the beginning.
*/

#define BOOST_VMD_SEQ_PUSH_FRONT(seq,elem) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(seq), \
            BOOST_VMD_IDENTITY((elem)), \
            BOOST_PP_SEQ_PUSH_FRONT \
            ) \
        (seq,elem) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_SEQ_PUSH_FRONT_HPP */
