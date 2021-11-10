//  (C) Copyright David Abrahams 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of Dragon Systems, Inc., in
//  producing this work.

//  Revision History:
//  04 Mar 01  Some fixes so it will compile with Intel C++ (Dave Abrahams)

#ifndef CONFIG_DWA052200_H_
# define CONFIG_DWA052200_H_

# include <boost/config.hpp>
# include <boost/detail/workaround.hpp>

# ifdef BOOST_NO_OPERATORS_IN_NAMESPACE
   // A gcc bug forces some symbols into the global namespace
#  define BOOST_PYTHON_BEGIN_CONVERSION_NAMESPACE
#  define BOOST_PYTHON_END_CONVERSION_NAMESPACE
#  define BOOST_PYTHON_CONVERSION
#  define BOOST_PYTHON_IMPORT_CONVERSION(x) using ::x
# else
#  define BOOST_PYTHON_BEGIN_CONVERSION_NAMESPACE namespace boost { namespace python {
#  define BOOST_PYTHON_END_CONVERSION_NAMESPACE }} // namespace boost::python
#  define BOOST_PYTHON_CONVERSION boost::python
#  define BOOST_PYTHON_IMPORT_CONVERSION(x) void never_defined() // so we can follow the macro with a ';'
# endif

# if defined(BOOST_MSVC)

#  pragma warning (disable : 4786) // disable truncated debug symbols
#  pragma warning (disable : 4251) // disable exported dll function
#  pragma warning (disable : 4800) //'int' : forcing value to bool 'true' or 'false'
#  pragma warning (disable : 4275) // non dll-interface class

# elif defined(__ICL) && __ICL < 600 // Intel C++ 5

#  pragma warning(disable: 985) // identifier was truncated in debug information

# endif

// The STLport puts all of the standard 'C' library names in std (as far as the
// user is concerned), but without it you need a fix if you're using MSVC or
// Intel C++
# if defined(BOOST_NO_STDC_NAMESPACE)
#  define BOOST_CSTD_
# else
#  define BOOST_CSTD_ std
# endif

/*****************************************************************************
 *
 *  Set up dll import/export options:
 *
 ****************************************************************************/

// backwards compatibility:
#ifdef BOOST_PYTHON_STATIC_LIB
#  define BOOST_PYTHON_STATIC_LINK
# elif !defined(BOOST_PYTHON_DYNAMIC_LIB)
#  define BOOST_PYTHON_DYNAMIC_LIB
#endif

#if defined(BOOST_PYTHON_DYNAMIC_LIB)
#  if defined(BOOST_SYMBOL_EXPORT)
#     if defined(BOOST_PYTHON_SOURCE)
#        define BOOST_PYTHON_DECL           BOOST_SYMBOL_EXPORT
#        define BOOST_PYTHON_DECL_FORWARD   BOOST_SYMBOL_FORWARD_EXPORT
#        define BOOST_PYTHON_DECL_EXCEPTION BOOST_EXCEPTION_EXPORT
#        define BOOST_PYTHON_BUILD_DLL
#     else
#        define BOOST_PYTHON_DECL           BOOST_SYMBOL_IMPORT
#        define BOOST_PYTHON_DECL_FORWARD   BOOST_SYMBOL_FORWARD_IMPORT
#        define BOOST_PYTHON_DECL_EXCEPTION BOOST_EXCEPTION_IMPORT
#     endif
#  endif
#endif

#ifndef BOOST_PYTHON_DECL
#  define BOOST_PYTHON_DECL
#endif

#ifndef BOOST_PYTHON_DECL_FORWARD
#  define BOOST_PYTHON_DECL_FORWARD
#endif

#ifndef BOOST_PYTHON_DECL_EXCEPTION
#  define BOOST_PYTHON_DECL_EXCEPTION
#endif

#if BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))
// Replace broken Tru64/cxx offsetof macro
# define BOOST_PYTHON_OFFSETOF(s_name, s_member) \
        ((size_t)__INTADDR__(&(((s_name *)0)->s_member)))
#else
# define BOOST_PYTHON_OFFSETOF offsetof
#endif

//  enable automatic library variant selection  ------------------------------// 

#if !defined(BOOST_PYTHON_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_PYTHON_NO_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define _BOOST_PYTHON_CONCAT(N, M, m) N ## M ## m
#define BOOST_PYTHON_CONCAT(N, M, m) _BOOST_PYTHON_CONCAT(N, M, m)
#define BOOST_LIB_NAME BOOST_PYTHON_CONCAT(boost_python, PY_MAJOR_VERSION, PY_MINOR_VERSION)
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#ifdef BOOST_PYTHON_DYNAMIC_LIB
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#undef BOOST_PYTHON_CONCAT
#undef _BOOST_PYTHON_CONCAT

#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
#define BOOST_PYTHON_SUPPORTS_PY_SIGNATURES // enables smooth transition
#endif

#if !defined(BOOST_ATTRIBUTE_UNUSED) && defined(__GNUC__) && (__GNUC__ >= 4)
#  define BOOST_ATTRIBUTE_UNUSED __attribute__((unused))
#endif

#endif // CONFIG_DWA052200_H_
