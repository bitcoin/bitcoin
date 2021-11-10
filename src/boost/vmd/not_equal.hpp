
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_NOT_EQUAL_HPP)
#define BOOST_VMD_NOT_EQUAL_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/logical/compl.hpp>
#include <boost/vmd/equal.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_NOT_EQUAL(sequence,...)

    \brief Tests any two sequences for inequality.

    sequence     = First sequence. <br/>
    ...          = variadic parameters, maximum of 2.
    
    The first variadic parameter is required and is the second sequence to test.
    The optional second variadic parameter is a VMD type as a filter.
    
    The macro tests any two sequences for inequality. For sequences to be unequal 
    either the VMD types of each sequence must be unequal or the individual elements of the
    sequence must be unequal. 
    
    The single optional parameter is a filter. The filter is a VMD type which specifies
    that both sequences to test must be of that VMD type, as well as being equal to
    each other, for the test to fail, else it succeeds.
    
    returns   = 1 upon success or 0 upon failure. Success means that the sequences are
                unequal or, if the optional parameter is specified, that the sequences are
                not of the optional VMD type; otherwise 0 is returned if the sequences
                are equal.
                
    The macro is implemented as the complement of BOOST_VMD_EQUAL, so that whenever
    BOOST_VMD_EQUAL would return 1 the macro returns 0 and whenever BOOST_VMD_EQUAL
    would return 0 the macro would return 1.
    
*/

#define BOOST_VMD_NOT_EQUAL(sequence,...) \
    BOOST_PP_COMPL(BOOST_VMD_EQUAL(sequence,__VA_ARGS__)) \
/**/

/** \def BOOST_VMD_NOT_EQUAL_D(d,sequence,...)

    \brief Tests any two sequences for inequality. Re-entrant version.

    d         = The next available BOOST_PP_WHILE iteration. <br/>
    sequence  = First sequence. <br/>
    ...       = variadic parameters, maximum of 2.
    
    The first variadic parameter is required and is the second sequence to test.
    The optional second variadic parameter is a VMD type as a filter.
    
    The macro tests any two sequences for inequality. For sequences to be unequal 
    either the VMD types of each sequence must be unequal or the individual elements of the
    sequence must be unequal. 
    
    The single optional parameter is a filter. The filter is a VMD type which specifies
    that both sequences to test must be of that VMD type, as well as being equal to
    each other, for the test to fail, else it succeeds.
    
    returns   = 1 upon success or 0 upon failure. Success means that the sequences are
                unequal or, if the optional parameter is specified, that the sequences are
                not of the optional VMD type; otherwise 0 is returned if the sequences
                are equal.
                
    The macro is implemented as the complement of BOOST_VMD_EQUAL, so that whenever
    BOOST_VMD_EQUAL would return 1 the macro returns 0 and whenever BOOST_VMD_EQUAL
    would return 0 the macro would return 1.
    
*/

#define BOOST_VMD_NOT_EQUAL_D(d,sequence,...) \
    BOOST_PP_COMPL(BOOST_VMD_EQUAL_D(d,sequence,__VA_ARGS__)) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_NOT_EQUAL_HPP */
