
//  (C) Copyright Edward Diener 2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_VMD_TUPLE_HPP)
#define BOOST_VMD_IS_VMD_TUPLE_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/control/iif.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_tuple.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_VMD_TUPLE(sequence)

    \brief Determines if a sequence is a VMD tuple.

    The macro checks that the sequence is a VMD tuple.
    A VMD tuple, which may be a Boost PP tuple or emptiness, is a superset of a Boost PP tuple.
    It returns 1 if it is a VMD tuple, else if returns 0.
    
    sequence = a possible Boost PP tuple

    returns = 1 if it a VMD tuple, else returns 0.
    
*/

#define BOOST_VMD_IS_VMD_TUPLE(sequence) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_IS_EMPTY(sequence), \
            BOOST_VMD_IDENTITY(1), \
            BOOST_VMD_IS_TUPLE \
            ) \
        (sequence) \
        ) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_VMD_TUPLE_HPP */
