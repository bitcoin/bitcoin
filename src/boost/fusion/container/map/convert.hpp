/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONVERT_MAIN_09232005_1340)
#define FUSION_CONVERT_MAIN_09232005_1340

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/map/map.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename It, bool is_assoc>
    struct pair_from
    {
        typedef typename result_of::value_of<It>::type type;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static inline type call(It const& it)
        {
            return *it;
        }
    };

    template <typename It>
    struct pair_from<It, true>
    {
        typedef typename result_of::key_of<It>::type key_type;
        typedef typename result_of::value_of_data<It>::type data_type;
        typedef typename fusion::pair<key_type, data_type> type;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static inline type call(It const& it)
        {
            return type(deref_data(it));
        }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
// Without variadics, we will use the PP version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_MAP)
# include <boost/fusion/container/map/detail/cpp03/convert.hpp>

#else
///////////////////////////////////////////////////////////////////////////////
// C++11 variadic implementation
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/container/map/detail/build_map.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence>
        struct as_map :
            detail::build_map<
                typename result_of::begin<Sequence>::type
              , typename result_of::end<Sequence>::type
              , is_base_of<
                    associative_tag
                  , typename traits::category_of<Sequence>::type>::value
            >
        {
        };
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::as_map<Sequence>::type
    as_map(Sequence& seq)
    {
        typedef result_of::as_map<Sequence> gen;
        return gen::call(fusion::begin(seq), fusion::end(seq));
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::as_map<Sequence const>::type
    as_map(Sequence const& seq)
    {
        typedef result_of::as_map<Sequence const> gen;
        return gen::call(fusion::begin(seq), fusion::end(seq));
    }

    namespace extension
    {
        template <typename T>
        struct convert_impl;

        template <>
        struct convert_impl<map_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef typename
                    result_of::as_map<Sequence>::type
                type;

                BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
                static type call(Sequence& seq)
                {
                    typedef result_of::as_map<Sequence> gen;
                    return gen::call(fusion::begin(seq), fusion::end(seq));
                }
            };
        };
    }
}}

#endif
#endif

