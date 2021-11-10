/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_OPERATOR_KLEENE_HPP
#define BOOST_SPIRIT_QI_OPERATOR_KLEENE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/qi/detail/fail_function.hpp>
#include <boost/spirit/home/qi/detail/pass_container.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    //[composite_parsers_kleene_enable_
    template <>
    struct use_operator<qi::domain, proto::tag::dereference> // enables *p
      : mpl::true_ {};
    //]
}}

namespace boost { namespace spirit { namespace qi
{
    //[composite_parsers_kleene
    template <typename Subject>
    struct kleene : unary_parser<kleene<Subject> >
    {
        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            // Build a std::vector from the subject's attribute. Note
            // that build_std_vector may return unused_type if the
            // subject's attribute is an unused_type.
            typedef typename
                traits::build_std_vector<
                    typename traits::
                        attribute_of<Subject, Context, Iterator>::type
                >::type
            type;
        };

        kleene(Subject const& subject_)
          : subject(subject_) {}

        template <typename F>
        bool parse_container(F f) const
        {
            while (!f (subject))
                ;
            return true;
        }

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            // ensure the attribute is actually a container type
            traits::make_container(attr_);

            typedef detail::fail_function<Iterator, Context, Skipper>
                fail_function;

            Iterator iter = first;
            fail_function f(iter, last, context, skipper);
            parse_container(detail::make_pass_container(f, attr_));

            first = f.first;
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("kleene", subject.what(context));
        }

        Subject subject;
    };
    //]

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    //[composite_parsers_kleene_generator
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::dereference, Elements, Modifiers>
      : make_unary_composite<Elements, kleene>
    {};
    //]

//     ///////////////////////////////////////////////////////////////////////////
//     // Define what attributes are compatible with a kleene
//     template <typename Attribute, typename Subject, typename Context, typename Iterator>
//     struct is_attribute_compatible<Attribute, kleene<Subject>, Context, Iterator>
//       : traits::is_container_compatible<qi::domain, Attribute
//               , kleene<Subject>, Context, Iterator>
//     {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<qi::kleene<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<qi::kleene<Subject>, Attribute
          , Context, Iterator>
      : mpl::true_ {}; 
}}}

#endif
