/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Definition of the abstract lexer interface

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_LEX_INTERFACE_HPP_E83F52A4_90AC_4FBE_A9A7_B65F7F94C497_INCLUDED)
#define BOOST_CPP_LEX_INTERFACE_HPP_E83F52A4_90AC_4FBE_A9A7_B65F7F94C497_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/language_support.hpp>

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
namespace cpplexer {

///////////////////////////////////////////////////////////////////////////////
//
//  The lex_input_interface decouples the lex_iterator_shim from the actual
//  lexer. This is done to allow compile time reduction.
//  Thanks to JCAB for having this idea.
//
///////////////////////////////////////////////////////////////////////////////

template <typename TokenT>
struct lex_input_interface
{
    typedef typename TokenT::position_type position_type;

    lex_input_interface() {}
    virtual ~lex_input_interface() {}

    virtual TokenT& get(TokenT&) = 0;
    virtual void set_position(position_type const &pos) = 0;
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    virtual bool has_include_guards(std::string& guard_name) const = 0;
#endif
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_LEX_INTERFACE_HPP_E83F52A4_90AC_4FBE_A9A7_B65F7F94C497_INCLUDED)
