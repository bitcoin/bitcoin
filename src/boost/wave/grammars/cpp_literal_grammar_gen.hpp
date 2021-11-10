/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_LITERAL_GRAMMAR_GEN_HPP_67794A6C_468A_4AAB_A757_DEDDB182F5A0_INCLUDED)
#define BOOST_CPP_LITERAL_GRAMMAR_GEN_HPP_67794A6C_468A_4AAB_A757_DEDDB182F5A0_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <boost/wave/grammars/cpp_value_error.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

// suppress warnings about dependent classes not being exported from the dll
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4251 4231 4660)
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace grammars {

///////////////////////////////////////////////////////////////////////////////
//
//  cpp_intlit_grammar_gen template class
//
//      This template helps separating the compilation of the intlit_grammar
//      class from the compilation of the expression_grammar. This is done
//      to safe compilation time.
//
///////////////////////////////////////////////////////////////////////////////
template <typename TokenT>
struct BOOST_WAVE_DECL intlit_grammar_gen {

    static uint_literal_type evaluate(TokenT const &tok, bool &is_unsigned);
};

///////////////////////////////////////////////////////////////////////////////
//
//  cpp_chlit_grammar_gen template class
//
//      This template helps separating the compilation of the chlit_grammar
//      class from the compilation of the expression_grammar. This is done
//      to safe compilation time.
//
///////////////////////////////////////////////////////////////////////////////
template <typename IntegralResult, typename TokenT>
struct BOOST_WAVE_DECL chlit_grammar_gen {

    static IntegralResult evaluate(TokenT const &tok, value_error& status);
};

///////////////////////////////////////////////////////////////////////////////
}   //  namespace grammars
}   //  namespace wave
}   //  namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_LITERAL_GRAMMAR_GEN_HPP_67794A6C_468A_4AAB_A757_DEDDB182F5A0_INCLUDED)
