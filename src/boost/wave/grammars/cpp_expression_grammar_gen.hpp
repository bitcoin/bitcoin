/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_EXPRESSION_GRAMMAR_GEN_HPP_42399258_6CDC_4101_863D_5C7D95B5A6CA_INCLUDED)
#define BOOST_CPP_EXPRESSION_GRAMMAR_GEN_HPP_42399258_6CDC_4101_863D_5C7D95B5A6CA_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <boost/wave/cpp_iteration_context.hpp>
#include <boost/wave/grammars/cpp_value_error.hpp>

#include <list>
#include <boost/pool/pool_alloc.hpp>

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
//  expression_grammar_gen template class
//
//      This template helps separating the compilation of the
//      expression_grammar class from the compilation of the main
//      pp_iterator. This is done to safe compilation time.
//
///////////////////////////////////////////////////////////////////////////////

template <typename TokenT>
struct BOOST_WAVE_DECL expression_grammar_gen {

    typedef TokenT token_type;
    typedef std::list<token_type, boost::fast_pool_allocator<token_type> >
        token_sequence_type;

    static bool evaluate(
        typename token_sequence_type::const_iterator const &first,
        typename token_sequence_type::const_iterator const &last,
        typename token_type::position_type const &tok,
        bool if_block_status, value_error &status);
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

#endif // !defined(BOOST_CPP_EXPRESSION_GRAMMAR_GEN_HPP_42399258_6CDC_4101_863D_5C7D95B5A6CA_INCLUDED)
