
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_EMPTY_HPP)
#define BOOST_VMD_EMPTY_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_EMPTY(...)

    \brief Outputs emptiness.

    ... = any variadic parameters. The parameters are ignored.
    
    This macro is used to output emptiness ( nothing ) no matter
    what is passed to it.
    
    If you use this macro to return a result, as in 'result BOOST_VMD_EMPTY'
    subsequently invoked, you should surround the result with 
    BOOST_VMD_IDENTITY_RESULT to smooth over a VC++ problem.
    
*/
    
#define BOOST_VMD_EMPTY(...)

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_EMPTY_HPP */
