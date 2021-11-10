//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c)      2010 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DIRECTIVE_AS_HPP
#define BOOST_SPIRIT_KARMA_DIRECTIVE_AS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/detail/as.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>

namespace boost { namespace spirit { namespace karma
{
    template <typename T>
    struct as
      : stateful_tag_type<T, tag::as>
    {
        BOOST_SPIRIT_ASSERT_MSG(
            (traits::is_container<T>::type::value),
            error_type_must_be_a_container,
            (T));
    };
}}}

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    // enables as_string[...]
    template <>
    struct use_directive<karma::domain, tag::as_string> 
      : mpl::true_ {};

    // enables as_wstring[...]
    template <>
    struct use_directive<karma::domain, tag::as_wstring> 
      : mpl::true_ {};

    // enables as<T>[...]
    template <typename T>
    struct use_directive<karma::domain, tag::stateful_tag<T, tag::as> > 
      : mpl::true_ 
    {};
}}

namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::as_string;
    using spirit::as_wstring;
#endif
    using spirit::as_string_type;
    using spirit::as_wstring_type;

    ///////////////////////////////////////////////////////////////////////////
    // as_directive allows to hook custom conversions to string into the
    // output generation process
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename T>
    struct as_directive 
      : unary_generator<as_directive<Subject, T> >
    {
        typedef Subject subject_type;
        typedef typename subject_type::properties properties;

        as_directive(Subject const& subject)
          : subject(subject) {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef T type;
        };

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            if (!traits::valid_as<T>(attr))
                return false;

            return subject.generate(sink, ctx, d, traits::as<T>(attr)) &&
                    karma::delimit_out(sink, d); // always do post-delimiting
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("as", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::as_string, Subject, Modifiers>
    {
        typedef as_directive<Subject, std::string> result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };

    template <typename Subject, typename Modifiers>
    struct make_directive<tag::as_wstring, Subject, Modifiers>
    {
        typedef as_directive<Subject, std::basic_string<wchar_t> > result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };
    
    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<tag::stateful_tag<T, tag::as>, Subject, Modifiers>
    {
        typedef as_directive<Subject, T> result_type;
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
    template <typename Subject, typename T>
    struct has_semantic_action<karma::as_directive<Subject, T> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename T, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<karma::as_directive<Subject, T>, Attribute
      , Context, Iterator>
      : mpl::false_ {};   // always dereference attribute if used in sequences
}}}

#endif
