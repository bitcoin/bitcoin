/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_DIRECTIVE_SKIP_HPP
#define BOOST_SPIRIT_QI_DIRECTIVE_SKIP_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/auxiliary/lazy.hpp>
#include <boost/spirit/home/qi/operator/kleene.hpp>
#include <boost/spirit/home/qi/directive/lexeme.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/detail/unused_skipper.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<qi::domain, tag::skip>     // enables skip[p]
      : mpl::true_ {};

    template <typename T>
    struct use_directive<qi::domain
      , terminal_ex<tag::skip                       // enables skip(s)[p]
        , fusion::vector1<T> >
    > : boost::spirit::traits::matches<qi::domain, T> {};

    template <>                                     // enables *lazy* skip(s)[p]
    struct use_lazy_directive<
        qi::domain
      , tag::skip
      , 1 // arity
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::skip;
#endif
    using spirit::skip_type;

    template <typename Subject>
    struct reskip_parser : unary_parser<reskip_parser<Subject> >
    {
        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                traits::attribute_of<Subject, Context, Iterator>::type
            type;
        };

        reskip_parser(Subject const& subject_)
          : subject(subject_) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        typename enable_if<detail::is_unused_skipper<Skipper>, bool>::type
        parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& u // --> The skipper is reintroduced
          , Attribute& attr_) const
        {
            return subject.parse(first, last, context
              , detail::get_skipper(u), attr_);
        }
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        typename disable_if<detail::is_unused_skipper<Skipper>, bool>::type
        parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            return subject.parse(first, last, context
              , skipper, attr_);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("skip", subject.what(context));
        }

        Subject subject;
    };

    template <typename Subject, typename Skipper>
    struct skip_parser : unary_parser<skip_parser<Subject, Skipper> >
    {
        typedef Subject subject_type;
        typedef Skipper skipper_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                traits::attribute_of<Subject, Context, Iterator>::type
            type;
        };

        skip_parser(Subject const& subject_, Skipper const& skipper_)
          : subject(subject_), skipper(skipper_) {}

        template <typename Iterator, typename Context
          , typename Skipper_, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper_ const& //skipper --> bypass the supplied skipper
          , Attribute& attr_) const
        {
            return subject.parse(first, last, context, skipper, attr_);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("skip", subject.what(context));
        }

        Subject subject;
        Skipper skipper;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::skip, Subject, Modifiers>
    {
        typedef reskip_parser<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject, unused_type) const
        {
            return result_type(subject);
        }
    };

    template <typename Skipper, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::skip, fusion::vector1<Skipper> >, Subject, Modifiers>
    {
        typedef typename
            result_of::compile<qi::domain, Skipper, Modifiers>::type
        skipper_type;

        typedef skip_parser<Subject, skipper_type> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , Modifiers const& modifiers) const
        {
            return result_type(subject
              , compile<qi::domain>(fusion::at_c<0>(term.args), modifiers));
        }
    };

}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<qi::reskip_parser<Subject> >
      : unary_has_semantic_action<Subject> {};

    template <typename Subject, typename Skipper>
    struct has_semantic_action<qi::skip_parser<Subject, Skipper> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
        , typename Iterator>
    struct handles_container<qi::reskip_parser<Subject>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};

    template <typename Subject, typename Skipper, typename Attribute
        , typename Context, typename Iterator>
    struct handles_container<qi::skip_parser<Subject, Skipper>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
