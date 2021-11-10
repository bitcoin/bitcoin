/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   mutable_constant.hpp
 * \author Andrey Semashev
 * \date   06.11.2007
 *
 * The header contains implementation of a mutable constant attribute.
 */

#ifndef BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_

#include <boost/static_assert.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/locks.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

/*!
 * \brief A class of an attribute that holds a single constant value with ability to change it
 *
 * The mutable_constant attribute stores a single value of type, specified as the first template argument.
 * This value is returned on each attribute value acquisition.
 *
 * The attribute also allows to modify the stored value, even if the attribute is registered in an attribute set.
 * In order to ensure thread safety of such modifications the \c mutable_constant class is also parametrized
 * with three additional template arguments: mutex type, scoped write and scoped read lock types. If not specified,
 * the lock types are automatically deduced based on the mutex type.
 *
 * The implementation may avoid using these types to actually create and use the mutex, if a more efficient synchronization method is
 * available (such as atomic operations on the value type). By default no synchronization is done.
 */
#ifdef BOOST_LOG_DOXYGEN_PASS
template< typename T, typename MutexT = void, typename ScopedWriteLockT = auto, typename ScopedReadLockT = auto >
#else // BOOST_LOG_DOXYGEN_PASS
template<
    typename T,
    typename MutexT = void,
    typename ScopedWriteLockT =
#ifndef BOOST_LOG_NO_THREADS
        typename boost::conditional<
            boost::log::aux::is_exclusively_lockable< MutexT >::value,
            boost::log::aux::exclusive_lock_guard< MutexT >,
            void
        >::type,
#else
        void,
#endif // BOOST_LOG_NO_THREADS
    typename ScopedReadLockT =
#ifndef BOOST_LOG_NO_THREADS
        typename boost::conditional<
            boost::log::aux::is_shared_lockable< MutexT >::value,
            boost::log::aux::shared_lock_guard< MutexT >,
            ScopedWriteLockT
        >::type
#else
        ScopedWriteLockT
#endif // BOOST_LOG_NO_THREADS
#endif // BOOST_LOG_DOXYGEN_PASS
>
class mutable_constant :
    public attribute
{
public:
    //! The attribute value type
    typedef T value_type;

protected:
    //! Factory implementation
    class BOOST_SYMBOL_VISIBLE impl :
        public attribute::impl
    {
    private:
        //! Mutex type
        typedef MutexT mutex_type;
        //! Shared lock type
        typedef ScopedReadLockT scoped_read_lock;
        //! Exclusive lock type
        typedef ScopedWriteLockT scoped_write_lock;
        BOOST_STATIC_ASSERT_MSG(!(is_void< mutex_type >::value || is_void< scoped_read_lock >::value || is_void< scoped_write_lock >::value), "Boost.Log: Mutex and both lock types either must not be void or must all be void");
        //! Attribute value wrapper
        typedef attribute_value_impl< value_type > attr_value;

    private:
        //! Thread protection mutex
        mutable mutex_type m_Mutex;
        //! Pointer to the actual attribute value
        intrusive_ptr< attr_value > m_Value;

    public:
        /*!
         * Initializing constructor
         */
        explicit impl(value_type const& value) : m_Value(new attr_value(value))
        {
        }
        /*!
         * Initializing constructor
         */
        explicit impl(BOOST_RV_REF(value_type) value) : m_Value(new attr_value(boost::move(value)))
        {
        }

        attribute_value get_value()
        {
            scoped_read_lock lock(m_Mutex);
            return attribute_value(m_Value);
        }

        void set(value_type const& value)
        {
            intrusive_ptr< attr_value > p = new attr_value(value);
            scoped_write_lock lock(m_Mutex);
            m_Value.swap(p);
        }

        void set(BOOST_RV_REF(value_type) value)
        {
            intrusive_ptr< attr_value > p = new attr_value(boost::move(value));
            scoped_write_lock lock(m_Mutex);
            m_Value.swap(p);
        }

        value_type get() const
        {
            scoped_read_lock lock(m_Mutex);
            return m_Value->get();
        }
    };

public:
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(value_type const& value) : attribute(new impl(value))
    {
    }
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(BOOST_RV_REF(value_type) value) : attribute(new impl(boost::move(value)))
    {
    }
    /*!
     * Constructor for casting support
     */
    explicit mutable_constant(cast_source const& source) : attribute(source.as< impl >())
    {
    }

    /*!
     * The method sets a new attribute value. The implementation exclusively locks the mutex in order
     * to protect the value assignment.
     */
    void set(value_type const& value)
    {
        get_impl()->set(value);
    }

    /*!
     * The method sets a new attribute value.
     */
    void set(BOOST_RV_REF(value_type) value)
    {
        get_impl()->set(boost::move(value));
    }

    /*!
     * The method acquires the current attribute value. The implementation non-exclusively locks the mutex in order
     * to protect the value acquisition.
     */
    value_type get() const
    {
        return get_impl()->get();
    }

protected:
    /*!
     * \returns Pointer to the factory implementation
     */
    impl* get_impl() const
    {
        return static_cast< impl* >(attribute::get_impl());
    }
};


/*!
 * \brief Specialization for unlocked case
 *
 * This version of attribute does not perform thread synchronization to access the stored value.
 */
template< typename T >
class mutable_constant< T, void, void, void > :
    public attribute
{
public:
    //! The attribute value type
    typedef T value_type;

protected:
    //! Factory implementation
    class BOOST_SYMBOL_VISIBLE impl :
        public attribute::impl
    {
    private:
        //! Attribute value wrapper
        typedef attribute_value_impl< value_type > attr_value;

    private:
        //! The actual value
        intrusive_ptr< attr_value > m_Value;

    public:
        /*!
         * Initializing constructor
         */
        explicit impl(value_type const& value) : m_Value(new attr_value(value))
        {
        }
        /*!
         * Initializing constructor
         */
        explicit impl(BOOST_RV_REF(value_type) value) : m_Value(new attr_value(boost::move(value)))
        {
        }

        attribute_value get_value()
        {
            return attribute_value(m_Value);
        }

        void set(value_type const& value)
        {
            m_Value = new attr_value(value);
        }
        void set(BOOST_RV_REF(value_type) value)
        {
            m_Value = new attr_value(boost::move(value));
        }

        value_type get() const
        {
            return m_Value->get();
        }
    };

public:
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(value_type const& value) : attribute(new impl(value))
    {
    }
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(BOOST_RV_REF(value_type) value) : attribute(new impl(boost::move(value)))
    {
    }
    /*!
     * Constructor for casting support
     */
    explicit mutable_constant(cast_source const& source) : attribute(source.as< impl >())
    {
    }

    /*!
     * The method sets a new attribute value.
     */
    void set(value_type const& value)
    {
        get_impl()->set(value);
    }

    /*!
     * The method sets a new attribute value.
     */
    void set(BOOST_RV_REF(value_type) value)
    {
        get_impl()->set(boost::move(value));
    }

    /*!
     * The method acquires the current attribute value.
     */
    value_type get() const
    {
        return get_impl()->get();
    }

protected:
    /*!
     * \returns Pointer to the factory implementation
     */
    impl* get_impl() const
    {
        return static_cast< impl* >(attribute::get_impl());
    }
};

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_
