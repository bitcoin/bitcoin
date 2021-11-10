
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)
#ifndef BOOST_PHOENIX_FUNCTION_DETAIL_FUNCTION_OPERATOR_HPP
#define BOOST_PHOENIX_FUNCTION_DETAIL_FUNCTION_OPERATOR_HPP

#include <boost/phoenix/function/detail/cpp03/preprocessed/function_operator.hpp>

#endif
#else

#if !BOOST_PHOENIX_IS_ITERATING

#ifndef BOOST_PHOENIX_FUNCTION_DETAIL_FUNCTION_OPERATOR_HPP
#define BOOST_PHOENIX_FUNCTION_DETAIL_FUNCTION_OPERATOR_HPP

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/function_operator_" BOOST_PHOENIX_LIMIT_STR ".hpp")
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
    (3, (1, BOOST_PHOENIX_ACTOR_LIMIT,                                          \
    <boost/phoenix/function/detail/cpp03/function_operator.hpp>))
#include BOOST_PHOENIX_ITERATE()

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif

#else
        template <typename This, BOOST_PHOENIX_typename_A>
        struct result<This(BOOST_PHOENIX_A_const_ref)>
            : detail::expression::function_eval<F, BOOST_PHOENIX_A>
        {};

        template <BOOST_PHOENIX_typename_A>
        typename detail::expression::function_eval<F, BOOST_PHOENIX_A>::type const
        operator()(BOOST_PHOENIX_A_const_ref_a) const
        {
            return detail::expression::function_eval<F, BOOST_PHOENIX_A>::make(f, BOOST_PHOENIX_a);
        }

#endif

#endif // PHOENIX_DONT_USE_PREPROCESSED_FILES
