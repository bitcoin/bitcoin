/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CATEGORY_OF_07202005_0308)
#define FUSION_CATEGORY_OF_07202005_0308

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/support/tag_of.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace boost { namespace fusion
{
    // Special tags:
    struct boost_tuple_tag; // boost::tuples::tuple tag
    struct boost_array_tag; // boost::array tag
    struct mpl_sequence_tag; // mpl sequence tag
    struct std_pair_tag; // std::pair tag

    struct incrementable_traversal_tag {};

    struct single_pass_traversal_tag
        : incrementable_traversal_tag {};

    struct forward_traversal_tag
        : single_pass_traversal_tag {};

    struct bidirectional_traversal_tag
        : forward_traversal_tag {};

    struct random_access_traversal_tag
        : bidirectional_traversal_tag {};

    struct associative_tag {};

    struct unbounded_tag {};

    namespace extension
    {
        template<typename Tag>
        struct category_of_impl
        {
            template<typename T>
            struct apply
            {
                typedef typename T::category type;
            };
        };

        template <>
        struct category_of_impl<boost_tuple_tag>;

        template <>
        struct category_of_impl<boost_array_tag>;

        template <>
        struct category_of_impl<mpl_sequence_tag>;

        template <>
        struct category_of_impl<std_pair_tag>;
    }

    namespace traits
    {
        template <typename T>
        struct category_of
            : extension::category_of_impl<typename fusion::detail::tag_of<T>::type>::
                template apply<T>
        {};

        template <typename T>
        struct is_associative
            : is_base_of<
                associative_tag
              , typename category_of<T>::type>
        {};

        template <typename T>
        struct is_incrementable
            : is_base_of<
                incrementable_traversal_tag
              , typename category_of<T>::type>
        {};

        template <typename T>
        struct is_single_pass
            : is_base_of<
                single_pass_traversal_tag
              , typename category_of<T>::type>
        {};

        template <typename T>
        struct is_forward
            : is_base_of<
                forward_traversal_tag
              , typename category_of<T>::type>
        {};

        template <typename T>
        struct is_bidirectional
            : is_base_of<
                bidirectional_traversal_tag
              , typename category_of<T>::type>
        {};

        template <typename T>
        struct is_random_access
            : is_base_of<
                random_access_traversal_tag
              , typename category_of<T>::type>
        {};

        template <typename T>
        struct is_unbounded
            : is_base_of<
                unbounded_tag
              , typename category_of<T>::type>
        {};
    }
}}

#endif
