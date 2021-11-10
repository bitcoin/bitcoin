
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_EMPTY_ARRAY_HPP)
#define BOOST_VMD_IS_EMPTY_ARRAY_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/vmd/is_array.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/detail/is_empty_array.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_EMPTY_ARRAY(sequence)

    \brief Tests whether a sequence is an empty Boost PP array.

    An empty Boost PP array is a two element tuple where the first
    size element is 0 and the second element is a tuple with a single 
    empty element, ie. '(0,())'.
    
    sequence = a possible empty array

    returns = 1 if the sequence is an empty Boost PP array,
              0 if it is not.
              
    The macro will generate a preprocessing error if the sequence
    is in the form of an array but its first tuple element, instead
    of being a number, is a preprocessor token which VMD cannot parse,
    as in the example '(&0,())' which is a valid tuple but an invalid
    array.
    
*/

#define BOOST_VMD_IS_EMPTY_ARRAY(sequence) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_ARRAY(sequence), \
            BOOST_VMD_DETAIL_IS_EMPTY_ARRAY_SIZE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (sequence) \
        ) \
/**/

/** \def BOOST_VMD_IS_EMPTY_ARRAY_D(d,sequence)

    \brief Tests whether a sequence is an empty Boost PP array. Re-entrant version.

    An empty Boost PP array is a two element tuple where the first
    size element is 0 and the second element is a tuple with a single 
    empty element, ie. '(0,())'.
    
    d        = The next available BOOST_PP_WHILE iteration. <br/>
    sequence = a possible empty array

    returns = 1 if the sequence is an empty Boost PP array,
              0 if it is not.
              
    The macro will generate a preprocessing error if the sequence
    is in the form of an array but its first tuple element, instead
    of being a number, is a preprocessor token which VMD cannot parse,
    as in the example '(&0,())' which is a valid tuple but an invalid
    array.
    
*/

#define BOOST_VMD_IS_EMPTY_ARRAY_D(d,sequence) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_ARRAY_D(d,sequence), \
            BOOST_VMD_DETAIL_IS_EMPTY_ARRAY_SIZE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (sequence) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_EMPTY_ARRAY_HPP */
