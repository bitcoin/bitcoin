//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_ACTION_MAR_07_2007_0851AM)
#define BOOST_SPIRIT_KARMA_ACTION_MAR_07_2007_0851AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/argument.hpp>
#include <boost/spirit/home/support/context.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/action_dispatch.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    BOOST_PP_REPEAT(SPIRIT_ARGUMENTS_LIMIT, SPIRIT_USING_ARGUMENT, _)

    template <typename Subject, typename Action>
    struct action : unary_generator<action<Subject, Action> >
    {
        typedef Subject subject_type;
        typedef typename subject_type::properties properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<Subject, Context, Iterator>
        {};

        action(Subject const& subject, Action f)
          : subject(subject), f(f) {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr_) const
        {
            typedef typename attribute<Context, unused_type>::type attr_type;

            // create a attribute if none is supplied
            // this creates a _copy_ of the attribute because the semantic
            // action will likely change parts of this
            typedef traits::transform_attribute<
                Attribute const, attr_type, domain> transform;

            attr_type attr = transform::pre(attr_);

            // call the function, passing the attribute, the context and a bool 
            // flag that the client can set to false to fail generating.
            return traits::action_dispatch<Subject>()(f, attr, ctx) && 
                   subject.generate(sink, ctx, d, attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            // the action is transparent (does not add any info)
            return subject.what(context);
        }

        subject_type subject;
        Action f;
    };

}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Karma action meta-compiler
    template <>
    struct make_component<karma::domain, tag::action>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename
                remove_const<typename Elements::car_type>::type
            subject_type;

            typedef typename
                remove_const<typename Elements::cdr_type::car_type>::type
            action_type;

            typedef karma::action<subject_type, action_type> type;
        };

        template <typename Elements>
        typename result<make_component(Elements, unused_type)>::type
        operator()(Elements const& elements, unused_type) const
        {
            typename result<make_component(Elements, unused_type)>::type
                result(elements.car, elements.cdr.car);
            return result;
        }
    };
}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Action>
    struct has_semantic_action<karma::action<Subject, Action> >
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Action, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<karma::action<Subject, Action>, Attribute
      , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
