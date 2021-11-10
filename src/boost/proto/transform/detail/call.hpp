#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/transform/detail/preprocessed/call.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #define BOOST_PROTO_NTH_RESULT_TYPE(Z, M, DATA)                                                 \
        typedef                                                                                     \
            typename when<_, BOOST_PP_CAT(A, M)>::template impl<Expr, State, Data>                  \
        BOOST_PP_CAT(a, M);                                                                         \
        typedef typename BOOST_PP_CAT(a, M)::result_type BOOST_PP_CAT(b, M);                        \
        /**/

    #define BOOST_PROTO_NTH_RESULT(Z, M, DATA)                                                      \
        detail::as_lvalue(BOOST_PP_CAT(a, M)()(e, s, d))                                            \
        /**/

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/call.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file call.hpp
    /// Contains definition of the call<> transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/transform/detail/call.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_NTH_RESULT
    #undef BOOST_PROTO_NTH_RESULT_TYPE

#else

    #define N BOOST_PP_ITERATION()

    #if N > 3
    /// \brief Call the PolymorphicFunctionObject \c Fun with the
    /// current expression, state and data, transformed according
    /// to \c A0 through \c AN.
    template<typename Fun BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct call<Fun(BOOST_PP_ENUM_PARAMS(N, A))> : transform<call<Fun(BOOST_PP_ENUM_PARAMS(N, A))> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            BOOST_PP_REPEAT(N, BOOST_PROTO_NTH_RESULT_TYPE, ~)

            typedef detail::poly_function_traits<Fun, Fun(BOOST_PP_ENUM_PARAMS(N, b))> function_traits;
            typedef typename function_traits::result_type result_type;

            /// Let \c ax be <tt>when\<_, Ax\>()(e, s, d)</tt>
            /// for each \c x in <tt>[0,N]</tt>.
            /// Return <tt>Fun()(a0, a1,... aN)</tt>.
            ///
            /// \param e The current expression
            /// \param s The current state
            /// \param d An arbitrary data
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param   e
              , typename impl::state_param  s
              , typename impl::data_param   d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(BOOST_PP_ENUM(N, BOOST_PROTO_NTH_RESULT, ~));
            }
        };
    };
    #endif

    #if N > 0
    /// \brief Call the PolymorphicFunctionObject \c Fun with the
    /// current expression, state and data, transformed according
    /// to \c A0 through \c AN.
    template<typename Fun BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct call<Fun(BOOST_PP_ENUM_PARAMS(N, A)...)> : transform<call<Fun(BOOST_PP_ENUM_PARAMS(N, A)...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value // BUGBUG this isn't right. Could be pack(_child), should use arity of child!
                  , BOOST_PP_CAT(A, BOOST_PP_DEC(N))
                  , detail::BOOST_PP_CAT(expand_pattern_rest_, BOOST_PP_DEC(N))<
                        Fun
                        BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_DEC(N), A)
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    #endif

    #undef N

#endif
