/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_IS_VIEW_03202006_0015)
#define FUSION_IS_VIEW_03202006_0015

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/fusion/support/tag_of.hpp>

namespace boost { namespace fusion
{
    // Special tags:
    struct non_fusion_tag;
    struct sequence_facade_tag;
    struct boost_tuple_tag; // boost::tuples::tuple tag
    struct boost_array_tag; // boost::array tag
    struct mpl_sequence_tag; // mpl sequence tag
    struct std_pair_tag; // std::pair tag

    namespace extension
    {
        template<typename Tag>
        struct is_view_impl
        {
            template <typename T>
            struct apply
            {
                typedef typename T::is_view type;
            };
        };

        template <>
        struct is_view_impl<non_fusion_tag>
        {
            template <typename T>
            struct apply : mpl::false_ {};
        };

        template <>
        struct is_view_impl<sequence_facade_tag>
        {
            template <typename Sequence>
            struct apply : Sequence::is_view {};
        };

        template <>
        struct is_view_impl<boost_tuple_tag>;

        template <>
        struct is_view_impl<boost_array_tag>;

        template <>
        struct is_view_impl<mpl_sequence_tag>;

        template <>
        struct is_view_impl<std_pair_tag>;
    }

    namespace traits
    {
        template <typename T>
        struct is_view :
            mpl::bool_<
                (bool)extension::is_view_impl<typename fusion::detail::tag_of<T>::type>::
                    template apply<T>::type::value
            >
        {};
    }
}}

#endif
