/*=============================================================================
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_SINGLE_VIEW_SIZE_IMPL_JUL_07_2011_1348PM)
#define FUSION_SINGLE_VIEW_SIZE_IMPL_JUL_07_2011_1348PM

namespace boost { namespace fusion
{
    struct single_view_tag;

    namespace extension
    {
        template <typename Tag>
        struct size_impl;

        template <>
        struct size_impl<single_view_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef mpl::int_<1> type;
            };
        };
    }
}}

#endif


