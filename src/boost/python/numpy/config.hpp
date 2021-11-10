//  (C) Copyright Samuli-Petrus Korhonen 2017.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of NMR Solutions, Inc., in
//  producing this work.

//  Revision History:
//  15 Feb 17  Initial version

#ifndef CONFIG_NUMPY20170215_H_
# define CONFIG_NUMPY20170215_H_

# include <boost/config.hpp>

/*****************************************************************************
 *
 *  Set up dll import/export options:
 *
 ****************************************************************************/

// backwards compatibility:
#ifdef BOOST_NUMPY_STATIC_LIB
#  define BOOST_NUMPY_STATIC_LINK
# elif !defined(BOOST_NUMPY_DYNAMIC_LIB)
#  define BOOST_NUMPY_DYNAMIC_LIB
#endif

#if defined(BOOST_NUMPY_DYNAMIC_LIB)
#  if defined(BOOST_SYMBOL_EXPORT)
#     if defined(BOOST_NUMPY_SOURCE)
#        define BOOST_NUMPY_DECL           BOOST_SYMBOL_EXPORT
#        define BOOST_NUMPY_DECL_FORWARD   BOOST_SYMBOL_FORWARD_EXPORT
#        define BOOST_NUMPY_DECL_EXCEPTION BOOST_EXCEPTION_EXPORT
#        define BOOST_NUMPY_BUILD_DLL
#     else
#        define BOOST_NUMPY_DECL           BOOST_SYMBOL_IMPORT
#        define BOOST_NUMPY_DECL_FORWARD   BOOST_SYMBOL_FORWARD_IMPORT
#        define BOOST_NUMPY_DECL_EXCEPTION BOOST_EXCEPTION_IMPORT
#     endif
#  endif

#endif

#ifndef BOOST_NUMPY_DECL
#  define BOOST_NUMPY_DECL
#endif

#ifndef BOOST_NUMPY_DECL_FORWARD
#  define BOOST_NUMPY_DECL_FORWARD
#endif

#ifndef BOOST_NUMPY_DECL_EXCEPTION
#  define BOOST_NUMPY_DECL_EXCEPTION
#endif

//  enable automatic library variant selection  ------------------------------// 

#if !defined(BOOST_NUMPY_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_NUMPY_NO_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define _BOOST_PYTHON_CONCAT(N, M, m) N ## M ## m
#define BOOST_PYTHON_CONCAT(N, M, m) _BOOST_PYTHON_CONCAT(N, M, m)
#define BOOST_LIB_NAME BOOST_PYTHON_CONCAT(boost_numpy, PY_MAJOR_VERSION, PY_MINOR_VERSION)
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#ifdef BOOST_NUMPY_DYNAMIC_LIB
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#undef BOOST_PYTHON_CONCAT
#undef _BOOST_PYTHON_CONCAT

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#endif // CONFIG_NUMPY20170215_H_
