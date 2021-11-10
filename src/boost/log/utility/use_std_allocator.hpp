/*
 *              Copyright Andrey Semashev 2021.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   use_std_allocator.hpp
 * \author Andrey Semashev
 * \date   04.03.2021
 *
 * The header defines \c use_std_allocator tag type.
 */

#ifndef BOOST_LOG_UTILITY_USE_STD_ALLOCATOR_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_USE_STD_ALLOCATOR_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace aux {

namespace usestdalloc_adl_block {

struct use_std_allocator {};

} // namespace usestdalloc_adl_block

} // namespace aux

using aux::usestdalloc_adl_block::use_std_allocator;

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Tag type that indicates that a specialization of \c std::allocator should be used for allocating memory
 *
 * This tag type can be used in template parameters in various components of Boost.Log. The type itself is not an allocator type.
 */
struct use_std_allocator {};

#endif // BOOST_LOG_DOXYGEN_PASS

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_USE_STD_ALLOCATOR_HPP_INCLUDED_
