/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CLEAR_09172005_1127)
#define FUSION_CLEAR_09172005_1127

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/vector/vector10.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence>
        struct clear
        {
            typedef vector0<> type;
        };
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::clear<Sequence const>::type
    clear(Sequence const& /*seq*/)
    {
        return vector0<>();
    }
}}

#endif

