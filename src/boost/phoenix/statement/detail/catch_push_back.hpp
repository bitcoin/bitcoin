
#if !BOOST_PHOENIX_IS_ITERATING

#ifndef BOOST_PHOENIX_STATEMENT_DETAIL_CATCH_PUSH_BACK_HPP
#define BOOST_PHOENIX_STATEMENT_DETAIL_CATCH_PUSH_BACK_HPP

#include <boost/phoenix/support/iterate.hpp>

#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)

#include <boost/phoenix/statement/detail/preprocessed/catch_push_back.hpp>

#else

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/catch_push_back_" BOOST_PHOENIX_LIMIT_STR ".hpp")
#endif

/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

    #define BOOST_PHOENIX_CATCH_PUSH_BACK_R0(Z, N, DATA)                        \
        BOOST_PP_COMMA_IF(N)                                                    \
        typename proto::result_of::child_c<TryCatch, N>::type                   \
    /**/

    #define BOOST_PHOENIX_CATCH_PUSH_BACK_R1(Z, N, DATA)                        \
        BOOST_PP_COMMA_IF(N) proto::child_c<N>(try_catch)                       \
    /**/

#define BOOST_PHOENIX_ITERATION_PARAMS                                          \
        (3, (2, BOOST_PHOENIX_CATCH_LIMIT,                                      \
        <boost/phoenix/statement/detail/catch_push_back.hpp>))
#include BOOST_PHOENIX_ITERATE()
    
    #undef BOOST_PHOENIX_CATCH_PUSH_BACK_R0
    #undef BOOST_PHOENIX_CATCH_PUSH_BACK_R1

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif // BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES

#endif

#else
        template <typename TryCatch, typename Exception, typename Capture, typename Expr>
        struct catch_push_back<TryCatch, Exception, Capture, Expr, BOOST_PHOENIX_ITERATION>
        {
            typedef
                typename proto::result_of::make_expr<
                    phoenix::tag::catch_
                  , proto::basic_default_domain
                  , catch_exception<Exception>
                  , Capture
                  , Expr
                >::type
                catch_expr;

            typedef phoenix::expression::try_catch<
                BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, BOOST_PHOENIX_CATCH_PUSH_BACK_R0, _)
              , catch_expr> gen_type;
            typedef typename gen_type::type type;

            static type
            make(
                TryCatch const& try_catch
              , Capture const& capture
              , Expr const& catch_
            )
            {
                return
                    gen_type::make(
                        BOOST_PP_REPEAT(
                            BOOST_PHOENIX_ITERATION
                          , BOOST_PHOENIX_CATCH_PUSH_BACK_R1
                          , _
                        )
                      , proto::make_expr<
                            phoenix::tag::catch_
                          , proto::basic_default_domain
                        >(catch_exception<Exception>(), capture, catch_)
                    );
            }
        };

        template <typename TryCatch, typename Exception, typename Expr>
        struct catch_push_back<TryCatch, Exception, void, Expr, BOOST_PHOENIX_ITERATION>
        {
            typedef
                typename proto::result_of::make_expr<
                    phoenix::tag::catch_
                  , proto::basic_default_domain
                  , catch_exception<Exception>
                  , Expr
                >::type
                catch_expr;

            typedef phoenix::expression::try_catch<
                BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, BOOST_PHOENIX_CATCH_PUSH_BACK_R0, _)
              , catch_expr> gen_type;
            typedef typename gen_type::type type;

            static type
            make(
                TryCatch const& try_catch
              , Expr const& catch_
            )
            {
                return
                    gen_type::make(
                        BOOST_PP_REPEAT(
                            BOOST_PHOENIX_ITERATION
                          , BOOST_PHOENIX_CATCH_PUSH_BACK_R1
                          , _
                        )
                      , proto::make_expr<
                            phoenix::tag::catch_
                          , proto::basic_default_domain
                        >(catch_exception<Exception>(), catch_)
                    );
            }
        };

        template <typename TryCatch, typename Expr>
        struct catch_all_push_back<TryCatch, Expr, BOOST_PHOENIX_ITERATION>
        {
            typedef
                typename proto::result_of::make_expr<
                    phoenix::tag::catch_all
                  , proto::basic_default_domain
                  , Expr
                >::type
                catch_expr;

            typedef phoenix::expression::try_catch<
                BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, BOOST_PHOENIX_CATCH_PUSH_BACK_R0, _)
              , catch_expr> gen_type;
            typedef typename gen_type::type type;

            static type
            make(
                TryCatch const& try_catch
              , Expr const& catch_
            )
            {
                return
                    gen_type::make(
                        BOOST_PP_REPEAT(
                            BOOST_PHOENIX_ITERATION
                          , BOOST_PHOENIX_CATCH_PUSH_BACK_R1
                          , _
                        )
                      , proto::make_expr<
                            phoenix::tag::catch_all
                          , proto::basic_default_domain
                        >(catch_)
                    );
            }
        };

#endif
