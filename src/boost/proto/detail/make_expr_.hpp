#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/make_expr_.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/make_expr_.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file make_expr_.hpp
    /// Contains definition of make_expr_\<\> class template.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    template<
        typename Tag
      , typename Domain
        BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(
            BOOST_PROTO_MAX_ARITY
          , typename A
          , = void BOOST_PP_INTERCEPT
        )
      , typename _ = void
    >
    struct make_expr_
    {};

    template<typename Domain, typename A>
    struct make_expr_<tag::terminal, Domain, A
        BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, void BOOST_PP_INTERCEPT)>
    {
        typedef typename proto::detail::protoify<A, Domain>::result_type result_type;

        BOOST_FORCEINLINE
        result_type operator()(typename add_reference<A>::type a) const
        {
            return proto::detail::protoify<A, Domain>()(a);
        }
    };

    template<typename A>
    struct make_expr_<tag::terminal, deduce_domain, A
        BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, void BOOST_PP_INTERCEPT)>
      : make_expr_<tag::terminal, default_domain, A>
    {};

    #define BOOST_PP_ITERATION_PARAMS_1                                                         \
        (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/make_expr_.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

#else // BOOST_PP_IS_ITERATING

    #define N BOOST_PP_ITERATION()
    #define M BOOST_PP_SUB(BOOST_PROTO_MAX_ARITY, N)

    template<typename Tag, typename Domain BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct make_expr_<Tag, Domain BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
        BOOST_PP_ENUM_TRAILING_PARAMS(M, void BOOST_PP_INTERCEPT), void>
    {
        typedef
            BOOST_PP_CAT(list, N)<
                BOOST_PP_ENUM(N, BOOST_PROTO_AS_CHILD_TYPE, (A, ~, Domain))
            >
        proto_args;

        typedef typename base_expr<Domain, Tag, proto_args>::type expr_type;
        typedef typename Domain::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;

        BOOST_FORCEINLINE
        result_type operator()(BOOST_PP_ENUM_BINARY_PARAMS(N, typename add_reference<A, >::type a)) const
        {
            expr_type const that = {
                BOOST_PP_ENUM(N, BOOST_PROTO_AS_CHILD, (A, a, Domain))
            };
            return proto_generator()(that);
        }
    };

    template<typename Tag BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct make_expr_<Tag, deduce_domain BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
        BOOST_PP_ENUM_TRAILING_PARAMS(M, void BOOST_PP_INTERCEPT), void>
      : make_expr_<
            Tag
          , typename BOOST_PP_CAT(deduce_domain, N)<BOOST_PP_ENUM_PARAMS(N, A)>::type
            BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
        >
    {};

    #undef N
    #undef M

#endif
