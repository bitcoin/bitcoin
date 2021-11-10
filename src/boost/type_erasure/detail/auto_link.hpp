// Boost.TypeErasure library
//
// Copyright 2015 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DETAIL_AUTO_LINK_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_AUTO_LINK_HPP_INCLUDED

#include <boost/config.hpp>

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TYPE_ERASURE_DYN_LINK)
    #ifdef BOOST_TYPE_ERASURE_SOURCE
        #define BOOST_TYPE_ERASURE_DECL BOOST_SYMBOL_EXPORT
    #else
        #define BOOST_TYPE_ERASURE_DECL BOOST_SYMBOL_IMPORT
    #endif
#else
    #define BOOST_TYPE_ERASURE_DECL
#endif

#if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_TYPE_ERASURE_NO_LIB) && !defined(BOOST_TYPE_ERASURE_SOURCE)

    #define BOOST_LIB_NAME boost_type_erasure

    #if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TYPE_ERASURE_DYN_LINK)
        #define BOOST_DYN_LINK
    #endif

    #include <boost/config/auto_link.hpp>

#endif

#endif
