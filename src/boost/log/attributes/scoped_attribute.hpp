/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   scoped_attribute.hpp
 * \author Andrey Semashev
 * \date   13.05.2007
 *
 * The header contains definition of facilities to define scoped attributes.
 */

#ifndef BOOST_LOG_ATTRIBUTES_SCOPED_ATTRIBUTE_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_SCOPED_ATTRIBUTE_HPP_INCLUDED_

#include <cstddef>
#include <utility>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/core/addressof.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/utility/unused_variable.hpp>
#include <boost/log/utility/unique_identifier_name.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A base class for all scoped attribute guards
class attribute_scope_guard
{
};

} // namespace aux

//! Scoped attribute guard type
typedef aux::attribute_scope_guard const& scoped_attribute;

namespace aux {

//! A scoped logger attribute guard
template< typename LoggerT >
class scoped_logger_attribute :
    public attribute_scope_guard
{
    BOOST_COPYABLE_AND_MOVABLE_ALT(scoped_logger_attribute)

private:
    //! Logger type
    typedef LoggerT logger_type;

private:
    //! A reference to the logger
    logger_type* m_pLogger;
    //! An iterator to the added attribute
    attribute_set::iterator m_itAttribute;

public:
    //! Constructor
    scoped_logger_attribute(logger_type& l, attribute_name const& name, attribute const& attr) :
        m_pLogger(boost::addressof(l))
    {
        std::pair<
            attribute_set::iterator,
            bool
        > res = l.add_attribute(name, attr);
        if (res.second)
            m_itAttribute = res.first;
        else
            m_pLogger = NULL; // if there already is a same-named attribute, don't register anything
    }
    //! Move constructor
    scoped_logger_attribute(BOOST_RV_REF(scoped_logger_attribute) that) BOOST_NOEXCEPT :
        m_pLogger(that.m_pLogger),
        m_itAttribute(that.m_itAttribute)
    {
        that.m_pLogger = NULL;
    }

    //! Destructor
    ~scoped_logger_attribute()
    {
        if (m_pLogger)
            m_pLogger->remove_attribute(m_itAttribute);
    }

#ifndef BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT
    BOOST_DELETED_FUNCTION(scoped_logger_attribute(scoped_logger_attribute const&))
#else // BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT
    scoped_logger_attribute(scoped_logger_attribute const& that) BOOST_NOEXCEPT : m_pLogger(that.m_pLogger), m_itAttribute(that.m_itAttribute)
    {
        const_cast< scoped_logger_attribute& >(that).m_pLogger = NULL;
    }
#endif // BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT

    BOOST_DELETED_FUNCTION(scoped_logger_attribute& operator= (scoped_logger_attribute const&))
};

} // namespace aux

//  Generator helper functions
/*!
 * Registers an attribute in the logger
 *
 * \param l Logger to register the attribute in
 * \param name Attribute name
 * \param attr The attribute. Must not be NULL.
 * \return An unspecified guard object which may be used to initialize a \c scoped_attribute variable.
 */
template< typename LoggerT >
BOOST_FORCEINLINE aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(LoggerT& l, attribute_name const& name, attribute const& attr)
{
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    return aux::scoped_logger_attribute< LoggerT >(l, name, attr);
#else
    aux::scoped_logger_attribute< LoggerT > guard(l, name, attr);
    return boost::move(guard);
#endif
}

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_SCOPED_LOGGER_ATTR_INTERNAL(logger, attr_name, attr, sentry_var_name)\
    BOOST_LOG_UNUSED_VARIABLE(::boost::log::scoped_attribute, sentry_var_name,\
        = ::boost::log::add_scoped_logger_attribute(logger, attr_name, (attr)));

#endif // BOOST_LOG_DOXYGEN_PASS

//! The macro sets a scoped logger-wide attribute in a more compact way
#define BOOST_LOG_SCOPED_LOGGER_ATTR(logger, attr_name, attr)\
    BOOST_LOG_SCOPED_LOGGER_ATTR_INTERNAL(\
        logger,\
        attr_name,\
        attr,\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_logger_attr_sentry_))

//! The macro sets a scoped logger-wide tag in a more compact way
#define BOOST_LOG_SCOPED_LOGGER_TAG(logger, attr_name, attr_value)\
    BOOST_LOG_SCOPED_LOGGER_ATTR(logger, attr_name, ::boost::log::attributes::make_constant(attr_value))

namespace aux {

//! A scoped thread-specific attribute guard
class scoped_thread_attribute :
    public attribute_scope_guard
{
    BOOST_COPYABLE_AND_MOVABLE_ALT(scoped_thread_attribute)

private:
    //! A pointer to the logging core
    core_ptr m_pCore;
    //! An iterator to the added attribute
    attribute_set::iterator m_itAttribute;

public:
    //! Constructor
    scoped_thread_attribute(attribute_name const& name, attribute const& attr) :
        m_pCore(core::get())
    {
        std::pair<
            attribute_set::iterator,
            bool
        > res = m_pCore->add_thread_attribute(name, attr);
        if (res.second)
            m_itAttribute = res.first;
        else
            m_pCore.reset(); // if there already is a same-named attribute, don't register anything
    }
    //! Move constructor
    scoped_thread_attribute(BOOST_RV_REF(scoped_thread_attribute) that) : m_itAttribute(that.m_itAttribute)
    {
        m_pCore.swap(that.m_pCore);
    }

    //! Destructor
    ~scoped_thread_attribute()
    {
        if (!!m_pCore)
            m_pCore->remove_thread_attribute(m_itAttribute);
    }

#ifndef BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT
    BOOST_DELETED_FUNCTION(scoped_thread_attribute(scoped_thread_attribute const&))
#else // BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT
    scoped_thread_attribute(scoped_thread_attribute const& that) : m_itAttribute(that.m_itAttribute)
    {
        m_pCore.swap(const_cast< scoped_thread_attribute& >(that).m_pCore);
    }
#endif // BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT

    BOOST_DELETED_FUNCTION(scoped_thread_attribute& operator= (scoped_thread_attribute const&))
};

} // namespace aux

//  Generator helper functions
/*!
 * Registers a thread-specific attribute
 *
 * \param name Attribute name
 * \param attr The attribute. Must not be NULL.
 * \return An unspecified guard object which may be used to initialize a \c scoped_attribute variable.
 */
BOOST_FORCEINLINE aux::scoped_thread_attribute add_scoped_thread_attribute(attribute_name const& name, attribute const& attr)
{
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    return aux::scoped_thread_attribute(name, attr);
#else
    aux::scoped_thread_attribute guard(name, attr);
    return boost::move(guard);
#endif
}

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_SCOPED_THREAD_ATTR_INTERNAL(attr_name, attr, sentry_var_name)\
    BOOST_LOG_UNUSED_VARIABLE(::boost::log::scoped_attribute, sentry_var_name,\
        = ::boost::log::add_scoped_thread_attribute(attr_name, (attr)));

#endif // BOOST_LOG_DOXYGEN_PASS

//! The macro sets a scoped thread-wide attribute in a more compact way
#define BOOST_LOG_SCOPED_THREAD_ATTR(attr_name, attr)\
    BOOST_LOG_SCOPED_THREAD_ATTR_INTERNAL(\
        attr_name,\
        attr,\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_thread_attr_sentry_))

//! The macro sets a scoped thread-wide tag in a more compact way
#define BOOST_LOG_SCOPED_THREAD_TAG(attr_name, attr_value)\
    BOOST_LOG_SCOPED_THREAD_ATTR(attr_name, ::boost::log::attributes::make_constant(attr_value))

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_SCOPED_ATTRIBUTE_HPP_INCLUDED_
