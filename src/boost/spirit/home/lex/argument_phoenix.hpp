//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c)      2011 Thomas Heller
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_ARGUMENT_PHEONIX_MARCH_25_2011_1841PM)
#define BOOST_SPIRIT_LEX_ARGUMENT_PHEONIX_MARCH_25_2011_1841PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/as_actor.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/v2_eval.hpp>
#include <boost/phoenix/core/value.hpp> // includes as_actor specialization
#include <boost/proto/traits.hpp>
#include <boost/proto/proto_fwd.hpp> // for transform placeholders

namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    //  The value_context is used as a noop Phoenix actor to create the 
    //  placeholder '_val' (see below). It is a noop actor because it is used
    //  as a placeholder only, while it is being converted either to a 
    //  value_getter (if used as a rvalue) or to a value_setter (if used as a 
    //  lvalue). The conversion is achieved by specializing and overloading a 
    //  couple of the Phoenix templates from the Phoenix expression composition
    //  engine (see the end of this file).
    struct value_context 
    {
        typedef mpl::true_ no_nullary;

        typedef unused_type result_type;

        template <typename Env>
        struct result
        {
            typedef unused_type type;
        };

        template <typename Env>
        unused_type
        eval(Env const&) const
        {
            return unused;
        }
    };

    // forward declarations
    struct value_getter;
    template <typename> struct value_setter;

    ///////////////////////////////////////////////////////////////////////////
    //  The state_context is used as a noop Phoenix actor to create the 
    //  placeholder '_state' (see below). It is a noop actor because it is used
    //  as a placeholder only, while it is being converted either to a 
    //  state_getter (if used as a rvalue) or to a state_setter (if used as a 
    //  lvalue). The conversion is achieved by specializing and overloading a 
    //  couple of the Phoenix templates from the Phoenix expression composition
    //  engine (see the end of this file).
    struct state_context 
    {
        typedef mpl::true_ no_nullary;

        typedef unused_type result_type;

        template <typename Env>
        struct result
        {
            typedef unused_type type;
        };

        template <typename Env>
        unused_type
        eval(Env const&) const
        {
            return unused;
        }
    };

    // forward declarations
    struct state_getter;
    template <typename> struct state_setter;
    struct eoi_getter;
}}}

///////////////////////////////////////////////////////////////////////////////

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(spirit)(lex)(value_setter)
  , (boost::phoenix::meta_grammar)
)

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(spirit)(lex)(state_setter)
  , (boost::phoenix::meta_grammar)
)

namespace boost { namespace phoenix
{
    namespace result_of
    {
        template <>
        struct is_nullary<custom_terminal<boost::spirit::lex::value_context> >
          : mpl::false_
        {};
    }
    
    template <typename Dummy>
    struct is_custom_terminal<boost::spirit::lex::value_context, Dummy>: mpl::true_ {};
    
    template <typename Dummy>
    struct custom_terminal<boost::spirit::lex::value_context, Dummy>
      : proto::call<
            v2_eval(
                proto::make<boost::spirit::lex::value_getter()>
              , proto::call<functional::env(proto::_state)>
            )
        >
    {};

    template <typename Dummy>
    struct is_nullary::when<spirit::lex::rule::value_setter, Dummy>
      : proto::make<mpl::false_()>
    {};

    template <typename Dummy>
    struct default_actions::when<spirit::lex::rule::value_setter, Dummy>
      : proto::call<
            v2_eval(
                proto::make<
                    spirit::lex::value_setter<proto::_child0>(
                        proto::_child0
                    )
                >
              , _env
            )
        >
    {};

    template <>
    struct actor<spirit::lex::value_context>
      : boost::phoenix::actor<proto::terminal<spirit::lex::value_context>::type>
    {
        typedef boost::phoenix::actor<
            proto::terminal<spirit::lex::value_context>::type
        > base_type;

        actor(base_type const & base = base_type())
            : base_type(base)
        {}

        template <typename Expr>
        typename spirit::lex::expression::value_setter<
            typename phoenix::as_actor<Expr>::type>::type const
        operator=(Expr const & expr) const
        {
            return
                spirit::lex::expression::value_setter<
                    typename phoenix::as_actor<Expr>::type
                >::make(phoenix::as_actor<Expr>::convert(expr));
        }
    };
    
    namespace result_of
    {
        template <>
        struct is_nullary<custom_terminal<boost::spirit::lex::state_context> >
          : mpl::false_
        {};
    }
    
    template <typename Dummy>
    struct is_custom_terminal<boost::spirit::lex::state_context, Dummy>: mpl::true_ {};
    
    template <typename Dummy>
    struct custom_terminal<boost::spirit::lex::state_context, Dummy>
      : proto::call<
            v2_eval(
                proto::make<boost::spirit::lex::state_getter()>
              , proto::call<functional::env(proto::_state)>
            )
        >
    {};

    template <typename Dummy>
    struct is_nullary::when<spirit::lex::rule::state_setter, Dummy>
      : proto::make<mpl::false_()>
    {};

    template <typename Dummy>
    struct default_actions::when<spirit::lex::rule::state_setter, Dummy>
      : proto::call<
            v2_eval(
                proto::make<
                    spirit::lex::state_setter<proto::_child0>(
                        proto::_child0
                    )
                >
              , _env
            )
        >
    {};

    template <>
    struct actor<spirit::lex::state_context>
      : boost::phoenix::actor<proto::terminal<spirit::lex::state_context>::type>
    {
        typedef boost::phoenix::actor<
            proto::terminal<spirit::lex::state_context>::type
        > base_type;

        actor(base_type const & base = base_type())
          : base_type(base)
        {}

        template <typename Expr>
        typename spirit::lex::expression::state_setter<
            typename phoenix::as_actor<Expr>::type>::type const
        operator=(Expr const & expr) const
        {
            return
                spirit::lex::expression::state_setter<
                    typename phoenix::as_actor<Expr>::type
                >::make(phoenix::as_actor<Expr>::convert(expr));
        }
    };
    
    namespace result_of
    {
        template <>
        struct is_nullary<custom_terminal<boost::spirit::lex::eoi_getter> >
          : mpl::false_
        {};
    }
    
    template <typename Dummy>
    struct is_custom_terminal<boost::spirit::lex::eoi_getter, Dummy>: mpl::true_ {};
    
    template <typename Dummy>
    struct custom_terminal<boost::spirit::lex::eoi_getter, Dummy>
      : proto::call<
            v2_eval(
                proto::make<boost::spirit::lex::eoi_getter()>
              , proto::call<functional::env(proto::_state)>
            )
        >
    {};
}}

#endif
