/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_PREV_IMPL_13122005_2110)
#define FUSION_PREV_IMPL_13122005_2110

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/iterator/prior.hpp>

namespace boost { namespace fusion 
{
    struct transform_view_iterator_tag;
    struct transform_view_iterator2_tag;

    template<typename First, typename F>
    struct transform_view_iterator;

    template <typename First1, typename First2, typename F>
    struct transform_view_iterator2;

    namespace extension
    {
        template<typename Tag>
        struct prior_impl;

        // Unary Version
        template<>
        struct prior_impl<transform_view_iterator_tag>
        {
            template<typename Iterator>
            struct apply
            {
                typedef typename Iterator::first_type first_type;
                typedef typename result_of::prior<first_type>::type prior_type;
                typedef typename Iterator::transform_type transform_type;
                typedef transform_view_iterator<prior_type, transform_type> type;

                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static type
                call(Iterator const& i)
                {
                    return type(fusion::prior(i.first), i.f);
                }
            };
        };

        // Binary Version
        template<>
        struct prior_impl<transform_view_iterator2_tag>
        {
            template<typename Iterator>
            struct apply
            {
                typedef typename Iterator::first1_type first1_type;
                typedef typename Iterator::first2_type first2_type;
                typedef typename result_of::prior<first1_type>::type prior1_type;
                typedef typename result_of::prior<first2_type>::type prior2_type;
                typedef typename Iterator::transform_type transform_type;
                typedef transform_view_iterator2<prior1_type, prior2_type, transform_type> type;

                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static type
                call(Iterator const& i)
                {
                    return type(fusion::prior(i.first1), fusion::prior(i.first2), i.f);
                }
            };
        };
    }
}}

#endif
