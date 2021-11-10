/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute_name.hpp
 * \author Andrey Semashev
 * \date   28.06.2010
 *
 * The header contains attribute name interface definition.
 */

#ifndef BOOST_LOG_ATTRIBUTE_NAME_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTE_NAME_HPP_INCLUDED_

#include <iosfwd>
#include <string>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/core/explicit_operator_bool.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief The class represents an attribute name in containers used by the library
 *
 * The class mostly serves for optimization purposes. Each attribute name that is used
 * with the library is automatically associated with a unique identifier, which is much
 * lighter in terms of memory footprint and operations complexity. This is done
 * transparently by this class, on object construction. Passing objects of this class
 * to other library methods, such as attribute lookup functions, will not require
 * this translation and/or string copying and thus will result in a more efficient code.
 */
class attribute_name
{
public:
    //! String type
    typedef std::string string_type;
#ifdef BOOST_LOG_DOXYGEN_PASS
    //! Associated identifier
    typedef unspecified id_type;
#else
    typedef uint32_t id_type;

private:
    class repository;
    friend class repository;

private:
    //! Associated identifier
    id_type m_id;
    //! A special identifier value indicating unassigned attribute name
    static BOOST_CONSTEXPR_OR_CONST id_type uninitialized = ~static_cast< id_type >(0u);
#endif

public:
    /*!
     * Default constructor. Creates an object that does not refer to any attribute name.
     */
    BOOST_CONSTEXPR attribute_name() BOOST_NOEXCEPT : m_id(uninitialized)
    {
    }
    /*!
     * Constructs an attribute name from the specified string
     *
     * \param name An attribute name
     * \pre \a name is not NULL and points to a zero-terminated string
     */
    attribute_name(const char* name) :
        m_id(get_id_from_string(name))
    {
    }
    /*!
     * Constructs an attribute name from the specified string
     *
     * \param name An attribute name
     */
    attribute_name(string_type const& name) :
        m_id(get_id_from_string(name.c_str()))
    {
    }

    /*!
     * Compares the attribute names
     *
     * \return \c true if <tt>*this</tt> and \c that refer to the same attribute name,
     *         and \c false otherwise.
     */
    bool operator== (attribute_name const& that) const BOOST_NOEXCEPT { return m_id == that.m_id; }
    /*!
     * Compares the attribute names
     *
     * \return \c true if <tt>*this</tt> and \c that refer to different attribute names,
     *         and \c false otherwise.
     */
    bool operator!= (attribute_name const& that) const BOOST_NOEXCEPT { return m_id != that.m_id; }

    /*!
     * Compares the attribute names
     *
     * \return \c true if <tt>*this</tt> and \c that refer to the same attribute name,
     *         and \c false otherwise.
     */
    bool operator== (const char* that) const { return (m_id != uninitialized) && (this->string() == that); }
    /*!
     * Compares the attribute names
     *
     * \return \c true if <tt>*this</tt> and \c that refer to different attribute names,
     *         and \c false otherwise.
     */
    bool operator!= (const char* that) const { return !operator== (that); }

    /*!
     * Compares the attribute names
     *
     * \return \c true if <tt>*this</tt> and \c that refer to the same attribute name,
     *         and \c false otherwise.
     */
    bool operator== (string_type const& that) const { return (m_id != uninitialized) && (this->string() == that); }
    /*!
     * Compares the attribute names
     *
     * \return \c true if <tt>*this</tt> and \c that refer to different attribute names,
     *         and \c false otherwise.
     */
    bool operator!= (string_type const& that) const { return !operator== (that); }

    /*!
     * Checks if the object was default-constructed
     *
     * \return \c true if <tt>*this</tt> was constructed with an attribute name, \c false otherwise
     */
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()
    /*!
     * Checks if the object was default-constructed
     *
     * \return \c true if <tt>*this</tt> was default-constructed and does not refer to any attribute name,
     *         \c false otherwise
     */
    bool operator! () const BOOST_NOEXCEPT { return (m_id == uninitialized); }

    /*!
     * \return The associated id value
     * \pre <tt>(!*this) == false</tt>
     */
    id_type id() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT(m_id != uninitialized);
        return m_id;
    }
    /*!
     * \return The attribute name string that was used during the object construction
     * \pre <tt>(!*this) == false</tt>
     */
    string_type const& string() const { return get_string_from_id(m_id); }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_API id_type get_id_from_string(const char* name);
    static BOOST_LOG_API string_type const& get_string_from_id(id_type id);
#endif
};

template< typename CharT, typename TraitsT >
BOOST_LOG_API std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& strm,
    attribute_name const& name);

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTE_NAME_HPP_INCLUDED_
