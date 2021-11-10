/*
 *                 Copyright Lingxi Li 2015.
 *              Copyright Andrey Semashev 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   permissions.hpp
 * \author Lingxi Li
 * \author Andrey Semashev
 * \date   14.10.2015
 *
 * The header contains an abstraction wrapper for security permissions.
 */

#ifndef BOOST_LOG_UTILITY_PERMISSIONS_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_PERMISSIONS_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifdef BOOST_WINDOWS
extern "C" {
struct _SECURITY_ATTRIBUTES;
}
#endif // BOOST_WINDOWS

namespace boost {

#ifdef BOOST_WINDOWS
#if defined(BOOST_GCC) && BOOST_GCC >= 40600
#pragma GCC diagnostic push
// type attributes ignored after type is already defined
#pragma GCC diagnostic ignored "-Wattributes"
#endif
namespace winapi {
struct BOOST_LOG_MAY_ALIAS _SECURITY_ATTRIBUTES;
}
#if defined(BOOST_GCC) && BOOST_GCC >= 40600
#pragma GCC diagnostic pop
#endif
#endif

namespace interprocess {
class permissions;
} // namespace interprocess

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief Access permissions wrapper.
 *
 * On Windows platforms, it represents a pointer to \c SECURITY_ATTRIBUTES. The user is responsible
 * for allocating and reclaiming resources associated with the pointer, \c permissions instance does
 * not own them.
 *
 * On POSIX platforms, it represents a \c mode_t value.
 */
class permissions
{
public:
#if defined(BOOST_LOG_DOXYGEN_PASS)
    //! The type of security permissions, specific to the operating system
    typedef implementation_defined native_type;
#elif defined(BOOST_WINDOWS)
    typedef ::_SECURITY_ATTRIBUTES* native_type;
#else
    // Equivalent to POSIX mode_t
    typedef unsigned int native_type;
#endif

#if !defined(BOOST_LOG_DOXYGEN_PASS)
private:
    native_type m_perms;
#endif

public:
    /*!
     * Default constructor. The method constructs an object that represents
     * a null \c SECURITY_ATTRIBUTES pointer on Windows platforms, and a
     * \c mode_t value \c 0644 on POSIX platforms.
     */
    permissions() BOOST_NOEXCEPT
    {
        set_default();
    }

    /*!
     * Copy constructor.
     */
    permissions(permissions const& that) BOOST_NOEXCEPT : m_perms(that.m_perms)
    {
    }

    /*!
     * Copy assignment.
     */
    permissions& operator=(permissions const& that) BOOST_NOEXCEPT
    {
        m_perms = that.m_perms;
        return *this;
    }

    /*!
     * Initializing constructor.
     */
    permissions(native_type perms) BOOST_NOEXCEPT : m_perms(perms)
    {
    }

#ifdef BOOST_WINDOWS
    permissions(boost::winapi::_SECURITY_ATTRIBUTES* perms) BOOST_NOEXCEPT : m_perms(reinterpret_cast< native_type >(perms))
    {
    }
#endif

    /*!
     * Initializing constructor.
     */
    BOOST_LOG_API permissions(boost::interprocess::permissions const& perms) BOOST_NOEXCEPT;

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    /*!
     * Move constructor.
     */
    permissions(permissions&& that) BOOST_NOEXCEPT : m_perms(that.m_perms)
    {
        that.set_default();
    }

    /*!
     * Move assignment.
     */
    permissions& operator=(permissions&& that) BOOST_NOEXCEPT
    {
        m_perms = that.m_perms;
        that.set_default();
        return *this;
    }
#endif

    /*!
     * Sets permissions from the OS-specific permissions.
     */
    void set_native(native_type perms) BOOST_NOEXCEPT
    {
        m_perms = perms;
    }

    /*!
     * Returns the underlying OS-specific permissions.
     */
    native_type get_native() const BOOST_NOEXCEPT
    {
        return m_perms;
    }

    /*!
     * Sets the default permissions, which are equivalent to \c NULL \c SECURITY_ATTRIBUTES
     * on Windows and \c 0644 on POSIX platforms.
     */
    void set_default() BOOST_NOEXCEPT
    {
#if defined(BOOST_WINDOWS)
        m_perms = 0;
#else
        m_perms = 0644;
#endif
    }

    /*!
     * Sets unrestricted permissions, which are equivalent to \c SECURITY_ATTRIBUTES with \c NULL DACL
     * on Windows and \c 0666 on POSIX platforms.
     */
    void set_unrestricted()
    {
#if defined(BOOST_WINDOWS)
        m_perms = get_unrestricted_security_attributes();
#else
        m_perms = 0666;
#endif
    }

    /*!
     * The method swaps the object with \a that.
     *
     * \param that The other object to swap with.
     */
    void swap(permissions& that) BOOST_NOEXCEPT
    {
        native_type perms = m_perms;
        m_perms = that.m_perms;
        that.m_perms = perms;
    }

    //! Swaps the two \c permissions objects.
    friend void swap(permissions& a, permissions& b) BOOST_NOEXCEPT
    {
        a.swap(b);
    }

#if !defined(BOOST_LOG_DOXYGEN_PASS) && defined(BOOST_WINDOWS)
private:
    static BOOST_LOG_API native_type get_unrestricted_security_attributes();
#endif
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_PERMISSIONS_HPP_INCLUDED_
