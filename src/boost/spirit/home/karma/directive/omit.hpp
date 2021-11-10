//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DIRECTIVE_OMIT_HPP
#define BOOST_SPIRIT_KARMA_DIRECTIVE_OMIT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::omit> // enables omit
      : mpl::true_ {};

    template <>
    struct use_directive<karma::domain, tag::skip> // enables skip
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::omit;
    using spirit::skip;
#endif
    using spirit::omit_type;
    using spirit::skip_type;

    ///////////////////////////////////////////////////////////////////////////
    // omit_directive consumes the attribute of subject generator without
    // generating anything
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, bool Execute>
    struct omit_directive : unary_generator<omit_directive<Subject, Execute> >
    {
        typedef Subject subject_type;

        typedef mpl::int_<
            generator_properties::disabling | subject_type::properties::value
        > properties;

        omit_directive(Subject const& subject)
          : subject(subject) {}

        template <typename Context, typename Iterator = unused_type>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            // We need to actually compile the output operation as we don't 
            // have any other means to verify, whether the passed attribute is 
            // compatible with the subject. 

#if defined(_MSC_VER) && _MSC_VER < 1900
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
#endif
            // omit[] will execute the code, while skip[] doesn't execute it
            if (Execute) {
#if defined(_MSC_VER) && _MSC_VER < 1900
# pragma warning(pop)
#endif
                // wrap the given output iterator to avoid output
                detail::disable_output<OutputIterator> disable(sink);
                return subject.generate(sink, ctx, d, attr);
            }
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info(Execute ? "omit" : "skip", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::omit, Subject, Modifiers>
    {
        typedef omit_directive<Subject, true> result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };

    template <typename Subject, typename Modifiers>
    struct make_directive<tag::skip, Subject, Modifiers>
    {
        typedef omit_directive<Subject, false> result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, bool Execute>
    struct has_semantic_action<karma::omit_directive<Subject, Execute> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, bool Execute, typename Attribute
        , typename Context, typename Iterator>
    struct handles_container<karma::omit_directive<Subject, Execute>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
