
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_IDENTIFIER_HPP)
#define BOOST_VMD_IS_IDENTIFIER_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/is_identifier.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_IDENTIFIER(...)

    \brief Tests whether a parameter is an identifier.

    ...       = variadic parameters
    
    The first variadic parameter is required and it is the input to test.
    
    Further variadic parameters are optional and are identifiers to match.
    The data may take one of two forms; it is either one or more single identifiers
    or a single Boost PP tuple of identifiers.

  @code
  
    returns   = 1 if the parameter is an identifier, otherwise 0.
                
                If the parameter is not an identifier, 
                or if optional identifiers are specified and the identifier
                does not match any of the optional identifiers, the macro returns 0.
                
  @endcode
  
    Identifiers are registered in VMD with:
    
  @code
  
        #define BOOST_VMD_REG_XXX (XXX) where XXX is a v-identifier.
    
  @endcode
  
    The identifier must be registered to be found.
    
    Identifiers are pre-detected in VMD with:
    
  @code
  
        #define BOOST_VMD_DETECT_XXX_XXX where XXX is an identifier.
    
  @endcode
  
    If you specify optional identifiers and have not specified the detection
    of an optional identifier, that optional identifier will never match the input.
                
    If the input is not a VMD data type this macro could lead to
    a preprocessor error. This is because the macro
    uses preprocessor concatenation to determine if the input
    is an identifier once it is determined that the input does not
    start with parenthesis. If the data being concatenated would
    lead to an invalid preprocessor token the compiler can issue
    a preprocessor error.
    
*/

#define BOOST_VMD_IS_IDENTIFIER(...) \
    BOOST_VMD_DETAIL_IS_IDENTIFIER(__VA_ARGS__) \
/**/

/** \def BOOST_VMD_IS_IDENTIFIER_D(d,...)

    \brief Tests whether a parameter is an identifier. Re-entrant version.

    d         = The next available BOOST_PP_WHILE iteration. <br/>
    ...       = variadic parameters
    
    The first variadic parameter is required and it is the input to test.
    
    Further variadic parameters are optional and are identifiers to match.
    The data may take one of two forms; it is either one or more single identifiers
    or a single Boost PP tuple of identifiers.

  @code
  
    returns   = 1 if the parameter is an identifier, otherwise 0.
                
                If the parameter is not an identifier, 
                or if optional identifiers are specified and the identifier
                does not match any of the optional identifiers, the macro returns 0.
                
  @endcode
  
    Identifiers are registered in VMD with:
    
  @code
  
        #define BOOST_VMD_REG_XXX (XXX) where XXX is a v-identifier.
    
  @endcode
  
    The identifier must be registered to be found.
    
    Identifiers are pre-detected in VMD with:
    
  @code
  
        #define BOOST_VMD_DETECT_XXX_XXX where XXX is an identifier.
    
  @endcode
  
    If you specify optional identifiers and have not specified the detection
    of an optional identifier, that optional identifier will never match the input.
                
    If the input is not a VMD data type this macro could lead to
    a preprocessor error. This is because the macro
    uses preprocessor concatenation to determine if the input
    is an identifier once it is determined that the input does not
    start with parenthesis. If the data being concatenated would
    lead to an invalid preprocessor token the compiler can issue
    a preprocessor error.
    
*/

#define BOOST_VMD_IS_IDENTIFIER_D(d,...) \
    BOOST_VMD_DETAIL_IS_IDENTIFIER_D(d,__VA_ARGS__) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_IDENTIFIER_HPP */
