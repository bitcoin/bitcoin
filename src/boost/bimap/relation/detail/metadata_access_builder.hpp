// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/detail/metadata_access_builder.hpp
/// \brief Define macros to help building metafunctions

#ifndef BOOST_BIMAP_RELATION_DETAIL_METADATA_ACCESS_BUILDER_HPP
#define BOOST_BIMAP_RELATION_DETAIL_METADATA_ACCESS_BUILDER_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/relation/support/is_tag_of_member_at.hpp>
#include <boost/bimap/detail/debug/static_error.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/preprocessor/cat.hpp>



/******************************************************************************
              BIMAP SYMMETRIC METADATA ACCESS INTERFACE
*******************************************************************************

template< class Tag, class SymmetricType >
struct NAME
{
    typedef -unspecified- type;
};

******************************************************************************/


/*===========================================================================*/
#define BOOST_BIMAP_SYMMETRIC_METADATA_ACCESS_BUILDER(                        \
                                                                              \
        NAME,                                                                 \
        METADATA_BY_LEFT,                                                     \
        METADATA_BY_RIGHT                                                     \
    )                                                                         \
                                                                              \
    template                                                                  \
    <                                                                         \
        class Tag,                                                            \
        class SymmetricType,                                                  \
        class Enable = void                                                   \
    >                                                                         \
    struct NAME                                                               \
    {                                                                         \
        BOOST_BIMAP_STATIC_ERROR(                                             \
            BOOST_PP_CAT(NAME,_FAILURE),                                      \
            (SymmetricType,Tag)                                               \
        );                                                                    \
    };                                                                        \
                                                                              \
    template< class Tag, class SymmetricType >                                \
    struct NAME                                                               \
    <                                                                         \
        Tag, SymmetricType,                                                   \
        BOOST_DEDUCED_TYPENAME enable_if                                      \
        <                                                                     \
            ::boost::bimaps::relation::support::is_tag_of_member_at_left      \
            <                                                                 \
                Tag,                                                          \
                SymmetricType                                                 \
            >                                                                 \
                                                                              \
        >::type                                                               \
    >                                                                         \
    {                                                                         \
        typedef BOOST_DEDUCED_TYPENAME SymmetricType::METADATA_BY_LEFT type;  \
    };                                                                        \
                                                                              \
    template< class Tag, class SymmetricType >                                \
    struct NAME                                                               \
    <                                                                         \
        Tag, SymmetricType,                                                   \
        BOOST_DEDUCED_TYPENAME enable_if                                      \
        <                                                                     \
            ::boost::bimaps::relation::support::is_tag_of_member_at_right     \
            <                                                                 \
                Tag,                                                          \
                SymmetricType                                                 \
            >                                                                 \
                                                                              \
        >::type                                                               \
    >                                                                         \
    {                                                                         \
        typedef BOOST_DEDUCED_TYPENAME SymmetricType::METADATA_BY_RIGHT type; \
    };
/*===========================================================================*/


#endif // BOOST_BIMAP_RELATION_DETAIL_METADATA_ACCES_BUILDER_HPP


