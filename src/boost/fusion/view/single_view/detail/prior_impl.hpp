/*=============================================================================
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_SINGLE_VIEW_PRIOR_IMPL_JUL_07_2011_1348PM)
#define BOOST_FUSION_SINGLE_VIEW_PRIOR_IMPL_JUL_07_2011_1348PM

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/prior.hpp>

namespace boost { namespace fusion
{
    struct single_view_iterator_tag;

    template <typename Sequence, typename Pos>
    struct single_view_iterator;

    namespace extension
    {
        template <typename Tag>
        struct prior_impl;

        template <>
        struct prior_impl<single_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply 
            {
                typedef single_view_iterator<
                    typename Iterator::single_view_type,
                    typename mpl::prior<typename Iterator::position>::type>
                type;

                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static type
                call(Iterator const& i)
                {
                    return type(i.view);
                }
            };
        };
    }

}}

#endif
