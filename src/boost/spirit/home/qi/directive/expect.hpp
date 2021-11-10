/*=============================================================================
Copyright (c) 2016 Frank Hein, maxence business consulting gmbh

Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_QI_DIRECTIVE_EXPECT_HPP
#define BOOST_SPIRIT_QI_DIRECTIVE_EXPECT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/qi/detail/expectation_failure.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>

namespace boost { namespace spirit {
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<qi::domain, tag::expect> // enables expect[p]
        : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi {

#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::expect;
#endif
    using spirit::expect_type;

    template <typename Subject>
    struct expect_directive : unary_parser<expect_directive<Subject> >
    {
        typedef result_of::compile<domain, Subject> subject_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef traits::attribute_of<subject_type, Context, Iterator>
                type;
        };

        expect_directive(Subject const& subject_) : subject(subject_) {}

        template <typename Iterator, typename Context
            , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
            , Context& context, Skipper const& skipper
            , Attribute& attr_) const
        {
            typedef expectation_failure<Iterator> exception;

            if (!subject.parse(first, last, context, skipper, attr_))
            {
                boost::throw_exception(
                    exception(first, last, subject.what(context)));
#if defined(BOOST_NO_EXCEPTIONS)
                return false;            // for systems not supporting exceptions
#endif
            }
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("expect", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::expect, Subject, Modifiers>
    {
        typedef expect_directive<Subject> result_type;
        
        result_type operator()
            (unused_type, Subject const& subject, unused_type) const
        {
            return result_type(subject);
        }
    };

}}}

namespace boost { namespace spirit { namespace traits {
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<qi::expect_directive<Subject> >
        : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute
        , typename Context, typename Iterator>
    struct handles_container<
        qi::expect_directive<Subject>, Attribute, Context, Iterator
    >
        : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
