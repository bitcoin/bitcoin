/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_DIRECTIVE_LEXEME_HPP
#define BOOST_SPIRIT_QI_DIRECTIVE_LEXEME_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/detail/unused_skipper.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<qi::domain, tag::lexeme> // enables lexeme
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::lexeme;
#endif
    using spirit::lexeme_type;

    template <typename Subject>
    struct lexeme_directive : unary_parser<lexeme_directive<Subject> >
    {
        typedef Subject subject_type;
        lexeme_directive(Subject const& subject_)
          : subject(subject_) {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                traits::attribute_of<subject_type, Context, Iterator>::type
            type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        typename disable_if<detail::is_unused_skipper<Skipper>, bool>::type
        parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            qi::skip_over(first, last, skipper);
            return subject.parse(first, last, context
              , detail::unused_skipper<Skipper>(skipper), attr_);
        }
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        typename enable_if<detail::is_unused_skipper<Skipper>, bool>::type
        parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            //  no need to pre-skip if skipper is unused
            //- qi::skip_over(first, last, skipper);
            return subject.parse(first, last, context
              , skipper, attr_);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("lexeme", subject.what(context));

        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::lexeme, Subject, Modifiers>
    {
        typedef lexeme_directive<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject, unused_type) const
        {
            return result_type(subject);
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<qi::lexeme_directive<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
        , typename Iterator>
    struct handles_container<qi::lexeme_directive<Subject>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
