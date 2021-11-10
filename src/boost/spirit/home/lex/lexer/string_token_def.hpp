//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_STRING_TOKEN_DEF_MAR_28_2007_0722PM)
#define BOOST_SPIRIT_LEX_STRING_TOKEN_DEF_MAR_28_2007_0722PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/lex/domain.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/spirit/home/lex/meta_compiler.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables strings
    template <typename T>
    struct use_terminal<lex::domain, T
      , typename enable_if<traits::is_string<T> >::type>
      : mpl::true_ {};

    // enables string(str)
    template <typename CharEncoding, typename A0>
    struct use_terminal<lex::domain
      , terminal_ex<
            tag::char_code<tag::string, CharEncoding>
          , fusion::vector1<A0> > > 
      : traits::is_string<A0> {};

    // enables string(str, ID)
    template <typename CharEncoding, typename A0, typename A1>
    struct use_terminal<lex::domain
      , terminal_ex<
            tag::char_code<tag::string, CharEncoding>
          , fusion::vector2<A0, A1> > > 
      : traits::is_string<A0> {};
}}

namespace boost { namespace spirit { namespace lex
{ 
    // use string from standard character set by default
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::standard::string;
#endif
    using spirit::standard::string_type;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  string_token_def 
    //      represents a string based token definition
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename String, typename IdType = std::size_t
      , typename CharEncoding = char_encoding::standard>
    struct string_token_def
      : primitive_lexer<string_token_def<String, IdType, CharEncoding> >
    {
        typedef typename
            remove_const<typename traits::char_type_of<String>::type>::type
        char_type;
        typedef std::basic_string<char_type> string_type;

        string_token_def(typename add_reference<String>::type str, IdType const& id)
          : str_(str), id_(id), unique_id_(std::size_t(~0))
          , token_state_(std::size_t(~0)) 
        {}

        template <typename LexerDef, typename String_>
        void collect(LexerDef& lexdef, String_ const& state
          , String_ const& targetstate) const
        {
            std::size_t state_id = lexdef.add_state(state.c_str());

            // If the following assertion fires you are probably trying to use 
            // a single string_token_def instance in more than one lexer state. 
            // This is not possible. Please create a separate token_def instance 
            // from the same regular expression for each lexer state it needs 
            // to be associated with.
            BOOST_ASSERT(
                (std::size_t(~0) == token_state_ || state_id == token_state_) &&
                "Can't use single string_token_def with more than one lexer state");

            char_type const* target = targetstate.empty() ? 0 : targetstate.c_str();
            if (target)
                lexdef.add_state(target);

            token_state_ = state_id;

            if (IdType(~0) == id_)
                id_ = IdType(lexdef.get_next_id());

            unique_id_ = lexdef.add_token (state.c_str(), str_, id_, target);
        }

        template <typename LexerDef>
        void add_actions(LexerDef&) const {}

        std::size_t id() const { return id_; }
        std::size_t unique_id() const { return unique_id_; }
        std::size_t state() const { return token_state_; }

        string_type str_;
        mutable IdType id_;
        mutable std::size_t unique_id_;
        mutable std::size_t token_state_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Lex generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Modifiers>
    struct make_primitive<T, Modifiers
      , typename enable_if<traits::is_string<T> >::type>
    {
        typedef typename add_const<T>::type const_string;
        typedef string_token_def<const_string> result_type;

        result_type operator()(
            typename add_reference<const_string>::type str, unused_type) const
        {
            return result_type(str, std::size_t(~0));
        }
    };

    template <typename Modifiers, typename CharEncoding, typename A0>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::string, CharEncoding>
          , fusion::vector1<A0> >
      , Modifiers>
    {
        typedef typename add_const<A0>::type const_string;
        typedef string_token_def<const_string, std::size_t, CharEncoding> 
            result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args), std::size_t(~0));
        }
    };

    template <typename Modifiers, typename CharEncoding, typename A0, typename A1>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::string, CharEncoding>
          , fusion::vector2<A0, A1> >
      , Modifiers>
    {
        typedef typename add_const<A0>::type const_string;
        typedef string_token_def<const_string, A1, CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args), fusion::at_c<1>(term.args));
        }
    };
}}}  // namespace boost::spirit::lex

#endif 
