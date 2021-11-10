
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)
#ifndef BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EXPRESSION_HPP
#define BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EXPRESSION_HPP

#include <boost/phoenix/support/iterate.hpp>

#include <boost/phoenix/statement/detail/preprocessed/try_catch_expression.hpp>

#endif
#else

#if !BOOST_PHOENIX_IS_ITERATING

#ifndef BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EXPRESSION_HPP
#define BOOST_PHOENIX_STATEMENT_DETAIL_TRY_CATCH_EXPRESSION_HPP

#include <boost/phoenix/support/iterate.hpp>

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/try_catch_expression_" BOOST_PHOENIX_LIMIT_STR ".hpp")
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

#define BOOST_PHOENIX_ITERATION_PARAMS                                          \
    (3, (0, BOOST_PHOENIX_CATCH_LIMIT,                                          \
    <boost/phoenix/statement/detail/try_catch_expression.hpp>))
#include BOOST_PHOENIX_ITERATE()

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif

#else

        template <
            typename Try
            BOOST_PP_COMMA_IF(BOOST_PHOENIX_ITERATION)
            BOOST_PHOENIX_typename_A
        >
        struct try_catch<
            Try
            BOOST_PP_COMMA_IF(BOOST_PHOENIX_ITERATION)
            BOOST_PHOENIX_A
        >
            : expr_ext<
                try_catch_actor
              , tag::try_catch
              , Try
              BOOST_PP_COMMA_IF(BOOST_PHOENIX_ITERATION) BOOST_PHOENIX_A
            >
        {};
    /**/

#endif

#endif // BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES
