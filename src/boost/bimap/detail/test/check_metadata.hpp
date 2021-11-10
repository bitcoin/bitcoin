// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_BIMAP_DETAIL_CHECK_METADATA_HPP
#define BOOST_BIMAP_DETAIL_CHECK_METADATA_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/preprocessor/cat.hpp>


// Easier way to call BOOST_MPL_ASSERT_MSG in class scope
/*===========================================================================*/
#define BOOST_BIMAP_MPL_ASSERT_MSG_ACS(p1,p2,p3)                              \
                                                                              \
    struct p2 {};                                                             \
    BOOST_MPL_ASSERT_MSG(p1,p2,p3);                                           \
/*===========================================================================*/


// Build a descriptive name.
/*===========================================================================*/
#define BOOST_BIMAP_WRONG_METADATA_MESSAGE(                                   \
                                                                              \
        P_CLASS,                                                              \
        P_NAME,                                                               \
        P_CORRECT_TYPE                                                        \
                                                                              \
    )                                                                         \
                                                                              \
    BOOST_PP_CAT                                                              \
    (                                                                         \
        WRONG_METADATA__,                                                     \
        BOOST_PP_CAT                                                          \
        (                                                                     \
            P_CLASS,                                                          \
            BOOST_PP_CAT                                                      \
            (                                                                 \
                __AT__,                                                       \
                BOOST_PP_CAT                                                  \
                (                                                             \
                    P_NAME,                                                   \
                    BOOST_PP_CAT                                              \
                    (                                                         \
                        __IS_DIFERENT_TO__,                                   \
                        P_CORRECT_TYPE                                        \
                    )                                                         \
                )                                                             \
            )                                                                 \
        )                                                                     \
    )
/*===========================================================================*/


// Check if the metadata have the correct type, and if not inform
// it with a useful compile time message.
/*===========================================================================*/
#define BOOST_BIMAP_CHECK_METADATA(                                           \
                                                                              \
        P_CLASS,                                                              \
        P_NAME,                                                               \
        P_CORRECT_TYPE                                                        \
                                                                              \
    )                                                                         \
                                                                              \
    BOOST_BIMAP_MPL_ASSERT_MSG_ACS                                            \
    (                                                                         \
        (                                                                     \
            ::boost::is_same                                                  \
            <                                                                 \
                P_CLASS::P_NAME,                                              \
                P_CORRECT_TYPE                                                \
                                                                              \
            >::value                                                          \
        ),                                                                    \
        BOOST_BIMAP_WRONG_METADATA_MESSAGE                                    \
        (                                                                     \
            P_CLASS,                                                          \
            P_NAME,                                                           \
            P_CORRECT_TYPE                                                    \
        ),                                                                    \
        (P_CLASS::P_NAME,P_CORRECT_TYPE)                                      \
    )
/*===========================================================================*/


// Just for autodocumment the test code
/*===========================================================================*/
#define BOOST_BIMAP_TEST_STATIC_FUNCTION(NAME)                                \
    namespace NAME
/*===========================================================================*/


// Just for autodocument the test code
/*===========================================================================*/
#define BOOST_BIMAP_CALL_TEST_STATIC_FUNCTION(NAME)
/*===========================================================================*/



#endif // BOOST_BIMAP_DETAIL_CHECK_METADATA_HPP

