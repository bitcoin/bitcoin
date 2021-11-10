#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/deduce_domain_n.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #define BOOST_PROTO_COMMON_DOMAIN2(Z, N, DATA)                                                  \
        typedef                                                                                     \
            typename common_domain2<common ## N, A ## N>::type                                      \
        BOOST_PP_CAT(common, BOOST_PP_INC(N));                                                      \
        /**/

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/deduce_domain_n.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    // deduce_domain_n.hpp
    // Definitions of common_domain[n] and deduce_domain[n] class templates.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (3, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/deduce_domain_n.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_COMMON_DOMAIN2

#else

    #define N BOOST_PP_ITERATION()

    template<BOOST_PP_ENUM_PARAMS(N, typename A)>
    struct BOOST_PP_CAT(common_domain, N)
    {
        typedef A0 common1;
        BOOST_PP_REPEAT_FROM_TO(1, N, BOOST_PROTO_COMMON_DOMAIN2, ~)
        typedef BOOST_PP_CAT(common, N) type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };

    template<BOOST_PP_ENUM_PARAMS(N, typename E)>
    struct BOOST_PP_CAT(deduce_domain, N)
      : BOOST_PP_CAT(common_domain, N)<
            BOOST_PP_ENUM_BINARY_PARAMS(
                N
              , typename domain_of<E, >::type BOOST_PP_INTERCEPT
            )
        >
    {};

    #undef N

#endif // BOOST_PROTO_DONT_USE_PREPROCESSED_FILES
