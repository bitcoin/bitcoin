//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_REFERENCE_APR_20_2009_0827AM)
#define BOOST_SPIRIT_LEX_REFERENCE_APR_20_2009_0827AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/lex/meta_compiler.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/spirit/home/qi/reference.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/ref.hpp>

namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    // reference is a lexer that references another lexer (its Subject)
    // all lexer components are at the same time
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename IdType = unused_type>
    struct reference; 

    template <typename Subject>
    struct reference<Subject, unused_type>
      : qi::reference<Subject>
      , lexer_type<reference<Subject> >
    {
        reference(Subject& subject)
          : qi::reference<Subject>(subject) {}

        template <typename LexerDef, typename String>
        void collect(LexerDef& lexdef, String const& state
          , String const& targetstate) const
        {
            this->ref.get().collect(lexdef, state, targetstate);
        }

        template <typename LexerDef>
        void add_actions(LexerDef& lexdef) const 
        {
            this->ref.get().add_actions(lexdef);
        }
    };

    template <typename Subject, typename IdType>
    struct reference : reference<Subject>
    {
        reference(Subject& subject)
          : reference<Subject>(subject) {}

        IdType id() const 
        { 
            return this->ref.get().id(); 
        }
        std::size_t unique_id() const 
        { 
            return this->ref.get().unique_id(); 
        }
        std::size_t state() const 
        { 
            return this->ref.get().state(); 
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename IdType
      , typename Attribute, typename Context, typename Iterator>
    struct handles_container<lex::reference<Subject, IdType>
          , Attribute, Context, Iterator>
      : handles_container<
            typename remove_const<Subject>::type, Attribute, Context, Iterator> 
    {};
}}}

#endif
