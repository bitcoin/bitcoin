/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/regex.hpp
 * \author Andrey Semashev
 * \date   18.07.2009
 *
 * This header enables Boost.Regex support for Boost.Log.
 */

#ifndef BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_

#include <string>
#include <boost/regex.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/functional/matches.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! This tag type is used if an expression is recognized as a Boost.Regex expression
struct boost_regex_expression_tag;

//! The metafunction detects the matching expression kind and returns a tag that is used to specialize \c match_traits
template< typename CharT, typename TraitsT >
struct matching_expression_kind< boost::basic_regex< CharT, TraitsT > >
{
    typedef boost_regex_expression_tag type;
};

//! The matching function implementation
template< typename ExpressionT >
struct match_traits< ExpressionT, boost_regex_expression_tag >
{
    typedef ExpressionT compiled_type;
    static compiled_type compile(ExpressionT const& expr) { return expr; }

    template< typename StringT, typename CharT, typename TraitsT >
    static bool matches(StringT const& str, boost::basic_regex< CharT, TraitsT > const& expr, boost::regex_constants::match_flag_type flags = boost::regex_constants::match_default)
    {
        return boost::regex_match(str.begin(), str.end(), expr, flags);
    }

    template< typename CharT, typename StringTraitsT, typename AllocatorT, typename ReTraitsT >
    static bool matches(
        std::basic_string< CharT, StringTraitsT, AllocatorT > const& str,
        boost::basic_regex< CharT, ReTraitsT > const& expr,
        boost::regex_constants::match_flag_type flags = boost::regex_constants::match_default)
    {
        const CharT* p = str.c_str();
        return boost::regex_match(p, p + str.size(), expr, flags);
    }
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_
