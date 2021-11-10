/*
 *             Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   utility/ipc/object_name.hpp
 * \author Andrey Semashev
 * \date   05.03.2016
 *
 * The header contains declaration of a system object name wrapper.
 */

#ifndef BOOST_LOG_UTILITY_IPC_OBJECT_NAME_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_IPC_OBJECT_NAME_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <cstddef>
#include <iosfwd>
#include <string>
#include <boost/move/core.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace ipc {

/*!
 * \brief A system object name class
 *
 * In order to identify a system-wide object such as a shared memory segment or a named synchronization primitive the object has to be given a name.
 * The format of the name is specific to the operating system and the \c object_name class provides an abstraction for names of objects. It also
 * provides means for scoping, which allows to avoid name clashes between different processes.
 *
 * The object name is a UTF-8 encoded string. The portable object name should consist of the following characters:
 *
 * <pre>
 * A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
 * a b c d e f g h i j k l m n o p q r s t u v w x y z
 * 0 1 2 3 4 5 6 7 8 9 . _ -
 * </pre>
 *
 * \note The character set corresponds to the POSIX Portable Filename Character Set (http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_278).
 *
 * Use of other characters may result in non-portable system-specific behavior.
 *
 * The name can have one of the following scopes:
 *
 * \li \c global - objects within this scope are visible to any process on the system. In order to use this scope the process may need to have
 *                 extended privileges. This scope is not available for Windows Store applications.
 * \li \c user - objects within this scope can be opened by processes running under the same user as the current process.
 * \li \c session - objects within this scope are visible to processes within the session of the current process. The definition of a session may vary between
 *                  operating systems. On POSIX, a session is typically a group of processes attached to a single virtual terminal device. On Windows a session is
 *                  started when a user logs into the system. There is also a separate session for Windows services.
 * \li \c process_group - objects within this scope are visible to processes within the process group of the current process. Currently, on Windows all processes
 *                        running in the current session are considered members of the same process group. This may change in future.
 *
 * The scopes are not overlapping. For instance, if an object is created in the global scope, the object cannot be opened with the same name but in user's scope.
 *
 * Note that name scoping is not a security feature. On some systems any process on the system has technical capability to open objects within any scope.
 * The scope is only used to help avoid name clashes between processes using \c object_name to identify objects.
 */
class object_name
{
public:
    //! Name scopes
    enum scope
    {
        global,        //!< The name has global scope; any process in the system has the potential to open the resource identified by the name
        user,          //!< The name is limited to processes running under the current user
        session,       //!< The name is limited to processes running in the current login session
        process_group  //!< The name is limited to processes running in the current process group
    };

#if !defined(BOOST_LOG_DOXYGEN_PASS)

    BOOST_COPYABLE_AND_MOVABLE(object_name)

private:
    std::string m_name;

#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

public:
    /*!
     * Default constructor. The method creates an empty object name.
     *
     * \post <tt>empty() == true</tt>
     */
    object_name() BOOST_NOEXCEPT
    {
    }

    /*!
     * Move constructor.
     */
    object_name(BOOST_RV_REF(object_name) that) BOOST_NOEXCEPT
    {
        m_name.swap(that.m_name);
    }

    /*!
     * Copy constructor.
     */
    object_name(object_name const& that) : m_name(that.m_name)
    {
    }

    /*!
     * Constructor from the native string.
     *
     * \param str The object name string, must not be \c NULL. The string format is specific to the operating system.
     */
    static object_name from_native(const char* str)
    {
        object_name name;
        name.m_name = str;
        return name;
    }

    /*!
     * Constructor from the native string.
     *
     * \param str The object name string. The string format is specific to the operating system.
     */
    static object_name from_native(std::string const& str)
    {
        object_name name;
        name.m_name = str;
        return name;
    }

    /*!
     * Constructor from the object name
     * \param ns The scope of the object name
     * \param str The object name, must not be NULL.
     */
    BOOST_LOG_API object_name(scope ns, const char* str);

    /*!
     * Constructor from the object name
     * \param ns The scope of the object name
     * \param str The object name
     */
    BOOST_LOG_API object_name(scope ns, std::string const& str);

    /*!
     * Move assignment
     */
    object_name& operator= (BOOST_RV_REF(object_name) that) BOOST_NOEXCEPT
    {
        m_name.clear();
        m_name.swap(that.m_name);
        return *this;
    }

    /*!
     * Copy assignment
     */
    object_name& operator= (BOOST_COPY_ASSIGN_REF(object_name) that)
    {
        m_name = that.m_name;
        return *this;
    }

    /*!
     * Returns \c true if the object name is empty
     */
    bool empty() const BOOST_NOEXCEPT { return m_name.empty(); }

    /*!
     * Returns length of the name, in bytes
     */
    std::size_t size() const BOOST_NOEXCEPT { return m_name.size(); }

    /*!
     * Returns the name string
     */
    const char* c_str() const BOOST_NOEXCEPT { return m_name.c_str(); }

    /*!
     * Swaps the object name with another object name
     */
    void swap(object_name& that) BOOST_NOEXCEPT { m_name.swap(that.m_name); }

    /*!
     * Swaps two object names
     */
    friend void swap(object_name& left, object_name& right) BOOST_NOEXCEPT
    {
        left.swap(right);
    }

    /*!
     * Returns string representation of the object name
     */
    friend std::string to_string(object_name const& name)
    {
        return name.m_name;
    }

    /*!
     * Equality operator
     */
    friend bool operator== (object_name const& left, object_name const& right) BOOST_NOEXCEPT
    {
        return left.m_name == right.m_name;
    }
    /*!
     * Inequality operator
     */
    friend bool operator!= (object_name const& left, object_name const& right) BOOST_NOEXCEPT
    {
        return left.m_name != right.m_name;
    }
    /*!
     * Less operator
     */
    friend bool operator< (object_name const& left, object_name const& right) BOOST_NOEXCEPT
    {
        return left.m_name < right.m_name;
    }
    /*!
     * Greater operator
     */
    friend bool operator> (object_name const& left, object_name const& right) BOOST_NOEXCEPT
    {
        return left.m_name > right.m_name;
    }
    /*!
     * Less or equal operator
     */
    friend bool operator<= (object_name const& left, object_name const& right) BOOST_NOEXCEPT
    {
        return left.m_name <= right.m_name;
    }
    /*!
     * Greater or equal operator
     */
    friend bool operator>= (object_name const& left, object_name const& right) BOOST_NOEXCEPT
    {
        return left.m_name >= right.m_name;
    }

    /*!
     * Stream ouput operator
     */
    template< typename CharT, typename TraitsT >
    friend std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, object_name const& name)
    {
        strm << name.c_str();
        return strm;
    }
};

} // namespace ipc

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_IPC_OBJECT_NAME_HPP_INCLUDED_
