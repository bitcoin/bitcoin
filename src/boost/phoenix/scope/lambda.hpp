/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin
    Copyright (c) 2010 Thomas Heller
    Copyright (c) 2016 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_SCOPE_LAMBDA_HPP
#define BOOST_PHOENIX_SCOPE_LAMBDA_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/mpl/int.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/scope/local_variable.hpp>
#include <boost/phoenix/scope/scoped_environment.hpp>

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(lambda_actor)
  , (proto::terminal<proto::_>) // Locals
    (proto::terminal<proto::_>) // Map
    (meta_grammar)              // Lambda
)

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(lambda)
  , (proto::terminal<proto::_>) // OuterEnv 
    (proto::terminal<proto::_>) // Locals
    (proto::terminal<proto::_>) // Map
    (meta_grammar)              // Lambda
)

namespace boost { namespace phoenix
{
    struct lambda_eval
    {
        BOOST_PROTO_CALLABLE()

        template <typename Sig>
        struct result;

        template <
            typename This
          , typename OuterEnv
          , typename Locals
          , typename Map
          , typename Lambda
          , typename Context
        >
        struct result<This(OuterEnv, Locals, Map, Lambda, Context)>
        {
            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<
                        OuterEnv
                    >::type
                >::type
                outer_env_type;

            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<
                        Locals
                    >::type
                >::type
                locals_type;

            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<
                        Map
                    >::type
                >::type
                map_type;
            
            typedef
                typename proto::detail::uncvref<
                    typename result_of::env<Context>::type
                >::type
                env_type;

                    typedef
                            typename result_of::eval<
                                Lambda
                                     , typename result_of::context<
                                       scoped_environment<
                                              env_type
                                            , outer_env_type
                                            , locals_type
                                            , map_type
                                            >
                          , typename result_of::actions<
                                Context
                                            >::type
                                       >::type
                                    >::type
                             type;
        };

        template <typename OuterEnv, typename Locals, typename Map, typename Lambda, typename Context>
        typename result<lambda_eval(OuterEnv const &, Locals const &, Map const &, Lambda const &, Context const &)>::type
        operator()(OuterEnv const & outer_env, Locals const & locals, Map const &, Lambda const & lambda, Context const & ctx) const
        {
            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<
                        OuterEnv
                    >::type
                >::type
                outer_env_type;

            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<
                        Locals
                    >::type
                >::type
                locals_type;

            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<
                        Map
                    >::type
                >::type
                map_type;
            
            typedef
                typename proto::detail::uncvref<
                    typename result_of::env<Context>::type
                >::type
                env_type;
            
                scoped_environment<
                env_type
              , outer_env_type
              , locals_type
              , map_type
            >
            env(phoenix::env(ctx), proto::value(outer_env), proto::value(locals));

            return eval(lambda, phoenix::context(env, phoenix::actions(ctx)));
        }
    };

    template <typename Dummy>
    struct default_actions::when<rule::lambda, Dummy>
        : call<lambda_eval, Dummy>
    {};

    template <typename Dummy>
    struct is_nullary::when<rule::lambda, Dummy>
        : proto::call<
            evaluator(
                proto::_child_c<3>
              , proto::call<
                    functional::context(
                        proto::make<
                            mpl::true_()
                        >
                      , proto::make<
                            detail::scope_is_nullary_actions()
                        >
                    )
                >
              , proto::make<
                    proto::empty_env()
                >
            )
        >
    {};

    template <typename Dummy>
    struct is_nullary::when<rule::lambda_actor, Dummy>
        : proto::or_<
            proto::when<
                expression::lambda_actor<
                    proto::terminal<vector0<> >
                  , proto::terminal<proto::_>
                  , meta_grammar
                >
              , mpl::true_()
            >
          , proto::when<
                expression::lambda_actor<
                    proto::terminal<proto::_>
                  , proto::terminal<proto::_>
                  , meta_grammar
                >
              , proto::fold<
                    proto::call<proto::_value(proto::_child_c<0>)>
                  , proto::make<mpl::true_()>
                  , proto::make<
                        mpl::and_<
                            proto::_state
                          , proto::call<
                                evaluator(
                                    proto::_
                                  , _context
                                  , proto::make<proto::empty_env()>
                                )
                            >
                        >()
                    >
                >
            >
        >
    {};

    struct lambda_actor_eval
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Vars, typename Map, typename Lambda, typename Context>
        struct result<This(Vars, Map, Lambda, Context)>
        {
            typedef
                typename proto::detail::uncvref<
                    typename result_of::env<Context>::type
                >::type
                env_type;
            typedef
                typename proto::detail::uncvref<
                    typename result_of::actions<Context>::type
                >::type
                actions_type;
            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<Vars>::type
                     >::type
                     vars_type;
            
            typedef typename
                detail::result_of::initialize_locals<
                    vars_type
                  , Context
                >::type
            locals_type;

            typedef
                typename expression::lambda<
                    env_type
                  , locals_type
                  , Map
                  , Lambda
                >::type const
                type;
        };

        template <
            typename Vars
          , typename Map
          , typename Lambda
          , typename Context
        >
        typename result<
            lambda_actor_eval(Vars const&, Map const &, Lambda const&, Context const &)
        >::type const
        operator()(Vars const& vars, Map const& map, Lambda const& lambda, Context const & ctx) const
        {
            typedef
                typename proto::detail::uncvref<
                    typename result_of::env<Context>::type
                >::type
                env_type;
            /*typedef
                typename proto::detail::uncvref<
                    typename result_of::actions<Context>::type
                >::type
                actions_type;*/
            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<Vars>::type
                     >::type
                     vars_type;
            /*typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<Map>::type
                     >::type
                     map_type;*/

            typedef typename
                detail::result_of::initialize_locals<
                    vars_type
                  , Context
                >::type
            locals_type;

            locals_type locals = initialize_locals(proto::value(vars), ctx);

            return
                expression::
                    lambda<env_type, locals_type, Map, Lambda>::
                        make(phoenix::env(ctx), locals, map, lambda);
        }
    };

    template <typename Dummy>
    struct default_actions::when<rule::lambda_actor, Dummy>
        : call<lambda_actor_eval, Dummy>
    {};
    
    template <typename Locals = vector0<>,
              typename Map = detail::map_local_index_to_tuple<>,
              typename Dummy = void>
    struct lambda_actor_gen;

    template <>
    struct lambda_actor_gen<vector0<>, detail::map_local_index_to_tuple<>, void>
    {
        template <typename Expr>
        typename expression::lambda_actor<vector0<>, detail::map_local_index_to_tuple<>, Expr>::type const
        operator[](Expr const & expr) const
        {
            typedef vector0<> locals_type;
            typedef detail::map_local_index_to_tuple<> map_type;
            return expression::lambda_actor<locals_type, map_type, Expr>::make(locals_type(), map_type(), expr);
        }
    };

    template <typename Locals, typename Map>
    struct lambda_actor_gen<Locals, Map>
    {
        lambda_actor_gen(Locals const & locals_)
            : locals(locals_)
        {}

        lambda_actor_gen(lambda_actor_gen const & o)
            : locals(o.locals)
        {};

        template <typename Expr>
        typename expression::lambda_actor<
            Locals
          , Map
          , Expr
        >::type const
        operator[](Expr const & expr) const
        {
            return expression::lambda_actor<Locals, Map, Expr>::make(locals, Map(), expr);
        }

        Locals locals;
    };

    struct lambda_local_gen
        : lambda_actor_gen<>
    {
#if defined(BOOST_PHOENIX_NO_VARIADIC_SCOPE)
        lambda_actor_gen<> const
        operator()() const
        {
            return lambda_actor_gen<>();
        }

        #include <boost/phoenix/scope/detail/cpp03/lambda.hpp>
#else
#define BOOST_PHOENIX_SCOPE_ACTOR_GEN_NAME lambda_actor_gen
#define BOOST_PHOENIX_SCOPE_ACTOR_GEN_FUNCTION operator()
#define BOOST_PHOENIX_SCOPE_ACTOR_GEN_CONST const
        #include <boost/phoenix/scope/detail/local_gen.hpp>
#undef BOOST_PHOENIX_SCOPE_ACTOR_GEN_NAME
#undef BOOST_PHOENIX_SCOPE_ACTOR_GEN_FUNCTION
#undef BOOST_PHOENIX_SCOPE_ACTOR_GEN_CONST
#endif
    };

    typedef lambda_local_gen lambda_type;
    lambda_local_gen const lambda = lambda_local_gen();

}}

#endif
