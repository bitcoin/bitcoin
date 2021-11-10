
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_EQUAL_HPP)
#define BOOST_VMD_EQUAL_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/equal.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_EQUAL(sequence,...)

    \brief Tests any two sequences for equality.

    sequence     = First sequence. <br/>
    ...          = variadic parameters, maximum of 2.
    
    The first variadic parameter is required and is the second sequence to test.
    The optional second variadic parameter is a VMD type as a filter.
    
    The macro tests any two sequences for equality. For sequences to be equal the
    VMD types of each sequence must be equal and the individual elements of the
    sequence must be equal. For Boost PP composite types the macro tests that
    the composite types have the same size and then tests that each element
    of the composite type is equal. This means that all elements of a composite
    type must be a VMD type in order to use this macro successfully.
    
    The single optional parameter is a filter. The filter is a VMD type which specifies
    that both sequences to test must be of that VMD type, as well as being equal to
    each other, for the test to succeed.
    
    returns   = 1 upon success or 0 upon failure. Success means that both sequences are
                equal and, if the optional parameter is specified, that the sequences are
                of the optional VMD type.
    
*/

#define BOOST_VMD_EQUAL(sequence,...) \
    BOOST_VMD_DETAIL_EQUAL(sequence,__VA_ARGS__) \
/**/

/** \def BOOST_VMD_EQUAL_D(d,sequence,...)

    \brief Tests any two sequences for equality. Re-entrant version.

    d            = The next available BOOST_PP_WHILE iteration. <br/>
    sequence     = First sequence. <br/>
    ...          = variadic parameters, maximum of 2.
    
    The first variadic parameter is required and is the second sequence to test.
    The optional second variadic parameter is a VMD type as a filter.
    
    The macro tests any two sequences for equality. For sequences to be equal the
    VMD types of each sequence must be equal and the individual elements of the
    sequence must be equal. For Boost PP composite types the macro tests that
    the composite types have the same size and then tests that each element
    of the composite type is equal. This means that all elements of a composite
    type must be a VMD type in order to use this macro successfully.
    
    The single optional parameter is a filter. The filter is a VMD type which specifies
    that both sequences to test must be of that VMD type, as well as being equal to
    each other, for the test to succeed.
    
    returns   = 1 upon success or 0 upon failure. Success means that both sequences are
                equal and, if the optional parameter is specified, that the sequences are
                of the optional VMD type.
    
*/

#define BOOST_VMD_EQUAL_D(d,sequence,...) \
    BOOST_VMD_DETAIL_EQUAL_D(d,sequence,__VA_ARGS__) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_EQUAL_HPP */
