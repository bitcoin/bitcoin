//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_LEX_LEXER_ACTION_HPP
#define BOOST_SPIRIT_LEX_LEXER_ACTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/lex/meta_compiler.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/spirit/home/lex/argument.hpp>
#include <boost/spirit/home/lex/lexer/support_functions.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_same.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Action>
    struct action : unary_lexer<action<Subject, Action> >
    {
        action(Subject const& subject, Action f)
          : subject(subject), f(f) {}

        template <typename LexerDef, typename String>
        void collect(LexerDef& lexdef, String const& state
          , String const& targetstate) const
        {
            // collect the token definition information for the token_def 
            // this action is attached to
            subject.collect(lexdef, state, targetstate);
        }

        template <typename LexerDef>
        void add_actions(LexerDef& lexdef) const
        {
            // call to add all actions attached further down the hierarchy 
            subject.add_actions(lexdef);

            // retrieve the id of the associated token_def and register the 
            // given semantic action with the lexer instance
            lexdef.add_action(subject.unique_id(), subject.state(), f);
        }

        Subject subject;
        Action f;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(action& operator= (action const&))
    };

}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Karma action meta-compiler
    template <>
    struct make_component<lex::domain, tag::action>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename
                remove_const<typename Elements::car_type>::type
            subject_type;

            typedef typename
                remove_const<typename Elements::cdr_type::car_type>::type
            action_type;

            typedef lex::action<subject_type, action_type> type;
        };

        template <typename Elements>
        typename result<make_component(Elements, unused_type)>::type
        operator()(Elements const& elements, unused_type) const
        {
            typename result<make_component(Elements, unused_type)>::type
                result(elements.car, elements.cdr.car);
            return result;
        }
    };
}}

#endif
