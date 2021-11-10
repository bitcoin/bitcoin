/* boost random auto_link.hpp header file
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_DETAIL_AUTO_LINK_HPP
#define BOOST_RANDOM_DETAIL_AUTO_LINK_HPP

#include <boost/config.hpp>

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_RANDOM_DYN_LINK)
    #if defined(BOOST_RANDOM_SOURCE)
        #define BOOST_RANDOM_DECL BOOST_SYMBOL_EXPORT
    #else
        #define BOOST_RANDOM_DECL BOOST_SYMBOL_IMPORT
    #endif
#endif

#ifndef BOOST_RANDOM_DECL
    #define BOOST_RANDOM_DECL
#endif

#if !defined(BOOST_RANDOM_NO_LIB) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_RANDOM_SOURCE)

#define BOOST_LIB_NAME boost_random

#if defined(BOOST_RANDOM_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
    #define BOOST_DYN_LINK
#endif

#include <boost/config/auto_link.hpp>

#endif

#endif
