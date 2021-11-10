/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden
    Copyright (c) 2009-2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DETAIL_EXTENSION_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DETAIL_EXTENSION_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/support/category_of.hpp>

namespace boost { namespace fusion
{
    struct struct_tag;
    struct struct_iterator_tag;
    struct assoc_struct_tag;
    struct fusion_sequence_tag;

    struct assoc_struct_category
      : random_access_traversal_tag, associative_tag
    {};

    namespace extension
    {
        struct no_such_member;

        struct access
        {
            template<typename Seq, int N>
            struct struct_member;

            template<typename Seq, int N>
            struct adt_attribute_access;
        };

        template <typename T, int N, bool Const>
        struct adt_attribute_proxy;

        template<typename Seq, int N>
        struct struct_member_name;

        template<typename Seq>
        struct struct_size;

        template<typename Seq>
        struct struct_is_view;

        template<typename Seq, int N>
        struct struct_assoc_key;

    }
}}

#endif
