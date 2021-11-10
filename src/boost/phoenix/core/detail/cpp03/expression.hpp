
#if !BOOST_PHOENIX_IS_ITERATING

#include <boost/phoenix/support/iterate.hpp>
#include <boost/preprocessor/comparison/equal.hpp>

#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)

#include <boost/phoenix/core/detail/cpp03/preprocessed/expression.hpp>

#else

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/expression_" BOOST_PHOENIX_LIMIT_STR ".hpp")
#endif

/*=============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

    template <
        template <typename> class Actor
      , typename Tag
      , BOOST_PHOENIX_typename_A_void(BOOST_PHOENIX_COMPOSITE_LIMIT)
      , typename Dummy = void>
    struct expr_ext;

    template <
        typename Tag
      , BOOST_PHOENIX_typename_A_void(BOOST_PHOENIX_COMPOSITE_LIMIT)
      , typename Dummy = void
    >
    struct expr : expr_ext<actor, Tag, BOOST_PHOENIX_A(BOOST_PHOENIX_COMPOSITE_LIMIT)> {};

#define M0(Z, N, D)                                                             \
    BOOST_PP_COMMA_IF(N)                                                        \
    typename proto::detail::uncvref<BOOST_PP_CAT(A, N)>::type

#define M1(Z, N, D)                                                             \
    BOOST_PP_COMMA_IF(N) typename boost::add_reference<typename boost::add_const<BOOST_PP_CAT(A, N)>::type>::type BOOST_PP_CAT(a, N)

#define BOOST_PHOENIX_ITERATION_PARAMS                                          \
    (3, (1, BOOST_PHOENIX_COMPOSITE_LIMIT,                                      \
    <boost/phoenix/core/detail/cpp03/expression.hpp>))                          \
/**/
    #include BOOST_PHOENIX_ITERATE()

#undef M0
#undef M1

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif // PHOENIX_DONT_USE_PREPROCESSED_FILES

#else
    template <template <typename> class Actor, typename Tag, BOOST_PHOENIX_typename_A>
    struct expr_ext<Actor, Tag, BOOST_PHOENIX_A>
        : proto::transform<expr_ext<Actor, Tag, BOOST_PHOENIX_A>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain //proto::basic_default_domain
              , BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, M0, _)
            >::type
            base_type;

        typedef Actor<base_type> type;

        typedef
            typename proto::nary_expr<Tag, BOOST_PHOENIX_A>::proto_grammar
            proto_grammar;

        static type make(BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, M1, _))
      { //?? actor or Actor??
        //Actor<base_type> const e =
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain // proto::basic_default_domain
                    >(BOOST_PHOENIX_a)
                };
            return e;
        }

        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};

        typedef Tag proto_tag;
    #define BOOST_PHOENIX_ENUM_CHILDREN(_, N, __)                               \
        typedef BOOST_PP_CAT(A, N) BOOST_PP_CAT(proto_child, N);                \
    /**/
        BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, BOOST_PHOENIX_ENUM_CHILDREN, _)
    #undef BOOST_PHOENIX_ENUM_CHILDREN
    };

#endif
