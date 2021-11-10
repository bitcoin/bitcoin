/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute.hpp
 * \author Andrey Semashev
 * \date   15.04.2007
 *
 * The header contains attribute interface definition.
 */

#ifndef BOOST_LOG_ATTRIBUTES_ATTRIBUTE_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_ATTRIBUTE_HPP_INCLUDED_

#include <new>
#include <boost/move/core.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/core/explicit_operator_bool.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

#ifndef BOOST_LOG_DOXYGEN_PASS

class attribute_value;

namespace aux {

//! Reference proxy object to implement \c operator[]
class attribute_set_reference_proxy;

} // namespace aux

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief A base class for an attribute value factory
 *
 * Every attribute is represented with a factory that is basically an attribute value generator.
 * The sole purpose of an attribute is to return an actual value when requested. A simplest attribute
 * can always return the same value that it stores internally, but more complex ones can
 * perform a considerable amount of work to return a value, and the returned values may differ
 * each time requested.
 *
 * A word about thread safety. An attribute should be prepared to be requested a value from
 * multiple threads concurrently.
 */
class attribute
{
    BOOST_COPYABLE_AND_MOVABLE(attribute)

public:
    /*!
     * \brief A base class for an attribute value factory
     *
     * All attributes must derive their implementation from this class.
     */
    struct BOOST_LOG_NO_VTABLE BOOST_SYMBOL_VISIBLE impl :
        public boost::intrusive_ref_counter< impl >
    {
        /*!
         * \brief Virtual destructor
         */
        virtual ~impl() {}

        /*!
         * \return The actual attribute value. It shall not return empty values (exceptions
         *         shall be used to indicate errors).
         */
        virtual attribute_value get_value() = 0;

        BOOST_LOG_API static void* operator new (std::size_t size);
        BOOST_LOG_API static void operator delete (void* p, std::size_t size) BOOST_NOEXCEPT;
    };

private:
    //! Pointer to the attribute factory implementation
    intrusive_ptr< impl > m_pImpl;

public:
    /*!
     * Default constructor. Creates an empty attribute value factory, which is not usable until
     * \c set_impl is called.
     */
    BOOST_DEFAULTED_FUNCTION(attribute(), {})

    /*!
     * Copy constructor
     */
    attribute(attribute const& that) BOOST_NOEXCEPT : m_pImpl(that.m_pImpl) {}

    /*!
     * Move constructor
     */
    attribute(BOOST_RV_REF(attribute) that) BOOST_NOEXCEPT { m_pImpl.swap(that.m_pImpl); }

    /*!
     * Initializing constructor
     *
     * \param p Pointer to the implementation. Must not be \c NULL.
     */
    explicit attribute(intrusive_ptr< impl > p) BOOST_NOEXCEPT { m_pImpl.swap(p); }

    /*!
     * Copy assignment
     */
    attribute& operator= (BOOST_COPY_ASSIGN_REF(attribute) that) BOOST_NOEXCEPT
    {
        m_pImpl = that.m_pImpl;
        return *this;
    }

    /*!
     * Move assignment
     */
    attribute& operator= (BOOST_RV_REF(attribute) that) BOOST_NOEXCEPT
    {
        m_pImpl.swap(that.m_pImpl);
        return *this;
    }

#ifndef BOOST_LOG_DOXYGEN_PASS
    attribute& operator= (aux::attribute_set_reference_proxy const& that) BOOST_NOEXCEPT;
#endif

    /*!
     * Verifies that the factory is not in empty state
     */
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()

    /*!
     * Verifies that the factory is in empty state
     */
    bool operator! () const BOOST_NOEXCEPT { return !m_pImpl; }

    /*!
     * \return The actual attribute value. It shall not return empty values (exceptions
     *         shall be used to indicate errors).
     */
    attribute_value get_value() const;

    /*!
     * The method swaps two factories (i.e. their implementations).
     */
    void swap(attribute& that) BOOST_NOEXCEPT { m_pImpl.swap(that.m_pImpl); }

protected:
    /*!
     * \returns The pointer to the implementation
     */
    impl* get_impl() const BOOST_NOEXCEPT { return m_pImpl.get(); }
    /*!
     * Sets the pointer to the factory implementation.
     *
     * \param p Pointer to the implementation. Must not be \c NULL.
     */
    void set_impl(intrusive_ptr< impl > p) BOOST_NOEXCEPT { m_pImpl.swap(p); }

    template< typename T >
    friend T attribute_cast(attribute const&);
};

/*!
 * The function swaps two attribute value factories
 */
inline void swap(attribute& left, attribute& right) BOOST_NOEXCEPT
{
    left.swap(right);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
#if defined(BOOST_LOG_ATTRIBUTES_ATTRIBUTE_VALUE_HPP_INCLUDED_)
#include <boost/log/detail/attribute_get_value_impl.hpp>
#endif

#endif // BOOST_LOG_ATTRIBUTES_ATTRIBUTE_HPP_INCLUDED_
