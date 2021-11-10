/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DEQUE_DETAIL_DEQUE_KEYED_VALUES_26112006_1330)
#define BOOST_FUSION_DEQUE_DETAIL_DEQUE_KEYED_VALUES_26112006_1330

#if defined(BOOST_FUSION_HAS_VARIADIC_DEQUE)
#error "C++03 only! This file should not have been included"
#endif

#include <boost/fusion/container/deque/detail/cpp03/limits.hpp>
#include <boost/fusion/container/deque/detail/keyed_element.hpp>

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/type_traits/add_reference.hpp>

#include <boost/mpl/plus.hpp>
#include <boost/mpl/int.hpp>

#define FUSION_VOID(z, n, _) void_

namespace boost { namespace fusion
{
    struct void_;
}}

#if !defined(BOOST_FUSION_DONT_USE_PREPROCESSED_FILES)
#include <boost/fusion/container/deque/detail/cpp03/preprocessed/deque_keyed_values.hpp>
#else
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/deque_keyed_values" FUSION_MAX_DEQUE_SIZE_STR ".hpp")
#endif

/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

namespace boost { namespace fusion { namespace detail
{
    template<typename Key, typename Value, typename Rest>
    struct keyed_element;

    struct nil_keyed_element;

    template<typename N, BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(FUSION_MAX_DEQUE_SIZE, typename T, void_)>
    struct deque_keyed_values_impl;

    template<typename N>
    struct deque_keyed_values_impl<N, BOOST_PP_ENUM(FUSION_MAX_DEQUE_SIZE, FUSION_VOID, _)>
    {
        typedef nil_keyed_element type;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static type construct()
        {
            return type();
        }

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static type forward_()
        {
            return type();
        }
    };

    template<typename N, BOOST_PP_ENUM_PARAMS(FUSION_MAX_DEQUE_SIZE, typename T)>
    struct deque_keyed_values_impl
    {
        typedef mpl::int_<mpl::plus<N, mpl::int_<1> >::value> next_index;

        typedef typename deque_keyed_values_impl<
            next_index,
            BOOST_PP_ENUM_SHIFTED_PARAMS(FUSION_MAX_DEQUE_SIZE, T)>::type tail;
        typedef keyed_element<N, T0, tail> type;

#include <boost/fusion/container/deque/detail/cpp03/deque_keyed_values_call.hpp>

    };

    template<BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(FUSION_MAX_DEQUE_SIZE, typename T, void_)>
    struct deque_keyed_values
        : deque_keyed_values_impl<mpl::int_<0>, BOOST_PP_ENUM_PARAMS(FUSION_MAX_DEQUE_SIZE, T)>
    {};

}}}

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif // BOOST_FUSION_DONT_USE_PREPROCESSED_FILES

#undef FUSION_VOID

#endif
