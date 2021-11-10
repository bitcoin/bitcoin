/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_DISTANCE_IMPL_13122005_2139)
#define FUSION_DISTANCE_IMPL_13122005_2139

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/iterator/distance.hpp>

namespace boost { namespace fusion {
    
    struct transform_view_iterator_tag;
    struct transform_view_iterator2_tag;

    namespace extension
    {
        template<typename Tag>
        struct distance_impl;

        // Unary Version
        template<>
        struct distance_impl<transform_view_iterator_tag>
        {
            template<typename First, typename Last>
            struct apply
                : result_of::distance<typename First::first_type, typename Last::first_type>
            {
                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static 
                typename result_of::distance<typename First::first_type, typename Last::first_type>::type
                call(First const& first, Last const& last)
                {
                    return boost::fusion::distance(first.first, last.first);
                }
            };
        };

        // Binary Version
        template<>
        struct distance_impl<transform_view_iterator2_tag>
        {
            template<typename First, typename Last>
            struct apply
                : result_of::distance<typename First::first1_type, typename Last::first1_type>
            {
                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static 
                typename result_of::distance<typename First::first1_type, typename Last::first1_type>::type
                call(First const& first, Last const& last)
                {
                    return boost::fusion::distance(first.first1, last.first1);
                }
            };
        };
    }
}}

#endif
