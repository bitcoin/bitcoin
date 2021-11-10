/*=============================================================================
    Copyright (c) 2009 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !defined(BOOST_FUSION_NVIEW_DISTANCE_IMPL_SEP_23_2009_0328PM)
#define BOOST_FUSION_NVIEW_DISTANCE_IMPL_SEP_23_2009_0328PM

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/iterator/distance.hpp>

namespace boost { namespace fusion 
{
    struct nview_iterator_tag;

    namespace extension
    {
        template<typename Tag>
        struct distance_impl;

        template<>
        struct distance_impl<nview_iterator_tag>
        {
            template<typename First, typename Last>
            struct apply
              : result_of::distance<typename First::first_type, typename Last::first_type>
            {
                typedef typename result_of::distance<
                    typename First::first_type, typename Last::first_type
                >::type type;

                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static type
                call(First const& /*first*/, Last const& /*last*/)
                {
                    return type();
                }
            };
        };
    }

}}

#endif
