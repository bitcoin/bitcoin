
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_ENUM_HPP)
#define BOOST_VMD_ENUM_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/sequence_enum.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_ENUM(...)

    \brief Converts a sequence to comma-separated elements which are the elements of the sequence.

    ...       = Variadic parameters.
    
    The first variadic parameter is required and is the sequence to convert.
    
    Further optional variadic parameters can be return type parameters. Return type
    parameters allow each element in the sequence to be converted to a two-element
    tuple where the first tuple element is the type and the second tuple element
    is the element data.
    
    The BOOST_VMD_RETURN_NO_TYPE, the default, does not return the type as part of each
    converted element but just the data. All of the rest return the type and data as the
    two-element tuple. If BOOST_VMD_RETURN_TYPE is specified the specific type of the element
    is returned in the tuple. If BOOST_VMD_RETURN_TYPE_ARRAY is specified an array type is 
    returned if the element is an array, else a tuple type is returned if the element is a tuple, 
    else the actual type is returned for non-tuple data. If BOOST_VMD_RETURN_TYPE_LIST is specified
    a list type is returned if the element is a list, else a tuple type is returned if the element 
    is a tuple, else the actual type is returned for non-tuple data. If BOOST_VMD_RETURN_TYPE_TUPLE 
    is specified a tuple type is returned for all tuple-like data, else the actual type is returned 
    for non-tuple data. If more than one return type optional parameter is specified the last one 
    specified determines the return type.
    
    returns   = Comma-separated data, otherwise known as variadic data.
                If the sequence is empty the variadic data is empty. If an 
                optional return type other than BOOST_VMD_RETURN_NO_TYPE
                is specified the type and the data of each element is
                returned as part of the variadic data. Otherwise just the data
                of each element is returned, which is the default.
    
*/

#define BOOST_VMD_ENUM(...) \
    BOOST_VMD_DETAIL_SEQUENCE_ENUM(__VA_ARGS__) \
/**/

/** \def BOOST_VMD_ENUM_D(d,...)

    \brief Converts a sequence to comma-separated elements which are the elements of the sequence. Re-entrant version.

    d         = The next available BOOST_PP_WHILE iteration. <br/>
    ...       = Variadic parameters.
    
    The first variadic parameter is required and is the sequence to convert.
    
    Further optional variadic parameters can be return type parameters. Return type
    parameters allow each element in the sequence to be converted to a two-element
    tuple where the first tuple element is the type and the second tuple element
    is the element data.
    
    The BOOST_VMD_RETURN_NO_TYPE, the default, does not return the type as part of each
    converted element but just the data. All of the rest return the type and data as the
    two-element tuple. If BOOST_VMD_RETURN_TYPE is specified the specific type of the element
    is returned in the tuple. If BOOST_VMD_RETURN_TYPE_ARRAY is specified an array type is 
    returned if the element is an array, else a tuple type is returned if the element is a tuple, 
    else the actual type is returned for non-tuple data. If BOOST_VMD_RETURN_TYPE_LIST is specified
    a list type is returned if the element is a list, else a tuple type is returned if the element 
    is a tuple, else the actual type is returned for non-tuple data. If BOOST_VMD_RETURN_TYPE_TUPLE 
    is specified a tuple type is returned for all tuple-like data, else the actual type is returned 
    for non-tuple data. If more than one return type optional parameter is specified the last one 
    specified determines the return type.
    
    returns   = Comma-separated data, otherwise known as variadic data.
                If the sequence is empty the variadic data is empty. If an 
                optional return type other than BOOST_VMD_RETURN_NO_TYPE
                is specified the type and the data of each element is
                returned as part of the variadic data. Otherwise just the data
                of each element is returned, which is the default.
    
*/

#define BOOST_VMD_ENUM_D(d,...) \
    BOOST_VMD_DETAIL_SEQUENCE_ENUM_D(d,__VA_ARGS__) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_ENUM_HPP */
