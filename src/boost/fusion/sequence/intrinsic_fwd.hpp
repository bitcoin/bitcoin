/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_SEQUENCE_INTRINSIC_FWD_HPP_INCLUDED)
#define BOOST_FUSION_SEQUENCE_INTRINSIC_FWD_HPP_INCLUDED

#include <boost/fusion/support/config.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/support/is_sequence.hpp>

namespace boost { namespace fusion
{
    namespace extension
    {
        template <typename Tag>
        struct at_impl;

        template <typename Tag>
        struct begin_impl;

        template <typename Tag>
        struct empty_impl;

        template <typename Tag>
        struct end_impl;

        template <typename Tag>
        struct has_key_impl;

        template <typename Tag>
        struct segments_impl;

        template <typename Tag>
        struct size_impl;

        template <typename Tag>
        struct value_at_impl;

        template <typename Tag>
        struct at_key_impl;

        template <typename Tag>
        struct value_at_key_impl;
    }

    namespace result_of
    {
        template <typename Sequence, typename N>
        struct at;

        template <typename Sequence, int N>
        struct at_c;

        template <typename Sequence>
        struct back;

        template <typename Sequence>
        struct begin;

        template <typename Sequence>
        struct empty;

        template <typename Sequence>
        struct end;

        template <typename Sequence>
        struct front;

        template <typename Sequence, typename Key>
        struct has_key;

        template <typename Sequence>
        struct segments;

        template <typename Sequence>
        struct size;

        template <typename Sequence, typename N>
        struct value_at;

        template <typename Sequence, int N>
        struct value_at_c;

        template <typename Sequence, typename Key>
        struct at_key;

        template <typename Sequence, typename N>
        struct value_at_key;
    }

    template <typename N, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename 
        lazy_disable_if<
            is_const<Sequence>
          , result_of::at<Sequence, N>
        >::type
    at(Sequence& seq);

    template <typename N, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::at<Sequence const, N>::type
    at(Sequence const& seq);

    template <int N, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename 
        lazy_disable_if<
            is_const<Sequence>
          , result_of::at_c<Sequence, N>
        >::type
    at_c(Sequence& seq);

    template <int N, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::at_c<Sequence const, N>::type
    at_c(Sequence const& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::back<Sequence>::type
    back(Sequence& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::back<Sequence const>::type
    back(Sequence const& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename
        lazy_enable_if<
            traits::is_sequence<Sequence>
          , result_of::begin<Sequence>
        >::type const
    begin(Sequence& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename
        lazy_enable_if<
            traits::is_sequence<Sequence>
          , result_of::begin<Sequence const>
        >::type const
    begin(Sequence const& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::empty<Sequence>::type
    empty(Sequence const&);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename
        lazy_enable_if<
            traits::is_sequence<Sequence>
          , result_of::end<Sequence>
        >::type const
    end(Sequence& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename
        lazy_enable_if<
            traits::is_sequence<Sequence>
          , result_of::end<Sequence const>
        >::type const
    end(Sequence const& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::front<Sequence>::type
    front(Sequence& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::front<Sequence const>::type
    front(Sequence const& seq);

    template <typename Key, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::has_key<Sequence, Key>::type
    has_key(Sequence const& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename
        lazy_disable_if<
            is_const<Sequence>
          , result_of::segments<Sequence>
        >::type
    segments(Sequence& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::segments<Sequence const>::type
    segments(Sequence const& seq);

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::size<Sequence>::type
    size(Sequence const&);

    template <typename Key, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename 
        lazy_disable_if<
            is_const<Sequence>
          , result_of::at_key<Sequence, Key>
        >::type
    at_key(Sequence& seq);

    template <typename Key, typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    typename result_of::at_key<Sequence const, Key>::type
    at_key(Sequence const& seq);
}}

#endif
