/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   setup_config.hpp
 * \author Andrey Semashev
 * \date   14.09.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html. In this file
 *         internal configuration macros are defined.
 */

#ifndef BOOST_LOG_DETAIL_SETUP_CONFIG_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_SETUP_CONFIG_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined(BOOST_LOG_SETUP_BUILDING_THE_LIB)

// Detect if we're dealing with dll
#   if defined(BOOST_LOG_SETUP_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#        define BOOST_LOG_SETUP_DLL
#   endif

#   if defined(BOOST_LOG_SETUP_DLL)
#       define BOOST_LOG_SETUP_API BOOST_SYMBOL_IMPORT
#   else
#       define BOOST_LOG_SETUP_API
#   endif

//
// Automatically link to the correct build variant where possible.
//
#   if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_LOG_SETUP_NO_LIB)
#       define BOOST_LIB_NAME boost_log_setup
#       if defined(BOOST_LOG_SETUP_DLL)
#           define BOOST_DYN_LINK
#       endif
#       include <boost/config/auto_link.hpp>
#   endif  // auto-linking disabled

#else // !defined(BOOST_LOG_SETUP_BUILDING_THE_LIB)

#   if defined(BOOST_LOG_SETUP_DLL)
#       define BOOST_LOG_SETUP_API BOOST_SYMBOL_EXPORT
#   else
#       define BOOST_LOG_SETUP_API BOOST_SYMBOL_VISIBLE
#   endif

#endif // !defined(BOOST_LOG_SETUP_BUILDING_THE_LIB)

#endif // BOOST_LOG_DETAIL_SETUP_CONFIG_HPP_INCLUDED_
