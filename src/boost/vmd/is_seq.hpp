
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_SEQ_HPP)
#define BOOST_VMD_IS_SEQ_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/is_seq.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_SEQ(sequence)

    \brief Determines if a sequence is a Boost PP seq.

    The macro checks that the sequence is a Boost PP seq.
    It returns 1 if it is a seq, else if returns 0.
    
    sequence = a possible Boost PP seq

    returns = 1 if it a seq, else returns 0.
    
    A single set of parentheses, with a single element, 
    is parsed as a tuple and not a seq. To be parsed as
    a seq the input needs to be more than one consecutive
    sets of parentheses, each with a single element of data.
    
*/

#define BOOST_VMD_IS_SEQ(sequence) \
    BOOST_VMD_DETAIL_IS_SEQ(sequence) \
/**/

/** \def BOOST_VMD_IS_SEQ_D(d,sequence)

    \brief Determines if a sequence is a Boost PP seq. Re-entrant version.

    The macro checks that the sequence is a Boost PP seq.
    It returns 1 if it is a seq, else if returns 0.
    
    d        = The next available BOOST_PP_WHILE iteration. <br/>
    sequence = a possible Boost PP seq

    returns = 1 if it a seq, else returns 0.
    
    A single set of parentheses, with a single element, 
    is parsed as a tuple and not a seq. To be parsed as
    a seq the input needs to be more than one consecutive
    sets of parentheses, each with a single element of data.
    
*/

#define BOOST_VMD_IS_SEQ_D(d,sequence) \
    BOOST_VMD_DETAIL_IS_SEQ_D(d,sequence) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_SEQ_HPP */
