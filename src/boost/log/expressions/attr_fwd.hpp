/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_fwd.hpp
 * \author Andrey Semashev
 * \date   21.07.2012
 *
 * The header contains forward declaration of a generic attribute placeholder in template expressions.
 */

#ifndef BOOST_LOG_EXPRESSIONS_ATTR_FWD_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_ATTR_FWD_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/fallback_policy_fwd.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace phoenix {

template< typename >
struct actor;

} // namespace phoenix

#endif

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

/*!
 * An attribute value extraction terminal
 */
template<
    typename T,
    typename FallbackPolicyT = fallback_to_none,
    typename TagT = void
>
class attribute_terminal;

/*!
 * An attribute value extraction terminal actor
 */
template<
    typename T,
    typename FallbackPolicyT = fallback_to_none,
    typename TagT = void,
    template< typename > class ActorT = phoenix::actor
>
class attribute_actor;

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_EXPRESSIONS_ATTR_FWD_HPP_INCLUDED_
