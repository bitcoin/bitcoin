
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)
#ifndef BOOST_PHOENIX_CORE_DETAIL_ACTOR_OPERATOR_HPP
#define BOOST_PHOENIX_CORE_DETAIL_ACTOR_OPERATOR_HPP

#include <boost/phoenix/support/iterate.hpp>

#include <boost/phoenix/core/detail/cpp03/preprocessed/actor_operator.hpp>

#endif
#else

#if !BOOST_PHOENIX_IS_ITERATING

#ifndef BOOST_PHOENIX_CORE_DETAIL_ACTOR_OPERATOR_HPP
#define BOOST_PHOENIX_CORE_DETAIL_ACTOR_OPERATOR_HPP

#include <boost/phoenix/support/iterate.hpp>

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/actor_operator_" BOOST_PHOENIX_LIMIT_STR ".hpp")
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

#define M0(Z, N, D)                                                             \
    typename detail::result_type_deduction_helper<BOOST_PP_CAT(A, N)>::type     \
/**/

#define BOOST_PHOENIX_ITERATION_PARAMS                                          \
        (3, (1, BOOST_PHOENIX_ACTOR_LIMIT,                                      \
        <boost/phoenix/core/detail/cpp03/actor_operator.hpp>))
#include BOOST_PHOENIX_ITERATE()

#undef M0

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif

#else

#if BOOST_PHOENIX_ITERATION >= BOOST_PHOENIX_PERFECT_FORWARD_LIMIT

        template <typename This, BOOST_PHOENIX_typename_A>
        struct result<This(BOOST_PHOENIX_A)>
            : result<This(BOOST_PHOENIX_A_const_ref)>
        {};

        template <typename This, BOOST_PHOENIX_typename_A>
        struct result<This(BOOST_PHOENIX_A_ref)>
            : result_of::actor<proto_base_expr, BOOST_PHOENIX_A_ref>
        {};

        template <BOOST_PHOENIX_typename_A>
        typename result_of::actor<proto_base_expr, BOOST_PHOENIX_A_ref>::type
        operator()(BOOST_PHOENIX_A_ref_a)
        {
            typedef
                BOOST_PP_CAT(vector, BOOST_PP_INC(BOOST_PHOENIX_ITERATION))<
                    const actor<Expr> *, BOOST_PHOENIX_A_ref
                >
                env_type;
            env_type env = {this, BOOST_PHOENIX_a};
            
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        template <BOOST_PHOENIX_typename_A>
        typename result_of::actor<proto_base_expr, BOOST_PHOENIX_A_ref>::type
        operator()(BOOST_PHOENIX_A_ref_a) const
        {
            typedef
                BOOST_PP_CAT(vector, BOOST_PP_INC(BOOST_PHOENIX_ITERATION))<
                    const actor<Expr> *, BOOST_PHOENIX_A_ref
                >
                env_type;
            env_type env = {this, BOOST_PHOENIX_a};
            
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        template <BOOST_PHOENIX_typename_A>
        typename result_of::actor<proto_base_expr, BOOST_PHOENIX_A_const_ref>::type
        operator()(BOOST_PHOENIX_A_const_ref_a)
        {
            typedef
                BOOST_PP_CAT(vector, BOOST_PP_INC(BOOST_PHOENIX_ITERATION))<
                    const actor<Expr> *, BOOST_PHOENIX_A_const_ref
                >
                env_type;
            env_type env = {this, BOOST_PHOENIX_a};
            
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

        template <BOOST_PHOENIX_typename_A>
        typename result_of::actor<proto_base_expr, BOOST_PHOENIX_A_const_ref>::type
        operator()(BOOST_PHOENIX_A_const_ref_a) const
        {
            typedef
                BOOST_PP_CAT(vector, BOOST_PP_INC(BOOST_PHOENIX_ITERATION))<
                    const actor<Expr> *, BOOST_PHOENIX_A_const_ref
                >
                env_type;
            env_type env = {this, BOOST_PHOENIX_a};
            
            return phoenix::eval(*this, phoenix::context(env, default_actions()));
        }

#else

// We need to define operator() for all permutations of reference types.
// For BOOST_PHOENIX_ITERATION <= BOOST_PHOENIX_LIMIT_PREFECT_FORWARD
// 2^BOOST_PHOENIX_ITERATION overloads are created
// For compile time reasons,
// if BOOST_PHOENIX_ITERATION > BOOST_PHOENIX_LIMIT_PERFECT_FORWARD
// only operator()(A const &...a) and operator()(A &...a) are generated
// this is all handled by the PP mumbo jumbo above
#define BOOST_PHOENIX_ACTOR_OPERATOR(_, I, __)                                  \
        template <typename This, BOOST_PHOENIX_typename_A>                      \
        struct result<This(BOOST_PHOENIX_PERM_A(I))>                            \
            : result_of::actor<proto_base_expr, BOOST_PHOENIX_PERM_A(I)>        \
        {};                                                                     \
                                                                                \
        template <BOOST_PHOENIX_typename_A>                                     \
        typename result_of::actor<proto_base_expr, BOOST_PHOENIX_PERM_A(I)>::type\
        operator()(BOOST_PHOENIX_PERM_A_a(I)) const                             \
        {                                                                       \
            typedef                                                             \
                BOOST_PP_CAT(vector, BOOST_PP_INC(BOOST_PHOENIX_ITERATION))<    \
                    const actor<Expr> *, BOOST_PHOENIX_PERM_A(I)                \
                >                                                               \
                env_type;                                                       \
            env_type env = {this, BOOST_PHOENIX_a};                             \
                                                                                \
            return phoenix::eval(*this, phoenix::context(env, default_actions()));\
        }                                                                       \
                                                                                \
        template <BOOST_PHOENIX_typename_A>                                     \
        typename result_of::actor<proto_base_expr, BOOST_PHOENIX_PERM_A(I)>::type\
        operator()(BOOST_PHOENIX_PERM_A_a(I))                                   \
        {                                                                       \
            typedef                                                             \
                BOOST_PP_CAT(vector, BOOST_PP_INC(BOOST_PHOENIX_ITERATION))<    \
                    const actor<Expr> *, BOOST_PHOENIX_PERM_A(I)                \
                >                                                               \
                env_type;                                                       \
            env_type env = {this, BOOST_PHOENIX_a};                             \
                                                                                \
            return phoenix::eval(*this, phoenix::context(env, default_actions()));\
        }                                                                       \
        /**/

        template <typename This, BOOST_PHOENIX_typename_A>
        struct result<This(BOOST_PHOENIX_A)>
            : result<This(BOOST_PP_ENUM(BOOST_PHOENIX_ITERATION, M0, _))>
        {};

        BOOST_PP_REPEAT(BOOST_PHOENIX_PERM_SIZE, BOOST_PHOENIX_ACTOR_OPERATOR, _)

#undef BOOST_PHOENIX_ACTOR_OPERATOR

#endif

#endif

#endif // BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES
