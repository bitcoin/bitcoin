/*=============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler
    Copyright (c) 2010 Thomas Heller
    Copyright (c) 2014 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_ACTOR_HPP
#define BOOST_PHOENIX_CORE_ACTOR_HPP

#include <boost/phoenix/core/limits.hpp>

#include <boost/is_placeholder.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/phoenix/core/domain.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/support/iterate.hpp>
#include <boost/phoenix/support/vector.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/make_expr.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/mpl/void.hpp>
#include <cstring>
#ifndef BOOST_PHOENIX_NO_VARIADIC_ACTOR
#   include <boost/mpl/if.hpp>
#   include <boost/type_traits/is_reference.hpp>
#   include <boost/phoenix/core/detail/index_sequence.hpp>
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4522) // 'this' used in base member initializer list
#pragma warning(disable: 4510) // default constructor could not be generated
#pragma warning(disable: 4610) // can never be instantiated - user defined cons
#endif

namespace boost { namespace phoenix
{
    template <typename Expr>
    struct actor;

    namespace detail
    {
        struct error_expecting_arguments
        {
            template <typename T>
            error_expecting_arguments(T const&) {}
        };

        struct error_invalid_lambda_expr
        {
            template <typename T>
            error_invalid_lambda_expr(T const&) {}
        };

        template <typename T>
        struct result_type_deduction_helper
        {
            typedef T const & type;
        };

        template <typename T>
        struct result_type_deduction_helper<T &>
        {
            typedef T & type;
        };

        template <typename T>
        struct result_type_deduction_helper<T const &>
        {
            typedef T const & type;
        };
    }

    namespace result_of
    {
#ifdef BOOST_PHOENIX_NO_VARIADIC_ACTOR
        // Bring in the result_of::actor<>
        #include <boost/phoenix/core/detail/cpp03/actor_result_of.hpp>
#else
        template <typename Expr, typename... A>
        struct actor_impl
        {
            typedef
                typename boost::phoenix::evaluator::impl<
                    Expr const&
                  , vector2<
                        typename vector_chooser<sizeof...(A) + 1>::
                          template apply<const ::boost::phoenix::actor<Expr> *, A...>::type&
                      , default_actions
                    > const &
                  , proto::empty_env
                >::result_type
                type;
        };

        template <typename Expr, typename... A>
        struct actor : actor_impl<Expr, A...> {};

        template <typename Expr>
        struct nullary_actor_result : actor_impl<Expr> {};
#endif

        template <typename Expr>
        struct actor<Expr>
        {
            typedef
                // avoid calling result_of::actor when this is false
                typename mpl::eval_if_c<
                    result_of::is_nullary<Expr>::value
                  , nullary_actor_result<Expr>
                  , mpl::identity<detail::error_expecting_arguments>
                >::type
            type;
        };
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    //  actor
    //
    //      The actor class. The main thing! In phoenix, everything is an actor
    //      This class is responsible for full function evaluation. Partial
    //      function evaluation involves creating a hierarchy of actor objects.
    //
    ////////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    struct actor
    {
        typedef typename
            mpl::eval_if_c<
                mpl::or_<
                    is_custom_terminal<Expr>
                  , mpl::bool_<is_placeholder<Expr>::value>
                >::value
              , proto::terminal<Expr>
              , mpl::identity<Expr>
            >::type
            expr_type;

        BOOST_PROTO_BASIC_EXTENDS(expr_type, actor<Expr>, phoenix_domain)
        BOOST_PROTO_EXTENDS_SUBSCRIPT()
        BOOST_PROTO_EXTENDS_ASSIGN_()

        template <typename Sig>
        struct result;

        typename result_of::actor<proto_base_expr>::type
        operator()()
        {
            typedef vector1<const actor<Expr> *> env_type;
            env_type env = {this};

            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        typename result_of::actor<proto_base_expr>::type
        operator()() const
        {
            typedef vector1<const actor<Expr> *> env_type;
            env_type env = {this};

            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        template <typename Env>
        typename evaluator::impl<
            proto_base_expr const &
          , typename result_of::context<
                Env const &
              , default_actions const &
            >::type
          , proto::empty_env
        >::result_type
        eval(Env const & env) const
        {
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

#ifdef BOOST_PHOENIX_NO_VARIADIC_ACTOR
        // Bring in the rest
        #include <boost/phoenix/core/detail/cpp03/actor_operator.hpp>
#else
        template <typename This, typename... A>
        struct result<This(A...)>
            : result_of::actor<
                proto_base_expr
              , typename mpl::if_<is_reference<A>, A, A const &>::type...
            >
        {};

        template <typename... A>
        typename result<actor(A...)>::type
        operator()(A &&... a)
        {
            typedef
                typename vector_chooser<sizeof...(A) + 1>::template apply<
                    const actor<Expr> *
                  , typename mpl::if_<is_reference<A>, A, A const &>::type...
                >::type
            env_type;

            env_type env = {this, a...};
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        template <typename... A>
        typename result<actor(A...)>::type
        operator()(A &&... a) const
        {
            typedef
                typename vector_chooser<sizeof...(A) + 1>::template apply<
                    const actor<Expr> *
                  , typename mpl::if_<is_reference<A>, A, A const &>::type...
                >::type
            env_type;

            env_type env = {this, a...};
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }
#endif

        BOOST_DELETED_FUNCTION(actor& operator=(actor const&))
    };
}}

namespace boost
{
    // specialize boost::result_of to return the proper result type
    template <typename Expr>
    struct result_of<phoenix::actor<Expr>()>
        : phoenix::result_of::actor<typename phoenix::actor<Expr>::proto_base_expr>
    {};

    template <typename Expr>
    struct result_of<phoenix::actor<Expr> const()>
        : phoenix::result_of::actor<typename phoenix::actor<Expr>::proto_base_expr>
    {};
}


#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

