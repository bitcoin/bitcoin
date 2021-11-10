/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
// No include guard. This file is meant to be included many times

#if !defined(FUSION_MACRO_05042005)
#define FUSION_MACRO_05042005

#define FUSION_VECTOR_CTOR_DEFAULT_INIT(z, n, _)                                \
    m##n()

#define FUSION_VECTOR_CTOR_INIT(z, n, _)                                        \
    m##n(_##n)

#define FUSION_VECTOR_MEMBER_CTOR_INIT(z, n, _)                                 \
    m##n(other.m##n)

#define FUSION_VECTOR_CTOR_FORWARD(z, n, _)                                     \
   m##n(BOOST_FUSION_FWD_ELEM(T##n, other.m##n))

#define FUSION_VECTOR_CTOR_ARG_FWD(z, n, _)                                     \
   m##n(BOOST_FUSION_FWD_ELEM(U##n, _##n))

#define FUSION_VECTOR_MEMBER_DECL(z, n, _)                                      \
    T##n m##n;

#define FUSION_VECTOR_MEMBER_FORWARD(z, n, _)                                   \
   BOOST_FUSION_FWD_ELEM(U##n, _##n)

#define FUSION_VECTOR_MEMBER_ASSIGN(z, n, _)                                    \
    this->BOOST_PP_CAT(m, n) = vec.BOOST_PP_CAT(m, n);

#define FUSION_VECTOR_MEMBER_DEREF_ASSIGN(z, n, _)                              \
    this->BOOST_PP_CAT(m, n) = *BOOST_PP_CAT(i, n);

#define FUSION_VECTOR_MEMBER_MOVE(z, n, _)                                      \
    this->BOOST_PP_CAT(m, n) = std::forward<                                    \
        BOOST_PP_CAT(T, n)>(vec.BOOST_PP_CAT(m, n));

#define FUSION_VECTOR_MEMBER_AT_IMPL(z, n, _)                                   \
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED                              \
        typename add_reference<T##n>::type                                      \
        at_impl(mpl::int_<n>) { return this->m##n; }                            \
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                                    \
        typename add_reference<typename add_const<T##n>::type>::type            \
        at_impl(mpl::int_<n>) const { return this->m##n; }

#define FUSION_VECTOR_MEMBER_ITER_DECL_VAR(z, n, _)                             \
    typedef typename result_of::next<                                           \
        BOOST_PP_CAT(I, BOOST_PP_DEC(n))>::type BOOST_PP_CAT(I, n);             \
    BOOST_PP_CAT(I, n) BOOST_PP_CAT(i, n)                                       \
        = fusion::next(BOOST_PP_CAT(i, BOOST_PP_DEC(n)));

#endif

#define N BOOST_PP_ITERATION()

    template <BOOST_PP_ENUM_PARAMS(N, typename T)>
    struct BOOST_PP_CAT(vector_data, N)
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector_data, N)()
            : BOOST_PP_ENUM(N, FUSION_VECTOR_CTOR_DEFAULT_INIT, _) {}

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#endif
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    (defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES))
        template <BOOST_PP_ENUM_PARAMS(N, typename U)>
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector_data, N)(BOOST_PP_ENUM_BINARY_PARAMS(N, U, && arg)
          , typename boost::enable_if<is_convertible<U0, T0> >::type* /*dummy*/ = 0
        )
            : BOOST_PP_ENUM(N, FUSION_VECTOR_CTOR_ARG_FWD, arg) {}

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector_data, N)(
            BOOST_PP_CAT(vector_data, N)&& other)
            : BOOST_PP_ENUM(N, FUSION_VECTOR_CTOR_FORWARD, arg) {}
#endif
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH endif
#endif

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector_data, N)(
            BOOST_PP_ENUM_BINARY_PARAMS(
                N, typename detail::call_param<T, >::type arg))
            : BOOST_PP_ENUM(N, FUSION_VECTOR_CTOR_INIT, arg) {}

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector_data, N)(
            BOOST_PP_CAT(vector_data, N) const& other)
            : BOOST_PP_ENUM(N, FUSION_VECTOR_MEMBER_CTOR_INIT, _) {}

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector_data, N)&
        operator=(BOOST_PP_CAT(vector_data, N) const& vec)
        {
            BOOST_PP_REPEAT(N, FUSION_VECTOR_MEMBER_ASSIGN, _)
            return *this;
        }

        template <typename Sequence>
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
        static BOOST_PP_CAT(vector_data, N)
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            BOOST_PP_REPEAT_FROM_TO(1, N, FUSION_VECTOR_MEMBER_ITER_DECL_VAR, _)
            return BOOST_PP_CAT(vector_data, N)(BOOST_PP_ENUM_PARAMS(N, *i));
        }

        template <typename Sequence>
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
        static BOOST_PP_CAT(vector_data, N)
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            BOOST_PP_REPEAT_FROM_TO(1, N, FUSION_VECTOR_MEMBER_ITER_DECL_VAR, _)
            return BOOST_PP_CAT(vector_data, N)(BOOST_PP_ENUM_PARAMS(N, *i));
        }

        BOOST_PP_REPEAT(N, FUSION_VECTOR_MEMBER_DECL, _)
    };

    template <BOOST_PP_ENUM_PARAMS(N, typename T)>
    struct BOOST_PP_CAT(vector, N)
      : BOOST_PP_CAT(vector_data, N)<BOOST_PP_ENUM_PARAMS(N, T)>
      , sequence_base<BOOST_PP_CAT(vector, N)<BOOST_PP_ENUM_PARAMS(N, T)> >
    {
        typedef BOOST_PP_CAT(vector, N)<BOOST_PP_ENUM_PARAMS(N, T)> this_type;
        typedef BOOST_PP_CAT(vector_data, N)<BOOST_PP_ENUM_PARAMS(N, T)> base_type;
        typedef mpl::BOOST_PP_CAT(vector, N)<BOOST_PP_ENUM_PARAMS(N, T)> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; // this gets picked up by MPL
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<N> size;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)() {}

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
#if (N == 1)
        explicit
#endif
        BOOST_PP_CAT(vector, N)(
            BOOST_PP_ENUM_BINARY_PARAMS(
                N, typename detail::call_param<T, >::type arg))
            : base_type(BOOST_PP_ENUM_PARAMS(N, arg)) {}

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#endif
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    (defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES))
        template <BOOST_PP_ENUM_PARAMS(N, typename U)>
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
#if (N == 1)
        explicit
        BOOST_PP_CAT(vector, N)(U0&& _0
          , typename boost::enable_if<is_convertible<U0, T0> >::type* /*dummy*/ = 0
          )
         : base_type(BOOST_FUSION_FWD_ELEM(U0, _0)) {}
#else
        BOOST_PP_CAT(vector, N)(BOOST_PP_ENUM_BINARY_PARAMS(N, U, && arg))
            : base_type(BOOST_PP_ENUM(N, FUSION_VECTOR_MEMBER_FORWARD, arg)) {}
#endif

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)(BOOST_PP_CAT(vector, N)&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)(BOOST_PP_CAT(vector, N) const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)&
        operator=(BOOST_PP_CAT(vector, N) const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)&
        operator=(BOOST_PP_CAT(vector, N)&& vec)
        {
            BOOST_PP_REPEAT(N, FUSION_VECTOR_MEMBER_MOVE, _)
            return *this;
        }
#endif
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH endif
#endif

        template <BOOST_PP_ENUM_PARAMS(N, typename U)>
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)(
            BOOST_PP_CAT(vector, N)<BOOST_PP_ENUM_PARAMS(N, U)> const& vec)
            : base_type(BOOST_PP_ENUM_PARAMS(N, vec.m)) {}

        template <typename Sequence>
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
#if (N == 1)
          , typename boost::disable_if<is_convertible<Sequence, T0> >::type* /*dummy*/ = 0
#endif
            )
            : base_type(base_type::init_from_sequence(seq)) {}

        template <typename Sequence>
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
FUSION_HASH endif
#else
#if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
#endif
#endif
        BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
#if (N == 1)
          , typename boost::disable_if<is_convertible<Sequence, T0> >::type* /*dummy*/ = 0
#endif
            )
            : base_type(base_type::init_from_sequence(seq)) {}

        template <BOOST_PP_ENUM_PARAMS(N, typename U)>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_PP_CAT(vector, N)&
        operator=(BOOST_PP_CAT(vector, N)<BOOST_PP_ENUM_PARAMS(N, U)> const& vec)
        {
            BOOST_PP_REPEAT(N, FUSION_VECTOR_MEMBER_ASSIGN, _)
            return *this;
        }

        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            BOOST_PP_REPEAT_FROM_TO(1, N, FUSION_VECTOR_MEMBER_ITER_DECL_VAR, _)
            BOOST_PP_REPEAT(N, FUSION_VECTOR_MEMBER_DEREF_ASSIGN, _)
            return *this;
        }

        BOOST_PP_REPEAT(N, FUSION_VECTOR_MEMBER_AT_IMPL, _)

        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }

        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };

#undef N
