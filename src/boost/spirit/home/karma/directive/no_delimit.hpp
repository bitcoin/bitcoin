//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_NO_DELIMIT_JAN_19_2010_0920AM)
#define BOOST_SPIRIT_KARMA_NO_DELIMIT_JAN_19_2010_0920AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/detail/unused_delimiter.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::no_delimit>   // enables no_delimit[]
      : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::no_delimit;
#endif
    using spirit::no_delimit_type;

    ///////////////////////////////////////////////////////////////////////////
    //  The no_delimit generator is used for no_delimit[...] directives.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct no_delimit_generator 
      : unary_generator<no_delimit_generator<Subject> >
    {
        typedef Subject subject_type;
        typedef typename subject_type::properties properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        no_delimit_generator(Subject const& subject)
          : subject(subject) {}

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            //  the no_delimit generator simply dispatches to the embedded 
            //  generator while supplying unused_delimiter as the new delimiter
            //  to avoid delimiting down the generator stream
            typedef detail::unused_delimiter<Delimiter> unused_delimiter;

            // the difference to verbatim[] is that this does not post-delimit
            return subject.generate(sink, ctx, unused_delimiter(d), attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("no_delimit", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::no_delimit, Subject, Modifiers>
    {
        typedef no_delimit_generator<Subject> result_type;

        result_type 
        operator()(unused_type, Subject const& subject, unused_type) const
        {
            return result_type(subject);
        }
    };

}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<karma::no_delimit_generator<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
        , typename Iterator>
    struct handles_container<karma::no_delimit_generator<Subject>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
