//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2003 Vaclav Vesely
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_REPOSITORY_QI_DIRECTIVE_DISTINCT_HPP
#define BOOST_SPIRIT_REPOSITORY_QI_DIRECTIVE_DISTINCT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/detail/unused_skipper.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/make_component.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/qi/auxiliary/eps.hpp>
#include <boost/spirit/home/qi/auxiliary/lazy.hpp>
#include <boost/spirit/home/qi/directive/lexeme.hpp>
#include <boost/spirit/home/qi/operator/not_predicate.hpp>

#include <boost/spirit/repository/home/support/distinct.hpp>

#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit 
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables distinct(...)[...]
    template <typename Tail>
    struct use_directive<qi::domain
          , terminal_ex<repository::tag::distinct, fusion::vector1<Tail> > >
      : mpl::true_ {};

    // enables *lazy* distinct(...)[...]
    template <>
    struct use_lazy_directive<qi::domain, repository::tag::distinct, 1> 
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace repository {namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using repository::distinct;
#endif
    using repository::distinct_type;

    template <typename Subject, typename Tail, typename Modifier>
    struct distinct_parser
      : spirit::qi::unary_parser<distinct_parser<Subject, Tail, Modifier> >
    {
        template <typename Context, typename Iterator>
        struct attribute 
          : traits::attribute_of<Subject, Context, Iterator>
        {};

        distinct_parser(Subject const& subject, Tail const& tail)
          : subject(subject), tail(tail) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, Attribute& attr) const
        {
            Iterator iter = first;

            spirit::qi::skip_over(iter, last, skipper);
            if (!subject.parse(iter, last, context
              , spirit::qi::detail::unused_skipper<Skipper>(skipper), attr))
                return false;

            Iterator i = iter;
            if (tail.parse(i, last, context, unused, unused))
                return false;

            first = iter;
            return true;
        }

        template <typename Context>
        info what(Context& ctx) const
        {
            return info("distinct", subject.what(ctx));
        }

        Subject subject;
        Tail tail;
    };

}}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Tail, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::distinct, fusion::vector1<Tail> >
      , Subject, Modifiers>
    {
        typedef typename result_of::compile<qi::domain, Tail, Modifiers>::type
            tail_type;

        typedef repository::qi::distinct_parser<
            Subject, tail_type, Modifiers> result_type;

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
    template <typename Subject, typename Tail, typename Modifier>
    struct has_semantic_action<
            repository::qi::distinct_parser<Subject, Tail, Modifier> >
      : unary_has_semantic_action<Subject> {};
}}}

#endif

