/*==============================================================================
    Copyright (c) 2013 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_FUSION_FLATTEN_VIEW_HPP_INCLUDED
#define BOOST_FUSION_FLATTEN_VIEW_HPP_INCLUDED


#include <boost/fusion/support/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/single_view.hpp>
#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/support/is_view.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/support/sequence_base.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/view/flatten_view/flatten_view_iterator.hpp>


namespace boost { namespace fusion
{
    struct forward_traversal_tag;
    struct flatten_view_tag;

    template <typename Sequence>
    struct flatten_view
      : sequence_base<flatten_view<Sequence> >
    {
        typedef flatten_view_tag fusion_tag;
        typedef fusion_sequence_tag tag; // this gets picked up by MPL
        typedef mpl::true_ is_view;
        typedef forward_traversal_tag category;

        typedef Sequence sequence_type;
        typedef typename result_of::begin<Sequence>::type first_type;
        typedef typename result_of::end<Sequence>::type last_type;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        explicit flatten_view(Sequence& seq)
          : seq(seq)
        {}

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        first_type first() const { return fusion::begin(seq); }
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        last_type last() const { return fusion::end(seq); }

        typename mpl::if_<traits::is_view<Sequence>, Sequence, Sequence&>::type seq;
    };
}}

namespace boost { namespace fusion { namespace extension
{
    template<>
    struct begin_impl<flatten_view_tag>
    {
        template<typename Sequence>
        struct apply
        {
            typedef typename Sequence::first_type first_type;

            typedef typename
                    result_of::begin<
                        mpl::single_view<
                            typename Sequence::sequence_type> >::type
            root_iterator;

            typedef
                detail::seek_descent<root_iterator, first_type>
            seek_descent;

            typedef typename seek_descent::type type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static inline
            type call(Sequence& seq)
            {
                return seek_descent::apply(root_iterator(), seq.first());
            }
        };
    };

    template<>
    struct end_impl<flatten_view_tag>
    {
        template<typename Sequence>
        struct apply
        {
            typedef typename Sequence::last_type last_type;

            typedef typename
                    result_of::end<
                        mpl::single_view<
                            typename Sequence::sequence_type> >::type
            type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static inline
            type call(Sequence&)
            {
                return type();
            }
        };
    };

    template<>
    struct size_impl<flatten_view_tag>
    {
        template <typename Sequence>
        struct apply
          : result_of::distance
            <
                typename result_of::begin<Sequence>::type
              , typename result_of::end<Sequence>::type
            >
        {};
    };

    template<>
    struct empty_impl<flatten_view_tag>
    {
        template <typename Sequence>
        struct apply
          : result_of::empty<typename Sequence::sequence_type>
        {};
    };
}}}


#endif
