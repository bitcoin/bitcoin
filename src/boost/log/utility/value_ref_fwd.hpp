/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   value_ref_fwd.hpp
 * \author Andrey Semashev
 * \date   27.07.2012
 *
 * The header contains forward declaration of a value reference wrapper.
 */

#ifndef BOOST_LOG_UTILITY_VALUE_REF_FWD_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_VALUE_REF_FWD_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief Reference wrapper for a stored attribute value.
 */
template< typename T, typename TagT = void >
class value_ref;

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_UTILITY_VALUE_REF_FWD_HPP_INCLUDED_
