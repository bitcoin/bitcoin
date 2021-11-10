/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_LIST_TO_CONS_07172005_1041)
#define FUSION_LIST_TO_CONS_07172005_1041

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/list/cons.hpp>
#include <boost/fusion/container/list/detail/cpp03/limits.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>

#define FUSION_VOID(z, n, _) void_

namespace boost { namespace fusion
{
    struct nil_;
    struct void_;
}}

#if !defined(BOOST_FUSION_DONT_USE_PREPROCESSED_FILES)
#include <boost/fusion/container/list/detail/cpp03/preprocessed/list_to_cons.hpp>
#else
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/list_to_cons" FUSION_MAX_LIST_SIZE_STR ".hpp")
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
    template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename T)>
    struct list_to_cons
    {
        typedef T0 head_type;
        typedef list_to_cons<
            BOOST_PP_ENUM_SHIFTED_PARAMS(FUSION_MAX_LIST_SIZE, T), void_>
        tail_list_to_cons;
        typedef typename tail_list_to_cons::type tail_type;

        typedef cons<head_type, tail_type> type;

        #include <boost/fusion/container/list/detail/cpp03/list_to_cons_call.hpp>
    };

    template <>
    struct list_to_cons<BOOST_PP_ENUM(FUSION_MAX_LIST_SIZE, FUSION_VOID, _)>
    {
        typedef nil_ type;
    };
}}}

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif // BOOST_FUSION_DONT_USE_PREPROCESSED_FILES

#undef FUSION_VOID
#endif
