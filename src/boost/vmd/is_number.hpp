
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_NUMBER_HPP)
#define BOOST_VMD_IS_NUMBER_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/is_number.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_NUMBER(sequence)

    \brief Tests whether a sequence is a Boost PP number.

    The macro checks to see if a sequence is a Boost PP number.
    A Boost PP number is a value from 0 to BOOST_PP_LIMIT_MAG.
    
    sequence = a possible number

    returns = 1 if the sequence is a Boost PP number,
              0 if it is not.
              
    If the input is not a VMD data type this macro could lead to
    a preprocessor error. This is because the macro
    uses preprocessor concatenation to determine if the input
    is a number once it is determined that the input does not
    start with parenthesis. If the data being concatenated would
    lead to an invalid preprocessor token the compiler can issue
    a preprocessor error.
              
*/

#define BOOST_VMD_IS_NUMBER(sequence) \
    BOOST_VMD_DETAIL_IS_NUMBER(sequence) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_NUMBER_HPP */
