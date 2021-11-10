/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_AT_10022005_1616)
#define FUSION_AT_10022005_1616

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/at.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>

namespace boost { 
namespace fusion
{
    struct fusion_sequence_tag;
}

namespace mpl
{
    template <typename Tag>
    struct at_impl;

    template <>
    struct at_impl<fusion::fusion_sequence_tag>
    {
        template <typename Sequence, typename N>
        struct apply : fusion::result_of::value_at<Sequence, N> {};
    };
}}

#endif

