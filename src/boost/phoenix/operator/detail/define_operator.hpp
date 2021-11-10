/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_DEFINE_OPERATOR_HPP
#define BOOST_PHOENIX_DEFINE_OPERATOR_HPP

#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#define BOOST_PHOENIX_UNARY_EXPRESSION(__, ___, name)                           \
    template <typename Operand>                                                 \
    struct name                                                                 \
        : expr<proto::tag::name, Operand>                                       \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_UNARY_RULE(__, ___, name)                                 \
    struct name                                                                 \
        : expression::name<meta_grammar>                                        \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_UNARY_FUNCTIONAL(__, ___, name)                           \
    namespace functional                                                        \
    {                                                                           \
        typedef                                                                 \
            proto::functional::make_expr<proto::tag::name>                      \
            BOOST_PP_CAT(make_, name);                                          \
    }                                                                           \
    namespace result_of                                                         \
    {                                                                           \
        template <typename Operand>                                             \
        struct BOOST_PP_CAT(make_, name)                                        \
            : boost::result_of<                                                 \
                functional:: BOOST_PP_CAT(make_, name)(                         \
                    Operand                                                     \
                )                                                               \
            >                                                                   \
        {};                                                                     \
    }                                                                           \
    template <typename Operand>                                                 \
    inline                                                                      \
    typename result_of::BOOST_PP_CAT(make_, name)<Operand>::type                \
    BOOST_PP_CAT(make_, name)(Operand const & operand)                          \
    {                                                                           \
        return functional::BOOST_PP_CAT(make_, name)()(operand);                \
    }                                                                           \
/**/

#define BOOST_PHOENIX_BINARY_EXPRESSION(__, ___, name)                          \
    template <typename Lhs, typename Rhs>                                       \
    struct name                                                                 \
        : expr<proto::tag::name, Lhs, Rhs>                                      \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_BINARY_RULE(__, ___, name)                                \
    struct name                                                                 \
        : expression::name<meta_grammar, meta_grammar>                          \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_BINARY_FUNCTIONAL(__, ___, name)                          \
    namespace functional                                                        \
    {                                                                           \
        typedef                                                                 \
            proto::functional::make_expr<proto::tag::name>                      \
            BOOST_PP_CAT(make_, name);                                          \
    }                                                                           \
    namespace result_of                                                         \
    {                                                                           \
        template <typename Lhs, typename Rhs>                                   \
        struct BOOST_PP_CAT(make_, name)                                        \
            : boost::result_of<                                                 \
                functional:: BOOST_PP_CAT(make_, name)(                         \
                    Lhs, Rhs                                                    \
                )                                                               \
            >                                                                   \
        {};                                                                     \
    }                                                                           \
    template <typename Rhs, typename Lhs>                                       \
    inline                                                                      \
    typename result_of::BOOST_PP_CAT(make_, name)<Rhs, Lhs>::type               \
    BOOST_PP_CAT(make_, name)(Lhs const & lhs, Rhs const & rhs)                 \
    {                                                                           \
        return functional::BOOST_PP_CAT(make_, name)()(lhs, rhs);               \
    }                                                                           \
/**/

#define BOOST_PHOENIX_GRAMMAR(_, __, name)                                      \
    template <typename Dummy>                                                   \
    struct meta_grammar::case_<proto::tag::name, Dummy>                         \
        : enable_rule<rule::name, Dummy>                                        \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_UNARY_OPERATORS(ops)                                      \
    namespace expression {                                                      \
        BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_UNARY_EXPRESSION, _, ops)           \
    }                                                                           \
    namespace rule {                                                            \
        BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_UNARY_RULE, _, ops)                 \
    }                                                                           \
    BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_GRAMMAR, _, ops)                        \
    BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_UNARY_FUNCTIONAL, _, ops)               \
/**/


#define BOOST_PHOENIX_BINARY_OPERATORS(ops)                                     \
    namespace expression {                                                      \
        BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_BINARY_EXPRESSION, _, ops)          \
    }                                                                           \
    namespace rule {                                                            \
        BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_BINARY_RULE, _, ops)                \
    }                                                                           \
    BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_GRAMMAR, _, ops)                        \
    BOOST_PP_SEQ_FOR_EACH(BOOST_PHOENIX_BINARY_FUNCTIONAL, _, ops)              \
/**/

#endif
