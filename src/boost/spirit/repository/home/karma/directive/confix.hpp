//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_REPOSITORY_KARMA_CONFIX_AUG_19_2008_1041AM)
#define BOOST_SPIRIT_REPOSITORY_KARMA_CONFIX_AUG_19_2008_1041AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>

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
    struct use_directive<karma::domain
          , terminal_ex<repository::tag::confix, fusion::vector2<Prefix, Suffix> > >
      : mpl::true_ {};

    // enables *lazy* confix(..., ...)[g]
    template <>
    struct use_lazy_directive<karma::domain, repository::tag::confix, 2> 
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace repository { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using repository::confix;
#endif
    using repository::confix_type;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Prefix, typename Suffix>
    struct confix_generator
      : spirit::karma::primitive_generator<confix_generator<Subject, Prefix, Suffix> >
    {
        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        confix_generator(Subject const& subject, Prefix const& prefix
              , Suffix const& suffix)
          : subject(subject), prefix(prefix), suffix(suffix) {}

        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            // generate the prefix, the embedded item and the suffix
            return prefix.generate(sink, ctx, d, unused) &&
                   subject.generate(sink, ctx, d, attr) &&
                   suffix.generate(sink, ctx, d, unused);
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
namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    // creates confix(..., ...)[] directive generator
    template <typename Prefix, typename Suffix, typename Subject
      , typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::confix, fusion::vector2<Prefix, Suffix> >
      , Subject, Modifiers>
    {
        typedef typename
            result_of::compile<karma::domain, Prefix, Modifiers>::type
        prefix_type;
        typedef typename
            result_of::compile<karma::domain, Suffix, Modifiers>::type
        suffix_type;

        typedef repository::karma::confix_generator<
            Subject, prefix_type, suffix_type> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , Modifiers const& modifiers) const
        {
            return result_type(subject
              , compile<karma::domain>(fusion::at_c<0>(term.args), modifiers)
              , compile<karma::domain>(fusion::at_c<1>(term.args), modifiers));
        }
    };

}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Subject, typename Prefix, typename Suffix>
    struct has_semantic_action<
            repository::karma::confix_generator<Subject, Prefix, Suffix> >
      : mpl::or_<
            has_semantic_action<Subject>
          , has_semantic_action<Prefix>
          , has_semantic_action<Suffix> 
        > {};
}}}

#endif
