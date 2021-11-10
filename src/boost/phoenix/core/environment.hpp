/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010-2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_ENVIRONMENT_HPP
#define BOOST_PHOENIX_CORE_ENVIRONMENT_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/phoenix/support/vector.hpp>
#include <boost/proto/transform/impl.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/result_of.hpp>

#include <typeinfo>

namespace boost { namespace phoenix 
{
    struct unused {};

    namespace result_of
    {
        template <typename Env, typename Actions>
        struct context
        {
            typedef vector2<Env, Actions> type;
        };
        
        template <typename Env, typename Actions>
        struct make_context
            : context<Env, Actions>
        {};

        template <typename Context>
        struct env
        {
            typedef
                typename fusion::result_of::at_c<
                    typename boost::remove_reference<Context>::type
                  , 0
                >::type
                type;
        };
        
        template <typename Context>
        struct actions
        {
            typedef
                typename fusion::result_of::at_c<
                    typename boost::remove_reference<Context>::type
                  , 1
                >::type
                type;
        };
    }

    namespace functional
    {
        struct context
        {
            BOOST_PROTO_CALLABLE()

            template <typename Sig>
            struct result;

            template <typename This, typename Env, typename Actions>
            struct result<This(Env, Actions)>
                : result<This(Env const &, Actions const &)>
            {};

            template <typename This, typename Env, typename Actions>
            struct result<This(Env &, Actions)>
                : result<This(Env &, Actions const &)>
            {};

            template <typename This, typename Env, typename Actions>
            struct result<This(Env, Actions &)>
                : result<This(Env const &, Actions &)>
            {};

            template <typename This, typename Env, typename Actions>
            struct result<This(Env &, Actions &)>
                : result_of::context<Env &, Actions &>
            {};

            template <typename Env, typename Actions>
            typename result_of::context<Env &, Actions &>::type
            operator()(Env & env, Actions & actions) const
            {
                vector2<Env &, Actions &> e = {env, actions};
                return e;
            }

            template <typename Env, typename Actions>
            typename result_of::context<Env const &, Actions &>::type
            operator()(Env const & env, Actions & actions) const
            {
                vector2<Env const &, Actions &> e = {env, actions};
                return e;
            }

            template <typename Env, typename Actions>
            typename result_of::context<Env &, Actions const &>::type
            operator()(Env & env, Actions const & actions) const
            {
                vector2<Env &, Actions const &> e = {env, actions};
                return e;
            }

            template <typename Env, typename Actions>
            typename result_of::context<Env const &, Actions const &>::type
            operator()(Env const & env, Actions const & actions) const
            {
                vector2<Env const&, Actions const &> e = {env, actions};
                return e;
            }
        };

        struct make_context
            : context
        {};

        struct env
        {
            BOOST_PROTO_CALLABLE()

            template <typename Sig>
            struct result;

            template <typename This, typename Context>
            struct result<This(Context)>
                : result<This(Context const &)>
            {};

            template <typename This, typename Context>
            struct result<This(Context &)>
                : result_of::env<Context>
            {};

            template <typename Context>
            typename result_of::env<Context const>::type
            operator()(Context const & ctx) const
            {
                return fusion::at_c<0>(ctx);
            }

            template <typename Context>
            typename result_of::env<Context>::type
            operator()(Context & ctx) const
            {
                return fusion::at_c<0>(ctx);
            }
        };
        
        struct actions
        {
            BOOST_PROTO_CALLABLE()

            template <typename Sig>
            struct result;

            template <typename This, typename Context>
            struct result<This(Context)>
                : result<This(Context const &)>
            {};

            template <typename This, typename Context>
            struct result<This(Context &)>
                : result_of::actions<Context>
            {};

            template <typename Context>
            typename result_of::actions<Context const>::type
            operator()(Context const & ctx) const
            {
                return fusion::at_c<1>(ctx);
            }

            template <typename Context>
            typename result_of::actions<Context>::type
            operator()(Context & ctx) const
            {
                return fusion::at_c<1>(ctx);
            }
        };

    }

    struct _context
        : proto::transform<_context>
    {
        template <typename Expr, typename State, typename Data>
        struct impl
            : proto::transform_impl<Expr, State, Data>
        {
            typedef vector2<State, Data> result_type;

            result_type operator()(
                typename impl::expr_param
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                vector2<State, Data> e = {s, d};
                return e;
            }
        };
    };

    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env const &, Actions const&>::type const
    context(Env const& env, Actions const& actions)
    {
        vector2<Env const&, Actions const &> e = {env, actions};
        return e;
    }

    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env const &, Actions const&>::type const
    make_context(Env const& env, Actions const& actions)
    {
        return context(env, actions);
    }

    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env &, Actions const&>::type const
    context(Env & env, Actions const& actions)
    {
        vector2<Env &, Actions const &> e = {env, actions};
        return e;
    }
    
    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env &, Actions const&>::type const
    make_context(Env & env, Actions const& actions)
    {
        return context(env, actions);
    }

    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env const &, Actions &>::type const
    context(Env const& env, Actions & actions)
    {
        vector2<Env const&, Actions &> e = {env, actions};
        return e;
    }
    
    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env const &, Actions &>::type const
    make_context(Env const& env, Actions & actions)
    {
        return context(env, actions);
    }
    
    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env &, Actions &>::type const
    context(Env & env, Actions & actions)
    {
        vector2<Env &, Actions &> e = {env, actions};
        return e;
    }
    
    template <typename Env, typename Actions>
    inline
    typename result_of::context<Env &, Actions &>::type const
    make_context(Env & env, Actions & actions)
    {
        return context(env, actions);
    }

    struct _env
        : proto::transform<_env>
    {
        template <typename Expr, typename State, typename Data>
        struct impl
            : proto::transform_impl<Expr, State, Data>
        {
            typedef State result_type;

            result_type operator()(
                typename impl::expr_param
              , typename impl::state_param s
              , typename impl::data_param
            ) const
            {
                return s;
            }
        };
    };

    template <typename Expr, typename State>
    struct _env::impl<Expr, State, proto::empty_env>
        : proto::transform_impl<Expr, State, proto::empty_env>
    {
            typedef
                typename fusion::result_of::at_c<
                    typename boost::remove_reference<State>::type
                  , 0
                >::type
                result_type;

            result_type operator()(
                typename impl::expr_param
              , typename impl::state_param s
              , typename impl::data_param
            ) const
            {
                return fusion::at_c<0>(s);
            }
    };

    template <typename Expr, typename State>
    struct _env::impl<Expr, State, unused>
        : _env::impl<Expr, State, proto::empty_env>
    {};

    template <typename Context>
    inline
    typename fusion::result_of::at_c<Context, 0>::type
    env(Context & ctx)
    {
        return fusion::at_c<0>(ctx);
    }

    template <typename Context>
    inline
    typename fusion::result_of::at_c<Context const, 0>::type
    env(Context const & ctx)
    {
        return fusion::at_c<0>(ctx);
    }

    struct _actions
        : proto::transform<_actions>
    {
        template <typename Expr, typename State, typename Data>
        struct impl
            : proto::transform_impl<Expr, State, Data>
        {
            typedef Data result_type;

            result_type operator()(
                typename impl::expr_param
              , typename impl::state_param
              , typename impl::data_param d
            ) const
            {
                return d;
            }
        };
    };

    template <typename Expr, typename State>
    struct _actions::impl<Expr, State, proto::empty_env>
        : proto::transform_impl<Expr, State, proto::empty_env>
    {
            typedef
                typename fusion::result_of::at_c<
                    typename boost::remove_reference<State>::type
                  , 1
                >::type
                result_type;

            result_type operator()(
                typename impl::expr_param
              , typename impl::state_param s
              , typename impl::data_param
            ) const
            {
                return fusion::at_c<1>(s);
            }
    };

    template <typename Expr, typename State>
    struct _actions::impl<Expr, State, unused>
        : _actions::impl<Expr, State, proto::empty_env>
    {};

    template <typename Context>
    inline
    typename fusion::result_of::at_c<Context, 1>::type
    actions(Context & ctx)
    {
        return fusion::at_c<1>(ctx);
    }

    template <typename Context>
    inline
    typename fusion::result_of::at_c<Context const, 1>::type
    actions(Context const & ctx)
    {
        return fusion::at_c<1>(ctx);
    }

    namespace result_of
    {
        template <
            BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
                BOOST_PHOENIX_LIMIT
              , typename A
              , mpl::void_
            )
          , typename Dummy = void
        >
        struct make_env;
        
    #define M0(Z, N, D)                                                         \
        template <BOOST_PHOENIX_typename_A(N)>                                  \
        struct make_env<BOOST_PHOENIX_A(N)>                                     \
        {                                                                       \
            typedef BOOST_PP_CAT(vector, N)<BOOST_PHOENIX_A(N)> type;           \
        };                                                                      \
    /**/
        BOOST_PP_REPEAT_FROM_TO(1, BOOST_PHOENIX_LIMIT, M0, _)
    #undef M0
    }

    inline
    result_of::make_env<>::type
    make_env()
    {
        return result_of::make_env<>::type();
    }
#define M0(Z, N, D)                                                             \
    template <BOOST_PHOENIX_typename_A(N)>                                      \
    inline                                                                      \
    typename result_of::make_env<BOOST_PHOENIX_A_ref(N)>::type                  \
    make_env(BOOST_PHOENIX_A_ref_a(N))                                          \
    {                                                                           \
        typename result_of::make_env<BOOST_PHOENIX_A_ref(N)>::type              \
            env =                                                               \
            {                                                                   \
                BOOST_PHOENIX_a(N)                                              \
            };                                                                  \
        return env;                                                             \
    }                                                                           \
    template <BOOST_PHOENIX_typename_A(N)>                                      \
    inline                                                                      \
    typename result_of::make_env<BOOST_PHOENIX_A_const_ref(N)>::type            \
    make_env(BOOST_PHOENIX_A_const_ref_a(N))                                    \
    {                                                                           \
        typename result_of::make_env<BOOST_PHOENIX_A_const_ref(N)>::type        \
            env =                                                               \
            {                                                                   \
                BOOST_PHOENIX_a(N)                                              \
            };                                                                  \
        return env;                                                             \
    }                                                                           \
    /**/
        BOOST_PP_REPEAT_FROM_TO(1, BOOST_PHOENIX_LIMIT, M0, _)
    #undef M0

    template <typename T, typename Enable = void>
    struct is_environment : fusion::traits::is_sequence<T> {};
}}

#endif

