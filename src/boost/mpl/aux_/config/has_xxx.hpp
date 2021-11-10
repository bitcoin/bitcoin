
#ifndef BOOST_MPL_AUX_CONFIG_HAS_XXX_HPP_INCLUDED
#define BOOST_MPL_AUX_CONFIG_HAS_XXX_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2002-2004
// Copyright David Abrahams 2002-2003
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/aux_/config/overload_resolution.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

// agurt, 11/jan/03: signals a stub-only 'has_xxx' implementation

#if !defined(BOOST_MPL_CFG_NO_HAS_XXX) \
    && (   defined(BOOST_MPL_CFG_BROKEN_OVERLOAD_RESOLUTION) \
        || BOOST_WORKAROUND(__GNUC__, <= 2) \
        || BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840)) \
        )

#   define BOOST_MPL_CFG_NO_HAS_XXX
#   define BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE

#endif

#endif // BOOST_MPL_AUX_CONFIG_HAS_XXX_HPP_INCLUDED
