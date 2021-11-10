/*=============================================================================
    A Standard compliant C++ preprocessor

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_PREDEF_MACROS_GEN_HPP_CADB6D2C_76A4_4988_83E1_EFFC6902B9A2_INCLUDED)
#define BOOST_CPP_PREDEF_MACROS_GEN_HPP_CADB6D2C_76A4_4988_83E1_EFFC6902B9A2_INCLUDED

#include <boost/spirit/include/classic_parse_tree.hpp>

#include <boost/wave/wave_config.hpp>

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
//  Here are the node id's of the different node of the cpp_grammar
//
///////////////////////////////////////////////////////////////////////////////
#define BOOST_WAVE_PLAIN_DEFINE_ID      5
#define BOOST_WAVE_MACRO_PARAMETERS_ID  6
#define BOOST_WAVE_MACRO_DEFINITION_ID  7

///////////////////////////////////////////////////////////////////////////////
//
//  predefined_macros_grammar_gen template class
//
//      This template helps separating the compilation of the
//      predefined_macros_grammar class from the compilation of the
//      main pp_iterator. This is done to safe compilation time.
//
//      This class helps parsing command line given macro definitions in a
//      similar way, as macros are parsed by the cpp_grammar class.
//
///////////////////////////////////////////////////////////////////////////////

template <typename LexIteratorT>
struct BOOST_WAVE_DECL predefined_macros_grammar_gen
{
    typedef LexIteratorT iterator_type;

    //  parse the cpp_grammar and return the resulting parse tree
    static boost::spirit::classic::tree_parse_info<iterator_type>
    parse_predefined_macro (iterator_type const &first, iterator_type const &last);
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace grammars
}   // namespace wave
}   // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_PREDEF_MACROS_GEN_HPP_CADB6D2C_76A4_4988_83E1_EFFC6902B9A2_INCLUDED)
