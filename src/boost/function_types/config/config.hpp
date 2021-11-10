
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#ifndef BOOST_FT_CONFIG_HPP_INCLUDED
#define BOOST_FT_CONFIG_HPP_INCLUDED

#include <boost/function_types/config/compiler.hpp>
#include <boost/function_types/config/cc_names.hpp>

// maximum allowed arity
#ifndef BOOST_FT_MAX_ARITY
#define BOOST_FT_MAX_ARITY 20
#endif

// the most common calling conventions for x86 architecture can be enabled at
// once in the compiler config
#ifdef BOOST_FT_COMMON_X86_CCs
#   ifndef BOOST_FT_CC_CDECL
#   define BOOST_FT_CC_CDECL BOOST_FT_COMMON_X86_CCs
#   endif
#   ifndef BOOST_FT_CC_STDCALL
#   define BOOST_FT_CC_STDCALL non_variadic|BOOST_FT_COMMON_X86_CCs
#   endif
#   ifndef BOOST_FT_CC_FASTCALL
#   define BOOST_FT_CC_FASTCALL non_variadic|BOOST_FT_COMMON_X86_CCs
#   endif
#endif

// where to place the cc specifier (the common way)
#ifndef BOOST_FT_SYNTAX
#   define BOOST_FT_SYNTAX(result,lparen,cc_spec,type_mod,name,rparen) \
                        result() lparen() cc_spec() type_mod() name() rparen()
#endif

// param for nullary functions
// set to "void" for compilers that require nullary functions to read 
// "R (void)" in template partial specialization
#ifndef BOOST_FT_NULLARY_PARAM
#define BOOST_FT_NULLARY_PARAM 
#endif

// there is a pending defect report on cv qualified function types, so support
// for these types is disabled, unless for compilers where it's known to work
#ifndef BOOST_FT_NO_CV_FUNC_SUPPORT
#define BOOST_FT_NO_CV_FUNC_SUPPORT 1
#endif

// full preprocessing implies preprocessing of the ccs
#if defined(BOOST_FT_PREPROCESSING_MODE) && !defined(BOOST_FT_CC_PREPROCESSING)
#   define BOOST_FT_CC_PREPROCESSING 1
#endif

#endif

