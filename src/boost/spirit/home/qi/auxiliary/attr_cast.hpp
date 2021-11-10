//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_QI_AUXILIARY_ATTR_CAST_HPP
#define BOOST_SPIRIT_QI_AUXILIARY_ATTR_CAST_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/auxiliary/attr_cast.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables attr_cast<>() pseudo parser
    template <typename Expr, typename Exposed, typename Transformed>
    struct use_terminal<qi::domain
          , tag::stateful_tag<Expr, tag::attr_cast, Exposed, Transformed> >
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::attr_cast;

    ///////////////////////////////////////////////////////////////////////////
    // attr_cast_parser consumes the attribute of subject generator without
    // generating anything
    ///////////////////////////////////////////////////////////////////////////
    template <typename Exposed, typename Transformed, typename Subject>
    struct attr_cast_parser 
      : unary_parser<attr_cast_parser<Exposed, Transformed, Subject> >
    {
        typedef typename result_of::compile<qi::domain, Subject>::type
            subject_type;

        typedef typename mpl::eval_if<
            traits::not_is_unused<Transformed>
          , mpl::identity<Transformed>
          , traits::attribute_of<subject_type> >::type
        transformed_attribute_type;

        attr_cast_parser(Subject const& subject_)
          : subject(subject_)
        {
            // If you got an error_invalid_expression error message here,
            // then the expression (Subject) is not a valid spirit qi
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Subject);
        }

        // If Exposed is given, we use the given type, otherwise all we can do
        // is to guess, so we expose our inner type as an attribute and
        // deal with the passed attribute inside the parse function.
        template <typename Context, typename Iterator>
        struct attribute
          : mpl::if_<traits::not_is_unused<Exposed>, Exposed
              , transformed_attribute_type>
        {};

        template <typename Iterator, typename Context, typename Skipper
          , typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_param) const
        {
            // Find the real exposed attribute. If exposed is given, we use it
            // otherwise we assume the exposed attribute type to be the actual
            // attribute type as passed by the user.
            typedef typename mpl::if_<
                traits::not_is_unused<Exposed>, Exposed, Attribute>::type
            exposed_attribute_type;

            // do down-stream transformation, provides attribute for embedded
            // parser
            typedef traits::transform_attribute<
                exposed_attribute_type, transformed_attribute_type, domain>
            transform;

            typename transform::type attr_ = transform::pre(attr_param);

            if (!compile<qi::domain>(subject).
                    parse(first, last, context, skipper, attr_))
            {
                transform::fail(attr_param);
                return false;
            }

            // do up-stream transformation, this mainly integrates the results
            // back into the original attribute value, if appropriate
            transform::post(attr_param, attr_);
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("attr_cast"
              , compile<qi::domain>(subject).what(context));
        }

        Subject subject;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(attr_cast_parser& operator= (attr_cast_parser const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generator: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Exposed, typename Transformed
      , typename Modifiers>
    struct make_primitive<
        tag::stateful_tag<Expr, tag::attr_cast, Exposed, Transformed>, Modifiers>
    {
        typedef attr_cast_parser<Exposed, Transformed, Expr> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            typedef tag::stateful_tag<
                Expr, tag::attr_cast, Exposed, Transformed> tag_type;
            using spirit::detail::get_stateful_data;
            return result_type(get_stateful_data<tag_type>::call(term));
        }
    };


}}}

#endif
