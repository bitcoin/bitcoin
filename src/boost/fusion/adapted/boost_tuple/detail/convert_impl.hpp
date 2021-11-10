/*=============================================================================
    Copyright (c) 2012-2014 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_CONVERT_IMPL_10172012_0120)
#define BOOST_FUSION_CONVERT_IMPL_10172012_0120

#include <boost/tuple/tuple.hpp>
#include <boost/fusion/adapted/boost_tuple/detail/build_cons.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>

namespace boost { namespace fusion
{
    struct boost_tuple_tag;

    namespace extension
    {
        template <typename T>
        struct convert_impl;

        template <>
        struct convert_impl<boost_tuple_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef typename
                    detail::build_tuple_cons<
                        typename result_of::begin<Sequence>::type
                      , typename result_of::end<Sequence>::type
                    >
                build_tuple_cons;

                typedef typename build_tuple_cons::type type;

                BOOST_FUSION_GPU_ENABLED
                static type
                call(Sequence& seq)
                {
                    return build_tuple_cons::call(fusion::begin(seq), fusion::end(seq));
                }
            };
        };
    }
}}

#endif
