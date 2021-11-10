/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_MINIMAL_DEBUG_HPP)
#define BOOST_SPIRIT_MINIMAL_DEBUG_HPP

#if !defined(BOOST_SPIRIT_DEBUG_MAIN_HPP)
#error "You must include boost/spirit/debug.hpp, not boost/spirit/debug/minimal.hpp"
#endif
///////////////////////////////////////////////////////////////////////////////
//
//  Minimum debugging tools support
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_DEBUG_OUT)
#define BOOST_SPIRIT_DEBUG_OUT std::cout
#endif

///////////////////////////////////////////////////////////////////////////
//
//  BOOST_SPIRIT_DEBUG_FLAGS controls the level of diagnostics printed
//
///////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_DEBUG_FLAGS_NONE)
#define BOOST_SPIRIT_DEBUG_FLAGS_NONE         0x0000  // no diagnostics at all
#endif

#if !defined(BOOST_SPIRIT_DEBUG_FLAGS_MAX)
#define BOOST_SPIRIT_DEBUG_FLAGS_MAX          0xFFFF  // print maximal diagnostics
#endif

#if !defined(BOOST_SPIRIT_DEBUG_FLAGS)
#define BOOST_SPIRIT_DEBUG_FLAGS BOOST_SPIRIT_DEBUG_FLAGS_MAX
#endif

#if !defined(BOOST_SPIRIT_DEBUG_PRINT_SOME)
#define BOOST_SPIRIT_DEBUG_PRINT_SOME 20
#endif

#if !defined(BOOST_SPIRIT_DEBUG_RULE)
#define BOOST_SPIRIT_DEBUG_RULE(r)
#endif // !defined(BOOST_SPIRIT_DEBUG_RULE)

#if !defined(BOOST_SPIRIT_DEBUG_NODE)
#define BOOST_SPIRIT_DEBUG_NODE(r)
#endif // !defined(BOOST_SPIRIT_DEBUG_NODE)

#if !defined(BOOST_SPIRIT_DEBUG_GRAMMAR)
#define BOOST_SPIRIT_DEBUG_GRAMMAR(r)
#endif // !defined(BOOST_SPIRIT_DEBUG_GRAMMAR)

#if !defined(BOOST_SPIRIT_DEBUG_TRACE_RULE)
#define BOOST_SPIRIT_DEBUG_TRACE_RULE(r, t)
#endif // !defined(BOOST_SPIRIT_DEBUG_TRACE_RULE)

#if !defined(BOOST_SPIRIT_DEBUG_TRACE_NODE)
#define BOOST_SPIRIT_DEBUG_TRACE_NODE(r, t)
#endif // !defined(BOOST_SPIRIT_DEBUG_TRACE_NODE)

#if !defined(BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR)
#define BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR(r, t)
#endif // !defined(BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR)

#if !defined(BOOST_SPIRIT_DEBUG_TRACE_RULE_NAME)
#define BOOST_SPIRIT_DEBUG_TRACE_RULE_NAME(r, n, t)
#endif // !defined(BOOST_SPIRIT_DEBUG_TRACE_RULE_NAME)

#if !defined(BOOST_SPIRIT_DEBUG_TRACE_NODE_NAME)
#define BOOST_SPIRIT_DEBUG_TRACE_NODE_NAME(r, n, t)
#endif // !defined(BOOST_SPIRIT_DEBUG_TRACE_NODE_NAME)

#if !defined(BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR_NAME)
#define BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR_NAME(r, n, t)
#endif // !defined(BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR_NAME)

#endif  // !defined(BOOST_SPIRIT_MINIMAL_DEBUG_HPP)
