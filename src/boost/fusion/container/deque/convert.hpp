/*=============================================================================
    Copyright (c) 2005-2013 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONVERT_20061213_2207)
#define FUSION_CONVERT_20061213_2207

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/deque/detail/convert_impl.hpp>
#include <boost/fusion/container/deque/deque.hpp>

#if !defined(BOOST_FUSION_HAS_VARIADIC_DEQUE)
///////////////////////////////////////////////////////////////////////////////
// C++03 (non-variadic) implementation
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/container/deque/detail/cpp03/build_deque.hpp>

#else
///////////////////////////////////////////////////////////////////////////////
// C++11 variadic implementation
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/container/deque/detail/build_deque.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence>
        struct as_deque :
            detail::build_deque<
                typename result_of::begin<Sequence>::type
              , typename result_of::end<Sequence>::type
            >
        {
        };
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::as_deque<Sequence>::type
    as_deque(Sequence& seq)
    {
        typedef result_of::as_deque<Sequence> gen;
        return gen::call(fusion::begin(seq), fusion::end(seq));
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::as_deque<Sequence const>::type
    as_deque(Sequence const& seq)
    {
        typedef result_of::as_deque<Sequence const> gen;
        return gen::call(fusion::begin(seq), fusion::end(seq));
    }
}}

#endif
#endif
