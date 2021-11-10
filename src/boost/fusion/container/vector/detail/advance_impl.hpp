/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_ADVANCE_IMPL_09172005_1156)
#define FUSION_ADVANCE_IMPL_09172005_1156

namespace boost { namespace fusion
{
    struct vector_iterator_tag;

    template <typename Vector, int N>
    struct vector_iterator;

    namespace extension
    {
        template <typename Tag>
        struct advance_impl;

        template <>
        struct advance_impl<vector_iterator_tag>
        {
            template <typename Iterator, typename N>
            struct apply
            {
                typedef typename Iterator::index index;
                typedef typename Iterator::vector vector;
                typedef vector_iterator<vector, index::value+N::value> type;

                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static type
                call(Iterator const& i)
                {
                    return type(i.vec);
                }
            };
        };
    }
}}

#endif
