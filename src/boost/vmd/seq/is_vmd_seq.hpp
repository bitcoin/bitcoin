
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_VMD_SEQ_HPP)
#define BOOST_VMD_IS_VMD_SEQ_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_seq.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_VMD_SEQ(sequence)

    \brief Determines if a sequence is a VMD seq.

    The macro checks that the sequence is a VMD seq.
    A VMD seq, which may be a Boost PP seq or emptiness, is a superset of a Boost PP seq.
    It returns 1 if it is a VMD seq, else if returns 0.
    
    sequence = a possible Boost PP seq

    returns = 1 if it a VMD seq, else returns 0.
    
*/

#define BOOST_VMD_IS_VMD_SEQ(sequence) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(sequence), \
            BOOST_VMD_IDENTITY(1), \
            BOOST_VMD_IS_SEQ \
            ) \
        (sequence) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_VMD_SEQ_HPP */
