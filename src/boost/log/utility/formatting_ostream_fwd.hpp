/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatting_ostream_fwd.hpp
 * \author Andrey Semashev
 * \date   11.07.2012
 *
 * The header contains forward declaration of a string stream used for log record formatting.
 */

#ifndef BOOST_LOG_UTILITY_FORMATTING_OSTREAM_FWD_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_FORMATTING_OSTREAM_FWD_HPP_INCLUDED_

#include <string>
#include <memory>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief Stream for log records formatting
 */
template<
    typename CharT,
    typename TraitsT = std::char_traits< CharT >,
    typename AllocatorT = std::allocator< CharT >
>
class basic_formatting_ostream;

#ifdef BOOST_LOG_USE_CHAR
typedef basic_formatting_ostream< char > formatting_ostream;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_formatting_ostream< wchar_t > wformatting_ostream;
#endif

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_UTILITY_FORMATTING_OSTREAM_FWD_HPP_INCLUDED_
