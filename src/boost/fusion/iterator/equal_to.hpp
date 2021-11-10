/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_EQUAL_TO_05052005_1208)
#define FUSION_EQUAL_TO_05052005_1208

#include <boost/fusion/support/config.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/fusion/support/tag_of.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/fusion/support/is_iterator.hpp>
#include <boost/mpl/and.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace fusion
{
    // Special tags:
    struct iterator_facade_tag; // iterator facade tag
    struct boost_array_iterator_tag; // boost::array iterator tag
    struct mpl_iterator_tag; // mpl sequence iterator tag
    struct std_pair_iterator_tag; // std::pair iterator tag

    namespace extension
    {
        template <typename Tag>
        struct equal_to_impl
        {
            // default implementation
            template <typename I1, typename I2>
            struct apply
                : is_same<typename add_const<I1>::type, typename add_const<I2>::type>
            {};
        };

        template <>
        struct equal_to_impl<iterator_facade_tag>
        {
            template <typename It1, typename It2, typename Tag1, typename Tag2>
            struct dispatch : mpl::false_ {};

            template <typename It1, typename It2, typename Tag>
            struct dispatch<It1, It2, Tag, Tag> // same tag
              : It1::template equal_to<It1, It2>
            {};

            template<typename It1, typename It2>
            struct apply : dispatch<It1, It2,
                typename It1::fusion_tag, typename It2::fusion_tag>
            {};
        };

        template <>
        struct equal_to_impl<boost_array_iterator_tag>;

        template <>
        struct equal_to_impl<mpl_iterator_tag>;

        template <>
        struct equal_to_impl<std_pair_iterator_tag>;
    }

    namespace result_of
    {
        template <typename I1, typename I2>
        struct equal_to
            : extension::equal_to_impl<typename detail::tag_of<I1>::type>::
                template apply<I1, I2>
        {};
    }

    namespace iterator_operators
    {
        template <typename Iter1, typename Iter2>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        inline typename
        boost::enable_if<
            mpl::and_<is_fusion_iterator<Iter1>, is_fusion_iterator<Iter2> >
            , bool
            >::type
        operator==(Iter1 const&, Iter2 const&)
        {
            return result_of::equal_to<Iter1, Iter2>::value;
        }

        template <typename Iter1, typename Iter2>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        inline typename
        boost::enable_if<
            mpl::and_<is_fusion_iterator<Iter1>, is_fusion_iterator<Iter2> >
            , bool
            >::type
        operator!=(Iter1 const&, Iter2 const&)
        {
            return !result_of::equal_to<Iter1, Iter2>::value;
        }
    }

    using iterator_operators::operator==;
    using iterator_operators::operator!=;
}}

#endif

