#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/transform/detail/preprocessed/pass_through_impl.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #define BOOST_PROTO_DEFINE_TRANSFORM_TYPE(Z, N, DATA)                                   \
        typename Grammar::BOOST_PP_CAT(proto_child, N)::template impl<                      \
            typename result_of::child_c<Expr, N>::type                                      \
          , State                                                                           \
          , Data                                                                            \
        >::result_type                                                                      \
        /**/

    #define BOOST_PROTO_DEFINE_TRANSFORM(Z, N, DATA)                                        \
        typename Grammar::BOOST_PP_CAT(proto_child, N)::template impl<                      \
            typename result_of::child_c<Expr, N>::type                                      \
          , State                                                                           \
          , Data                                                                            \
        >()(                                                                                \
            e.proto_base().BOOST_PP_CAT(child, N), s, d                                     \
        )                                                                                   \
        /**/

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/pass_through_impl.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file pass_through_impl.hpp
    ///
    /// Specializations of pass_through_impl, used in the implementation of the
    /// pass_through transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/transform/detail/pass_through_impl.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_DEFINE_TRANSFORM
    #undef BOOST_PROTO_DEFINE_TRANSFORM_TYPE

#else

    #define N BOOST_PP_ITERATION()

    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, N>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;

        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , BOOST_PP_CAT(list, N)<
                    BOOST_PP_ENUM(N, BOOST_PROTO_DEFINE_TRANSFORM_TYPE, ~)
                >
            >::type
        expr_type;

        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;

        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                BOOST_PP_ENUM(N, BOOST_PROTO_DEFINE_TRANSFORM, ~)
            };
            // Without this, MSVC complains that "that" is uninitialized,
            // and it actually triggers a runtime check in debug mode when
            // built with VC8.
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };

    #undef N

#endif
