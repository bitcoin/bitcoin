
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)
#ifndef BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EVAL_HPP
#define BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EVAL_HPP

#include <boost/phoenix/support/iterate.hpp>

#include <boost/phoenix/statement/detail/preprocessed/try_catch_eval.hpp>

#endif
#else

#if !BOOST_PHOENIX_IS_ITERATING

#ifndef BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EVAL_HPP
#define BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EVAL_HPP

#include <boost/phoenix/support/iterate.hpp>

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/try_catch_eval_" BOOST_PHOENIX_LIMIT_STR ".hpp")
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

    #define BOOST_PHOENIX_TRY_CATCH_EVAL_R(Z, N, DATA)                          \
            catch(                                                              \
                typename proto::result_of::value<                               \
                    typename proto::result_of::child_c<                         \
                        BOOST_PP_CAT(A, N)                                      \
                      , 0                                                       \
                    >::type                                                     \
                >::type::type &e                                                \
            )                                                                   \
            {                                                                   \
                eval_catch_body(BOOST_PP_CAT(a, N), e, ctx);                    \
            }                                                                   \
    /**/


#define BOOST_PHOENIX_ITERATION_PARAMS                                          \
        (3, (1, BOOST_PHOENIX_CATCH_LIMIT,                                      \
        <boost/phoenix/statement/detail/try_catch_eval.hpp>))
#include BOOST_PHOENIX_ITERATE()

    #undef BOOST_PHOENIX_TRY_CATCH_EVAL_R

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif

#else

        template <typename Try, BOOST_PHOENIX_typename_A, typename Context>
        typename boost::enable_if<
            proto::matches<
                BOOST_PP_CAT(A, BOOST_PP_DEC(BOOST_PHOENIX_ITERATION))
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, BOOST_PHOENIX_A_const_ref_a, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, BOOST_PHOENIX_TRY_CATCH_EVAL_R, _)
        }

        template <typename Try, BOOST_PHOENIX_typename_A, typename Context>
        typename boost::disable_if<
            proto::matches<
                BOOST_PP_CAT(A, BOOST_PP_DEC(BOOST_PHOENIX_ITERATION))
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, BOOST_PHOENIX_A_const_ref_a, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            BOOST_PP_REPEAT(
                BOOST_PP_DEC(BOOST_PHOENIX_ITERATION)
              , BOOST_PHOENIX_TRY_CATCH_EVAL_R, _
            )
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        BOOST_PP_CAT(a, BOOST_PP_DEC(BOOST_PHOENIX_ITERATION))
                    )
                  , ctx);
            }
        }

#endif

#endif // BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES
