
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_PARENS_EMPTY_HPP)
#define BOOST_VMD_IS_PARENS_EMPTY_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/is_empty_tuple.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_PARENS_EMPTY(sequence)

    \brief Determines if the sequence is a set of parens with no data.

    sequence = a VMD sequence

    returns = 1 if the sequence is a set of parens with no data,
              else returns 0.
              
  @code
  
    A set of parens with no data may be:
    
    1) a tuple whose size is a single element which is empty
    
                or
                
    2) a single element seq whose data is empty
    
  @endcode
  
*/

#define BOOST_VMD_IS_PARENS_EMPTY(sequence) \
    BOOST_VMD_DETAIL_IS_EMPTY_TUPLE(sequence) \
/**/

/** \def BOOST_VMD_IS_PARENS_EMPTY_D(d,sequence)

    \brief Determines if the sequence is a set of parens with no data. Re-entrant version.

    d        = The next available BOOST_PP_WHILE iteration. <br/>
    sequence = a VMD sequence

    returns = 1 if the sequence is a set of parens with no data,
              else returns 0.
              
  @code
  
    A set of parens with no data may be:
    
    1) a tuple whose size is a single element which is empty
    
                or
                
    2) a single element seq whose data is empty
    
  @endcode
  
*/

#define BOOST_VMD_IS_PARENS_EMPTY_D(d,sequence) \
    BOOST_VMD_DETAIL_IS_EMPTY_TUPLE_D(d,sequence) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_PARENS_EMPTY_HPP */
