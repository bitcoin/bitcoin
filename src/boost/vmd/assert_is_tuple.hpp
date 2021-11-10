
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_ASSERT_IS_TUPLE_HPP)
#define BOOST_VMD_ASSERT_IS_TUPLE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_ASSERT_IS_TUPLE(sequence)

    \brief Asserts that the sequence is a Boost PP tuple.

    The macro checks that the sequence is a Boost PP tuple.
    If it is not a Boost PP tuple, it forces a compiler error.
    
    The macro normally checks for a Boost PP tuple only in 
    debug mode. However an end-user can force the macro 
    to check or not check by defining the macro 
    BOOST_VMD_ASSERT_DATA to 1 or 0 respectively.

    sequence = a possible Boost PP tuple.

  @code
  
    returns  = Normally the macro returns nothing. 
    
               If the sequence is a Boost PP tuple, nothing is 
               output.
              
               For VC++, because there is no sure way of forcing  
               a compiler error from within a macro without producing
               output, if the sequence is not a Boost PP tuple the 
               macro forces a compiler error by outputting invalid C++. 
              
               For all other compilers a compiler error is forced 
               without producing output if the sequence is not a 
               Boost PP tuple.
              
  @endcode
  
*/

#if !BOOST_VMD_ASSERT_DATA

#define BOOST_VMD_ASSERT_IS_TUPLE(sequence)

#else

#include <boost/vmd/assert.hpp>
#include <boost/vmd/is_tuple.hpp>

#define BOOST_VMD_ASSERT_IS_TUPLE(sequence) \
   BOOST_VMD_ASSERT \
     ( \
     BOOST_VMD_IS_TUPLE(sequence), \
     BOOST_VMD_ASSERT_IS_TUPLE_ERROR \
     ) \
/**/

#endif /* BOOST_VMD_ASSERT_DATA */

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_ASSERT_IS_TUPLE_HPP */
