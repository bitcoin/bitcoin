/*=============================================================================
    Copyright (c) 2006 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_SEGMENTS_04052005_1141)
#define BOOST_FUSION_SEGMENTS_04052005_1141

#include <boost/fusion/support/config.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/sequence/intrinsic_fwd.hpp>
#include <boost/fusion/support/tag_of.hpp>

namespace boost { namespace fusion
{
    // Special tags:
    struct sequence_facade_tag;
    struct iterator_range_tag;

    // segments: returns a sequence of sequences
    namespace extension
    {
        template <typename Tag>
        struct segments_impl
        {
            template <typename Sequence>
            struct apply {};
        };

        template <>
        struct segments_impl<sequence_facade_tag>
        {
            template <typename Sequence>
            struct apply : Sequence::template segments<Sequence> {};
        }; 

        template <>
        struct segments_impl<iterator_range_tag>;
    }

    namespace result_of
    {
        template <typename Sequence>
        struct segments
        {
            typedef typename traits::tag_of<Sequence>::type tag_type;

            typedef typename
                extension::segments_impl<tag_type>::template apply<Sequence>::type
            type;
        };
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename
        lazy_disable_if<
            is_const<Sequence>
          , result_of::segments<Sequence>
        >::type
    segments(Sequence& seq)
    {
        typedef typename traits::tag_of<Sequence>::type tag_type;
        return extension::segments_impl<tag_type>::template apply<Sequence>::call(seq);
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::segments<Sequence const>::type
    segments(Sequence const& seq)
    {
        typedef typename traits::tag_of<Sequence const>::type tag_type;
        return extension::segments_impl<tag_type>::template apply<Sequence const>::call(seq);
    }
}}

#endif
