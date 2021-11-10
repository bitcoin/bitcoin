/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONVERT_10022005_1442)
#define FUSION_CONVERT_10022005_1442

#include <boost/fusion/support/config.hpp>
#if BOOST_WORKAROUND(BOOST_GCC, < 30500)
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_const.hpp>
#define BOOST_FUSION_WA_GCC34(type1, type2) \
    boost::lazy_disable_if<boost::is_const<Sequence>, type1, type2>
#else
#define BOOST_FUSION_WA_GCC34(type1, type2) type1, type2
#endif

namespace boost { namespace fusion
{
    namespace extension
    {
        template <typename Tag>
        struct convert_impl;
    }

    namespace result_of
    {
        template <typename Tag, typename Sequence>
        struct convert
        {
            typedef typename
                extension::convert_impl<Tag>::template apply<Sequence>
            gen;

            typedef typename gen::type type;
        };
    }

    template <typename Tag, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename BOOST_FUSION_WA_GCC34(result_of::convert<Tag, Sequence>)::type
    convert(Sequence& seq)
    {
        typedef typename result_of::convert<Tag, Sequence>::gen gen;
        return gen::call(seq);
    }

    template <typename Tag, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::convert<Tag, Sequence const>::type
    convert(Sequence const& seq)
    {
        typedef typename result_of::convert<Tag, Sequence const>::gen gen;
        return gen::call(seq);
    }
}}

#undef BOOST_FUSION_WA_GCC34
#endif
