/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_SCOPE_SCOPED_ENVIRONMENT_HPP
#define BOOST_PHOENIX_SCOPE_SCOPED_ENVIRONMENT_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/mpl/int.hpp>
#include <boost/fusion/sequence/sequence_facade.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/include/pop_front.hpp>
#include <boost/utility/result_of.hpp>

namespace boost { namespace phoenix
{
    template<typename Env, typename OuterEnv, typename Locals, typename Map>
    struct scoped_environment
        : fusion::sequence_facade<
            scoped_environment<Env, OuterEnv, Locals, Map>
          , fusion::random_access_traversal_tag
        >
    {
        typedef Env env_type;
        typedef OuterEnv outer_env_type;
        typedef Locals locals_type;
        typedef Map map_type;

        scoped_environment(
            Env const & env_
          , OuterEnv const &outer_env_
          , Locals const &locals_
        )
            : env(env_)
            , outer_env(outer_env_)
            , locals(locals_)
        {}

        scoped_environment(scoped_environment const & o)
            : env(o.env)
            , outer_env(o.outer_env)
            , locals(o.locals)
        {}

        Env      const & env;
        OuterEnv const & outer_env;
        Locals   const & locals;

        typedef typename
            fusion::result_of::pop_front<
                typename add_const<
                    typename proto::detail::uncvref<Env>::type
                >::type
            >::type
            args_type;

        args_type args() const
        {
            return fusion::pop_front(env);
        }
    
        #define BOOST_PHOENIX_ADAPT_SCOPED_ENVIRONMENT(INTRINSIC)               \
        template <typename Seq>                                                 \
        struct INTRINSIC                                                        \
        {                                                                       \
            typedef                                                             \
                typename fusion::result_of::INTRINSIC<                          \
                    typename mpl::eval_if_c<                                    \
                        is_const<                                               \
                            typename remove_reference<                          \
                                typename Seq::env_type                          \
                            >::type                                             \
                        >::value                                                \
                      , add_const<                                              \
                            typename proto::detail::uncvref<                    \
                                typename Seq::env_type                          \
                            >::type                                             \
                        >                                                       \
                      , proto::detail::uncvref<                                 \
                            typename Seq::env_type                              \
                        >                                                       \
                    >::type                                                     \
                >::type                                                         \
                type;                                                           \
                                                                                \
            static type call(Seq & seq)                                         \
            {                                                                   \
                return fusion::INTRINSIC(seq.env);                              \
            }                                                                   \
        }                                                                       \
        /**/
        BOOST_PHOENIX_ADAPT_SCOPED_ENVIRONMENT(begin);
        BOOST_PHOENIX_ADAPT_SCOPED_ENVIRONMENT(end);
        BOOST_PHOENIX_ADAPT_SCOPED_ENVIRONMENT(size);
        #undef BOOST_PHOENIX_ADAPT_SCOPED_ENVIRONMENT
    
        template <typename Seq, typename N>
        struct value_at
        {
            typedef
                typename fusion::result_of::value_at<
                    typename mpl::eval_if_c<
                        is_const<
                            typename remove_reference<
                                typename Seq::env_type
                            >::type
                        >::value
                      , add_const<
                            typename proto::detail::uncvref<
                                typename Seq::env_type
                            >::type
                        >
                      , proto::detail::uncvref<
                            typename Seq::env_type
                        >
                    >::type
                  , N
                >::type
                type;
        };
        
        template <typename Seq, typename N>
        struct at
        {
            typedef
                typename fusion::result_of::at<
                    typename mpl::eval_if_c<
                        is_const<
                            typename remove_reference<
                                typename Seq::env_type
                            >::type
                        >::value
                      , add_const<
                            typename proto::detail::uncvref<
                                typename Seq::env_type
                            >::type
                        >
                      , proto::detail::uncvref<
                            typename Seq::env_type
                        >
                    >::type
                  , N
                >::type
                type;

            static type call(Seq & seq)
            {
                return fusion::at<N>(seq.env);
            }
        };
    };

    template <typename Env, typename Dummy = void>
    struct is_scoped_environment : mpl::false_ {};
    
    template <typename Env>
    struct is_scoped_environment<Env&> : is_scoped_environment<Env> {};
    
    template <typename Env, typename OuterEnv, typename Locals, typename Map>
    struct is_scoped_environment<scoped_environment<Env, OuterEnv, Locals, Map> >
        : mpl::true_
    {};

    template <typename Env, typename OuterEnv, typename Locals, typename Map>
    struct is_scoped_environment<scoped_environment<Env, OuterEnv, Locals, Map> const>
        : mpl::true_
    {};
}}

#endif
