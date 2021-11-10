// Boost.Geometry

// Copyright (c) 2019 Barend Gehrels, Amsterdam, the Netherlands.

// Copyright (c) 2018-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_CONFIG_HPP
#define BOOST_GEOMETRY_CORE_CONFIG_HPP

#include <boost/config.hpp>

// NOTE: workaround for VC++ 12 (aka 2013): cannot specify explicit initializer for arrays
#if !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX) && (!defined(_MSC_VER) || (_MSC_VER >= 1900))
#define BOOST_GEOMETRY_CXX11_ARRAY_UNIFIED_INITIALIZATION
#endif

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_VARIADIC_TEMPLATES)
#define BOOST_GEOMETRY_CXX11_TUPLE
#endif

// Defining this selects Kramer rule for segment-intersection
// That is default behaviour.
#define BOOST_GEOMETRY_USE_KRAMER_RULE

// Rescaling is turned on, unless NO_ROBUSTNESS is defined
// In future versions of Boost.Geometry, it will be turned off by default
#if ! defined(BOOST_GEOMETRY_NO_ROBUSTNESS)
#define BOOST_GEOMETRY_USE_RESCALING
#endif

#endif // BOOST_GEOMETRY_CORE_CONFIG_HPP
