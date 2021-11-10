#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/lambda_matches.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #define BOOST_PROTO_DEFINE_LAMBDA_MATCHES(Z, N, DATA)                                           \
        lambda_matches<                                                                             \
            BOOST_PP_CAT(Expr, N)                                                                   \
          , BOOST_PP_CAT(Grammar, N)                                                                \
        >

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/lambda_matches.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file lambda_matches.hpp
    /// Specializations of the lambda_matches template
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (2, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/lambda_matches.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_DEFINE_LAMBDA_MATCHES

#else // BOOST_PP_IS_ITERATING

    #define N BOOST_PP_ITERATION()

    template<
        template<BOOST_PP_ENUM_PARAMS(N, typename BOOST_PP_INTERCEPT)> class T
        BOOST_PP_ENUM_TRAILING_PARAMS(N, typename Expr)
        BOOST_PP_ENUM_TRAILING_PARAMS(N, typename Grammar)
    >
    struct lambda_matches<
        T<BOOST_PP_ENUM_PARAMS(N, Expr)>
      , T<BOOST_PP_ENUM_PARAMS(N, Grammar)>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(N)
    >
      : BOOST_PP_CAT(and_, N)<
            BOOST_PROTO_DEFINE_LAMBDA_MATCHES(~, 0, ~)::value,
            BOOST_PP_ENUM_SHIFTED(N, BOOST_PROTO_DEFINE_LAMBDA_MATCHES, ~)
        >
    {};

    #undef N

#endif
