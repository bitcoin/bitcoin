
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_ASSERT_IS_TYPE_HPP)
#define BOOST_VMD_ASSERT_IS_TYPE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_ASSERT_IS_TYPE(sequence)

    \brief Asserts that the sequence is a VMD type.

    The macro checks that the sequence is a VMD type.
    If it is not a VMD type, it forces a compiler error.
    
    The macro normally checks for a VMD type only in 
    debug mode. However an end-user can force the macro 
    to check or not check by defining the macro 
    BOOST_VMD_ASSERT_DATA to 1 or 0 respectively.

    sequence = a possible VMD type.

  @code
  
    returns = Normally the macro returns nothing. 
    
              If the sequence is a VMD type, nothing is 
              output.
              
              For VC++, because there is no sure way of forcing  
              a compiler error from within a macro without producing
              output, if the sequence is not a VMD type the 
              macro forces a compiler error by outputting invalid C++.
              
              For all other compilers a compiler error is forced 
              without producing output if the sequence is not a 
              VMD type.
    
  @endcode
  
*/

/** \def BOOST_VMD_ASSERT_IS_TYPE_D(d,sequence)

    \brief Asserts that the sequence is a VMD type. Re-entrant version.

    The macro checks that the sequence is a VMD type.
    If it is not a VMD type, it forces a compiler error.
    
    The macro normally checks for a VMD type only in 
    debug mode. However an end-user can force the macro 
    to check or not check by defining the macro 
    BOOST_VMD_ASSERT_DATA to 1 or 0 respectively.

    d       = The next available BOOST_PP_WHILE iteration. <br/>
    sequence = a possible VMD type.

  @code
  
    returns = Normally the macro returns nothing. 
    
              If the sequence is a VMD type, nothing is 
              output.
              
              For VC++, because there is no sure way of forcing  
              a compiler error from within a macro without producing
              output, if the sequence is not a VMD type the 
              macro forces a compiler error by outputting invalid C++.
              
              For all other compilers a compiler error is forced 
              without producing output if the sequence is not a 
              VMD type.
    
  @endcode
  
*/

#if !BOOST_VMD_ASSERT_DATA

#define BOOST_VMD_ASSERT_IS_TYPE(sequence)
#define BOOST_VMD_ASSERT_IS_TYPE_D(d,sequence)

#else

#include <boost/vmd/assert.hpp>
#include <boost/vmd/is_type.hpp>

#define BOOST_VMD_ASSERT_IS_TYPE(sequence) \
    BOOST_VMD_ASSERT \
          ( \
          BOOST_VMD_IS_TYPE(sequence), \
          BOOST_VMD_IS_TYPE_ASSERT_ERROR \
          ) \
/**/

#define BOOST_VMD_ASSERT_IS_TYPE_D(d,sequence) \
    BOOST_VMD_ASSERT \
          ( \
          BOOST_VMD_IS_TYPE_D(d,sequence), \
          BOOST_VMD_IS_TYPE_ASSERT_ERROR \
          ) \
/**/

#endif // !BOOST_VMD_ASSERT_DATA

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_ASSERT_IS_TYPE_HPP */
