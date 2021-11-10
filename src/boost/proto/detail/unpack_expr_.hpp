#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/unpack_expr_.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_NEXT_ITERATOR_TYPE(Z, N, DATA)                                       \
        typedef typename fusion::result_of::next<                                                   \
            BOOST_PP_CAT(fusion_iterator, N)>::type                                                 \
                BOOST_PP_CAT(fusion_iterator, BOOST_PP_INC(N));                                     \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_ITERATORS_TYPE(N)                                                    \
        typedef                                                                                     \
            typename fusion::result_of::begin<Sequence const>::type                                 \
        fusion_iterator0;                                                                           \
        BOOST_PP_REPEAT(BOOST_PP_DEC(N), BOOST_PROTO_FUSION_NEXT_ITERATOR_TYPE, fusion_iterator)    \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AT_TYPE(Z, N, DATA)                                                  \
        typename add_const<                                                                         \
            typename fusion::result_of::value_of<                                                   \
                BOOST_PP_CAT(fusion_iterator, N)                                                    \
            >::type                                                                                 \
        >::type                                                                                     \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_NEXT_ITERATOR(Z, N, DATA)                                            \
        BOOST_PP_CAT(fusion_iterator, BOOST_PP_INC(N)) BOOST_PP_CAT(it, BOOST_PP_INC(N)) =          \
            fusion::next(BOOST_PP_CAT(it, N));                                                      \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_ITERATORS(N)                                                         \
        fusion_iterator0 it0 = fusion::begin(sequence);                                             \
        BOOST_PP_REPEAT(BOOST_PP_DEC(N), BOOST_PROTO_FUSION_NEXT_ITERATOR, fusion_iterator)         \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AT(Z, N, DATA)                                                       \
        *BOOST_PP_CAT(it, N)                                                                        \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AS_CHILD_AT_TYPE(Z, N, DATA)                                         \
        typename detail::protoify<                                                                  \
            BOOST_PROTO_FUSION_AT_TYPE(Z, N, DATA)                                                  \
          , Domain                                                                                  \
        >::result_type                                                                              \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AS_CHILD_AT(Z, N, DATA)                                              \
        detail::protoify<                                                                           \
            BOOST_PROTO_FUSION_AT_TYPE(Z, N, DATA)                                                  \
          , Domain                                                                                  \
        >()(BOOST_PROTO_FUSION_AT(Z, N, DATA))                                                      \
        /**/

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/unpack_expr_.hpp")
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

    template<typename Tag, typename Domain, typename Sequence, std::size_t Size>
    struct unpack_expr_
    {};

    template<typename Domain, typename Sequence>
    struct unpack_expr_<tag::terminal, Domain, Sequence, 1u>
    {
        typedef
            typename add_const<
                typename fusion::result_of::value_of<
                    typename fusion::result_of::begin<Sequence>::type
                >::type
            >::type
        terminal_type;

        typedef
            typename proto::detail::protoify<
                terminal_type
              , Domain
            >::result_type
        type;

        BOOST_FORCEINLINE
        static type const call(Sequence const &sequence)
        {
            return proto::detail::protoify<terminal_type, Domain>()(fusion::at_c<0>(sequence));
        }
    };

    template<typename Sequence>
    struct unpack_expr_<tag::terminal, deduce_domain, Sequence, 1u>
      : unpack_expr_<tag::terminal, default_domain, Sequence, 1u>
    {};

    #define BOOST_PP_ITERATION_PARAMS_1                                                         \
        (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/unpack_expr_.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

    #undef BOOST_PROTO_FUSION_AT
    #undef BOOST_PROTO_FUSION_AT_TYPE
    #undef BOOST_PROTO_FUSION_AS_CHILD_AT
    #undef BOOST_PROTO_FUSION_AS_CHILD_AT_TYPE
    #undef BOOST_PROTO_FUSION_NEXT_ITERATOR
    #undef BOOST_PROTO_FUSION_NEXT_ITERATOR_TYPE
    #undef BOOST_PROTO_FUSION_ITERATORS
    #undef BOOST_PROTO_FUSION_ITERATORS_TYPE

#else // BOOST_PP_IS_ITERATING

    #define N BOOST_PP_ITERATION()
    #define M BOOST_PP_SUB(BOOST_PROTO_MAX_ARITY, N)

    template<typename Tag, typename Domain, typename Sequence>
    struct unpack_expr_<Tag, Domain, Sequence, N>
    {
        BOOST_PROTO_FUSION_ITERATORS_TYPE(N)

        typedef
            BOOST_PP_CAT(list, N)<
                BOOST_PP_ENUM(N, BOOST_PROTO_FUSION_AS_CHILD_AT_TYPE, ~)
            >
        proto_args;

        typedef typename base_expr<Domain, Tag, proto_args>::type expr_type;
        typedef typename Domain::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type type;

        BOOST_FORCEINLINE
        static type const call(Sequence const &sequence)
        {
            BOOST_PROTO_FUSION_ITERATORS(N)
            expr_type const that = {
                BOOST_PP_ENUM(N, BOOST_PROTO_FUSION_AS_CHILD_AT, ~)
            };
            return proto_generator()(that);
        }
    };

    template<typename Tag, typename Sequence>
    struct unpack_expr_<Tag, deduce_domain, Sequence, N>
    {
        BOOST_PROTO_FUSION_ITERATORS_TYPE(N)

        typedef
            unpack_expr_<
                Tag
              , typename BOOST_PP_CAT(deduce_domain, N)<
                    BOOST_PP_ENUM(N, BOOST_PROTO_FUSION_AT_TYPE, ~)
                >::type
              , Sequence
              , N
            >
        other;

        typedef typename other::type type;

        BOOST_FORCEINLINE
        static type const call(Sequence const &sequence)
        {
            return other::call(sequence);
        }
    };

    #undef N
    #undef M

#endif
