//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_DELIMIT_MAR_02_2007_0217PM)
#define BOOST_SPIRIT_KARMA_DELIMIT_MAR_02_2007_0217PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/detail/unused_delimiter.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::delimit>   // enables delimit[]
      : mpl::true_ {};

    // enables delimit(d)[g], where d is a generator
    template <typename T>
    struct use_directive<karma::domain
          , terminal_ex<tag::delimit, fusion::vector1<T> > > 
      : boost::spirit::traits::matches<karma::domain, T> {};

    // enables *lazy* delimit(d)[g]
    template <>
    struct use_lazy_directive<karma::domain, tag::delimit, 1> 
      : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::delimit;
#endif
    using spirit::delimit_type;

    ///////////////////////////////////////////////////////////////////////////
    //  The redelimit_generator generator is used for delimit[...] directives.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct redelimit_generator : unary_generator<redelimit_generator<Subject> >
    {
        typedef Subject subject_type;

        typedef typename subject_type::properties properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        redelimit_generator(Subject const& subject)
          : subject(subject) {}

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            //  The delimit_space generator simply dispatches to the embedded
            //  generator while supplying either the delimiter which has been 
            //  used before a surrounding verbatim[] directive or a single 
            //  space as the new delimiter to use (if no surrounding verbatim[]
            //  was specified).
            return subject.generate(sink, ctx
              , detail::get_delimiter(d, compile<karma::domain>(' ')), attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("delimit", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The delimit_generator is used for delimit(d)[...] directives.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Delimiter>
    struct delimit_generator 
      : unary_generator<delimit_generator<Subject, Delimiter> >
    {
        typedef Subject subject_type;
        typedef Delimiter delimiter_type;

        typedef typename subject_type::properties properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        delimit_generator(Subject const& subject, Delimiter const& delimiter)
          : subject(subject), delimiter(delimiter) {}

        template <typename OutputIterator, typename Context
          , typename Delimiter_, typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter_ const&
          , Attribute const& attr) const
        {
            //  the delimit generator simply dispatches to the embedded
            //  generator while supplying it's argument as the new delimiter
            //  to use
            return subject.generate(sink, ctx, delimiter, attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("delimit", subject.what(context));
        }

        Subject subject;
        Delimiter delimiter;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::delimit, Subject, Modifiers>
    {
        typedef redelimit_generator<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };

    template <typename Delimiter, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::delimit, fusion::vector1<Delimiter> >
      , Subject, Modifiers>
    {
        typedef typename
            result_of::compile<karma::domain, Delimiter, Modifiers>::type
        delimiter_type;

        typedef delimit_generator<Subject, delimiter_type> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
            return result_type(subject
              , compile<karma::domain>(fusion::at_c<0>(term.args)));
        }
    };

}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<karma::redelimit_generator<Subject> >
      : unary_has_semantic_action<Subject> {};

    template <typename Subject, typename Delimiter>
    struct has_semantic_action<karma::delimit_generator<Subject, Delimiter> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute
            , typename Context, typename Iterator>
    struct handles_container<karma::redelimit_generator<Subject>, Attribute
      , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};

    template <typename Subject, typename Delimiter, typename Attribute
            , typename Context, typename Iterator>
    struct handles_container<karma::delimit_generator<Subject, Delimiter>
      , Attribute, Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
