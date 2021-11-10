/*=============================================================================
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_ARRAY_CATEGORY_OF_IMPL_HPP
#define BOOST_FUSION_ADAPTED_ARRAY_CATEGORY_OF_IMPL_HPP

namespace boost { namespace fusion
{
    struct random_access_traversal_tag;

    namespace extension
    {
        template<typename>
        struct category_of_impl;

        template<>
        struct category_of_impl<po_array_tag>
        {
            template<typename Seq>
            struct apply
            {
                typedef random_access_traversal_tag type;
            };
        };
    }
}}

#endif
