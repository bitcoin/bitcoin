//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_AUXILIARY_ATTR_CAST_HPP
#define BOOST_SPIRIT_KARMA_AUXILIARY_ATTR_CAST_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/auxiliary/attr_cast.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // enables attr_cast<>() pseudo generator
    template <typename Expr, typename Exposed, typename Transformed>
    struct use_terminal<karma::domain
          , tag::stateful_tag<Expr, tag::attr_cast, Exposed, Transformed> >
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace karma
{
    using spirit::attr_cast;

    ///////////////////////////////////////////////////////////////////////////
    // attr_cast_generator consumes the attribute of subject generator without
    // generating anything
    ///////////////////////////////////////////////////////////////////////////
    template <typename Exposed, typename Transformed, typename Subject>
    struct attr_cast_generator
      : unary_generator<attr_cast_generator<Exposed, Transformed, Subject> >
    {
        typedef typename result_of::compile<karma::domain, Subject>::type
            subject_type;

        typedef mpl::int_<subject_type::properties::value> properties;

        typedef typename mpl::eval_if<
            traits::not_is_unused<Transformed>
          , mpl::identity<Transformed>
          , traits::attribute_of<subject_type> >::type 
        transformed_attribute_type;

        attr_cast_generator(Subject const& subject)
          : subject(subject) 
        {
            // If you got an error_invalid_expression error message here,
            // then the expression (Subject) is not a valid spirit karma
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Subject);
        }

        // If Exposed is given, we use the given type, otherwise all we can do
        // is to guess, so we expose our inner type as an attribute and
        // deal with the passed attribute inside the parse function.
        template <typename Context, typename Unused>
        struct attribute
          : mpl::if_<traits::not_is_unused<Exposed>, Exposed
              , transformed_attribute_type>
        {};

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            typedef traits::transform_attribute<
                Attribute const, transformed_attribute_type, domain> 
            transform;

            return compile<karma::domain>(subject).generate(
                sink, ctx, d, transform::pre(attr));
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("attr_cast"
              , compile<karma::domain>(subject).what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generator: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Exposed, typename Transformed
      , typename Modifiers>
    struct make_primitive<
        tag::stateful_tag<Expr, tag::attr_cast, Exposed, Transformed>, Modifiers>
    {
        typedef attr_cast_generator<Exposed, Transformed, Expr> result_type;

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
