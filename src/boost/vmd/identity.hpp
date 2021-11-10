
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_IDENTITY_HPP)
#define BOOST_VMD_IDENTITY_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#if BOOST_VMD_MSVC
#include <boost/preprocessor/cat.hpp>
#endif
#include <boost/vmd/empty.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_IDENTITY(item)

    \brief Macro which expands to its argument when invoked with any number of parameters.

    item = any single argument
    
    When BOOST_VMD_IDENTITY(item) is subsequently invoked with any number of parameters it expands
    to 'item'. Subsequently invoking the macro is done as 'BOOST_VMD_IDENTITY(item)(zero_or_more_arguments)'.
    
    The macro is equivalent to the Boost PP macro BOOST_PP_IDENTITY(item) with the difference
    being that BOOST_PP_IDENTITY(item) is always invoked with no arguments, as in
    'BOOST_VMD_IDENTITY(item)()' whereas BOOST_VMD_IDENTITY can be invoked with any number of
    arguments.
    
    The macro is meant to be used in BOOST_PP_IF and BOOST_PP_IIF statements when only one
    of the clauses needs to be invoked with calling another macro and the other is meant to
    return an 'item'.
    
    returns = the macro as 'BOOST_VMD_IDENTITY(item)', when invoked with any number of parameters
              as in '(zero_or_more_arguments)', returns 'item'. The macro itself returns
              'item BOOST_VMD_EMPTY'.
    
*/

#define BOOST_VMD_IDENTITY(item) item BOOST_VMD_EMPTY

/** \def BOOST_VMD_IDENTITY_RESULT(result)

    \brief Macro which wraps any result which can return its value using BOOST_VMD_IDENTITY or 'item BOOST_VMD_EMPTY'.

    result = any single result returned when BOOST_VMD_IDENTITY is used or 'item BOOST_VMD_EMPTY'.
    
    The reason for this macro is to smooth over a problem when using VC++ with BOOST_VMD_IDENTITY.
    If your BOOST_VMD_IDENTITY macro can be used where VC++ is the compiler then you need to
    surround your macro code which could return a result with this macro in order that VC++ handles
    BOOST_VMD_IDENTITY correctly.
    
    If you are not using VC++ you do not have to use this macro, but doing so does no harm.
    
*/

#if BOOST_VMD_MSVC
#define BOOST_VMD_IDENTITY_RESULT(result) BOOST_PP_CAT(result,)
#else
#define BOOST_VMD_IDENTITY_RESULT(result) result
#endif

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_IDENTITY_HPP */
