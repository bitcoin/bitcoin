#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/traits.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #define BOOST_PROTO_CHILD(Z, N, DATA)                                                           \
        /** INTERNAL ONLY */                                                                        \
        typedef BOOST_PP_CAT(DATA, N) BOOST_PP_CAT(proto_child, N);                                 \
        /**/

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/traits.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file traits.hpp
    /// Definitions of proto::function, proto::nary_expr and proto::result_of::child_c
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (0, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/traits.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_CHILD

#else // BOOST_PP_IS_ITERATING

    #define N BOOST_PP_ITERATION()

    #if N > 0
        /// \brief A metafunction for generating function-call expression types,
        /// a grammar element for matching function-call expressions, and a
        /// PrimitiveTransform that dispatches to the <tt>pass_through\<\></tt>
        /// transform.
        template<BOOST_PP_ENUM_PARAMS(N, typename A)>
        struct function
        #if N != BOOST_PROTO_MAX_ARITY
        <
            BOOST_PP_ENUM_PARAMS(N, A)
            BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_SUB(BOOST_PROTO_MAX_ARITY, N), void BOOST_PP_INTERCEPT)
        >
        #endif
          : proto::transform<
                function<
                    BOOST_PP_ENUM_PARAMS(N, A)
                    BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_SUB(BOOST_PROTO_MAX_ARITY, N), void BOOST_PP_INTERCEPT)
                >
              , int
            >
        {
            typedef proto::expr<proto::tag::function, BOOST_PP_CAT(list, N)<BOOST_PP_ENUM_PARAMS(N, A)>, N> type;
            typedef proto::basic_expr<proto::tag::function, BOOST_PP_CAT(list, N)<BOOST_PP_ENUM_PARAMS(N, A)>, N> proto_grammar;

            template<typename Expr, typename State, typename Data>
            struct impl
              : detail::pass_through_impl<function, deduce_domain, Expr, State, Data>
            {};

            /// INTERNAL ONLY
            typedef proto::tag::function proto_tag;
            BOOST_PP_REPEAT(N, BOOST_PROTO_CHILD, A)
            BOOST_PP_REPEAT_FROM_TO(
                N
              , BOOST_PROTO_MAX_ARITY
              , BOOST_PROTO_CHILD
              , detail::if_vararg<BOOST_PP_CAT(A, BOOST_PP_DEC(N))> BOOST_PP_INTERCEPT
            )
        };

        /// \brief A metafunction for generating n-ary expression types with a
        /// specified tag type,
        /// a grammar element for matching n-ary expressions, and a
        /// PrimitiveTransform that dispatches to the <tt>pass_through\<\></tt>
        /// transform.
        ///
        /// Use <tt>nary_expr\<_, vararg\<_\> \></tt> as a grammar element to match any
        /// n-ary expression; that is, any non-terminal.
        template<typename Tag BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct nary_expr
        #if N != BOOST_PROTO_MAX_ARITY
        <
            Tag
            BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
            BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_SUB(BOOST_PROTO_MAX_ARITY, N), void BOOST_PP_INTERCEPT)
        >
        #endif
          : proto::transform<
                nary_expr<
                    Tag
                    BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
                    BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_SUB(BOOST_PROTO_MAX_ARITY, N), void BOOST_PP_INTERCEPT)
                >
              , int
            >
        {
            typedef proto::expr<Tag, BOOST_PP_CAT(list, N)<BOOST_PP_ENUM_PARAMS(N, A)>, N> type;
            typedef proto::basic_expr<Tag, BOOST_PP_CAT(list, N)<BOOST_PP_ENUM_PARAMS(N, A)>, N> proto_grammar;

            template<typename Expr, typename State, typename Data>
            struct impl
              : detail::pass_through_impl<nary_expr, deduce_domain, Expr, State, Data>
            {};

            /// INTERNAL ONLY
            typedef Tag proto_tag;
            BOOST_PP_REPEAT(N, BOOST_PROTO_CHILD, A)
            BOOST_PP_REPEAT_FROM_TO(
                N
              , BOOST_PROTO_MAX_ARITY
              , BOOST_PROTO_CHILD
              , detail::if_vararg<BOOST_PP_CAT(A, BOOST_PP_DEC(N))> BOOST_PP_INTERCEPT
            )
        };

        namespace detail
        {
            template<
                template<BOOST_PP_ENUM_PARAMS(N, typename BOOST_PP_INTERCEPT)> class T
              , BOOST_PP_ENUM_PARAMS(N, typename A)
            >
            struct is_callable_<T<BOOST_PP_ENUM_PARAMS(N, A)> BOOST_PROTO_TEMPLATE_ARITY_PARAM(N)>
              : is_same<BOOST_PP_CAT(A, BOOST_PP_DEC(N)), callable>
            {};
        }

    #endif

        namespace result_of
        {
            /// \brief A metafunction that returns the type of the Nth child
            /// of a Proto expression.
            ///
            /// A metafunction that returns the type of the Nth child
            /// of a Proto expression. \c N must be less than
            /// \c Expr::proto_arity::value.
            template<typename Expr>
            struct child_c<Expr, N>
            {
                /// Verify that we are not operating on a terminal
                BOOST_STATIC_ASSERT(0 != Expr::proto_arity_c);

                /// The raw type of the Nth child as it is stored within
                /// \c Expr. This may be a value or a reference
                typedef typename Expr::BOOST_PP_CAT(proto_child, N) value_type;

                /// The "value" type of the child, suitable for return by value,
                /// computed as follows:
                /// \li <tt>T const &</tt> becomes <tt>T</tt>
                /// \li <tt>T &</tt> becomes <tt>T</tt>
                /// \li <tt>T</tt> becomes <tt>T</tt>
                typedef typename detail::expr_traits<typename Expr::BOOST_PP_CAT(proto_child, N)>::value_type type;
            };

            template<typename Expr>
            struct child_c<Expr &, N>
            {
                /// Verify that we are not operating on a terminal
                BOOST_STATIC_ASSERT(0 != Expr::proto_arity_c);

                /// The raw type of the Nth child as it is stored within
                /// \c Expr. This may be a value or a reference
                typedef typename Expr::BOOST_PP_CAT(proto_child, N) value_type;

                /// The "reference" type of the child, suitable for return by
                /// reference, computed as follows:
                /// \li <tt>T const &</tt> becomes <tt>T const &</tt>
                /// \li <tt>T &</tt> becomes <tt>T &</tt>
                /// \li <tt>T</tt> becomes <tt>T &</tt>
                typedef typename detail::expr_traits<typename Expr::BOOST_PP_CAT(proto_child, N)>::reference type;

                /// INTERNAL ONLY
                ///
                BOOST_FORCEINLINE
                static type call(Expr &e)
                {
                    return e.proto_base().BOOST_PP_CAT(child, N);
                }
            };

            template<typename Expr>
            struct child_c<Expr const &, N>
            {
                /// Verify that we are not operating on a terminal
                BOOST_STATIC_ASSERT(0 != Expr::proto_arity_c);

                /// The raw type of the Nth child as it is stored within
                /// \c Expr. This may be a value or a reference
                typedef typename Expr::BOOST_PP_CAT(proto_child, N) value_type;

                /// The "const reference" type of the child, suitable for return by
                /// const reference, computed as follows:
                /// \li <tt>T const &</tt> becomes <tt>T const &</tt>
                /// \li <tt>T &</tt> becomes <tt>T &</tt>
                /// \li <tt>T</tt> becomes <tt>T const &</tt>
                typedef typename detail::expr_traits<typename Expr::BOOST_PP_CAT(proto_child, N)>::const_reference type;

                /// INTERNAL ONLY
                ///
                BOOST_FORCEINLINE
                static type call(Expr const &e)
                {
                    return e.proto_base().BOOST_PP_CAT(child, N);
                }
            };
        }

    #undef N

#endif
