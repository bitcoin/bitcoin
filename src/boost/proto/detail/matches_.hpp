#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/matches_.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #define BOOST_PROTO_MATCHES_N_FUN(Z, N, DATA)                                                   \
        matches_<                                                                                   \
            typename detail::expr_traits<typename Args1::BOOST_PP_CAT(child, N)>::value_type::proto_derived_expr \
          , typename detail::expr_traits<typename Args1::BOOST_PP_CAT(child, N)>::value_type::proto_grammar \
          , typename Args2::BOOST_PP_CAT(child, N)::proto_grammar                                   \
        >

    #define BOOST_PROTO_DEFINE_MATCHES(Z, N, DATA)                                                  \
        matches_<                                                                                   \
            Expr                                                                                    \
          , BasicExpr                                                                               \
          , typename BOOST_PP_CAT(G, N)::proto_grammar                                              \
        >

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/matches_.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file matches_.hpp
    /// Definitions of matches_ specializations
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (2, BOOST_PP_MAX(BOOST_PROTO_MAX_ARITY, BOOST_PROTO_MAX_LOGICAL_ARITY), <boost/proto/detail/matches_.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_DEFINE_MATCHES
    #undef BOOST_PROTO_MATCHES_N_FUN

#else // BOOST_PP_IS_ITERATING

    #define N BOOST_PP_ITERATION()

    #if N <= BOOST_PROTO_MAX_LOGICAL_ARITY

        // handle proto::or_
        template<typename Expr, typename BasicExpr BOOST_PP_ENUM_TRAILING_PARAMS(N, typename G)>
        struct matches_<Expr, BasicExpr, proto::or_<BOOST_PP_ENUM_PARAMS(N, G)> >
          : BOOST_PP_CAT(or_, N)<
                matches_<Expr, BasicExpr, typename G0::proto_grammar>::value,
                Expr, BasicExpr BOOST_PP_ENUM_TRAILING_PARAMS(N, G)
            >
        {};

        // handle proto::and_
        template<typename Expr, typename BasicExpr, BOOST_PP_ENUM_PARAMS(N, typename G)>
        struct matches_<Expr, BasicExpr, proto::and_<BOOST_PP_ENUM_PARAMS(N, G)> >
          : detail::BOOST_PP_CAT(and_, N)<
                BOOST_PROTO_DEFINE_MATCHES(~, 0, ~)::value,
                BOOST_PP_ENUM_SHIFTED(N, BOOST_PROTO_DEFINE_MATCHES, ~)
            >
        {};

    #endif

    #if N <= BOOST_PROTO_MAX_ARITY

        template<typename Expr, typename Tag, typename Args1, typename Args2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, N>, proto::basic_expr<Tag, Args2, N> >
          : BOOST_PP_CAT(and_, N)<
                BOOST_PROTO_MATCHES_N_FUN(~, 0, ~)::value,
                BOOST_PP_ENUM_SHIFTED(N, BOOST_PROTO_MATCHES_N_FUN, ~)
            >
        {};

        template<typename Expr, typename Tag, typename Args1, typename Args2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, N>, proto::basic_expr<proto::_, Args2, N> >
          : BOOST_PP_CAT(and_, N)<
                BOOST_PROTO_MATCHES_N_FUN(~, 0, ~)::value,
                BOOST_PP_ENUM_SHIFTED(N, BOOST_PROTO_MATCHES_N_FUN, ~)
            >
        {};

    #endif

    #undef N

#endif
