/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_LAZY_MARCH_27_2007_1002AM)
#define BOOST_SPIRIT_LAZY_MARCH_27_2007_1002AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/lazy.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/proto/make_expr.hpp>
#include <boost/proto/tags.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/mpl/not.hpp>

namespace boost { namespace phoenix
{
    template <typename Expr>
    struct actor;
}}

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <typename Eval>
    struct use_terminal<qi::domain, phoenix::actor<Eval> >  // enables phoenix actors
        : mpl::true_ {};

    // forward declaration
    template <typename Terminal, typename Actor, int Arity>
    struct lazy_terminal;
}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::lazy;
    typedef modify<qi::domain> qi_modify;

    namespace detail
    {
        template <typename Parser, typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool lazy_parse_impl(Parser const& p
          , Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr, mpl::false_)
        {
            return p.parse(first, last, context, skipper, attr);
        }

        template <typename Parser, typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool lazy_parse_impl(Parser const& p
          , Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& /*attr*/, mpl::true_)
        {
            // If DeducedAuto is false (semantic actions is present), the
            // component's attribute is unused.
            return p.parse(first, last, context, skipper, unused);
        }

        template <typename Parser, typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool lazy_parse_impl_main(Parser const& p
          , Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr)
        {
            // If DeducedAuto is true (no semantic action), we pass the parser's
            // attribute on to the component.
            typedef typename traits::has_semantic_action<Parser>::type auto_rule;
            return lazy_parse_impl(p, first, last, context, skipper, attr, auto_rule());
        }
    }

    template <typename Function, typename Modifiers>
    struct lazy_parser : parser<lazy_parser<Function, Modifiers> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                boost::result_of<qi_modify(tag::lazy_eval, Modifiers)>::type
            modifier;

            typedef typename
                remove_reference<
                    typename boost::result_of<Function(unused_type, Context)>::type
                >::type
            expr_type;

            // If you got an error_invalid_expression error message here,
            // then the expression (expr_type) is not a valid spirit qi
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(qi::domain, expr_type);

            typedef typename
                result_of::compile<qi::domain, expr_type, modifier>::type
            parser_type;

            typedef typename
                traits::attribute_of<parser_type, Context, Iterator>::type
            type;
        };

        lazy_parser(Function const& function_, Modifiers const& modifiers_)
          : function(function_), modifiers(modifiers_) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
        {
            return detail::lazy_parse_impl_main(
                  compile<qi::domain>(function(unused, context)
                , qi_modify()(tag::lazy_eval(), modifiers))
                , first, last, context, skipper, attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("lazy"
              , compile<qi::domain>(function(unused, context)
                , qi_modify()(tag::lazy_eval(), modifiers))
                    .what(context)
            );
        }

        Function function;
        Modifiers modifiers;
    };


    template <typename Function, typename Subject, typename Modifiers>
    struct lazy_directive
        : unary_parser<lazy_directive<Function, Subject, Modifiers> >
    {
        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                boost::result_of<qi_modify(tag::lazy_eval, Modifiers)>::type
            modifier;

            typedef typename
                remove_reference<
                    typename boost::result_of<Function(unused_type, Context)>::type
                >::type
            directive_expr_type;

            typedef typename
                proto::result_of::make_expr<
                    proto::tag::subscript
                  , directive_expr_type
                  , Subject
                >::type
            expr_type;

            // If you got an error_invalid_expression error message here,
            // then the expression (expr_type) is not a valid spirit qi
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(qi::domain, expr_type);

            typedef typename
                result_of::compile<qi::domain, expr_type, modifier>::type
            parser_type;

            typedef typename
                traits::attribute_of<parser_type, Context, Iterator>::type
            type;
        };

        lazy_directive(
            Function const& function_
          , Subject const& subject_
          , Modifiers const& modifiers_)
          : function(function_), subject(subject_), modifiers(modifiers_) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
        {
            return detail::lazy_parse_impl_main(compile<qi::domain>(
                proto::make_expr<proto::tag::subscript>(
                    function(unused, context)
                  , subject)
                , qi_modify()(tag::lazy_eval(), modifiers))
                , first, last, context, skipper, attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("lazy-directive"
              , compile<qi::domain>(
                    proto::make_expr<proto::tag::subscript>(
                        function(unused, context)
                      , subject
                    ), qi_modify()(tag::lazy_eval(), modifiers))
                    .what(context)
            );
        }

        Function function;
        Subject subject;
        Modifiers modifiers;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Eval, typename Modifiers>
    struct make_primitive<phoenix::actor<Eval>, Modifiers>
    {
        typedef lazy_parser<phoenix::actor<Eval>, Modifiers> result_type;
        result_type operator()(phoenix::actor<Eval> const& f
          , Modifiers const& modifiers) const
        {
            return result_type(f, modifiers);
        }
    };

    template <typename Terminal, typename Actor, int Arity, typename Modifiers>
    struct make_primitive<lazy_terminal<Terminal, Actor, Arity>, Modifiers>
    {
        typedef lazy_parser<Actor, Modifiers> result_type;
        result_type operator()(
            lazy_terminal<Terminal, Actor, Arity> const& lt
          , Modifiers const& modifiers) const
        {
            return result_type(lt.actor, modifiers);
        }
    };

    template <typename Terminal, typename Actor, int Arity, typename Subject, typename Modifiers>
    struct make_directive<lazy_terminal<Terminal, Actor, Arity>, Subject, Modifiers>
    {
        typedef lazy_directive<Actor, Subject, Modifiers> result_type;
        result_type operator()(
            lazy_terminal<Terminal, Actor, Arity> const& lt
          , Subject const& subject, Modifiers const& modifiers) const
        {
            return result_type(lt.actor, subject, modifiers);
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Actor, typename Modifiers, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<
        qi::lazy_parser<Actor, Modifiers>, Attribute, Context, Iterator>
      : handles_container<
          typename qi::lazy_parser<Actor, Modifiers>::template
              attribute<Context, Iterator>::parser_type
        , Attribute, Context, Iterator>
    {};

    template <typename Subject, typename Actor, typename Modifiers
      , typename Attribute, typename Context, typename Iterator>
    struct handles_container<
        qi::lazy_directive<Actor, Subject, Modifiers>, Attribute
      , Context, Iterator>
      : handles_container<
          typename qi::lazy_directive<Actor, Subject, Modifiers>::template
              attribute<Context, Iterator>::parser_type
        , Attribute, Context, Iterator>
    {};
}}}

#endif
