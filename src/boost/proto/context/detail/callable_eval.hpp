#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/context/detail/preprocessed/callable_eval.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #define BOOST_PROTO_CHILD_N_TYPE(Z, N, Expr)                                                    \
        typedef typename proto::result_of::child_c<Expr const &, N>::type BOOST_PP_CAT(child, N);   \
        /**/

    #define BOOST_PROTO_CHILD_N(Z, N, expr)                                                         \
        proto::child_c<N>(expr)                                                                     \
        /**/

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/callable_eval.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file callable_eval.hpp
    /// Contains specializations of the callable_eval\<\> class template.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/context/detail/callable_eval.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_CHILD_N_TYPE
    #undef BOOST_PROTO_CHILD_N

#else

    #define N BOOST_PP_ITERATION()

    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, N>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;

            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            BOOST_PP_ENUM_TRAILING(N, BOOST_PROTO_CHILD_N, sexpr_)
                        ), 0)
                    )
                );

            typedef mpl::bool_<value> type;
        };
    }

    namespace context
    {
        /// \brief A BinaryFunction that accepts a Proto expression and a
        /// callable context and calls the context with the expression tag
        /// and children as arguments, effectively fanning the expression
        /// out.
        ///
        /// <tt>callable_eval\<\></tt> requires that \c Context is a
        /// PolymorphicFunctionObject that can be invoked with \c Expr's
        /// tag and children as expressions, as follows:
        ///
        /// \code
        /// context(Expr::proto_tag(), child_c\<0\>(expr), child_c\<1\>(expr), ...)
        /// \endcode
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, N>
        {
            BOOST_PP_REPEAT(N, BOOST_PROTO_CHILD_N_TYPE, Expr)

            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        BOOST_PP_ENUM_TRAILING_PARAMS(N, child)
                    )
                >::type
            result_type;

            /// \param expr The current expression
            /// \param context The callable evaluation context
            /// \return <tt>context(Expr::proto_tag(), child_c\<0\>(expr), child_c\<1\>(expr), ...)</tt>
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    BOOST_PP_ENUM_TRAILING(N, BOOST_PROTO_CHILD_N, expr)
                );
            }
        };
    }

    #undef N

#endif
