
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_EMPTY_LIST_HPP)
#define BOOST_VMD_IS_EMPTY_LIST_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/is_list.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_EMPTY_LIST(sequence)

    \brief Tests whether a sequence is an empty Boost PP list.

    An empty Boost PP list consists of the single identifier 'BOOST_PP_NIL'.
    This identifier also serves as a list terminator for a non-empty list.
    
    sequence = a preprocessor parameter

    returns = 1 if the sequence is an empty Boost PP list,
              0 if it is not.
              
    The macro will generate a preprocessing error if the input
    as an empty list marker, instead of being an identifier, is 
    a preprocessor token which VMD cannot parse, as in the 
    example '&BOOST_PP_NIL'.
    
*/

#define BOOST_VMD_IS_EMPTY_LIST(sequence) \
    BOOST_VMD_DETAIL_IS_LIST_IS_EMPTY_LIST_PROCESS(sequence) \
/**/

/** \def BOOST_VMD_IS_EMPTY_LIST_D(d,sequence)

    \brief Tests whether a sequence is an empty Boost PP list. Re-entrant version.

    An empty Boost PP list consists of the single identifier 'BOOST_PP_NIL'.
    This identifier also serves as a list terminator for a non-empty list.
    
    d        = The next available BOOST_PP_WHILE iteration <br/>
    sequence = a preprocessor parameter

    returns = 1 if the sequence is an empty Boost PP list,
              0 if it is not.
              
    The macro will generate a preprocessing error if the input
    as an empty list marker, instead of being an identifier, is 
    a preprocessor token which VMD cannot parse, as in the 
    example '&BOOST_PP_NIL'.
    
*/

#define BOOST_VMD_IS_EMPTY_LIST_D(d,sequence) \
    BOOST_VMD_DETAIL_IS_LIST_IS_EMPTY_LIST_PROCESS_D(d,sequence) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_EMPTY_LIST_HPP */
