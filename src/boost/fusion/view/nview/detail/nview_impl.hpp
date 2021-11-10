/*=============================================================================
    Copyright (c) 2014 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_FUSION_NVIEW_IMPL_17122014_1948
#define BOOST_FUSION_NVIEW_IMPL_17122014_1948

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/vector/detail/config.hpp>

///////////////////////////////////////////////////////////////////////////////
// Without variadics, we will use the PP version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_VECTOR)
# include <boost/fusion/view/nview/detail/cpp03/nview_impl.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 interface
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/container/vector.hpp>
#include <boost/mpl/int.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence, int ...I>
        struct as_nview
        {
            typedef vector<mpl::int_<I>...> index_type;
            typedef nview<Sequence, index_type> type;
        };
    }

    template <int ...I, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline nview<Sequence, vector<mpl::int_<I>...> >
    as_nview(Sequence& s)
    {
        typedef vector<mpl::int_<I>...> index_type;
        return nview<Sequence, index_type>(s);
    }
}}

#endif
#endif

