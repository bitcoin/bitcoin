//  Copyright (c) 2009 Chris Hoeppler
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_REPOSITORY_QI_CONFIX_JUN_22_2009_1041AM)
#define BOOST_SPIRIT_REPOSITORY_QI_CONFIX_JUN_22_2009_1041AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/auxiliary/lazy.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>

#include <boost/spirit/repository/home/support/confix.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/mpl/or.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit 
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables confix(..., ...)[]
    template <typename Prefix, typename Suffix>
    struct use_directive<qi::domain
          , terminal_ex<repository::tag::confix, fusion::vector2<Prefix, Suffix> > >
      : mpl::true_ {};

    // enables *lazy* confix(..., ...)[]
    template <>
    struct use_lazy_directive<qi::domain, repository::tag::confix, 2> 
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace repository { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using repository::confix;
#endif
    using repository::confix_type;

    ///////////////////////////////////////////////////////////////////////////
    // the confix() generated parser
    template <typename Subject, typename Prefix, typename Suffix>
    struct confix_parser
      : spirit::qi::unary_parser<confix_parser<Subject, Prefix, Suffix> >
    {
        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        confix_parser(Subject const& subject, Prefix const& prefix
              , Suffix const& suffix)
          : subject(subject), prefix(prefix), suffix(suffix) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
        {
            Iterator iter = first;

            if (!(prefix.parse(iter, last, context, skipper, unused) &&
                subject.parse(iter, last, context, skipper, attr) &&
                suffix.parse(iter, last, context, skipper, unused)))
            {
                return false;
            }

            first = iter;
            return true;
        }

        template <typename Context>
        info what(Context const& ctx) const
        {
            return info("confix", subject.what(ctx));
        }

        Subject subject;
        Prefix prefix;
        Suffix suffix;
    };

}}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    // creates confix(..., ...)[] directive
    template <typename Prefix, typename Suffix, typename Subject
      , typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::confix, fusion::vector2<Prefix, Suffix> >
      , Subject, Modifiers>
    {
        typedef typename
            result_of::compile<qi::domain, Prefix, Modifiers>::type
        prefix_type;
        typedef typename
            result_of::compile<qi::domain, Suffix, Modifiers>::type
        suffix_type;

        typedef repository::qi::confix_parser<
            Subject, prefix_type, suffix_type> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , Modifiers const& modifiers) const
        {
            return result_type(subject
              , compile<qi::domain>(fusion::at_c<0>(term.args), modifiers)
              , compile<qi::domain>(fusion::at_c<1>(term.args), modifiers));
        }
    };

}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Subject, typename Prefix, typename Suffix>
    struct has_semantic_action<
            repository::qi::confix_parser<Subject, Prefix, Suffix> >
      : mpl::or_<
            has_semantic_action<Subject>
          , has_semantic_action<Prefix>
          , has_semantic_action<Suffix> 
        > {};
}}}

#endif

