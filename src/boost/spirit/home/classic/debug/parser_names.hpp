/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_NAMES_HPP)
#define BOOST_SPIRIT_PARSER_NAMES_HPP

#if defined(BOOST_SPIRIT_DEBUG)

//////////////////////////////////
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  Declaration of helper functions, which return the name of a concrete
//  parser instance. The functions are specialized on the parser types. The
//  functions declared in this file are for the predefined parser types from
//  the Spirit core library only, so additional functions might be provided as
//  needed.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  from actions.hpp
    template <typename ParserT, typename ActionT>
    std::string
    parser_name(action<ParserT, ActionT> const& p);

///////////////////////////////////////////////////////////////////////////////
//  from directives.hpp
    template <typename ParserT>
    std::string
    parser_name(contiguous<ParserT> const& p);

    template <typename ParserT>
    std::string
    parser_name(inhibit_case<ParserT> const& p);

    template <typename A, typename B>
    std::string
    parser_name(longest_alternative<A, B> const& p);

    template <typename A, typename B>
    std::string
    parser_name(shortest_alternative<A, B> const& p);

///////////////////////////////////////////////////////////////////////////////
//  from grammar.hpp
    template <typename DerivedT, typename ContextT>
    std::string
    parser_name(grammar<DerivedT, ContextT> const& p);

///////////////////////////////////////////////////////////////////////////////
//  from numerics.hpp
    template <typename T, int Radix, unsigned MinDigits, int MaxDigits>
    std::string
    parser_name(uint_parser<T, Radix, MinDigits, MaxDigits> const& p);

    template <typename T, int Radix, unsigned MinDigits, int MaxDigits>
    std::string
    parser_name(int_parser<T, Radix, MinDigits, MaxDigits> const& p);

    template <typename T, typename RealPoliciesT>
    std::string
    parser_name(real_parser<T, RealPoliciesT> const& p);

///////////////////////////////////////////////////////////////////////////////
//  from operators.hpp
    template <typename A, typename B>
    std::string
    parser_name(sequence<A, B> const& p);

    template <typename A, typename B>
    std::string
    parser_name(sequential_or<A, B> const& p);

    template <typename A, typename B>
    std::string
    parser_name(alternative<A, B> const& p);

    template <typename A, typename B>
    std::string
    parser_name(intersection<A, B> const& p);

    template <typename A, typename B>
    std::string
    parser_name(difference<A, B> const& p);

    template <typename A, typename B>
    std::string
    parser_name(exclusive_or<A, B> const& p);

    template <typename S>
    std::string
    parser_name(optional<S> const& p);

    template <typename S>
    std::string
    parser_name(kleene_star<S> const& p);

    template <typename S>
    std::string
    parser_name(positive<S> const& p);

///////////////////////////////////////////////////////////////////////////////
//  from parser.hpp
    template <typename DerivedT>
    std::string
    parser_name(parser<DerivedT> const& p);

///////////////////////////////////////////////////////////////////////////////
//  from primitives.hpp
    template <typename DerivedT>
    std::string
    parser_name(char_parser<DerivedT> const &p);

    template <typename CharT>
    std::string
    parser_name(chlit<CharT> const &p);

    template <typename CharT>
    std::string
    parser_name(range<CharT> const &p);

    template <typename IteratorT>
    std::string
    parser_name(chseq<IteratorT> const &p);

    template <typename IteratorT>
    std::string
    parser_name(strlit<IteratorT> const &p);

    std::string
    parser_name(nothing_parser const &p);

    std::string
    parser_name(epsilon_parser const &p);

    std::string
    parser_name(anychar_parser const &p);

    std::string
    parser_name(alnum_parser const &p);

    std::string
    parser_name(alpha_parser const &p);

    std::string
    parser_name(cntrl_parser const &p);

    std::string
    parser_name(digit_parser const &p);

    std::string
    parser_name(graph_parser const &p);

    std::string
    parser_name(lower_parser const &p);

    std::string
    parser_name(print_parser const &p);

    std::string
    parser_name(punct_parser const &p);

    std::string
    parser_name(blank_parser const &p);

    std::string
    parser_name(space_parser const &p);

    std::string
    parser_name(upper_parser const &p);

    std::string
    parser_name(xdigit_parser const &p);

    std::string
    parser_name(eol_parser const &p);

    std::string
    parser_name(end_parser const &p);

///////////////////////////////////////////////////////////////////////////////
//  from rule.hpp
    template<typename T0, typename T1, typename T2>
    std::string
    parser_name(rule<T0, T1, T2> const& p);

///////////////////////////////////////////////////////////////////////////////
//  from subrule.hpp
    template <typename FirstT, typename RestT>
    std::string
    parser_name(subrule_list<FirstT, RestT> const &p);

    template <int ID, typename DefT, typename ContextT>
    std::string
    parser_name(subrule_parser<ID, DefT, ContextT> const &p);

    template <int ID, typename ContextT>
    std::string
    parser_name(subrule<ID, ContextT> const &p);

///////////////////////////////////////////////////////////////////////////////
//  from chset.hpp

///////////////////////////////////////////////////////////////////////////////
//
//  Decide, if a node is to be traced or not
//
///////////////////////////////////////////////////////////////////////////////
    template<
        typename DerivedT, typename EmbedT, 
        typename T0, typename T1, typename T2
    >
    bool
    trace_parser(impl::rule_base<DerivedT, EmbedT, T0, T1, T2> 
        const& p);

    template <typename DerivedT, typename ContextT>
    bool
    trace_parser(grammar<DerivedT, ContextT> const& p);

    template <int ID, typename ContextT>
    bool
    trace_parser(subrule<ID, ContextT> const& p);

    template <typename ParserT, typename ActorTupleT>
    struct init_closure_parser;

    template <typename ParserT, typename ActorTupleT>
    bool
    trace_parser(init_closure_parser<ParserT, ActorTupleT> const& p);

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

//////////////////////////////////
#include <boost/spirit/home/classic/debug/impl/parser_names.ipp>

#endif // defined(BOOST_SPIRIT_DEBUG)

#endif // !defined(BOOST_SPIRIT_PARSER_NAMES_HPP)
