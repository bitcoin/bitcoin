/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#if !defined(BOOST_SPIRIT_AS_VARIANT_NOVEMBER_16_2007_0420PM)
#define BOOST_SPIRIT_AS_VARIANT_NOVEMBER_16_2007_0420PM

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/variant/variant_fwd.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/next.hpp>
#include <boost/fusion/include/value_of.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/limits/list.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace detail
{
    template <int size>
    struct as_variant_impl;

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
#else
    template <>
    struct as_variant_impl<0>
    {
        template <typename Iterator>
        struct apply
        {
            typedef variant<> type;
        };
    };
#endif

#define BOOST_FUSION_NEXT_ITERATOR(z, n, data)                                  \
    typedef typename fusion::result_of::next<BOOST_PP_CAT(I, n)>::type          \
        BOOST_PP_CAT(I, BOOST_PP_INC(n));

#define BOOST_FUSION_NEXT_CALL_ITERATOR(z, n, data)                             \
    typename gen::BOOST_PP_CAT(I, BOOST_PP_INC(n))                              \
        BOOST_PP_CAT(i, BOOST_PP_INC(n)) = fusion::next(BOOST_PP_CAT(i, n));

#define BOOST_FUSION_VALUE_OF_ITERATOR(z, n, data)                              \
    typedef typename fusion::result_of::value_of<BOOST_PP_CAT(I, n)>::type      \
        BOOST_PP_CAT(T, n);

#define BOOST_PP_FILENAME_1 <boost/spirit/home/support/detail/as_variant.hpp>

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_MPL_LIMIT_LIST_SIZE)
#else
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_VARIANT_LIMIT_TYPES)
#endif

#include BOOST_PP_ITERATE()

#undef BOOST_FUSION_NEXT_ITERATOR
#undef BOOST_FUSION_NEXT_CALL_ITERATOR
#undef BOOST_FUSION_VALUE_OF_ITERATOR

    template <typename Sequence>
    struct as_variant
    {
        // build a variant generator being able to generate a variant holding
        // all of the types as given in the typelist
        typedef typename
            detail::as_variant_impl<fusion::result_of::size<Sequence>::value>
        gen;

        // use this generator to create the actual variant
        typedef typename gen::template apply<
                typename fusion::result_of::begin<Sequence>::type
            >::type
        type;
    };
}}}

#endif
#else // defined(BOOST_PP_IS_ITERATING)
///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////

#define N BOOST_PP_ITERATION()

    template <>
    struct as_variant_impl<N>
    {
        template <typename I0>
        struct apply
        {
            BOOST_PP_REPEAT(N, BOOST_FUSION_NEXT_ITERATOR, _)
            BOOST_PP_REPEAT(N, BOOST_FUSION_VALUE_OF_ITERATOR, _)
            typedef variant<BOOST_PP_ENUM_PARAMS(N, T)> type;
        };
    };

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)

