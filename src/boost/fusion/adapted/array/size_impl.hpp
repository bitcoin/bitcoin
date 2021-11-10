/*=============================================================================
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_ARRAY_SIZE_IMPL_HPP
#define BOOST_FUSION_ADAPTED_ARRAY_SIZE_IMPL_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/type_traits/rank.hpp>
#include <boost/type_traits/extent.hpp>

namespace boost { namespace fusion { namespace extension
{
    template<typename>
    struct size_impl;

    template<>
    struct size_impl<po_array_tag>
    {
        template<typename Seq>
        struct apply
          : extent<Seq,rank<Seq>::value-1>
        {};
    };
}}}

#endif
