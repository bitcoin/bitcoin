/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_ACTION_ACTION_HPP
#define BOOST_SPIRIT_QI_ACTION_ACTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/argument.hpp>
#include <boost/spirit/home/support/context.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/action_dispatch.hpp>
#include <boost/spirit/home/support/handles_container.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace qi
{
    BOOST_PP_REPEAT(SPIRIT_ARGUMENTS_LIMIT, SPIRIT_USING_ARGUMENT, _)

    template <typename Subject, typename Action>
    struct action : unary_parser<action<Subject, Action> >
    {
        typedef Subject subject_type;
        typedef Action action_type;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<Subject, Context, Iterator>
        {};

        action(Subject const& subject_, Action f_)
          : subject(subject_), f(f_) {}

#ifndef BOOST_SPIRIT_ACTIONS_ALLOW_ATTR_COMPAT
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            typedef typename attribute<Context, Iterator>::type attr_type;

            // create an attribute if one is not supplied
            typedef traits::transform_attribute<
                Attribute, attr_type, domain> transform;

            typename transform::type attr = transform::pre(attr_);

            Iterator save = first;
            if (subject.parse(first, last, context, skipper, attr))
            {
                // call the function, passing the attribute, the context.
                // The client can return false to fail parsing.
                if (traits::action_dispatch<Subject>()(f, attr, context)) 
                {
                    // Do up-stream transformation, this integrates the results
                    // back into the original attribute value, if appropriate.
                    transform::post(attr_, attr);
                    return true;
                }

                // reset iterators if semantic action failed the match
                // retrospectively
                first = save;
            }
            return false;
        }
#else
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
        {
            Iterator save = first;
            if (subject.parse(first, last, context, skipper, attr)) // Use the attribute as-is
            {
                // call the function, passing the attribute, the context.
                // The client can return false to fail parsing.
                if (traits::action_dispatch<Subject>()(f, attr, context))
                    return true;

                // reset iterators if semantic action failed the match
                // retrospectively
                first = save;
            }
            return false;
        }

        template <typename Iterator, typename Context
          , typename Skipper>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , unused_type) const
        {
            typedef typename attribute<Context, Iterator>::type attr_type;

            // synthesize the attribute since one is not supplied
            attr_type attr = attr_type();

            Iterator save = first;
            if (subject.parse(first, last, context, skipper, attr))
            {
                // call the function, passing the attribute, the context.
                // The client can return false to fail parsing.
                if (traits::action_dispatch<Subject>()(f, attr, context))
                    return true;

                // reset iterators if semantic action failed the match
                // retrospectively
                first = save;
            }
            return false;
        }
#endif

        template <typename Context>
        info what(Context& context) const
        {
            // the action is transparent (does not add any info)
            return subject.what(context);
        }

        Subject subject;
        Action f;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(action& operator= (action const&))
    };
}}}

namespace boost { namespace spirit
{
    // Qi action meta-compiler
    template <>
    struct make_component<qi::domain, tag::action>
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

            typedef qi::action<subject_type, action_type> type;
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
    struct has_semantic_action<qi::action<Subject, Action> >
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Action, typename Attribute
        , typename Context, typename Iterator>
    struct handles_container<qi::action<Subject, Action>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
