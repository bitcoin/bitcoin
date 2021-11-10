/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_PP_IS_ITERATING)
#if !defined(BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_FORWARD_CTOR_04122006_2212)
#define BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_FORWARD_CTOR_04122006_2212

#if defined(BOOST_FUSION_HAS_VARIADIC_DEQUE)
#error "C++03 only! This file should not have been included"
#endif

#define FUSION_DEQUE_FORWARD_CTOR_FORWARD(z, n, _)    BOOST_FUSION_FWD_ELEM(T_##n, t##n)

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1 \
    <boost/fusion/container/deque/detail/cpp03/deque_forward_ctor.hpp>
#define BOOST_PP_ITERATION_LIMITS (2, FUSION_MAX_DEQUE_SIZE)
#include BOOST_PP_ITERATE()

#undef FUSION_DEQUE_FORWARD_CTOR_FORWARD
#endif
#else

#define N BOOST_PP_ITERATION()

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#endif
#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    (defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES))
BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
deque(BOOST_PP_ENUM_BINARY_PARAMS(N, typename detail::call_param<T, >::type t))
    : base(detail::deque_keyed_values<BOOST_PP_ENUM_PARAMS(N, T)>::construct(BOOST_PP_ENUM_PARAMS(N, t)))
{}
#endif
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH endif
#endif

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#endif
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    (defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES))
BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
deque(BOOST_PP_ENUM_BINARY_PARAMS(N, T, const& t))
    : base(detail::deque_keyed_values<BOOST_PP_ENUM_PARAMS(N, T)>::construct(BOOST_PP_ENUM_PARAMS(N, t)))
{}

template <BOOST_PP_ENUM_PARAMS(N, typename T_)>
BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
deque(BOOST_PP_ENUM_BINARY_PARAMS(N, T_, && t))
    : base(detail::deque_keyed_values<BOOST_PP_ENUM_PARAMS(N, T)>::
      forward_(BOOST_PP_ENUM(N, FUSION_DEQUE_FORWARD_CTOR_FORWARD, _)))
{}
#endif
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH endif
#endif

#undef N
#endif
