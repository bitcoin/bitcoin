/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_JOIN_200601222109)
#define FUSION_JOIN_200601222109

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/view/joint_view.hpp>

namespace boost { namespace fusion {

    namespace result_of
    {
        template<typename LhSequence, typename RhSequence>
        struct join
        {
            typedef joint_view<LhSequence, RhSequence> type;
        };
    }

    template<typename LhSequence, typename RhSequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::join<LhSequence const, RhSequence const>::type
    join(LhSequence const& lhs, RhSequence const& rhs)
    {
        return typename result_of::join<LhSequence const, RhSequence const>::type(
            lhs, rhs);
    }
}}

#endif
