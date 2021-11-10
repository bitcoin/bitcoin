/*=============================================================================
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_CORE_DETAIL_EXPRESSION_HPP
#define BOOST_PHOENIX_CORE_DETAIL_EXPRESSION_HPP

#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/pop_back.hpp>
#include <boost/preprocessor/seq/reverse.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/repeat_from_to.hpp>

#define BOOST_PHOENIX_DEFINE_EXPRESSION(NAME_SEQ, SEQ)                          \
    BOOST_PHOENIX_DEFINE_EXPRESSION_BASE(                                       \
        NAME_SEQ                                                                \
      , SEQ                                                                     \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_DEFAULT                      \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RULE_DEFAULT                            \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_DEFAULT                  \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_DEFAULT                 \
      , _                                                                       \
    )                                                                           \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_VARARG(NAME_SEQ, GRAMMAR_SEQ, LIMIT)    \
    BOOST_PHOENIX_DEFINE_EXPRESSION_BASE(                                       \
        NAME_SEQ                                                                \
      , GRAMMAR_SEQ                                                             \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_VARARG                       \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RULE_VARARG                             \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_VARARG                   \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_VARARG                  \
      , LIMIT                                                                   \
    )                                                                           \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXT(ACTOR, NAME_SEQ, GRAMMAR_SEQ)       \
    BOOST_PHOENIX_DEFINE_EXPRESSION_BASE(                                       \
        NAME_SEQ                                                                \
      , GRAMMAR_SEQ                                                             \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_EXT                          \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RULE_DEFAULT                            \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_DEFAULT                  \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_DEFAULT                 \
      , ACTOR                                                                   \
    )                                                                           \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXT_VARARG(ACTOR, NAME, GRAMMAR, LIMIT) \
    BOOST_PHOENIX_DEFINE_EXPRESSION_BASE(                                       \
        NAME_SEQ                                                                \
      , GRAMMAR_SEQ                                                             \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_VARARG_EXT                   \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RULE_VARARG                             \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_VARARG                   \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_VARARG                  \
      , ACTOR                                                                   \
    )                                                                           \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_NAMESPACE(R, D, E)                      \
namespace E {                                                                   \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_NAMESPACE_END(R, D, E)                  \
}                                                                               \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_NS(R, D, E)                             \
E ::                                                                            \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_BASE(NAME_SEQ, GRAMMAR_SEQ, EXPRESSION, RULE, RESULT_OF_MAKE, MAKE_EXPRESSION, DATA) \
BOOST_PP_SEQ_FOR_EACH(                                                          \
    BOOST_PHOENIX_DEFINE_EXPRESSION_NAMESPACE                                   \
  , _                                                                           \
  , BOOST_PP_SEQ_POP_BACK(NAME_SEQ)                                             \
)                                                                               \
    namespace tag                                                               \
    {                                                                           \
        struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ)) {};            \
        template <typename Ostream>                                             \
        inline Ostream &operator<<(                                             \
            Ostream & os                                                        \
          , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ)))                  \
        {                                                                       \
            os << BOOST_PP_STRINGIZE(                                           \
                BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))               \
            );                                                                  \
            return os;                                                          \
        }                                                                       \
    }                                                                           \
    namespace expression                                                        \
    {                                                                           \
        EXPRESSION(NAME_SEQ, GRAMMAR_SEQ, DATA)                                 \
    }                                                                           \
    namespace rule                                                              \
    {                                                                           \
        RULE(NAME_SEQ, GRAMMAR_SEQ, DATA)                                       \
    }                                                                           \
    namespace functional                                                        \
    {                                                                           \
        typedef                                                                 \
            boost::proto::functional::make_expr<                                \
                    tag:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))     \
            >                                                                   \
            BOOST_PP_CAT(                                                       \
                make_                                                           \
              , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))               \
            );                                                                  \
    }                                                                           \
    namespace result_of                                                         \
    {                                                                           \
        RESULT_OF_MAKE(NAME_SEQ, GRAMMAR_SEQ, DATA)                             \
    }                                                                           \
    MAKE_EXPRESSION(NAME_SEQ, GRAMMAR_SEQ, DATA)                                \
                                                                                \
BOOST_PP_SEQ_FOR_EACH(                                                          \
    BOOST_PHOENIX_DEFINE_EXPRESSION_NAMESPACE_END                               \
  , _                                                                           \
  , BOOST_PP_SEQ_POP_BACK(NAME_SEQ)                                             \
)                                                                               \
namespace boost { namespace phoenix                                             \
{                                                                               \
    template <typename Dummy>                                                   \
    struct meta_grammar::case_<                                                 \
        :: BOOST_PP_SEQ_FOR_EACH(                                               \
            BOOST_PHOENIX_DEFINE_EXPRESSION_NS                                  \
          , _                                                                   \
          , BOOST_PP_SEQ_POP_BACK(NAME_SEQ)                                     \
        ) tag:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))               \
      , Dummy                                                                   \
    >                                                                           \
        : enable_rule<                                                          \
            :: BOOST_PP_SEQ_FOR_EACH(                                           \
                BOOST_PHOENIX_DEFINE_EXPRESSION_NS                              \
              , _                                                               \
              , BOOST_PP_SEQ_POP_BACK(NAME_SEQ)                                 \
            ) rule:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))          \
         , Dummy                                                                \
        >                                                                       \
    {};                                                                         \
} }                                                                             \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_DEFAULT(NAME_SEQ, GRAMMAR_SEQ, D) \
        template <BOOST_PHOENIX_typename_A(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))>     \
        struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                \
            : boost::phoenix::expr<                                             \
                :: BOOST_PP_SEQ_FOR_EACH(                                       \
                    BOOST_PHOENIX_DEFINE_EXPRESSION_NS                          \
                  , _                                                           \
                  , BOOST_PP_SEQ_POP_BACK(NAME_SEQ)                             \
                ) tag:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))       \
              , BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ), A)>        \
        {};                                                                     \
/**/
        
#define BOOST_PHOENIX_DEFINE_EXPRESSION_RULE_DEFAULT(NAME_SEQ, GRAMMAR_SEQ, D)  \
    struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                    \
        : expression:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))        \
            <BOOST_PP_SEQ_ENUM(GRAMMAR_SEQ)>                                    \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_DEFAULT(NAME_SEQ, GRAMMAR_SEQ, D) \
    template <BOOST_PHOENIX_typename_A(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))>         \
    struct BOOST_PP_CAT(make_, BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))) \
        : boost::result_of<                                                     \
            functional::                                                        \
                BOOST_PP_CAT(                                                   \
                    make_                                                       \
                  , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))           \
                )(BOOST_PHOENIX_A(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ)))              \
        >                                                                       \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_DEFAULT(NAME_SEQ, GRAMMAR_SEQ, D) \
    template <BOOST_PHOENIX_typename_A(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))>         \
    inline                                                                      \
    typename                                                                    \
        result_of::BOOST_PP_CAT(                                                \
            make_                                                               \
          , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                   \
        )<                                                                      \
            BOOST_PHOENIX_A(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))                     \
        >::type const                                                           \
    BOOST_PP_CAT(                                                               \
        make_                                                                   \
      , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                       \
    )(                                                                          \
        BOOST_PHOENIX_A_const_ref_a(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))             \
    )                                                                           \
    {                                                                           \
        return                                                                  \
            functional::BOOST_PP_CAT(                                           \
                make_                                                           \
              , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))               \
            )()(                                                                \
              BOOST_PHOENIX_a(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))                   \
            );                                                                  \
    }                                                                           \
/**/

#ifndef BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_VARARG(NAME_SEQ, _G, _L)     \
    template <typename A0, typename... A>                                       \
    struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                    \
        : boost::phoenix::expr<                                                 \
            tag:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))             \
          , A0, A...                                                            \
        >                                                                       \
    {};                                                                         \
/**/

#else // BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_VARARG_R(_, N, NAME)                    \
    template <                                                                  \
        BOOST_PHOENIX_typename_A(                                               \
            BOOST_PP_ADD(                                                       \
                N                                                               \
              , BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(2, 1, NAME))              \
            )                                                                   \
        )                                                                       \
    >                                                                           \
    struct BOOST_PP_TUPLE_ELEM(2, 0, NAME)<                                     \
        BOOST_PHOENIX_A(                                                        \
            BOOST_PP_ADD(N, BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(2, 1, NAME))) \
        )                                                                       \
    >                                                                           \
        : boost::phoenix::expr<                                                 \
            tag:: BOOST_PP_TUPLE_ELEM(2, 0, NAME)                               \
          , BOOST_PHOENIX_A(                                                    \
                BOOST_PP_ADD(                                                   \
                    N                                                           \
                  , BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(2, 1, NAME))          \
                )                                                               \
            )                                                                   \
        >                                                                       \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_VARARG(NAME_SEQ, GRAMMAR_SEQ, LIMIT) \
        template <                                                              \
            BOOST_PHOENIX_typename_A_void(                                      \
                BOOST_PP_ADD(                                                   \
                    LIMIT, BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ)))        \
            )                                                                   \
          , typename Dummy = void                                               \
        >                                                                       \
        struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ));               \
                                                                                \
        BOOST_PP_REPEAT_FROM_TO(                                                \
            1                                                                   \
          , BOOST_PP_ADD(LIMIT, BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ)))   \
          , BOOST_PHOENIX_DEFINE_EXPRESSION_VARARG_R                            \
          , (                                                                   \
                BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))               \
              , BOOST_PP_SEQ_POP_BACK(GRAMMAR_SEQ)                              \
            )                                                                   \
        )                                                                       \
/**/
#endif // BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_RULE_VARARG(NAME_SEQ, GRAMMAR_SEQ, LIMIT) \
        struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                \
            : expression:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ)) <  \
                BOOST_PP_IF(                                                    \
                    BOOST_PP_EQUAL(1, BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))           \
                  , BOOST_PP_EMPTY                                              \
                  , BOOST_PP_IDENTITY(                                          \
                      BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_POP_BACK(GRAMMAR_SEQ))     \
                    )                                                           \
                )()                                                             \
                BOOST_PP_COMMA_IF(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))) \
                boost::proto::vararg<                                           \
                    BOOST_PP_SEQ_ELEM(                                          \
                        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))            \
                      , GRAMMAR_SEQ                                             \
                    )                                                           \
                >                                                               \
            >                                                                   \
        {};                                                                     \
/**/

#ifndef BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_VARARG(NAME_SEQ, _G, _L) \
    template <typename A0, typename... A>                                       \
    struct BOOST_PP_CAT(make_, BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))) \
        : boost::result_of<                                                     \
            functional:: BOOST_PP_CAT(make_, BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ)))( \
                A0, A...                                                        \
            )                                                                   \
        >                                                                       \
    {};                                                                         \
/**/

#else // BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_VARARG_R(Z, N, NAME)     \
    template <BOOST_PHOENIX_typename_A(N)>                                      \
    struct BOOST_PP_CAT(make_, NAME) <BOOST_PHOENIX_A(N)>                       \
        : boost::result_of<                                                     \
            functional:: BOOST_PP_CAT(make_, NAME)(                             \
                BOOST_PHOENIX_A(N)                                              \
            )                                                                   \
        >                                                                       \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_VARARG(NAME_SEQ, GRAMMAR_SEQ, LIMIT) \
    template <BOOST_PHOENIX_typename_A_void(LIMIT), typename Dummy = void>      \
    struct BOOST_PP_CAT(                                                        \
        make_                                                                   \
      , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                       \
    );                                                                          \
    BOOST_PP_REPEAT_FROM_TO(                                                    \
        1                                                                       \
      , LIMIT                                                                   \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_RESULT_OF_MAKE_VARARG_R                 \
      , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                       \
    )                                                                           \
/**/
#endif // BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#ifndef BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_VARARG(NAME_SEQ, GRAMMAR_SEQ, LIMIT) \
    template <typename A0, typename... A>                                       \
    inline                                                                      \
    typename                                                                    \
        result_of:: BOOST_PP_CAT(make_, BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ)))< \
            A0, A...                                                            \
        >::type                                                                 \
    BOOST_PP_CAT(make_, BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ)))(A0 const& a0, A const&... a) \
    {                                                                           \
        return functional::BOOST_PP_CAT(make_, BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ)))()(a0, a...); \
    }                                                                           \
/**/

#else // BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_VARARG_R(Z, N, NAME)    \
    template <BOOST_PHOENIX_typename_A(N)>                                      \
    inline                                                                      \
    typename                                                                    \
        result_of:: BOOST_PP_CAT(make_, NAME)<                                  \
            BOOST_PHOENIX_A(N)                                                  \
        >::type                                                                 \
    BOOST_PP_CAT(make_, NAME)(BOOST_PHOENIX_A_const_ref_a(N))                   \
    {                                                                           \
        return functional::BOOST_PP_CAT(make_, NAME)()(BOOST_PHOENIX_a(N));     \
    }                                                                           \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_VARARG(NAME_SEQ, GRAMMAR_SEQ, LIMIT) \
    BOOST_PP_REPEAT_FROM_TO(                                                    \
        1                                                                       \
      , LIMIT                                                                   \
      , BOOST_PHOENIX_DEFINE_EXPRESSION_MAKE_EXPRESSION_VARARG_R                \
      , BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                       \
    )                                                                           \
/**/
#endif // BOOST_PHOENIX_NO_VARIADIC_EXPRESSION

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXPRESSION_EXT(NAME_SEQ, GRAMMAR_SEQ, ACTOR) \
        template <BOOST_PHOENIX_typename_A(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))>     \
        struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))                \
            : ::boost::phoenix::expr_ext<                                       \
                ACTOR                                                           \
              , tag:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(NAME_SEQ))         \
              , BOOST_PHOENIX_A(BOOST_PP_SEQ_SIZE(GRAMMAR_SEQ))>                \
        {};                                                                     \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXT_VARARG_R(_, N, NAME)                \
    template <                                                                  \
        BOOST_PHOENIX_typename_A(                                               \
            BOOST_PP_ADD(                                                       \
                N                                                               \
              , BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3, 1, NAME))              \
            )                                                                   \
        )                                                                       \
    >                                                                           \
    struct BOOST_PP_TUPLE_ELEM(3, 0, NAME)<                                     \
        BOOST_PHOENIX_A(                                                        \
            BOOST_PP_ADD(N, BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3, 1, NAME))) \
        )                                                                       \
    >                                                                           \
        : expr_ext<                                                             \
            BOOST_PP_TUPLE_ELEM(3, 2, NAME)                                     \
          , tag:: BOOST_PP_TUPLE_ELEM(3, 0, NAME)                               \
          , BOOST_PHOENIX_A(                                                    \
                BOOST_PP_ADD(                                                   \
                    N                                                           \
                  , BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3, 1, NAME))          \
                )                                                               \
            )                                                                   \
        >                                                                       \
    {};                                                                         \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_EXRPESSION_VARARG_EXT(N, G, D)          \
        template <                                                              \
            BOOST_PHOENIX_typename_A_void(                                      \
                BOOST_PP_ADD(LIMIT, BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(G)))         \
            )                                                                   \
          , typename Dummy = void                                               \
        >                                                                       \
        struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(N));                      \
                                                                                \
        BOOST_PP_REPEAT_FROM_TO(                                                \
            1                                                                   \
          , BOOST_PP_ADD(LIMIT, BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(G)))             \
          , BOOST_PHOENIX_DEFINE_EXPRESSION_EXT_VARARG_R                        \
          , (                                                                   \
              BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(N))                        \
            , BOOST_PP_SEQ_POP_BACK(G)                                          \
            , ACTOR                                                             \
          )                                                                     \
        )                                                                       \
/**/

#define BOOST_PHOENIX_DEFINE_EXPRESSION_RULE_VARARG_EXT(N, GRAMMAR, D)          \
        struct BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(N))                       \
            : expression:: BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(N)) <         \
                BOOST_PP_IF(                                                    \
                    BOOST_PP_EQUAL(1, BOOST_PP_SEQ_SIZE(GRAMMAR))               \
                  , BOOST_PP_EMPTY                                              \
                  , BOOST_PP_IDENTITY(                                          \
                      BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_POP_BACK(GRAMMAR))         \
                    )                                                           \
                )()                                                             \
                BOOST_PP_COMMA_IF(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(GRAMMAR)))     \
                proto::vararg<                                                  \
                    BOOST_PP_SEQ_ELEM(                                          \
                        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(GRAMMAR))                \
                      , GRAMMAR                                                 \
                    )                                                           \
                >                                                               \
            >                                                                   \
        {};                                                                     \


#endif
