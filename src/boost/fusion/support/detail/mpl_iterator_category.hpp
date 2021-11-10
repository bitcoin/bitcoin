/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MPL_ITERATOR_CATEGORY_07212005_0923)
#define FUSION_MPL_ITERATOR_CATEGORY_07212005_0923

namespace boost { namespace mpl 
{
    struct forward_iterator_tag;
    struct bidirectional_iterator_tag;
    struct random_access_iterator_tag;
}}

namespace boost { namespace fusion
{
    struct forward_traversal_tag;
    struct bidirectional_traversal_tag;
    struct random_access_traversal_tag;
}}

namespace boost { namespace fusion { namespace detail
{
    template <typename Category>
    struct mpl_iterator_category;
    
    template <>
    struct mpl_iterator_category<mpl::forward_iterator_tag>
    {
        typedef forward_traversal_tag type;
    };

    template <>
    struct mpl_iterator_category<mpl::bidirectional_iterator_tag>
    {
        typedef bidirectional_traversal_tag type;
    };

    template <>
    struct mpl_iterator_category<mpl::random_access_iterator_tag>
    {
        typedef random_access_traversal_tag type;
    };

    template <>
    struct mpl_iterator_category<forward_traversal_tag>
    {
        typedef forward_traversal_tag type;
    };

    template <>
    struct mpl_iterator_category<bidirectional_traversal_tag>
    {
        typedef bidirectional_traversal_tag type;
    };

    template <>
    struct mpl_iterator_category<random_access_traversal_tag>
    {
        typedef random_access_traversal_tag type;
    };
}}}

#endif
