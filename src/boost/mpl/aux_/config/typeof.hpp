
#ifndef BOOST_MPL_AUX_CONFIG_TYPEOF_HPP_INCLUDED
#define BOOST_MPL_AUX_CONFIG_TYPEOF_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/aux_/config/gcc.hpp>

#if !defined(BOOST_MPL_CFG_HAS_TYPEOF) \
    && !defined(BOOST_MPL_PREPROCESSING_MODE) \
    && (   defined(BOOST_MPL_CFG_GCC) && BOOST_MPL_CFG_GCC >= 0x0302 \
        || defined(__MWERKS__) && __MWERKS__ >= 0x3000 \
        )

#   define BOOST_MPL_CFG_HAS_TYPEOF

#endif


#if !defined(BOOST_MPL_CFG_TYPEOF_BASED_SEQUENCES) \
    && !defined(BOOST_MPL_PREPROCESSING_MODE) \
    && defined(BOOST_MPL_CFG_HAS_TYPEOF)

#   define BOOST_MPL_CFG_TYPEOF_BASED_SEQUENCES

#endif

#endif // BOOST_MPL_AUX_CONFIG_TYPEOF_HPP_INCLUDED
