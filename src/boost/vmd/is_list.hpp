
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_LIST_HPP)
#define BOOST_VMD_IS_LIST_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/is_list.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_LIST(sequence)

    \brief Determines if a sequence is a Boost pplib list.

    The macro checks that the sequence is a pplib list.
    It returns 1 if it is a list, else if returns 0.
    
    sequence = input as a possible Boost PP list.
    
    returns = 1 if it a list, else returns 0.
    
    The macro will generate a preprocessing error if the input
    is in the form of a list but its end-of-list marker, instead
    of being an identifier, is a preprocessor token which VMD cannot parse,
    as in the example '(anything,&BOOST_PP_NIL)' which is a valid tuple but
    an invalid list.
    
*/

#define BOOST_VMD_IS_LIST(sequence) \
    BOOST_VMD_DETAIL_IS_LIST(sequence) \
/**/

/** \def BOOST_VMD_IS_LIST_D(d,sequence)

    \brief Determines if a sequence is a Boost pplib list. Re-entrant version.

    The macro checks that the sequence is a pplib list.
    It returns 1 if it is a list, else if returns 0.
    
    d        = The next available BOOST_PP_WHILE iteration. <br/>
    sequence = input as a possible Boost PP list.
    
    returns = 1 if it a list, else returns 0.
    
    The macro will generate a preprocessing error if the input
    is in the form of a list but its end-of-list marker, instead
    of being an identifier, is a preprocessor token which VMD cannot parse,
    as in the example '(anything,&BOOST_PP_NIL)' which is a valid tuple but
    an invalid list.
    
*/

#define BOOST_VMD_IS_LIST_D(d,sequence) \
    BOOST_VMD_DETAIL_IS_LIST_D(d,sequence) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_LIST_HPP */
