//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_LAZY_MARCH_27_2007_1231PM)
#define BOOST_SPIRIT_KARMA_LAZY_MARCH_27_2007_1231PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/lazy.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/proto/make_expr.hpp>
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
    struct use_terminal<karma::domain, phoenix::actor<Eval> >  // enables phoenix actors
        : mpl::true_ {};

    // forward declaration
    template <typename Terminal, typename Actor, int Arity>
    struct lazy_terminal;

}}

namespace boost { namespace spirit { namespace karma
{
    using spirit::lazy;
    typedef modify<karma::domain> karma_modify;

    namespace detail
    {
        template <typename Generator, typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool lazy_generate_impl(Generator const& g, OutputIterator& sink
          , Context& context, Delimiter const& delim
          , Attribute const& attr, mpl::false_)
        {
            return g.generate(sink, context, delim, attr);
        }

        template <typename Generator, typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool lazy_generate_impl(Generator const& g, OutputIterator& sink
          , Context& context, Delimiter const& delim
          , Attribute const& /* attr */, mpl::true_)
        {
            // If DeducedAuto is false (semantic actions is present), the
            // component's attribute is unused.
            return g.generate(sink, context, delim, unused);
        }

        template <typename Generator, typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool lazy_generate_impl_main(Generator const& g, OutputIterator& sink
          , Context& context, Delimiter const& delim, Attribute const& attr)
        {
            // If DeducedAuto is true (no semantic action), we pass the parser's
            // attribute on to the component.
            typedef typename traits::has_semantic_action<Generator>::type auto_rule;
            return lazy_generate_impl(g, sink, context, delim, attr, auto_rule());
        }
    }

    template <typename Function, typename Modifiers>
    struct lazy_generator : generator<lazy_generator<Function, Modifiers> >
    {
        typedef mpl::int_<generator_properties::all_properties> properties;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                boost::result_of<karma_modify(tag::lazy_eval, Modifiers)>::type
            modifier;

            typedef typename
                remove_reference<
                    typename boost::result_of<Function(unused_type, Context)>::type
                >::type
            expr_type;

            // If you got an error_invalid_expression error message here,
            // then the expression (expr_type) is not a valid spirit karma
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(karma::domain, expr_type);

            typedef typename
                result_of::compile<karma::domain, expr_type, modifier>::type
            generator_type;

            typedef typename
                traits::attribute_of<generator_type, Context, Iterator>::type
            type;
        };

        lazy_generator(Function const& func, Modifiers const& modifiers)
          : func(func), modifiers(modifiers) {}

        template <
            typename OutputIterator, typename Context, 
            typename Delimiter, typename Attribute
        >
        bool generate(OutputIterator& sink, Context& context, 
            Delimiter const& d, Attribute const& attr) const
        {
            return detail::lazy_generate_impl_main(
                compile<karma::domain>(func(unused, context)
              , karma_modify()(tag::lazy_eval(), modifiers))
              , sink, context, d, attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("lazy"
              , compile<karma::domain>(func(unused, context)
                , karma_modify()(tag::lazy_eval(), modifiers))
                    .what(context)
            );
        }

        Function func;
        Modifiers modifiers;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(lazy_generator& operator= (lazy_generator const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Function, typename Subject, typename Modifiers>
    struct lazy_directive 
      : unary_generator<lazy_directive<Function, Subject, Modifiers> >
    {
        typedef mpl::int_<generator_properties::all_properties> properties;

        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                boost::result_of<karma_modify(tag::lazy_eval, Modifiers)>::type
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
            // then the expression (expr_type) is not a valid spirit karma
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(karma::domain, expr_type);

            typedef typename
                result_of::compile<karma::domain, expr_type, modifier>::type
            generator_type;

            typedef typename
                traits::attribute_of<generator_type, Context, Iterator>::type
            type;
        };

        lazy_directive(Function const& function, Subject const& subject
              , Modifiers const& modifiers)
          : function(function), subject(subject), modifiers(modifiers) {}

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            return detail::lazy_generate_impl_main(compile<karma::domain>(
                proto::make_expr<proto::tag::subscript>(
                    function(unused, ctx), subject)
                  , karma_modify()(tag::lazy_eval(), modifiers))
                  , sink, ctx, d, attr);
        }

        template <typename Context>
        info what(Context& ctx) const
        {
            return info("lazy-directive"
              , compile<karma::domain>(
                    proto::make_expr<proto::tag::subscript>(
                        function(unused, ctx), subject)
                      , karma_modify()(tag::lazy_eval(), modifiers))
                    .what(ctx)
            );
        }

        Function function;
        Subject subject;
        Modifiers modifiers;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Eval, typename Modifiers>
    struct make_primitive<phoenix::actor<Eval>, Modifiers>
    {
        typedef lazy_generator<phoenix::actor<Eval>, Modifiers> result_type;
        result_type operator()(phoenix::actor<Eval> const& f
          , Modifiers const& modifiers) const
        {
            return result_type(f, modifiers);
        }
    };

    template <typename Terminal, typename Actor, int Arity, typename Modifiers>
    struct make_primitive<lazy_terminal<Terminal, Actor, Arity>, Modifiers>
    {
        typedef lazy_generator<Actor, Modifiers> result_type;
        result_type operator()(
            lazy_terminal<Terminal, Actor, Arity> const& lt
          , Modifiers const& modifiers) const
        {
            return result_type(lt.actor, modifiers);
        }
    };

    template <
        typename Terminal, typename Actor, int Arity, typename Subject
      , typename Modifiers>
    struct make_directive<lazy_terminal<Terminal, Actor, Arity>
      , Subject, Modifiers>
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

#endif
