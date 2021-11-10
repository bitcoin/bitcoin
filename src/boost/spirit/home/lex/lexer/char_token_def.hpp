//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_CHAR_TOKEN_DEF_MAR_28_2007_0626PM)
#define BOOST_SPIRIT_LEX_CHAR_TOKEN_DEF_MAR_28_2007_0626PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/lex/domain.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/spirit/home/lex/meta_compiler.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables 'x'
    template <>
    struct use_terminal<lex::domain, char>
      : mpl::true_ {};

    // enables "x"
    template <>
    struct use_terminal<lex::domain, char[2]>
      : mpl::true_ {};

    // enables wchar_t
    template <>
    struct use_terminal<lex::domain, wchar_t>
      : mpl::true_ {};

    // enables L"x"
    template <>
    struct use_terminal<lex::domain, wchar_t[2]>
      : mpl::true_ {};

    // enables char_('x'), char_("x")
    template <typename CharEncoding, typename A0>
    struct use_terminal<lex::domain
      , terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<A0> > > 
      : mpl::true_ {};

    // enables char_('x', ID), char_("x", ID)
    template <typename CharEncoding, typename A0, typename A1>
    struct use_terminal<lex::domain
      , terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector2<A0, A1> > > 
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace lex
{ 
    // use char_ from standard character set by default
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::standard::char_;
#endif
    using spirit::standard::char_type;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  char_token_def 
    //      represents a single character token definition
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding = char_encoding::standard
      , typename IdType = std::size_t>
    struct char_token_def
      : primitive_lexer<char_token_def<CharEncoding, IdType> >
    {
        typedef typename CharEncoding::char_type char_type;

        char_token_def(char_type ch, IdType const& id) 
          : ch(ch), id_(id), unique_id_(std::size_t(~0))
          , token_state_(std::size_t(~0)) 
        {}

        template <typename LexerDef, typename String>
        void collect(LexerDef& lexdef, String const& state
          , String const& targetstate) const
        {
            std::size_t state_id = lexdef.add_state(state.c_str());

            // If the following assertion fires you are probably trying to use 
            // a single char_token_def instance in more than one lexer state. 
            // This is not possible. Please create a separate token_def instance 
            // from the same regular expression for each lexer state it needs 
            // to be associated with.
            BOOST_ASSERT(
                (std::size_t(~0) == token_state_ || state_id == token_state_) &&
                "Can't use single char_token_def with more than one lexer state");

            char_type const* target = targetstate.empty() ? 0 : targetstate.c_str();
            if (target)
                lexdef.add_state(target);

            token_state_ = state_id;
            unique_id_ = lexdef.add_token (state.c_str(), ch, id_, target);
        }

        template <typename LexerDef>
        void add_actions(LexerDef&) const {}

        IdType id() const { return id_; }
        std::size_t unique_id() const { return unique_id_; }
        std::size_t state() const { return token_state_; }

        char_type ch;
        mutable IdType id_;
        mutable std::size_t unique_id_;
        mutable std::size_t token_state_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Lexer generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename CharEncoding>
        struct basic_literal
        {
            typedef char_token_def<CharEncoding> result_type;

            template <typename Char>
            result_type operator()(Char ch, unused_type) const
            {
                return result_type(ch, ch);
            }

            template <typename Char>
            result_type operator()(Char const* str, unused_type) const
            {
                return result_type(str[0], str[0]);
            }
        };
    }

    // literals: 'x', "x"
    template <typename Modifiers>
    struct make_primitive<char, Modifiers>
      : detail::basic_literal<char_encoding::standard> {};

    template <typename Modifiers>
    struct make_primitive<char const(&)[2], Modifiers>
      : detail::basic_literal<char_encoding::standard> {};

    // literals: L'x', L"x"
    template <typename Modifiers>
    struct make_primitive<wchar_t, Modifiers>
      : detail::basic_literal<char_encoding::standard_wide> {};

    template <typename Modifiers>
    struct make_primitive<wchar_t const(&)[2], Modifiers>
      : detail::basic_literal<char_encoding::standard_wide> {};

    // handle char_('x')
    template <typename CharEncoding, typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<A0>
        >
      , Modifiers>
    {
        typedef char_token_def<CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args), fusion::at_c<0>(term.args));
        }
    };

    // handle char_("x")
    template <typename CharEncoding, typename Modifiers, typename Char>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<Char(&)[2]>   // single char strings
        >
      , Modifiers>
    {
        typedef char_token_def<CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            Char ch = fusion::at_c<0>(term.args)[0];
            return result_type(ch, ch);
        }
    };

    // handle char_('x', ID)
    template <typename CharEncoding, typename Modifiers, typename A0, typename A1>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector2<A0, A1>
        >
      , Modifiers>
    {
        typedef char_token_def<CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args), fusion::at_c<1>(term.args));
        }
    };

    // handle char_("x", ID)
    template <typename CharEncoding, typename Modifiers, typename Char, typename A1>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector2<Char(&)[2], A1>   // single char strings
        >
      , Modifiers>
    {
        typedef char_token_def<CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args)[0], fusion::at_c<1>(term.args));
        }
    };
}}}  // namespace boost::spirit::lex

#endif 
