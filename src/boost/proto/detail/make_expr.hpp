#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/make_expr.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/make_expr.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file make_expr.hpp
    /// Contains overloads of make_expr() free function.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (2, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/make_expr.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

#else // BOOST_PP_IS_ITERATING

    #define N BOOST_PP_ITERATION()

    /// \overload
    ///
    template<typename Tag BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            BOOST_PP_ENUM_TRAILING_PARAMS(N, const A)
        >
    >::type const
    make_expr(BOOST_PP_ENUM_BINARY_PARAMS(N, const A, &a))
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            BOOST_PP_ENUM_TRAILING_PARAMS(N, const A)
        >()(BOOST_PP_ENUM_PARAMS(N, a));
    }

    /// \overload
    ///
    template<typename Tag, typename Domain BOOST_PP_ENUM_TRAILING_PARAMS(N, typename C)>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        BOOST_PP_ENUM_TRAILING_PARAMS(N, const C)
    >::type const
    make_expr(BOOST_PP_ENUM_BINARY_PARAMS(N, const C, &c))
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            BOOST_PP_ENUM_TRAILING_PARAMS(N, const C)
        >()(BOOST_PP_ENUM_PARAMS(N, c));
    }

    #undef N

#endif
