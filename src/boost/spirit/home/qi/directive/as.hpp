/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2010 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_DIRECTIVE_AS_HPP
#define BOOST_SPIRIT_QI_DIRECTIVE_AS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/support/container.hpp>

namespace boost { namespace spirit { namespace qi
{
    template <typename T>
    struct as
      : stateful_tag_type<T, tag::as>
    {
        //~ BOOST_SPIRIT_ASSERT_MSG(
            //~ (traits::is_container<T>::type::value),
            //~ error_type_must_be_a_container,
            //~ (T));
    };
}}}

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    // enables as_string[...]
    template <>
    struct use_directive<qi::domain, tag::as_string>
      : mpl::true_ {};

    // enables as_wstring[...]
    template <>
    struct use_directive<qi::domain, tag::as_wstring>
      : mpl::true_ {};

    // enables as<T>[...]
    template <typename T>
    struct use_directive<qi::domain, tag::stateful_tag<T, tag::as> >
      : mpl::true_
    {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::as_string;
    using spirit::as_wstring;
#endif
    using spirit::as_string_type;
    using spirit::as_wstring_type;

    template <typename Subject, typename T>
    struct as_directive : unary_parser<as_directive<Subject, T> >
    {
        typedef Subject subject_type;
        as_directive(Subject const& subject_)
          : subject(subject_) {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef T type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, Attribute& attr_) const
        {
            Iterator i = first;
            T as_attr;
            if (subject.parse(i, last, context, skipper, as_attr))
            {
                spirit::traits::assign_to(as_attr, attr_);
                first = i;
                return true;
            }
            return false;
        }

        template <typename Iterator, typename Context, typename Skipper>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, T& attr_) const
        {
            Iterator i = first;
            if (subject.parse(i, last, context, skipper, attr_))
            {
                first = i;
                return true;
            }
            return false;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("as", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
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
    struct has_semantic_action<qi::as_directive<Subject, T> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename T, typename Attribute
        , typename Context, typename Iterator>
    struct handles_container<qi::as_directive<Subject, T>, Attribute
        , Context, Iterator>
      : mpl::false_ {};
}}}

#endif
