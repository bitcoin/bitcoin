//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c)      2010 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_STATE_SWITCHER_SEP_23_2007_0714PM)
#define BOOST_SPIRIT_LEX_STATE_SWITCHER_SEP_23_2007_0714PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/mpl/print.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables set_state(s)
    template <typename A0>
    struct use_terminal<qi::domain
      , terminal_ex<tag::set_state, fusion::vector1<A0> >
    > : traits::is_string<A0> {};

    // enables *lazy* set_state(s)
    template <>
    struct use_lazy_terminal<
        qi::domain, tag::set_state, 1
    > : mpl::true_ {};

    // enables in_state(s)[p]
    template <typename A0>
    struct use_directive<qi::domain
      , terminal_ex<tag::in_state, fusion::vector1<A0> >
    > : traits::is_string<A0> {};

    // enables *lazy* in_state(s)[p]
    template <>
    struct use_lazy_directive<
        qi::domain, tag::in_state, 1
    > : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::set_state;
    using spirit::in_state;
#endif
    using spirit::set_state_type;
    using spirit::in_state_type;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Iterator>
        inline std::size_t
        set_lexer_state(Iterator& it, std::size_t state)
        {
            return it.set_state(state);
        }

        template <typename Iterator, typename Char>
        inline std::size_t
        set_lexer_state(Iterator& it, Char const* statename)
        {
            std::size_t state = it.map_state(statename);

            //  If the following assertion fires you probably used the
            //  set_state(...) or in_state(...)[...] lexer state switcher with
            //  a lexer state name unknown to the lexer (no token definitions
            //  have been associated with this lexer state).
            BOOST_ASSERT(std::size_t(~0) != state);
            return it.set_state(state);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  Parser switching the state of the underlying lexer component.
    //  This parser gets used for the set_state(...) construct.
    ///////////////////////////////////////////////////////////////////////////
    template <typename State>
    struct state_switcher
      : primitive_parser<state_switcher<State> >
    {
        typedef typename 
            remove_const<typename traits::char_type_of<State>::type>::type 
        char_type;
        typedef std::basic_string<char_type> string_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef unused_type type;
        };

        state_switcher(char_type const* state)
          : state(state) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& /*attr*/) const
        {
            qi::skip_over(first, last, skipper);   // always do a pre-skip

            // just switch the state and return success
            detail::set_lexer_state(first, state.c_str());
            return true;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("set_state");
        }

        string_type state;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Iterator>
        struct reset_state_on_exit
        {
            template <typename State>
            reset_state_on_exit(Iterator& it_, State state_)
              : it(it_)
              , state(set_lexer_state(it_, traits::get_c_string(state_))) 
            {}

            ~reset_state_on_exit()
            {
                // reset the state of the underlying lexer instance
                set_lexer_state(it, state);
            }

            Iterator& it;
            std::size_t state;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(reset_state_on_exit& operator= (reset_state_on_exit const&))
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parser, which switches the state of the underlying lexer component
    // for the execution of the embedded sub-parser, switching the state back
    // afterwards. This parser gets used for the in_state(...)[p] construct.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename State>
    struct state_switcher_context 
      : unary_parser<state_switcher_context<Subject, State> >
    {
        typedef Subject subject_type;
        typedef typename traits::char_type_of<State>::type char_type;
        typedef typename remove_const<char_type>::type non_const_char_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                traits::attribute_of<subject_type, Context, Iterator>::type
            type;
        };

        state_switcher_context(Subject const& subject
              , typename add_reference<State>::type state)
          : subject(subject), state(state) {}

        // The following conversion constructors are needed to make the 
        // in_state_switcher template usable
        template <typename String>
        state_switcher_context(
                state_switcher_context<Subject, String> const& rhs)
          : subject(rhs.subject), state(traits::get_c_string(rhs.state)) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
        {
            qi::skip_over(first, last, skipper);   // always do a pre-skip

            // the state has to be reset at exit in any case
            detail::reset_state_on_exit<Iterator> guard(first, state);
            return subject.parse(first, last, context, skipper, attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("in_state", subject.what(context));
        }

        Subject subject;
        State state;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(state_switcher_context& operator= (state_switcher_context const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename State>
    struct make_primitive<terminal_ex<tag::set_state, fusion::vector1<State> >
      , Modifiers, typename enable_if<traits::is_string<State> >::type>
    {
        typedef typename add_const<State>::type const_string;
        typedef state_switcher<const_string> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(traits::get_c_string(fusion::at_c<0>(term.args)));
        }
    };

    template <typename State, typename Subject, typename Modifiers>
    struct make_directive<terminal_ex<tag::in_state, fusion::vector1<State> >
      , Subject, Modifiers>
    {
        typedef typename add_const<State>::type const_string;
        typedef state_switcher_context<Subject, const_string> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args));
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename State>
    struct has_semantic_action<qi::state_switcher_context<Subject, State> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename State, typename Attribute
        , typename Context, typename Iterator>
    struct handles_container<qi::state_switcher_context<Subject, State>
          , Attribute, Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {}; 
}}}

#endif
