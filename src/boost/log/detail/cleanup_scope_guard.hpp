/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   cleanup_scope_guard.hpp
 * \author Andrey Semashev
 * \date   11.03.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_CLEANUP_SCOPE_GUARD_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_CLEANUP_SCOPE_GUARD_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! Cleanup scope guard
template< typename T >
struct cleanup_guard
{
    explicit cleanup_guard(T& obj) : m_Obj(obj) {}
    ~cleanup_guard() { m_Obj.clear(); }

    // Copying prohibited
    BOOST_DELETED_FUNCTION(cleanup_guard(cleanup_guard const&))
    BOOST_DELETED_FUNCTION(cleanup_guard& operator= (cleanup_guard const&))

private:
    T& m_Obj;
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_CLEANUP_SCOPE_GUARD_HPP_INCLUDED_
