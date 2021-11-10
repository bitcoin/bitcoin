/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_SIZE_10022005_1617)
#define FUSION_SIZE_10022005_1617

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/size.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>

namespace boost { namespace mpl
{
    template <typename Tag>
    struct size_impl;

    template <>
    struct size_impl<fusion::fusion_sequence_tag>
    {
        template <typename Sequence>
        struct apply : fusion::result_of::size<Sequence> {};
    };
}}

#endif
