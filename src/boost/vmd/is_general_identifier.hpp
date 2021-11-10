
//  (C) Copyright Edward Diener 2020
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IS_GENERAL_IDENTIFIER_HPP)
#define BOOST_VMD_IS_GENERAL_IDENTIFIER_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/is_general_identifier.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IS_GENERAL_IDENTIFIER(...)

    \brief Tests whether a parameter is a general identifier.

    ...       = variadic parameters
    
    The first variadic parameter is required and it is the input to test.
    
    Passing more than one variadic argument is an error.

  @code
  
    returns   = 1 if the parameter is any general identifier and only a single variadic argument is given, otherwise 0.
                
  @endcode
  
    The argument to the macro should be a single possible identifier
    and not a VMD sequence of preprocessor tokens.
  
    If the input is not a VMD data type this macro could lead to
    a preprocessor error. This is because the macro
    uses preprocessor concatenation to determine if the input
    is an identifier once it is determined that the input is not empty
    and does not start with parenthesis. If the data being concatenated would
    lead to an invalid preprocessor token the compiler can issue
    a preprocessor error.
    
*/

#define BOOST_VMD_IS_GENERAL_IDENTIFIER(...) \
    BOOST_VMD_DETAIL_IS_GENERAL_IDENTIFIER(__VA_ARGS__) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IS_GENERAL_IDENTIFIER_HPP */
