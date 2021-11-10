
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_SIZE_HPP)
#define BOOST_VMD_SIZE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/sequence_size.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_SIZE(sequence)

    \brief Returns the size of a sequence.

    sequence  = A sequence to test.

    returns   = If the sequence is empty returns 0, else returns the number of elements
                in the sequence.
    
*/

#define BOOST_VMD_SIZE(sequence) \
    BOOST_VMD_DETAIL_SEQUENCE_SIZE(sequence) \
/**/

/** \def BOOST_VMD_SIZE_D(d,sequence)

    \brief Returns the size of a sequence. Re-entrant version.

    d         = The next available BOOST_PP_WHILE iteration. <br/>
    sequence  = A sequence to test.

    returns   = If the sequence is empty returns 0, else returns the number of elements
                in the sequence.
    
*/

#define BOOST_VMD_SIZE_D(d,sequence) \
    BOOST_VMD_DETAIL_SEQUENCE_SIZE_D(d,sequence) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_SIZE_HPP */
