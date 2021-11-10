/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   value_extraction_fwd.hpp
 * \author Andrey Semashev
 * \date   01.03.2008
 *
 * The header contains forward declaration of tools for extracting attribute values
 * from the view.
 */

#ifndef BOOST_LOG_ATTRIBUTES_VALUE_EXTRACTION_FWD_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_VALUE_EXTRACTION_FWD_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/fallback_policy_fwd.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace result_of {

/*!
 * \brief A metafunction that allows to acquire the result of the value extraction
 */
template< typename T, typename DefaultT = T, typename TagT = void >
struct extract_or_default;

/*!
 * \brief A metafunction that allows to acquire the result of the value extraction
 */
template< typename T, typename TagT = void >
struct extract_or_throw;

/*!
 * \brief A metafunction that allows to acquire the result of the value extraction
 */
template< typename T, typename TagT = void >
struct extract;

} // namespace result_of

/*!
 * \brief Generic attribute value extractor
 */
template< typename T, typename FallbackPolicyT = fallback_to_none, typename TagT = void >
class value_extractor;

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_VALUE_EXTRACTION_FWD_HPP_INCLUDED_
